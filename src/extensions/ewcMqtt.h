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

// See AsynMqttClient: https://github.com/marvinroger/async-mqtt-client

#ifndef EWC_MQTT_h
#define EWC_MQTT_h

#if defined(ESP8266)
    #include "ESP8266WiFi.h"
    #include "ESP8266WebServer.h"
    # define WebServer ESP8266WebServer
#elif defined(ESP32)
    #include "WiFi.h"
    #include "WebServer.h"
#endif
#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include "../ewcConfigInterface.h"


namespace EWC {

class Mqtt : public ConfigInterface
{
public:
    Mqtt();
    ~Mqtt();

    AsyncMqttClient& client() { return _mqttClient; }

    /** === ConfigInterface Methods === **/    
    void setup(JsonDocument& config, bool resetConfig=false);
    void fillJson(JsonDocument& config);
    bool paramEnabled;
    String paramDiscoveryPrefix;

protected:
    AsyncMqttClient _mqttClient;
    Ticker _mqttReconnectTimer;
#ifdef ESP8266
    WiFiEventHandler _wifiConnectHandler;
    WiFiEventHandler _wifiDisconnectHandler;
#endif


    /** === Parameter === **/
    String _paramServer;
    uint16_t _paramPort;
    String _paramUser;
    String _paramPassword;


    void _initParams();
    void _fromJson(JsonDocument& config);
    void _onMqttConfig(WebServer* request);
    void _onMqttState(WebServer* request);
    void _onMqttSave(WebServer* request);

    #if defined(ESP8266)
    void _onWifiConnect(const WiFiEventStationModeGotIP& event);
    void _onWifiDisconnect(const WiFiEventStationModeDisconnected& event);
    #else
    void _onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info);
    void _onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info);
    #endif

    void _connectToMqtt();
    void _onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    void _onMqttConnect(bool sessionPresent);

private:
    bool _connecting;
    bool _failed;
    String _failedReason;
};

};
#endif
