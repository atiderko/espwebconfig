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

#include "ewcConfigServer.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "ewcInterface.h"
#include "generated/webBaseCSS.h"
#include "generated/webIndexHTML.h"
#include "generated/webPostloadJS.h"
#include "generated/webWifiiconsCSS.h"
#include "generated/wifiConnectHTML.h"
#include "generated/wifiSetupHTML.h"

using namespace EWC;

ConfigServer::ConfigServer(uint16_t port) :_server(80)
{
    ETCI::get()._server = this;
    ETCI::get()._configFS = &_configFS;
    ETCI::get()._logger = &_logger;
    ETCI::get()._rtc = &_rtc;
    _configFS.addConfig(_config);
}

void ConfigServer::setup()
{
    _configFS.setup();
    if (!_config.paramWifiDisabled){
        if (_config.paramAPStartAlways) {
            WiFi.mode(WIFI_AP_STA);
            _startAP();
        } else {
            WiFi.mode(WIFI_STA);
        }
        _connect();
        WiFi.scanNetworks(true);
    } else {
        WiFi.mode(WIFI_OFF);
    }
    // _server.reset(); why do we need this?
    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
    _server.on("/", std::bind(&ConfigServer::_sendContent_P, this, std::placeholders::_1, HTML_WIFI_SETUP, FPSTR(PROGMEM_CONFIG_TEXT_HTML))).setFilter(ON_AP_FILTER);
    _server.on("/bbs", std::bind(&ConfigServer::_sendContent_P, this, std::placeholders::_1, HTML_WEB_INDEX, FPSTR(PROGMEM_CONFIG_TEXT_HTML))).setFilter(ON_AP_FILTER);
    _server.on("/menu", std::bind(&ConfigServer::_sendMenu, this, std::placeholders::_1));
    _server.on("/css/base.css", std::bind(&ConfigServer::_sendContent_P, this, std::placeholders::_1, CSS_WEB_BASE, FPSTR(PROGMEM_CONFIG_TEXT_CSS)));
    _server.on("/css/wifi_icons.css", std::bind(&ConfigServer::_sendContent_P, this, std::placeholders::_1, CSS_WEB_WIFIICONS, FPSTR(PROGMEM_CONFIG_TEXT_CSS)));
    _server.on("/js/postload.js", std::bind(&ConfigServer::_sendContent_P, this, std::placeholders::_1, JS_WEB_POSTLOAD, FPSTR(PROGMEM_CONFIG_APPLICATION_JS)));
    // _server.on("/js/postload.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     // if (ESPUI.basicAuth && !request->authenticate(ESPUI.basicAuthUsername, ESPUI.basicAuthPassword))
    //     // {
    //     //   return request->requestAuthentication();
    //     // }
    //     ETCI::get().logger() << "send postload js: " << ESP.getFreeHeap() << endl;
    //     request->send_P(200, "application/javascript", JS_POSTLOAD);
    //     ETCI::get().logger() << "finished postload js: " << ESP.getFreeHeap() << endl;
    // });
    _server.rewrite("/wifi", "/wifi/setup");
    insertMenuCb("WiFi", "/wifi/setup", "menu_wifi", std::bind(&ConfigServer::_sendContent_P, this, std::placeholders::_1, HTML_WIFI_SETUP, FPSTR(PROGMEM_CONFIG_TEXT_HTML)));
    _server.on("/wifi/connect", std::bind(&ConfigServer::_onWiFiConnect, this, std::placeholders::_1));
    _server.on("/wifi/connecting", std::bind(&ConfigServer::_sendContent_P, this, std::placeholders::_1, HTML_WIFI_CONNECT, FPSTR(PROGMEM_CONFIG_TEXT_HTML)));
    _server.on("/wifi/disconnect", std::bind(&ConfigServer::_onWiFiDisconnect, this, std::placeholders::_1));  //.setFilter(ON_AP_FILTER);
    _server.on("/wifi/state", std::bind(&ConfigServer::_onWifiState, this, std::placeholders::_1));
    _server.on("/wifi/stations", std::bind(&ConfigServer::_onWifiScan, this, std::placeholders::_1));
    _server.on("/fwlink", std::bind(&ConfigServer::_sendContent_P, this, std::placeholders::_1, HTML_WIFI_SETUP, FPSTR(PROGMEM_CONFIG_TEXT_HTML))).setFilter(ON_AP_FILTER);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    _server.on("/favicon.ico", std::bind(&ConfigServer::_sendFileContent, this, std::placeholders::_1, "/favicon.ico", "image/x-icon"));
    _server.onNotFound (std::bind(&ConfigServer::_onNotFound,this,std::placeholders::_1));
    _server.begin(); // Web server start
}

