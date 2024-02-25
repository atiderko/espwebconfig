/**************************************************************

This file is a part of
https://github.com/atiderko/espwebconfig

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
#include "../ewcConfigServer.h"
#include "ewcMqttHA.h"

using namespace EWC;

HAPropertyConfig::HAPropertyConfig(String component, String uniqueId, String name, String deviceClass, String objectId, String unit, bool retained)
{
  this->uniqueId = uniqueId;
  this->component = component;
  this->name = name;
  this->deviceClass = deviceClass;
  this->objectId = objectId;
  this->unit = unit;
  this->settable = false;
  this->retained = retained;
}
HAPropertyConfig::HAPropertyConfig(String component, String uniqueId, String name, String deviceClass, String objectId, AsyncMqttClientInternals::OnMessageUserCallback callback, String unit, bool retained)
{
  this->uniqueId = uniqueId;
  this->component = component;
  this->name = name;
  this->deviceClass = deviceClass;
  this->objectId = objectId;
  this->unit = unit;
  this->retained = retained;
  this->callback = callback;
  this->settable = true;
}

String HAPropertyConfig::getStateTopic(String discoveryPrefix)
{
  return discoveryPrefix + "/" + component + "/" + I::get().config().getChipId() + "/" + objectId + "/state";
}
String HAPropertyConfig::getCmdTopic(String discoveryPrefix)
{
  return discoveryPrefix + "/" + component + "/" + I::get().config().getChipId() + "/" + objectId + "/set";
}

HAProperty::HAProperty(String discoveryPrefix, HAPropertyConfig &config, HADevice &device)
{
  this->uniqueId = config.uniqueId;
  this->stateTopic = config.getStateTopic(discoveryPrefix);
  if (config.settable)
  {
    this->settable = true;
    this->commandTopic = config.getCmdTopic(discoveryPrefix);
    this->callback = config.callback;
  }
  this->discoveryTopic = discoveryPrefix + "/" + config.component + "/" + I::get().config().getChipId() + "/" + config.objectId + "/config";
  // create json configuration for discovery in home assistant
  if (config.name.length() > 0)
  {
    jsonConfig["name"] = config.name;
  }
  if (config.deviceClass.length() > 0)
  {
    jsonConfig["device_class"] = config.deviceClass;
  }
  jsonConfig["state_topic"] = this->stateTopic;
  if (config.unit.length() > 0)
  {
    jsonConfig["unit_of_measurement"] = config.unit;
  }
  jsonConfig["unique_id"] = this->uniqueId;
  if (this->settable)
  {
    jsonConfig["command_topic"] = this->commandTopic;
  }
  JsonObject jsonDevice = jsonConfig["device"].to<JsonObject>();
  JsonArray jsonIds = jsonDevice["identifiers"].to<JsonArray>();
  jsonIds.add(device.id);
  // if device information is shared between multiple entities, the device name must be included in each entity's device configuration
  jsonDevice["name"] = I::get().config().paramDeviceName;
  if (!device.added)
  {
    if (device.model.length() > 0)
    {
      jsonDevice["model"] = device.model;
    }
    if (device.swVersion.length() > 0)
    {
      jsonDevice["sw_version"] = device.swVersion;
    }
    if (device.configurationUrl.length() > 0)
    {
      jsonDevice["configuration_url"] = device.configurationUrl;
    }
    device.added = true;
  }
}

uint16_t HAProperty::publishConfig(EWC::Mqtt &mqtt)
{
  I::get().logger() << F("[MqttHA]: publish configuration for ") << uniqueId << F(" to ") << this->discoveryTopic << endl;
  // publish config
  String output;
  serializeJson(this->jsonConfig, output);
  I::get().logger() << F("[MqttHA]: ") << output << endl;
  return mqtt.client().publish(this->discoveryTopic.c_str(), 2, false, output.c_str());
}

MqttHA::MqttHA()
{
  _ewcMqtt = nullptr;
}

MqttHA::~MqttHA()
{
}

void MqttHA::setup(EWC::Mqtt &mqtt, String deviceId, String deviceName, String model)
{
  _ewcMqtt = &mqtt;
  _properties.clear();
  _propertyConfigs.clear();
  _mqttDevice = HADevice();
  _mqttDevice.id = deviceId;
  _mqttDevice.name = deviceName;
  _mqttDevice.model = model;
  _mqttDevice.serialNumber = I::get().config().getChipId();
  _mqttDevice.swVersion = I::get().server().version();

  mqtt.client().onMessage(std::bind(&MqttHA::_onMqttMessage, this, std::placeholders::_1, std::placeholders::_2,
                                    std::placeholders::_3, std::placeholders::_4,
                                    std::placeholders::_5, std::placeholders::_6));
  mqtt.client().onConnect(std::bind(&MqttHA::_onMqttConnect, this, std::placeholders::_1));
  mqtt.client().onPublish(std::bind(&MqttHA::_onMqttAck, this, std::placeholders::_1));
}

bool MqttHA::_hasProperty(String uniqueId)
{
  for (auto itn = _propertyConfigs.begin(); itn != _propertyConfigs.end(); itn++)
  {
    if (itn->uniqueId.compareTo(uniqueId) == 0)
    {
      return true;
    }
  }
  return false;
}

bool MqttHA::addProperty(String component, String uniqueId, String name, String deviceClass, String objectId, String unit, bool retained)
{
  if (_hasProperty(uniqueId))
  {
    I::get().logger() << F("✘ [MqttHA] can not add property with id: ") << uniqueId << F(", already exists!") << endl;
    return false;
  }
  HAPropertyConfig hp(component, uniqueId, name, deviceClass, objectId, unit, retained);
  _propertyConfigs.push_back(hp);
  I::get().logger() << F("[MqttHA] added property with id: ") << uniqueId << endl;
  return true;
}

bool MqttHA::addPropertySettable(String component, String uniqueId, String name, String deviceClass, String objectId, AsyncMqttClientInternals::OnMessageUserCallback callback, String unit, bool retained)
{
  if (_hasProperty(uniqueId))
  {
    I::get().logger() << F("✘ [MqttHA] can not add settable property with id: ") << uniqueId << F(", already exists!") << endl;
    return false;
  }
  HAPropertyConfig hp(component, uniqueId, name, deviceClass, objectId, callback, unit, retained);
  _propertyConfigs.push_back(hp);
  I::get().logger() << F("[MqttHA] added settable property with id: ") << uniqueId << endl;
  return true;
}

void MqttHA::_onMqttConnect(bool sessionPresent)
{
  _properties.clear();
  I::get().logger() << F("[MqttHA] setup on connect, session present: ") << sessionPresent << endl;
  I::get().logger() << "[MqttHA]: ESP heap: _onMqttConnect: " << ESP.getFreeHeap() << endl;
  String prefix = _ewcMqtt->paramDiscoveryPrefix;
  prefix.replace("/", "");
  if (prefix.length() == 0)
  {
    prefix = "homeassistant";
  }

  _mqttDevice.configurationUrl = "http://" + WiFi.localIP().toString();

  // create properties from config
  for (auto itn = _propertyConfigs.begin(); itn != _propertyConfigs.end(); itn++)
  {
    _properties.push_back(HAProperty(prefix, *itn, _mqttDevice));
  }
  _statusTopic = prefix + "/status";
  // String statusValue = "online";
  // _ewcMqtt->client().publish(_statusTopic.c_str(), 1, true, statusValue.c_str());
  _ewcMqtt->client().subscribe(_statusTopic.c_str(), 2);
  // publish first configuration
  _waitForPacketId = -1;
  _idxPublishConfig = 0;
  _waitForPacketId = _properties[_idxPublishConfig].publishConfig(*_ewcMqtt);
  _properties[_idxPublishConfig].publishedConfig = true;
  if (!_properties[_idxPublishConfig].settable)
  {
    _idxPublishConfig++;
  }
}

void MqttHA::publishState(String uniqueId, String value, bool retain, uint8_t qos)
{
  for (auto itc = _properties.begin(); itc != _properties.end(); itc++)
  {
    if (strcmp(uniqueId.c_str(), itc->uniqueId.c_str()) == 0)
    {
      I::get().logger() << F("[MqttHA] publish ") << value << F(" to ") << itc->stateTopic << endl;
      _ewcMqtt->client().publish(itc->stateTopic.c_str(), qos, retain, value.c_str());
    }
  }
}

void MqttHA::_onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  I::get().logger() << F("[MqttHA] onMqttMessage; topic: ") << topic << F("; payload: ") << payload << endl;
  for (auto itc = _properties.begin(); itc != _properties.end(); itc++)
  {
    if (strcmp((char *)topic, itc->commandTopic.c_str()) == 0)
    {
      itc->callback(topic, payload, properties, len, index, total);
    }
  }
  if (_statusTopic.compareTo(topic) == 0 && String("online").compareTo(payload) == 0)
  {
    I::get().logger() << F("[MqttHA] republish configuration; topic: ") << topic << F("; payload: ") << payload << endl;
    // publish configuration
    _waitForPacketId = -1;
    _idxPublishConfig = 0;
    _waitForPacketId = _properties[_idxPublishConfig].publishConfig(*_ewcMqtt);
    _properties[_idxPublishConfig].publishedConfig = true;
    if (!_properties[_idxPublishConfig].settable)
    {
      _idxPublishConfig++;
    }
  }
}

void MqttHA::_onMqttAck(uint16_t packetId)
{
  I::get().logger() << F("[MqttHA]: received ack for ") << packetId << endl;
  if (packetId == _waitForPacketId)
  {
    if (_idxPublishConfig < _properties.size())
    {
      if (!_properties[_idxPublishConfig].publishedConfig)
      {
        I::get().logger() << F("[MqttHA] publish config for ") << _properties[_idxPublishConfig].stateTopic << endl;
        _waitForPacketId = _properties[_idxPublishConfig].publishConfig(*_ewcMqtt);
        _properties[_idxPublishConfig].publishedConfig = true;
        if (!_properties[_idxPublishConfig].settable)
        {
          _idxPublishConfig++;
        }
      }
      else
      {
        I::get().logger() << F("[MqttHA] subscribe to ") << _properties[_idxPublishConfig].commandTopic << endl;
        _waitForPacketId = _ewcMqtt->client().subscribe(_properties[_idxPublishConfig].commandTopic.c_str(), 2);
        _idxPublishConfig++;
      }
    }
    else
    {
      // I::get().logger() << F("[MqttHA]: all registration topics sent, current free memory: ") << ESP.getFreeHeap() << endl;
      // _propertyConfigs.clear();
      // I::get().logger() << F("[MqttHA]: free memory after clear: ") << ESP.getFreeHeap() << endl;
    }
  }
}
