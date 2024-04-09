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

#include "ewcMqtt.h"
#include "ewcConfigServer.h"
#include <ArduinoJSON.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include "generated/mqttSetupHTML.h"
#include "generated/mqttStateHTML.h"

using namespace EWC;

Mqtt::Mqtt(String prefix) : ConfigInterface("mqtt"), _defaultPrefix(prefix)
{
}

Mqtt::~Mqtt()
{
}

void Mqtt::setup(JsonDocument &config, bool resetConfig)
{
  I::get().logger() << F("[EWC Mqtt] setup") << endl;
  _initParams();
  _fromJson(config);
  _initMqtt();
  EWC::I::get().server().insertMenuG("MQTT", "/mqtt/setup", "menu_mqtt", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_MQTT_SETUP_GZIP, sizeof(HTML_MQTT_SETUP_GZIP), true, 0);
  EWC::I::get().server().webServer().on("/mqtt/config.json", std::bind(&Mqtt::_onMqttConfig, this, &EWC::I::get().server().webServer()));
  EWC::I::get().server().webServer().on("/mqtt/config/save", std::bind(&Mqtt::_onMqttSave, this, &EWC::I::get().server().webServer()));
  EWC::I::get().server().webServer().on("/mqtt/state.html", std::bind(&ConfigServer::sendContentG, &EWC::I::get().server(), &EWC::I::get().server().webServer(), FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_MQTT_STATE_GZIP, sizeof(HTML_MQTT_STATE_GZIP)));
  EWC::I::get().server().webServer().on("/mqtt/state.json", std::bind(&Mqtt::_onMqttState, this, &EWC::I::get().server().webServer()));
  _mqttClient.onMessage(std::bind(&Mqtt::_messageReceived, this, std::placeholders::_1, std::placeholders::_2));
#ifdef ESP8266
  _wifiConnectHandler = WiFi.onStationModeGotIP(std::bind(&Mqtt::_onWifiConnect, this, std::placeholders::_1));
  _wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&Mqtt::_onWifiDisconnect, this, std::placeholders::_1));
