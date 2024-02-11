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

#ifdef ESP8266
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #define WebServer ESP8266WebServer
    #include "ewcRTC.h"
#elif defined(ESP32)
    #include <WiFi.h>
    #include <WebServer.h>
#endif
typedef std::function<void ()> WebServerHandlerFunction;

#include <DNSServer.h>
#include "ewcConfigFS.h"
#include "ewcLogger.h"
#include "ewcConfig.h"
#include "ewcTickerLED.h"
#include "ewcInterface.h"

namespace EWC {

const char PROGMEM_CONFIG_APPLICATION_JS[] PROGMEM = "application/javascript; charset=utf-8";
const char PROGMEM_CONFIG_APPLICATION_JSON[] PROGMEM = "application/json; charset=utf-8";
const char PROGMEM_CONFIG_TEXT_HTML[] PROGMEM = "text/html; charset=utf-8";
const char PROGMEM_CONFIG_TEXT_CSS[] PROGMEM = "text/css; charset=utf-8";

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
    /** Add your own pages to menu. 
     * @param name: displayed name in menu
     * @param uri:  called if menu was clicked
     * @param entry_id: the html element id, is used for translation
     * @param visible: true, if menu item is visible
     * @param position: use negativ or positiv value to shift this menu item on insert. It is only relative to previous inserted items.
     * @param onRequest: callback on menu item
     * @param content: send content on menu item **/
    void insertMenu(const char* name, const char* uri, const char* entry_id, bool visible=true, int position=255);
    void insertMenuCb(const char* name, const char* uri, const char* entry_id, WebServerHandlerFunction onRequest, bool visible=true, int position=255);
    void insertMenuP(const char* name, const char* uri, const char* entry_id, const String& contentType, PGM_P content, bool visible=true, int position=255);
    void insertMenuG(const char* name, const char* uri, const char* entry_id, const String& contentType, const uint8_t* content, size_t len, bool visible=true, int position=255);
    void insertMenuNoAuthP(const char* name, const char* uri, const char* entry_id, const String& contentType, PGM_P content, bool visible=true, int position=255);
    void insertMenuNoAuthG(const char* name, const char* uri, const char* entry_id, const String& contentType, const uint8_t* content, size_t len, bool visible=true, int position=255);
    /** Grands access to configuration under "/ewc/config". The result is a JSON object.
     * Call enableConfigUri() before setup to enabled access. **/
    void enableConfigUri() { _publicConfig = true; }
    /** The configuration portal (AP) will be disabled after (Default: 5 Minutes).
     * You can increase the timeout with setTimeoutPortal() or disable the shutdown
     * of AP with disablePortalTimeout(). **/
    void disablePortalTimeout() { setTimeoutPortal(0); }
    void setTimeoutPortal(uint32_t seconds) { _msConfigPortalTimeout = seconds * 1000; }
    /** After connect timeout a configuration portal will be open. **/
    void setTimeoutConnect(uint32_t seconds) { _msConnectTimeout = seconds * 1000; }
    /** Sets brand and version of the firmware.
     * Brand is description on the left side in the menu. **/
    void setBrand(const char* brand, const char* version="not-set");
    void setBrandUri(const char* uri) { _branduri = uri; }
    String& brand() { return _brand; }
    String& version() { return _version; }