void ConfigServer::_startAP() {
    ETCI::get().logger() << endl;
    _msConfigPortalStart = millis();
    ETCI::get().logger() << F("Configuring access point... ") << _config.paramAPName << endl;
    //optional soft ip config
    // if (_ap_static_ip) {
    //     DEBUG_WM(F("Custom AP IP/GW/Subnet"));
    //     WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn);
    // }
    if (!_config.getAPPass().isEmpty()) {
        WiFi.softAP(_config.paramAPName.c_str(), _config.getAPPass().c_str());
    } else {
        WiFi.softAP(_config.paramAPName.c_str());
    }
    delay(500); // Without delay I've seen the IP address blank
    _ap_address = WiFi.softAPIP();
    ETCI::get().logger() << F("AP IP address: ") << _ap_address << endl;
    /* Setup the DNS server redirecting all the domains to the apIP */
    _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer.start(DNS_PORT, "*", _ap_address);
    ETCI::get().logger() << F("HTTP server started") << endl;
}

void ConfigServer::_connect(const char* ssid, const char* pass)
{
    ETCI::get().logger() << F("Connecting as wifi client... ssid: '") << ssid << "', pass: '" << pass << "'" << endl;
    _msConnectStart = millis();
    // check if we've got static_ip settings, if we do, use those.
    if (_sta_static_ip) {
        ETCI::get().logger() << F("Custom STA IP/GW/Subnet/DNS") << endl;
        WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns1, _sta_static_dns2);
        ETCI::get().logger() << F("  Local IP: ") << WiFi.localIP() << endl;
    }
    if (ssid != nullptr && strlen(ssid) > 0) {
        //trying to fix connection in progress hanging
        ETCI::get().logger() << F("Try to connect with new credentials with SSID: ") << ssid << endl;
        WiFi.disconnect(false);
        WiFi.begin(ssid, pass);
    } else {
        ETCI::get().logger() << F("Try to connect with saved credentials for SSID: ") << WiFi.SSID() << endl;
        WiFi.begin();
    }
}

void ConfigServer::insertMenu(const char* name, const char* uri, const char* entry_id, bool visible, std::size_t position)
{
    MenuItem item;
    item.name = name;
    item.link = uri;
    item.entry_id = entry_id;
    item.visible = visible;
    if (position == 255 || position >= _menu.size()) {
        _menu.push_back(item);
    } else {
        _menu.insert(_menu.begin() + position, item);
    }
}

void ConfigServer::insertMenuCb(const char* name, const char* uri, const char* entry_id, ArRequestHandlerFunction onRequest, bool visible, std::size_t position)
{
  insertMenu(name, uri, entry_id, visible, position);
  _server.on(uri, onRequest);
}

