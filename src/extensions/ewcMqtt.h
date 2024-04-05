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

// See AsyncMqttClient: https://github.com/marvinroger/async-mqtt-client
// When using many mqtt messages, we had to struggle with stability problems.
// We have therefore switched to
// https://github.com/256dpi/arduino-mqtt.git
// The publish() method only blocks for QOS > 0.
// In order not to block the loop() too much when sending many messages,
// onFakeAck() can be used. See sending the configuration in MqttHA.

#ifndef EWC_MQTT_h
#define EWC_MQTT_h

#if defined(ESP8266)
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#define WebServer ESP8266WebServer
#elif defined(ESP32)
#include "WiFi.h"
#include "WebServer.h"
#include <vector>
#endif
#include <Arduino.h>
#include <MQTT.h>
#include "../ewcConfigInterface.h"

namespace EWC
{

  class Mqtt : public ConfigInterface
  {
  public:
    typedef std::function<void()> MqttConnectedFunction;
    typedef std::function<void(String &topic, String &payload)> MqttMessageFunction;
    typedef std::function<void(uint16_t packetId)> MqttAck;
    class Message
    {
    public:
      String topic;
      String payload;
      Message(String &topic, String &payload)
      {
        this->topic = topic;
        this->payload = payload;
      }
    };

    Mqtt(String prefix = "ewc");
    ~Mqtt();

    MQTTClient &client() { return _mqttClient; }
    /** === ConfigInterface Methods === **/
    void setup(JsonDocument &config, bool resetConfig = false);
    void fillJson(JsonDocument &config);

    void loop();

    /** Getter **/
    String &getDiscoveryPrefix() { return _paramDiscoveryPrefix; }

    /** Callbacks **/
    void onConnected(MqttConnectedFunction callback) { _cbConnected = callback; }
    void onMessage(MqttMessageFunction callback) { _cbOnMessage = callback; }
    void onFakeAck(MqttAck callback) { _cbOnFakeAck = callback; }

    /** Functions **/
    bool subscribe(const String &topic, int qos);
    /** Returns packet id of message sent. The id is only valid for qos > 0.
     * In the next loop() we take the first packet id and call onFakeAck(). **/
    uint16_t publish(const String &topic, const String &payload, bool retained = false, int qos = 0);

  protected:
    WiFiClient _net;
    MQTTClient _mqttClient;
    unsigned long _reconnectTs = 0;
#ifdef ESP8266
    WiFiEventHandler _wifiConnectHandler;
    WiFiEventHandler _wifiDisconnectHandler;
#endif

    /** === Parameter === **/
    bool _paramEnabled;
    String _defaultPrefix;
    String _paramDiscoveryPrefix;
    String _paramServer;
    uint16_t _paramPort;
    String _paramUser;
    String _paramPassword;

    /** === Callbacks === **/
    MqttConnectedFunction _cbConnected;
    MqttMessageFunction _cbOnMessage;
    MqttAck _cbOnFakeAck;

    void _initParams();
    void _initMqtt();
    void _fromJson(JsonDocument &config);
    void _onMqttConfig(WebServer *request);
    void _onMqttState(WebServer *request);
    void _onMqttSave(WebServer *request);

#if defined(ESP8266)
    void _onWifiConnect(const WiFiEventStationModeGotIP &event);
    void _onWifiDisconnect(const WiFiEventStationModeDisconnected &event);
#else
    void _onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info);
    void _onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info);
#endif

    void _connectToMqtt();
    void _messageReceived(String &topic, String &payload);

  private:
    bool _connectionStateLast = false;
    std::vector<uint16_t> _lastSendAcks;
    std::vector<Message> _messages;
  };
};
#endif
