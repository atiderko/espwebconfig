/**************************************************************

This file is a part of BalkonBewaesserungsSteuerung
https://github.com/atiderko/bbs

Copyright [2020] Alexander Tiderko

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

**************************************************************/
#include "../ewcInterface.h"
#include "../ewcConfig.h"
#include "ewcMqttHomie.h"

using namespace EWC;


MqttConfigTopic::MqttConfigTopic(String topic, String value, uint8_t qos, bool retain)
{
    _topic = topic;
    _value = value;
    _qos = qos;
    _retain = retain;
    I::get().logger() << "[MQTTHomie]: created " << _topic << ", value: " << _value << endl;

}

MqttConfigTopic::~MqttConfigTopic()
{
}

uint16_t MqttConfigTopic::publish(EWC::Mqtt& mqtt)
{
    I::get().logger() << "[MQTTHomie]: publish " << _value << " to " << _topic << endl;
    return mqtt.client().publish(_topic.c_str(), _qos, _retain, _value.c_str());
}

MqttHomie::MqttHomie()
{
    _ewcMqtt = nullptr;
}

MqttHomie::~MqttHomie()
{
}

void MqttHomie::setup(EWC::Mqtt& mqtt, String deviceName, String deviceId) {
    _ewcMqtt = &mqtt;
    for (auto itn = _homieDevice.nodes.begin(); itn != _homieDevice.nodes.end(); itn++) {
        itn->properties.clear();
    }
    _homieDevice.nodes.clear();
    _homieDevice.name = deviceName;
    _homieDevice.id = deviceId;
    mqtt.client().onMessage(std::bind(&MqttHomie::_onMqttMessage, this, std::placeholders::_1, std::placeholders::_2,
                                      std::placeholders::_3, std::placeholders::_4,
                                      std::placeholders::_5, std::placeholders::_6));
    mqtt.client().onConnect(std::bind(&MqttHomie::_onMqttConnect, this, std::placeholders::_1));
    mqtt.client().onPublish(std::bind(&MqttHomie::_onMqttAck, this, std::placeholders::_1));
}

bool MqttHomie::addNode(String id, String name, String type)
{
    I::get().logger() << F("[MQTTHomie] add node with id: ") << id << endl;
    HomieNode hn;
    hn.id = id;
    if (!name.isEmpty()) {
        hn.name = name;
    } else {
        hn.name = id;
    }
    hn.type = type;
    for (auto itn = _homieDevice.nodes.begin(); itn != _homieDevice.nodes.end(); itn++) {
        if (itn->id.compareTo(id) == 0) {
            return false;
        }
    }
    _homieDevice.nodes.push_back(hn);
    return true;
}

bool MqttHomie::addProperty(String nodeId, String propertyId, String name, String datatype, String format, String unit, bool retained)
{
    for (auto itn = _homieDevice.nodes.begin(); itn != _homieDevice.nodes.end(); itn++) {
        if (itn->id.compareTo(nodeId) == 0) {
            I::get().logger() << F("[MQTTHomie] add property with id: ") << propertyId << F(" to nodeid: ") << nodeId << endl;
            HomieProperty hp;
            hp.id = propertyId;
            hp.name = name;
            hp.datatype = datatype;
            hp.format = format;
            hp.unit = unit;
            hp.settable = false;
            hp.retained = retained;
            itn->properties.push_back(hp);
            return true;
        }
    }
    return false;
}

bool MqttHomie::addPropertySettable(String nodeId, String propertyId, String name, String datatype, AsyncMqttClientInternals::OnMessageUserCallback callback, String format, String unit, bool retained)
{
    for (auto itn = _homieDevice.nodes.begin(); itn != _homieDevice.nodes.end(); itn++) {
        if (itn->id.compareTo(nodeId) == 0) {
            I::get().logger() << F("[MQTTHomie] add settable property with id: ") << propertyId << F(" to nodeid: ") << nodeId << endl;
            HomieProperty hp;
            hp.id = propertyId;
            hp.name = name;
            hp.datatype = datatype;
            hp.format = format;
            hp.unit = unit;
            hp.settable = true;
            hp.retained = retained;
            itn->properties.push_back(hp);
            _callbacks.push_back(CallbackTopic(nodeId, propertyId, callback));
            return true;
        }
    }
    return false;
}