void ConfigServer::_onWiFiConnect(AsyncWebServerRequest *request)
{
    ETCI::get().logger() << "ESP heap: _onWiFiConnect: " << ESP.getFreeHeap() << endl;
    for (size_t idx = 0; idx < request->args(); idx++) {
        ETCI::get().logger() << idx << ": " << request->argName(idx) << request->arg(idx) << endl;
    }
    const char* ssid = request->arg("ssid").c_str();
    const char* pass = request->arg("passphrase").c_str();
    if (request->hasArg("staip")) {
        ETCI::get().logger() << F("static ip: ") << request->arg("staip") << endl;
        //_sta_static_ip.fromString(request->arg("ip"));
        String ip = request->arg("staip");
        _optionalIPFromString(&_sta_static_ip, ip.c_str());
    }
    if (request->hasArg("gtway")) {
        ETCI::get().logger() << F("static gateway") << request->arg("gtway") << endl;
        String gw = request->arg("gtway");
        _optionalIPFromString(&_sta_static_gw, gw.c_str());
    }
    if (request->hasArg("ntmsk")) {
        ETCI::get().logger() << F("static netmask") << request->arg("ntmsk") << endl;
        String sn = request->arg("ntmsk");
        _optionalIPFromString(&_sta_static_sn, sn.c_str());
    }
    if (request->hasArg("dns1")) {
        ETCI::get().logger() << F("static DNS 1") << request->arg("dns1") << endl;
        String dns1 = request->arg("dns1");
        _optionalIPFromString(&_sta_static_dns1, dns1.c_str());
    }
    if (request->hasArg("dns2")) {
        ETCI::get().logger() << F("static DNS 2") << request->arg("dns2") << endl;
        String dns2 = request->arg("dns2");
        _optionalIPFromString(&_sta_static_dns2, dns2.c_str());
    }
    _connect(ssid, pass);
    ETCI::get().logger() << "ESP heap: _onWiFiConnect: " << ESP.getFreeHeap() << endl;
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
    response->addHeader("Location", "/wifi/connecting");
    request->send ( response);
}

void ConfigServer::_onWiFiDisconnect(AsyncWebServerRequest *request)
{
    ETCI::get().logger() << "ESP heap: _onWiFiDisconnect: " << ESP.getFreeHeap() << endl;
    AsyncWebServerResponse *response = request->beginResponse(302,"text/plain","");
    response->addHeader("Location", "/wifi/setup");
    request->send ( response);
    ETCI::get().logger() << "ESP heap: _onWiFiDisconnect: " << ESP.getFreeHeap() << endl;
    WiFi.disconnect(false);
}


String ConfigServer::_token_STATION_STATUS(bool failedOnly)
{
    PGM_P wlStatusSymbol = PSTR("");
    PGM_P wlStatusSymbols[] = {
#if defined(ARDUINO_ARCH_ESP8266)
        PSTR("IDLE"),
        PSTR("CONNECTING"),
        PSTR("WRONG_PASSWORD"),
        PSTR("NO_AP_FOUND"),
        PSTR("CONNECT_FAIL"),
        PSTR("GOT_IP")
    };
    switch (wifi_station_get_connect_status()) {
        case STATION_IDLE:
            wlStatusSymbol = wlStatusSymbols[0];
            break;
        case STATION_CONNECTING:
            if (!failedOnly)
                wlStatusSymbol = wlStatusSymbols[1];
            break;
        case STATION_WRONG_PASSWORD:
            wlStatusSymbol = wlStatusSymbols[2];
            break;
        case STATION_NO_AP_FOUND:
            wlStatusSymbol = wlStatusSymbols[3];
            break;
        case STATION_CONNECT_FAIL:
            wlStatusSymbol = wlStatusSymbols[4];
            break;
        case STATION_GOT_IP:
            if (!failedOnly)
                wlStatusSymbol = wlStatusSymbols[5];
            break;
#elif defined(ARDUINO_ARCH_ESP32)
      PSTR("IDLE"),
      PSTR("NO_SSID_AVAIL"),
      PSTR("SCAN_COMPLETED"),
      PSTR("CONNECTED"),
      PSTR("CONNECT_FAILED"),
      PSTR("CONNECTION_LOST"),
      PSTR("DISCONNECTED"),
      PSTR("NO_SHIELD")
    };
    switch (_rsConnect) {
        case WL_IDLE_STATUS:
            wlStatusSymbol = wlStatusSymbols[0];
            break;
        case WL_NO_SSID_AVAIL:
            wlStatusSymbol = wlStatusSymbols[1];
            break;
        case WL_SCAN_COMPLETED:
            if (!failedOnly)
                wlStatusSymbol = wlStatusSymbols[2];
            break;
        case WL_CONNECTED:
            if (!failedOnly)
                wlStatusSymbol = wlStatusSymbols[3];
            break;
        case WL_CONNECT_FAILED:
            wlStatusSymbol = wlStatusSymbols[4];
            break;
        case WL_CONNECTION_LOST:
            wlStatusSymbol = wlStatusSymbols[5];
            break;
        case WL_DISCONNECTED:
            if (!failedOnly)
                wlStatusSymbol = wlStatusSymbols[6];
            break;
        case WL_NO_SHIELD:
            wlStatusSymbol = wlStatusSymbols[7];
            break;
#endif
    }
    return String(FPSTR(wlStatusSymbol));
}

