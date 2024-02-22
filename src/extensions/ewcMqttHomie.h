/**************************************************************

This file is a part of
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
#ifndef BBS_MQTT_HOMIE_H
#define BBS_MQTT_HOMIE_H

#include <Arduino.h>
#include "ewcMqtt.h"
#include "../ewcConfig.h"

namespace EWC
{

  const char SEP[2] = "/";

  struct HomieProperty
  {
    String id;
    String name;
    String datatype;

    String format;
    String unit;
    bool retained;
    bool settable;
  };

  struct HomieNode
  {
    String id;
    String name;
    std::vector<HomieProperty> properties;

    String type;
  };

  struct HomieDevice
  {
    String prefix;
    String id;
    String name;
    String state;
    std::vector<HomieNode> nodes;
  };

  /** Stores the callback for the settable topic. */
  class CallbackTopic
  {
  public:
    AsyncMqttClientInternals::OnMessageUserCallback callback;
    String topic;

    CallbackTopic(String nodeId, String propertyId, AsyncMqttClientInternals::OnMessageUserCallback callbackFunc)
    {
      _nodeId = nodeId;
      _propertyId = propertyId;
      callback = callbackFunc;
    }

    void createTopic(String homiePrefix, String deviceId)
    {
      topic = homiePrefix + SEP + deviceId + SEP + _nodeId + SEP + _propertyId + SEP + "set";
    }

  protected:
    String _nodeId;
    String _propertyId;
  };

  /** Stores configuration topic, which is send after connected to a broker. */
  class MqttConfigTopic
  {
  public:
    MqttConfigTopic(String topic, String value, uint8_t qos, bool retain);
    ~MqttConfigTopic();

    uint16_t publish(EWC::Mqtt &mqtt);

  private:
    String _topic;
    String _value;
    uint8_t _qos;
    bool _retain;
  };

  class MqttHomie
  {
  public:
    MqttHomie();
    ~MqttHomie();

    void setup(EWC::Mqtt &mqtt, String deviceName, String deviceId = "ewc-" + I::get().config().getChipId());
    /** Adds a node to the device. */
    bool addNode(String id, String name, String type = "");
    /** Adds property to an existing node. The node should be inserted first. */
    bool addProperty(String nodeId, String propertyId, String name, String datatype, String format = "", String unit = "", bool retained = true);
    /** Adds a settable property to an existing node. The node should be inserted first. */
    bool addPropertySettable(String nodeId, String propertyId, String name, String datatype, AsyncMqttClientInternals::OnMessageUserCallback callback, String format = "", String unit = "", bool retained = true);
    /** Publishes a value a property. */
    void publishState(String nodeId, String propertyId, String value, bool retain = false, uint8_t qos = 1);

  protected:
    EWC::Mqtt *_ewcMqtt;
    String _homieStateTopic;
    std::vector<CallbackTopic> _callbacks;
    HomieDevice _homieDevice;

    // variables for publishing of configuration
    std::vector<MqttConfigTopic> _configTopics;
    uint32_t _idxPublishConfig;
    uint16_t _waitForPacketId;

    void _onMqttConnect(bool sessionPresent);
    void _onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    void _onMqttAck(uint16_t packetId);
  };
};
#endif