#else
  WiFi.onEvent(std::bind(&Mqtt::_onWifiConnect, this, std::placeholders::_1, std::placeholders::_2), WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(std::bind(&Mqtt::_onWifiDisconnect, this, std::placeholders::_1, std::placeholders::_2), WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#endif
}

void Mqtt::loop()
{
  if (_paramEnabled)
  {
    _mqttClient.loop();
    if (!_mqttClient.connected())
    {
      if (_connectionStateLast)
      {
        // reconnect in 7 seconds
        _reconnectTs = millis() + 7000;
      }
      if (_reconnectTs == 0 || millis() > _reconnectTs)
      {
        _reconnectTs = 0;
        _connectToMqtt();
      }
    }
    if (_messages.size() > 0)
    {
      if (_cbOnMessage)
      {
        Message msg = _messages.at(0);
        _cbOnMessage(msg.topic, msg.payload);
        _messages.erase(_messages.begin());
      }
    }
    if (_lastSendAcks.size() > 0)
    {
      if (_cbOnFakeAck)
      {
        uint16_t packetId = _lastSendAcks.at(0);
        _cbOnFakeAck(packetId);
        _lastSendAcks.erase(_lastSendAcks.begin());
      }
    }
  }
  _connectionStateLast = _mqttClient.connected();
}

void Mqtt::fillJson(JsonDocument &config)
{
  config["mqtt"]["enabled"] = _paramEnabled;
  config["mqtt"]["server"] = _paramServer;
  config["mqtt"]["port"] = _paramPort;
  config["mqtt"]["user"] = _paramUser;
  config["mqtt"]["pass"] = _paramPassword;
  config["mqtt"]["prefix"] = _paramDiscoveryPrefix;
  config["mqtt"]["send_interval"] = _paramSendInterval;
}

void Mqtt::_initParams()
{
  _paramEnabled = false;
  _paramServer = "";
  _paramPort = 1883;
  _paramUser = "";
  _paramPassword = "";
  _paramDiscoveryPrefix = _defaultPrefix;
  _paramSendInterval = 0;
}

void Mqtt::_initMqtt()
{
  // disconnect before (re-)init mqtt client
  if (_mqttClient.connected())
  {
    _mqttClient.disconnect();
  }

  if (_paramEnabled)
  {
    _mqttClient.begin(_paramServer.c_str(), _paramPort, _net);
  }
}

void Mqtt::_fromJson(JsonDocument &config)
{
  bool paramEnabled = false;
  JsonVariant jv = config["mqtt"]["enabled"];
  if (!jv.isNull())
  {
    paramEnabled = jv.as<bool>();
  }
  _paramEnabled = paramEnabled;
  jv = config["mqtt"]["server"];
  if (!jv.isNull())
  {
    _paramServer = jv.as<String>();
  }
  jv = config["mqtt"]["port"];
  if (!jv.isNull())
  {
    _paramPort = jv.as<int>();
  }
  jv = config["mqtt"]["user"];
  if (!jv.isNull())
  {
    _paramUser = jv.as<String>();
  }
  jv = config["mqtt"]["pass"];
  if (!jv.isNull())
  {
    _paramPassword = jv.as<String>();
  }
  jv = config["mqtt"]["prefix"];
  if (!jv.isNull())
  {
    _paramDiscoveryPrefix = jv.as<String>();
  }
  jv = config["mqtt"]["send_interval"];
  if (!jv.isNull())
  {
    _paramSendInterval = jv.as<int>();
  }
}

void Mqtt::_onMqttConfig(WebServer *request)
{
  if (!I::get().server().isAuthenticated(request))
  {
    return request->requestAuthentication();
  }
  JsonDocument jsonDoc;
  fillJson(jsonDoc);
  String output;
  serializeJson(jsonDoc, output);
  request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void Mqtt::_onMqttSave(WebServer *request)
{
  if (!I::get().server().isAuthenticated(request))
  {
    return request->requestAuthentication();
  }
  for (int i = 0; i < request->args(); i++)
  {
    I::get().logger() << "  " << request->argName(i) << ": " << request->arg(i) << endl;
  }
  JsonDocument config;
  bool mqtt_enabled = false;
  if (request->hasArg("mqtt_enabled"))
  {
    mqtt_enabled = request->arg("mqtt_enabled").equals("true");
  }
  config["mqtt"]["enabled"] = mqtt_enabled;
  if (request->hasArg("mqtt_server"))
  {
    config["mqtt"]["server"] = request->arg("mqtt_server");
  }
  if (request->hasArg("mqtt_port") && !request->arg("mqtt_port").isEmpty())
  {
    config["mqtt"]["port"] = request->arg("mqtt_port").toInt();
  }
  if (request->hasArg("mqtt_user"))
  {
    config["mqtt"]["user"] = request->arg("mqtt_user");
  }
  if (request->hasArg("mqtt_pass"))
  {
    config["mqtt"]["pass"] = request->arg("mqtt_pass");
  }
  if (request->hasArg("mqtt_prefix"))
  {
    config["mqtt"]["prefix"] = request->arg("mqtt_prefix");
  }
  if (request->hasArg("mqtt_send_interval") && !request->arg("mqtt_send_interval").isEmpty())
  {
    config["mqtt"]["send_interval"] = request->arg("mqtt_send_interval").toInt();
  }
  _fromJson(config);
  I::get().configFS().save();
  String details;
  serializeJsonPretty(config["mqtt"], details);
  I::get().server().sendPageSuccess(request, "EWC MQTT save", "Save successful!", "/mqtt/setup", "<pre id=\"json\">" + details + "</pre>", "Back", "/mqtt/state.html", "MQTT State");
  _initMqtt();
}

void Mqtt::_onMqttState(WebServer *request)
{
  if (!I::get().server().isAuthenticated(request))
  {
    return request->requestAuthentication();
  }
  JsonDocument jsonDoc;
  jsonDoc["enabled"] = _paramEnabled;
  jsonDoc["connecting"] = _paramEnabled && !_mqttClient.connected() && (_reconnectTs == 0 || millis() > _reconnectTs);
  jsonDoc["connected"] = _mqttClient.connected();
  jsonDoc["failed"] = false;
  jsonDoc["reason"] = "";
  jsonDoc["server"] = _paramServer;
  jsonDoc["port"] = _paramPort;
  jsonDoc["send_interval"] = _paramSendInterval;
  String output;
  serializeJson(jsonDoc, output);
  request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void Mqtt::_connectToMqtt()
{
  if (!_paramEnabled)
    return;
  if (_mqttClient.connected())
  {
    return;
  }
  I::get().logger() << F("[EWC MQTT] Connecting to MQTT...") << _paramServer << ":" << _paramPort << endl;
  bool connectResult = _mqttClient.connect("", _paramUser.c_str(), _paramPassword.c_str());
  if (connectResult)
  {
    I::get().logger() << F("[EWC MQTT] connected to MQTT ") << _paramServer << ":" << _paramPort << endl;
    if (_cbConnected)
    {
      _cbConnected();
    }
  }
  else
  {
    // reconnect in 7 seconds
    _reconnectTs = millis() + 7000;
  }
}

#ifdef ESP8266
void Mqtt::_onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  I::get().logger() << F("[EWC MQTT] Connected to Wi-Fi.") << endl;
  _connectToMqtt();
}

void Mqtt::_onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  I::get().logger() << F("[EWC MQTT] Disconnected from Wi-Fi.") << endl;
  _reconnectTs = 0; // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
}
#else
void Mqtt::_onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info)
{
  I::get().logger() << F("[EWC MQTT] Connected to Wi-Fi.") << endl;
  _connectToMqtt();
}

void Mqtt::_onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info)
{
  I::get().logger() << F("[EWC MQTT] Disconnected from Wi-Fi.") << endl;
  _reconnectTs = 0; // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
}
#endif

bool Mqtt::subscribe(const String &topic, int qos)
{
  return _mqttClient.subscribe(topic, qos);
}

uint16_t Mqtt::publish(const String &topic, const String &payload, bool retained, int qos)
{
  bool result = _mqttClient.publish(topic, payload, retained, qos);
  if (result)
  {
    uint16_t packetId = 0;
    if (qos > 0)
    {
      packetId = _mqttClient.lastPacketID();
      _lastSendAcks.push_back(packetId);
    }
    return packetId;
  }
  I::get().logger() << F("[EWC MQTT] failed to send message, error: ") << _mqttClient.lastError() << endl;
  return 0;
}

void Mqtt::_messageReceived(String &topic, String &payload)
{
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
  _messages.push_back(Message(topic, payload));
}