void MqttHomie::_onMqttConnect(bool sessionPresent) {
    I::get().logger() << F("[MQTTHomie] setup on connect, session present: ") << sessionPresent << endl;
    I::get().logger() << "[MQTTHomie]: ESP heap: _onMqttConnect: " << ESP.getFreeHeap() << endl;
    String prefix = _ewcMqtt->paramDiscoveryPrefix;
    prefix.replace(SEP, "");
    if (prefix.length() == 0) {
        prefix = "homie";
    }
    _homieDevice.prefix = prefix;
    _homieStateTopic = _homieDevice.prefix + SEP + _homieDevice.id + SEP + "$state";
    I::get().logger() << F("[MQTTHomie] configure homie topics") << endl;
    _ewcMqtt->client().setWill(_homieStateTopic.c_str(), 2, true, "lost");
    _waitForPacketId = 0;
    _idxPublishConfig = 0;
    _configTopics.clear();
    // create configuration topics https://homieiot.github.io/
    // create topics for device configuration
    String devicePrefix = _homieDevice.prefix + SEP + _homieDevice.id + SEP;
    _configTopics.push_back(MqttConfigTopic(devicePrefix + "$homie", "4.0", 2, true));
    _configTopics.push_back(MqttConfigTopic(devicePrefix + "$name", _homieDevice.name, 2, true));
    _configTopics.push_back(MqttConfigTopic(_homieStateTopic, "init", 2, false));
    // create nodes string
    String nodes;
    for (auto itn = _homieDevice.nodes.begin(); itn != _homieDevice.nodes.end(); itn++) {
        if (nodes.isEmpty()) {
            nodes = itn->id;
        } else {
            nodes += "," + itn->id;
        }
    }
    _configTopics.push_back(MqttConfigTopic(devicePrefix + "$nodes", nodes, 2, true));
    // update callable topics
    for (auto itc = _callbacks.begin(); itc != _callbacks.end(); itc++) {
        itc->createTopic(_homieDevice.prefix, _homieDevice.id);
    }
    // create topics for node configuration
    for (auto itn = _homieDevice.nodes.begin(); itn != _homieDevice.nodes.end(); itn++) {
        _configTopics.push_back(MqttConfigTopic(devicePrefix + itn->id + SEP + "$name", itn->name, 2, true));
        String properties;
        for (auto itp = itn->properties.begin(); itp != itn->properties.end(); itp++) {
            if (properties.isEmpty()) {
                properties = itp->id;
            } else {
                properties += "," + itp->id;
            }
        }
        _configTopics.push_back(MqttConfigTopic(devicePrefix + itn->id + SEP + "$properties", properties, 2, true));
        // create topics for property configuration
        for (auto itp = itn->properties.begin(); itp != itn->properties.end(); itp++) {
            String propPrefix = devicePrefix + itn->id + SEP + itp->id + SEP;
            _configTopics.push_back(MqttConfigTopic(propPrefix + "$name", itp->name, 2, true));
            _configTopics.push_back(MqttConfigTopic(propPrefix + "$datatype", itp->datatype, 2, true));
            if (!itp->unit.isEmpty()) {
                _configTopics.push_back(MqttConfigTopic(propPrefix + "$unit", itp->unit, 2, true));
            }
            if (!itp->format.isEmpty()) {
                _configTopics.push_back(MqttConfigTopic(propPrefix + "$format", itp->format, 2, true));
            }
            if (!itp->retained) {
                _configTopics.push_back(MqttConfigTopic(propPrefix + "$retained", "false", 2, true));
            }
            if (itp->settable) {
                _configTopics.push_back(MqttConfigTopic(propPrefix + "$settable", "true", 2, true));
                // subscribe to callable topics of this property
                String tpset = propPrefix + "set";
                for (auto itc = _callbacks.begin(); itc != _callbacks.end(); itc++) {
                    if (tpset.compareTo(itc->topic) == 0) {
                        I::get().logger() << F("[MQTTHomie] subscribe to ") << tpset << endl;
                        _ewcMqtt->client().subscribe(itc->topic.c_str(), 2);
                        break;
                    }
                }
            }
        }
    }
    _configTopics.push_back(MqttConfigTopic(_homieStateTopic, "ready", 2, false));
    _waitForPacketId = _configTopics[0].publish(*_ewcMqtt);
}

void MqttHomie::publishState(String nodeId, String propertyId, String value, bool retain, uint8_t qos)
{
    String topic = _homieDevice.prefix + SEP + _homieDevice.id + SEP + nodeId + SEP + propertyId;
    I::get().logger() << F("[MQTTHomie] publish ") << value << F(" to ") << topic << endl;
    _ewcMqtt->client().publish(topic.c_str(), qos, retain, value.c_str());   
}

void MqttHomie::_onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  I::get().logger() << F("[MQTTHomie] onMqttMessage; topic: ") << topic << F("; payload: ") << payload << endl;
  for (auto itc = _callbacks.begin(); itc != _callbacks.end(); itc++) {
    if (strcmp((char *)topic, itc->topic.c_str()) == 0) {
        itc->callback(topic, payload, properties, len, index, total);
    }
  }
}

void MqttHomie::_onMqttAck(uint16_t packetId)
{
    I::get().logger() << F("[MQTTHomie]: received ack for ") << packetId << endl;
    if (packetId == _waitForPacketId) {
        _idxPublishConfig++;
        if (_idxPublishConfig < _configTopics.size()) {
            _waitForPacketId = _configTopics[_idxPublishConfig].publish(*_ewcMqtt);
        } else {
            I::get().logger() << F("[MQTTHomie]: all registration topics sent, current free memory: ") << ESP.getFreeHeap() << endl;
            _configTopics.clear();
            I::get().logger() << F("[MQTTHomie]: free memory after clear: ") << ESP.getFreeHeap() << endl;
        }
    }
}