    void setup();
    void loop();
    /** Returns true if AP is enabled and IP of the AP is valid. **/
    bool isAP() { 
#ifdef ESP8266
        return _ap_address.isSet();
#else
        return _ap_address != 0;
#endif
    }
    /** Returns true if device is connected to an AP. **/
    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    Config& config() { return _config; }
    TickerLed& led() { return _led; }
    WebServer& webserver() { return _server; }
    /** Sends content to the client. The authentication is carried out before send depending on the configuration. **/
    void sendContentP(WebServer* request, const String& contentType, PGM_P content);
    void sendContentG(WebServer* request, const String& contentType, const uint8_t* content, size_t len);
    /** Creates a page with successfull result.**/
    void sendPageSuccess(WebServer* request, String title, String summary, String urlBack, String details="", String nameBack="Back", String urlForward="/", String nameForward="Home");
    /** Creates a page with failed result. **/
    void sendPageFailed(WebServer* request, String title, String summary, String urlBack, String details="", String nameBack="Back", String urlForward="/", String nameForward="Home");
    /** Send header with redirect to given url. **/
    void sendRedirect(WebServer* request, String url);
    /** Returns true if the client is authenticated. **/
    bool isAuthenticated(WebServer* request);

protected:
    WebServer _server;
    DNSServer _dnsServer;
    ConfigFS _configFS;
    Logger _logger;
#ifdef ESP8266
    RTC _rtc;
#endif
    TickerLed _led;
    Config _config;
    String _brand;
    String _version;
    String _branduri;
    std::vector<MenuItem> _menu;
    IPAddress _ap_address;
    static PGM_P wlStatusSymbols[];
    bool _publicConfig;
    bool _connected_wifi;
    bool _ap_disabled_after_timeout;  //< once disabled the portal will open only after reboot or error after successful connect
    int _softAPClientCount;  //< do not disabled Config Portal if one is connected
    uint8_t _disconnect_state;
    String _disconnect_reason;

    unsigned long _msConnectStart    = 0;
    unsigned long _msWifiScanStart   = 0;
    unsigned long _msConfigPortalStart    = 0;
    unsigned long _msConfigPortalTimeout  = 300000;
    unsigned long _msConnectTimeout       = 60000;

    // IPAddress _ap_static_ip;
    // IPAddress _ap_static_gw;
    // IPAddress _ap_static_sn;
    IPAddress _sta_static_ip;
    IPAddress _sta_static_gw;
    IPAddress _sta_static_sn;
    IPAddress _sta_static_dns1= (uint32_t)0x00000000;
    IPAddress _sta_static_dns2= (uint32_t)0x00000000;

    // DNS server
    const byte    DNS_PORT = 53;

    bool _captivePortal(WebServer* request);
    void _connect(const char* ssid=nullptr, const char* pass=nullptr);
    void _startAP();
    void _startWiFiScan(bool force=false);
    /** === web handler === **/
    String _token_WIFI_MODE();
    void _sendMenu(WebServer* request);
    void _sendFileContent(WebServer* request, const String& contentType, const String& filename);
    void _sendContentNoAuthP(WebServer* request, const String& contentType, PGM_P content);
    void _sendContentNoAuthG(WebServer* request, const String& contentType, const uint8_t* content, size_t len);
    void _onAccessGet(WebServer* request);
    void _onAccessSave(WebServer* request);
    void _onGetInfo(WebServer* request);
    void _onWiFiConnect(WebServer* request);
    void _onWiFiDisconnect(WebServer* request);
    void _onWifiState(WebServer* request);
    void _onWifiScan(WebServer* request);
    void _onNotFound(WebServer* request);

    /** === WiFi handler === **/
#if defined(ESP8266)
    void _wifiOnStationModeConnected(const WiFiEventStationModeConnected& event);
    void _wifiOnStationModeDisconnected(const WiFiEventStationModeDisconnected& event);
    // void _wifiOnStationModeAuthModeChanged(const WiFiEventStationModeAuthModeChanged& event);
    // void _wifiOnStationModeGotIP(const WiFiEventStationModeGotIP& event);
    // void _wifiOnStationModeDHCPTimeout();
    void _wifiOnSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& event);
    void _wifiOnSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& event);
#else
    // events for ESP32
    void _wifiOnStationModeConnected(WiFiEvent_t event, WiFiEventInfo_t info);
    void _wifiOnStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
    void _wifiOnSoftAPModeStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
    void _wifiOnSoftAPModeStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
#endif
    //helpers
    static String _toMACAddressString(const uint8_t mac[]);
    bool _isIp(const String& str);
    String _toStringIp(const IPAddress& ip);

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
