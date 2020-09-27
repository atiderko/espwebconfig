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
#include "ewcTickerLED.h"
#include "ewcInterface.h"

namespace EWC {

const char PROGMEM_CONFIG_APPLICATION_JS[] PROGMEM = "application/javascript";
const char PROGMEM_CONFIG_APPLICATION_JSON[] PROGMEM = "application/json";
const char PROGMEM_CONFIG_TEXT_HTML[] PROGMEM = "text/html";
const char PROGMEM_CONFIG_TEXT_CSS[] PROGMEM = "text/css";

struct MenuItem
{
    String name;
    String link;
    String entry_id;
    bool visible;
};


typedef enum {
    EWC_STATION_IDLE = 0,
    EWC_STATION_CONNECTING,
    EWC_STATION_CONNECTED,  // ESP32
    EWC_STATION_GOT_IP,
    EWC_STATION_SCAN_COMPLETED,  // ESP32
    EWC_STATION_WRONG_PASSWORD,
    EWC_STATION_NO_AP_FOUND,
    EWC_STATION_CONNECT_FAIL,
    EWC_STATION_CONNECTION_LOST,  // ESP32
    EWC_STATION_DISCONNECTED,  // ESP32
    EWC_STATION_NO_SHIELD  // ESP32
} wifi_status_t;


class ConfigServer
{
public:
    ConfigServer(uint16_t port=80);
    void insertMenu(const char* name, const char* uri, const char* entry_id, bool visible=true, int position=255);
    void insertMenuCb(const char* name, const char* uri, const char* entry_id, ArRequestHandlerFunction onRequest, bool visible=true, int position=255);
    void insertMenuP(const char* name, const char* uri, const char* entry_id, PGM_P content, const String& contentType, bool visible=true, int position=255);
    void insertMenuNoAuthP(const char* name, const char* uri, const char* entry_id, PGM_P content, const String& contentType, bool visible=true, int position=255);
    void setup();
    bool isAP() { return _ap_address.isSet(); }
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    void loop();
    void setBrand(const char* brand, const char* version="not-set");
    void setBrandUri(const char* uri) { _branduri = uri; }
    String& brand() { return _brand; }
    String& version() { return _version; }
    Config& config() { return _config; }
    AsyncWebServer& webserver() { return _server; }
    void sendContentP(AsyncWebServerRequest* request, PGM_P content, const String& contentType);
    void sendPageSuccess(AsyncWebServerRequest *request, String title, String summary, String urlBack, String details="", String nameBack="Back", String urlForward="/", String nameForward="Home");
    void sendPageFailed(AsyncWebServerRequest *request, String title, String summary, String urlBack, String details="", String nameBack="Back", String urlForward="/", String nameForward="Home");
    bool isAuthenticated(AsyncWebServerRequest *request);
    void disabledConfigUri() { _publicConfig = false; }

private:
    AsyncWebServer _server;
    DNSServer _dnsServer;
    ConfigFS _configFS;
    Logger _logger;
    RTC _rtc;
    TickerLed _led;
    Config _config;
    String _brand;
    String _version;
    String _branduri;
    std::vector<MenuItem> _menu;
    IPAddress _ap_address;
    static PGM_P wlStatusSymbols[];
    bool _publicConfig;

    void _startAP();
    void _connect(const char* ssid=nullptr, const char* pass=nullptr);
    /** === web handler === **/
    String _token_WIFI_MODE();
    String _token_STATION_STATUS(bool failedOnly);
    wifi_status_t _station_status();
    void _sendMenu(AsyncWebServerRequest* request);
    void _sendFileContent(AsyncWebServerRequest* request, const String& filename, const String& contentType);
    void _sendContentNoAuthP(AsyncWebServerRequest* request, PGM_P content, const String& contentType);
    void _onAccessGet(AsyncWebServerRequest* request);
    void _onAccessSave(AsyncWebServerRequest* request);
    void _onGetInfo(AsyncWebServerRequest* request);
    void _onWiFiConnect(AsyncWebServerRequest* request);
    void _onWiFiDisconnect(AsyncWebServerRequest* request);
    void _onWifiState(AsyncWebServerRequest* request);
    void _onWifiScan(AsyncWebServerRequest* request);
    void _onNotFound(AsyncWebServerRequest* request);

    /** === WiFi handler === **/
#if defined(ESP8266)
    void _wifiOnStationModeConnected(const WiFiEventStationModeConnected& event);
    void _wifiOnStationModeDisconnected(const WiFiEventStationModeDisconnected& event);
    void _wifiOnStationModeAuthModeChanged(const WiFiEventStationModeAuthModeChanged& event);
    void _wifiOnStationModeGotIP(const WiFiEventStationModeGotIP& event);
    void _wifiOnStationModeDHCPTimeout();
    void _wifiOnSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& event);
    void _wifiOnSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& event);
#else
// TODO: events for ESP32
#endif
    static String _toMACAddressString(const uint8_t mac[]);
    void _startWiFiScan(bool force=false);

    unsigned long _msConnectStart    = 0;
    unsigned long _msWifiScanStart   = 0;
    unsigned long _msConfigPortalStart    = 0;
    unsigned long _msConfigPortalTimeout  = 300000;
    unsigned long _msConnectTimeout       = 60000;

    // IPAddress     _ap_static_ip;
    // IPAddress     _ap_static_gw;
    // IPAddress     _ap_static_sn;
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
    auto _optionalIPFromString(T* obj, const char* s) -> decltype(  obj->fromString(s)  ) {
        return  obj->fromString(s);
    }
    auto _optionalIPFromString(...) -> bool {
        Serial.println("NO fromString METHOD ON IPAddress, you need ESP8266 core 2.1.0 or newer for Custom IP configuration to work.");
        return false;
    }
};
};
#endif