void ConfigServer::_sendMenu(AsyncWebServerRequest *request) {
    ETCI::get().logger() << "ESP heap: _sendMenu: " << ESP.getFreeHeap() << endl;
    int n = WiFi.scanComplete();
    Serial.println(n);
    DynamicJsonDocument jsonDoc(2048);
    JsonObject json = jsonDoc.to<JsonObject>();
    json["brand"] = "BBS";
    JsonArray elements = json.createNestedArray("elements");
    std::vector<MenuItem>::iterator it;
    for (it = _menu.begin(); it != _menu.end(); it++) {
        JsonObject jsonElements = elements.createNestedObject();
        jsonElements["name"] = it->name;
        jsonElements["id"] = it->entry_id;
        jsonElements["href"] = it->link;
        jsonElements["visible"] = it->visible;
    }
    String output;
    serializeJson(json, output);
    ETCI::get().logger() << "ESP heap: _sendMenu: " << ESP.getFreeHeap() << endl;
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::_onWifiState(AsyncWebServerRequest *request)
{
    ETCI::get().logger() << "ESP heap: _onWifiState: " << ESP.getFreeHeap() << endl;
    //int n = WiFi.scanComplete();
    DynamicJsonDocument jsonDoc(2048);
    JsonObject json = jsonDoc.to<JsonObject>();
    json["ssid"] = WiFi.SSID();
    json["connected"] = WiFi.status() == WL_CONNECTED;
    String reason = _token_STATION_STATUS(true);
    json["failed"] = reason.length() > 0;
    json["reason"] = reason;
    String output;
    serializeJson(json, output);
    ETCI::get().logger() << "ESP heap: _onWifiState: " << ESP.getFreeHeap() << endl;
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void _wiFiState2Json(JsonObject& json, bool finished, bool failed, const char* reason)
{
    json["finished"] = finished;
    json["failed"] = failed;
    json["reason"] = reason;
}

void ConfigServer::_onWifiScan(AsyncWebServerRequest *request)
{
    ETCI::get().logger() << "ESP heap: _onWifiScan: " << ESP.getFreeHeap() << endl;
    int n = WiFi.scanComplete();
    DynamicJsonDocument jsonDoc(2048);
    JsonObject json = jsonDoc.to<JsonObject>();
    if (n == WIFI_SCAN_FAILED) {
        ETCI::get().logger() << F("scanNetworks returned: WIFI_SCAN_FAILED!") << endl;
        _wiFiState2Json(json, true, true, "scanNetworks returned: WIFI_SCAN_FAILED!");
	  } else if(n == WIFI_SCAN_RUNNING) {
        ETCI::get().logger() << F("scanNetworks returned: WIFI_SCAN_RUNNING!") << endl;
        _wiFiState2Json(json, false, false, "");
	  } else if(n < 0) {
        ETCI::get().logger() << F("scanNetworks failed with unknown error code!") << endl;
        _wiFiState2Json(json, true, true, "scanNetworks failed with unknown error code!");
	  } else if (n == 0) {
        ETCI::get().logger() << F("No networks found") << endl;
        _wiFiState2Json(json, true, true, "No networks found");
    }
    if (n > 0) {
        _wiFiState2Json(json, true, false, "");
        JsonArray networks = json.createNestedArray("networks");
        std::set<String> ssids;
        String ssid;
        uint8_t encType;
        int32_t rssi;
        uint8_t* bssid;
        int32_t channel;
        bool isHidden;
        for (uint8_t i = 0; i < n; i++) {
            bool result = WiFi.getNetworkInfo(i, ssid, encType, rssi, bssid, channel, isHidden);
            // do not add the same station twice
            if (result && !ssid.isEmpty() && ssids.find(ssid.c_str()) == ssids.end()) {
                Serial.println(ssid.c_str());
                JsonObject jsonNetwork = networks.createNestedObject();
                jsonNetwork["ssid"] = ssid;
                jsonNetwork["encrypted"] = encType == ENC_TYPE_NONE ? false : true;
                jsonNetwork["rssi"] = rssi;
                jsonNetwork["channel"] = channel;
                char mac[18] = { 0 };
                sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
                jsonNetwork["bssid"] = mac;
                jsonNetwork["hidden"] = isHidden ? true : false;
                ssids.insert(ssid);
            }
        }
        String output;
        serializeJson(json, output);
        ETCI::get().logger() << "ESP heap: _onWifiScan: " << ESP.getFreeHeap() << endl;
        Serial.println(output);
        request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
        ETCI::get().logger() << "ESP heap: _onWifiScan: " << ESP.getFreeHeap() << endl;
        //WiFi.scanDelete();
        return;
    } else if (n == -2) {
        WiFi.scanNetworks(true);
    }
    String output;
    json["finished"] = "false";
    serializeJson(json, output);
    ETCI::get().logger() << "ESP heap: _onWifiScan: " << ESP.getFreeHeap() << endl;
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::loop()
{
#ifndef USE_EADNS	
    _dnsServer.processNextRequest();
#endif
    if (!_config.paramWifiDisabled) {
        if (WiFi.status() != WL_CONNECTED) {
            bool startAP = false;
            // check for timeout
            if (millis() - _msConnectStart > _msConnectTimeout) {
                startAP = true;
            } else {
                // check for errors
                String res = _token_STATION_STATUS(true);
                if (!res.isEmpty()) {
                    startAP = true;
                }
            }
            if  (startAP && !isAP()) {
                // start AP
                WiFi.mode(WIFI_AP_STA);
                _startAP();
            }
        }
    }
}

void ConfigServer::_sendFileContent(AsyncWebServerRequest *request, const String& filename, const String& contentType)
{
    ETCI::get().logger() << F("Handle sendFileContent: ") << filename << endl;
    File reqFile = LittleFS.open(filename, "r");
    if (reqFile && reqFile.isFile()) {
        request->send(LittleFS, filename, contentType);
        reqFile.close();
        return;
    }
    _onNotFound(request);
}

void ConfigServer::_sendContent_P(AsyncWebServerRequest *request, PGM_P content, const String& contentType)
{
    ETCI::get().logger() << "send " << request->url() << ":" << ESP.getFreeHeap() << endl;
    request->send_P(200, contentType.c_str(), content);
    ETCI::get().logger() << "finished " << request->url() << ":" << ESP.getFreeHeap() << endl;
}

void ConfigServer::_onNotFound(AsyncWebServerRequest *request) {
    ETCI::get().logger() << F("Handle not found") << endl;
    if (captivePortal(request)) { // If captive portal redirect instead of displaying the error page.
        return;
    }
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += ( request->method() == HTTP_GET ) ? "GET" : "POST";
    message += "\nArguments: ";
    message += request->args();
    message += "\n";
    for ( uint8_t i = 0; i < request->args(); i++ ) {
        message += " " + request->argName ( i ) + ": " + request->arg ( i ) + "\n";
    }
    AsyncWebServerResponse *response = request->beginResponse(404,"text/plain",message);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send (response );
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
bool ConfigServer::captivePortal(AsyncWebServerRequest *request)
{
    if (!isIp(request->host()) ) {
        ETCI::get().logger() << F("Request host is not an IP: ") << request->host() << endl;
        ETCI::get().logger() << F("Request redirected to captive portal: ") << toStringIp(request->client()->localIP()) << endl;
        AsyncWebServerResponse *response = request->beginResponse(302,"text/plain","");
        response->addHeader("Location", String("http://") + toStringIp(request->client()->localIP()));
        request->send ( response);
        return true;
    }
    return false;
}

//start up config portal callback
// void ConfigServer::setAPCallback( void (*func)(ConfigServer* myESPAsyncWebConf) ) {
//   _apcallback = func;
// }

//start up save config callback
// void ConfigServer::setSaveConfigCallback( void (*func)(void) ) {
//   _savecallback = func;
// }


/** Is this an IP? */
bool ConfigServer::isIp(const String& str)
{
    for (unsigned int i = 0; i < str.length(); i++) {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9')) {
            return false;
        }
    }
    return true;
}

/** IP to String? */
String ConfigServer::toStringIp(const IPAddress& ip)
{
    String res = "";
    for (int i = 0; i < 3; i++) {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}