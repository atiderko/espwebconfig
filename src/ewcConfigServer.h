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
#ifndef EWC_CONFIG_SERVER_H
#define EWC_CONFIG_SERVER_H

#if defined(ESP8266)
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#else
#include <WiFi.h>
#include "esp_wps.h"
#define ESP_WPS_MODE WPS_TYPE_PBC
#endif
#include <ESPAsyncWebServer.h>

#ifdef USE_EADNS
#include <ESPAsyncDNSServer.h>    //https://github.com/devyte/ESPAsyncDNSServer
                                  //https://github.com/me-no-dev/ESPAsyncUDP
typedef ESPAsyncDNSServer DNSServer;
#else
#include <DNSServer.h>
#endif
#include "ewcConfigFS.h"
#include "ewcLogger.h"
#include "ewcRTC.h"
#include "ewcConfig.h"

const char PROGMEM_CONFIG_APPLICATION_JS[] PROGMEM = "application/javascript";
const char PROGMEM_CONFIG_APPLICATION_JSON[] PROGMEM = "application/json";
const char PROGMEM_CONFIG_TEXT_HTML[] PROGMEM = "text/html";
const char PROGMEM_CONFIG_TEXT_CSS[] PROGMEM = "text/css";

namespace EWC {

struct MenuItem
{
    String name;
    String link;
    String entry_id;
    bool visible;
};


class ConfigServer
{
public:
    ConfigServer(uint16_t port=80);
    void insertMenu(const char* name, const char* uri, const char* entry_id, bool visible=true, std::size_t position=255);
    void insertMenuCb(const char* name, const char* uri, const char* entry_id, ArRequestHandlerFunction onRequest, bool visible=true, std::size_t position=255);
    void setup();
    bool isAP() { return _ap_address.isSet(); }
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    void loop();
    Config& config() { return _config; }
    AsyncWebServer& webserver() { return _server; }

private:
    AsyncWebServer _server;
    DNSServer _dnsServer;
    ConfigFS _configFS;
    Config _config;
    Logger _logger;
    RTC _rtc;
    std::vector<MenuItem> _menu;
    IPAddress _ap_address;

    void _startAP();
    void _connect(const char* ssid=nullptr, const char* pass=nullptr);
    /** === web handler === **/
    String _token_STATION_STATUS(bool failedOnly);
    void _sendMenu(AsyncWebServerRequest *request);
    void _sendFileContent(AsyncWebServerRequest *request, const String& filename, const String& contentType);
    void _sendContent_P(AsyncWebServerRequest *request, PGM_P content, const String& contentType);
    void _onWiFiConnect(AsyncWebServerRequest *request);
    void _onWiFiDisconnect(AsyncWebServerRequest *request);
    void _onWifiState(AsyncWebServerRequest *request);
    void _onWifiScan(AsyncWebServerRequest* request);
    void _onNotFound(AsyncWebServerRequest* request);

    unsigned long _msConnectStart    = 0;
    unsigned long _msConfigPortalStart    = 0;
    unsigned long _msConfigPortalTimeout  = 300000;
    unsigned long _msConnectTimeout       = 60000;

    IPAddress     _ap_static_ip;
    IPAddress     _ap_static_gw;
    IPAddress     _ap_static_sn;
    IPAddress     _sta_static_ip;
    IPAddress     _sta_static_gw;
    IPAddress     _sta_static_sn;
    IPAddress     _sta_static_dns1= (uint32_t)0x00000000;
    IPAddress     _sta_static_dns2= (uint32_t)0x00000000;
    bool       captivePortal(AsyncWebServerRequest*);

    // DNS server
    const byte    DNS_PORT = 53;
    //helpers
    bool isIp(const String& str);
    String        toStringIp(const IPAddress& ip);

    template <class T>
    auto _optionalIPFromString(T *obj, const char *s) -> decltype(  obj->fromString(s)  ) {
        return  obj->fromString(s);
    }
    auto _optionalIPFromString(...) -> bool {
        Serial.println("NO fromString METHOD ON IPAddress, you need ESP8266 core 2.1.0 or newer for Custom IP configuration to work.");
        return false;
    }
};
};
#endif