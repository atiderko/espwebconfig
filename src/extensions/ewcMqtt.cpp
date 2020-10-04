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
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include "generated/mqttSetupHTML.h"
#include "generated/mqttConnectHTML.h"

using namespace EWC;

Mqtt::Mqtt() : ConfigInterface("mqtt")
{
}

Mqtt::~Mqtt()
{
}

void Mqtt::setup(JsonDocument& config, bool resetConfig)
{
    I::get().logger() << F("[EWC Mqtt] setup") << endl;
    _initParams();
    _fromJson(config);
    EWC::I::get().server().insertMenuP("MQTT", "/mqtt/setup", "menu_mqtt", HTML_MQTT_SETUP, FPSTR(PROGMEM_CONFIG_TEXT_HTML), true, 0);
    EWC::I::get().server().webserver().on("/mqtt/config.json", std::bind(&Mqtt::_onMqttConfig, this, std::placeholders::_1));
    EWC::I::get().server().webserver().on("/mqtt/save", std::bind(&Mqtt::_onMqttSave, this, std::placeholders::_1));
    EWC::I::get().server().webserver().on("/mqtt/state.json", std::bind(&Mqtt::_onMqttState, this, std::placeholders::_1));
    EWC::I::get().server().webserver().on("/mqtt/connect", std::bind(&ConfigServer::sendContentP, &EWC::I::get().server(), std::placeholders::_1, HTML_MQTT_CONNECT, FPSTR(PROGMEM_CONFIG_TEXT_HTML)));
    _mqttClient.onConnect(std::bind(&Mqtt::_onMqttConnect, this, std::placeholders::_1));
    _mqttClient.onDisconnect(std::bind(&Mqtt::_onMqttDisconnect, this, std::placeholders::_1));
    _wifiConnectHandler = WiFi.onStationModeGotIP(std::bind(&Mqtt::_onWifiConnect, this, std::placeholders::_1));
    _wifiDisconnectHandler = WiFi.onStationModeDisconnected(std::bind(&Mqtt::_onWifiDisconnect, this, std::placeholders::_1));
}

void Mqtt::fillJson(JsonDocument& config)
{
    config["mqtt"]["enabled"] = paramEnabled;
    config["mqtt"]["server"] = _paramServer;
    config["mqtt"]["port"] = _paramPort;
    config["mqtt"]["user"] = _paramUser;
    config["mqtt"]["pass"] = _paramPassword;
    config["mqtt"]["prefix"] = paramDiscoveryPrefix;
}

void Mqtt::_initParams()
{
    paramEnabled = false;
    _paramServer = "";
    _paramPort = 1883;
    _paramUser = "";
    _paramPassword = "";
    paramDiscoveryPrefix = "ewc";
}

void Mqtt::_fromJson(JsonDocument& config)
{
    JsonVariant jv = config["mqtt"]["enabled"];
    if (!jv.isNull()) {
        paramEnabled = jv.as<bool>();
    }
    jv = config["mqtt"]["server"];
    if (!jv.isNull()) {
        _paramServer = jv.as<String>();
    }
    jv = config["mqtt"]["port"];
    if (!jv.isNull()) {
        _paramPort = jv.as<int>();
    }
    jv = config["mqtt"]["user"];
    if (!jv.isNull()) {
        _paramUser = jv.as<String>();
    }
    jv = config["mqtt"]["pass"];
    if (!jv.isNull()) {
        _paramPassword = jv.as<String>();
    }
    jv = config["mqtt"]["prefix"];
    if (!jv.isNull()) {
        paramDiscoveryPrefix = jv.as<String>();
    }
    if (paramEnabled) {
        // set contact informations
        _mqttClient.setServer(_paramServer.c_str(), _paramPort);
        if (_paramUser.length() > 0) {
            const char* pass = nullptr;
            if (_paramPassword.length() > 0) {
                pass = _paramPassword.c_str();
            }
            _mqttClient.setCredentials(_paramUser.c_str(), pass);
        }
        // reconnect
        if (_mqttClient.connected()) {
            _mqttClient.disconnect(true);
        }
        _mqttReconnectTimer.once(1, std::bind(&Mqtt::_connectToMqtt, this));
    }
}

void Mqtt::_onMqttConfig(AsyncWebServerRequest *request)
{
    if (!I::get().server().isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    DynamicJsonDocument jsonDoc(512);
    fillJson(jsonDoc);
    String output;
    serializeJson(jsonDoc, output);
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void Mqtt::_onMqttSave(AsyncWebServerRequest *request)
{
    if (!I::get().server().isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    for (size_t i = 0; i < request->params(); i++) {
        I::get().logger() << "  " << request->argName(i) << ": " << request->arg(i) << endl;
    }
    DynamicJsonDocument config(512);
    if (request->hasArg("mqtt_enabled")) {
        bool a = request->arg("mqtt_enabled").equals("true");
        config["mqtt"]["enabled"] = a;
        I::get().logger() << "  _onMqttSave, enabled " << a << " -> "<< request->arg("mqtt_enabled")<< endl;
    }
    if (request->hasArg("mqtt_server")) {
        config["mqtt"]["server"] = request->arg("mqtt_server");
    }
    if (request->hasArg("mqtt_port") && !request->arg("mqtt_port").isEmpty()) {
        config["mqtt"]["port"] = request->arg("mqtt_port").toInt();
    }
    if (request->hasArg("mqtt_user")) {
        config["mqtt"]["user"] = request->arg("mqtt_user");
    }
    if (request->hasArg("mqtt_pass")) {
        config["mqtt"]["pass"] = request->arg("mqtt_pass");
    }
    if (request->hasArg("mqtt_prefix")) {
        config["mqtt"]["prefix"] = request->arg("mqtt_prefix");
    }
    _fromJson(config);
    I::get().configFS().save();
    String details;
    serializeJsonPretty(config["mqtt"], details);
    I::get().server().sendPageSuccess(request, "EWC MQTT save", "Save successful!", "/mqtt/setup", "<pre id=\"json\">" + details + "</pre>", "Back", "/mqtt/connect", "MQTT State");
}

void Mqtt::_onMqttState(AsyncWebServerRequest *request)
{
    if (!I::get().server().isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    DynamicJsonDocument jsonDoc(512);
    jsonDoc["enabled"] = paramEnabled;
    jsonDoc["connecting"] = _connecting;
    jsonDoc["connected"] = _mqttClient.connected();
    jsonDoc["failed"] = false;
    jsonDoc["reason"] = "";
    jsonDoc["server"] = _paramServer;
    jsonDoc["port"] = _paramPort;
    String output;
    serializeJson(jsonDoc, output);
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}


void Mqtt::_connectToMqtt() {
    I::get().logger() << F("[EWC MQTT] Connecting to MQTT...") << endl;
    _connecting = true;
    _mqttClient.connect();
}

void Mqtt::_onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    I::get().logger() << F("[EWC MQTT] Disconnected from MQTT, reason: ") << (uint32_t)reason << endl;
    if (WiFi.isConnected() && (uint32_t)reason < 4) {
        _mqttReconnectTimer.once(2, std::bind(&Mqtt::_connectToMqtt, this));
    }
}

void Mqtt::_onMqttConnect(bool sessionPresent) {
    I::get().logger() << F("[EWC MQTT] Connected to MQTT. Session present: ") << sessionPresent << endl;
    _connecting = false;
}

void Mqtt::_onWifiConnect(const WiFiEventStationModeGotIP& event) {
  I::get().logger() << F("[EWC MQTT] Connected to Wi-Fi.") << endl;
  _connectToMqtt();
}

void Mqtt::_onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  I::get().logger() << F("[EWC MQTT] Disconnected from Wi-Fi.") << endl;
  _mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
}

