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
#include "generated/ewcInfoHTML.h"
#include "generated/ewcSecurityHTML.h"
#include "generated/webBaseCSS.h"
#include "generated/webIndexHTML.h"
#include "generated/webPostloadJS.h"
#include "generated/webPreJS.h"
#include "generated/webTableCSS.h"
#include "generated/webWifiiconsCSS.h"
#include "generated/webWifiJS.h"
#include "generated/wifiConnectHTML.h"
#include "generated/wifiSetupHTML.h"
#include "ewcPages.h"

using namespace EWC;

InterfaceData I::_interface;  // need to define the static variable


PGM_P ConfigServer::wlStatusSymbols[] = {
    "IDLE",
    "CONNECTING",
    "CONNECTED",
    "GOT_IP",
    "SCAN_COMPLETED",
    "WRONG_PASSWORD",
    "NO_AP_FOUND",
    "CONNECT_FAIL",
    "CONNECTION_LOST",
    "DISCONNECTED",
    "NO_SHIELD"
};

WiFiEventHandler p1;
WiFiEventHandler p2;
WiFiEventHandler p3;
WiFiEventHandler p4;
WiFiEventHandler p5;
WiFiEventHandler p6;
WiFiEventHandler p7;

ConfigServer::ConfigServer(uint16_t port)
  : _server(port),
  _brand("ESP Web Config")
{

    I::get()._server = this;
    I::get()._configFS = &_configFS;
    I::get()._logger = &_logger;
    I::get()._rtc = &_rtc;
    _configFS.addConfig(_config);
}

void ConfigServer::setup()
{
    I::get().logger() << F("[EWC CS]: setup configFS") << endl;
    _configFS.setup();
    if (_configFS.resetDetected()) {
        WiFi.disconnect(true);
    }
    I::get().logger() << F("[EWC CS]: setup wifi events") << endl;
#if defined(ESP8266)
    p1 = WiFi.onStationModeConnected(std::bind(&ConfigServer::_wifiOnStationModeConnected, this, std::placeholders::_1));
    p2 = WiFi.onStationModeDisconnected(std::bind(&ConfigServer::_wifiOnStationModeDisconnected, this, std::placeholders::_1));
    p3 = WiFi.onStationModeAuthModeChanged(std::bind(&ConfigServer::_wifiOnStationModeAuthModeChanged, this, std::placeholders::_1));
    p4 = WiFi.onStationModeGotIP(std::bind(&ConfigServer::_wifiOnStationModeGotIP, this, std::placeholders::_1));
    p5 = WiFi.onStationModeDHCPTimeout(std::bind(&ConfigServer::_wifiOnStationModeDHCPTimeout, this));
    p6 = WiFi.onSoftAPModeStationConnected(std::bind(&ConfigServer::_wifiOnSoftAPModeStationConnected, this, std::placeholders::_1));
    p7 = WiFi.onSoftAPModeStationDisconnected(std::bind(&ConfigServer::_wifiOnSoftAPModeStationDisconnected, this, std::placeholders::_1));
#else
// TODO: events for ESP32
#endif
    I::get().logger() << F("[EWC CS]: configure urls") << endl;
    if (!_config.paramWifiDisabled){
        if (_config.paramAPStartAlways || _config.getBootMode() == BootMode::CONFIGURATION || _config.getBootMode() == BootMode::STANDALONE) {
            WiFi.mode(WIFI_AP_STA);
            _startAP();
        } else if (_config.getBootMode() != BootMode::STANDALONE) {
            WiFi.mode(WIFI_STA);
            delay(500);
        }
        _connect();
        _startWiFiScan();
    } else {
        WiFi.mode(WIFI_OFF);
    }
    // _server.reset(); do we need this?
    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
    _server.on("/", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, HTML_WIFI_SETUP, FPSTR(PROGMEM_CONFIG_TEXT_HTML))).setFilter(ON_AP_FILTER);
    _server.on("/menu", std::bind(&ConfigServer::_sendMenu, this, std::placeholders::_1));
    _server.on("/css/base.css", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, CSS_WEB_BASE, FPSTR(PROGMEM_CONFIG_TEXT_CSS)));
    _server.on("/css/table.css", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, CSS_WEB_TABLE, FPSTR(PROGMEM_CONFIG_TEXT_CSS)));
    _server.on("/css/wifiicons.css", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, CSS_WEB_WIFIICONS, FPSTR(PROGMEM_CONFIG_TEXT_CSS)));
    _server.on("/js/postload.js", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, JS_WEB_POSTLOAD, FPSTR(PROGMEM_CONFIG_APPLICATION_JS)));
    _server.on("/js/pre.js", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, JS_WEB_PRE, FPSTR(PROGMEM_CONFIG_APPLICATION_JS)));
    _server.on("/js/wifi.js", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, JS_WEB_WIFI, FPSTR(PROGMEM_CONFIG_APPLICATION_JS)));
    _server.rewrite("/wifi", "/wifi/setup");
    insertMenuCb("WiFi", "/wifi/setup", "menu_wifi", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, HTML_WIFI_SETUP, FPSTR(PROGMEM_CONFIG_TEXT_HTML)));
    _server.on("/wifi/connect", std::bind(&ConfigServer::_onWiFiConnect, this, std::placeholders::_1));
    _server.on("/wifi/disconnect", std::bind(&ConfigServer::_onWiFiDisconnect, this, std::placeholders::_1));  //.setFilter(ON_AP_FILTER);
    _server.on("/wifi/state.json", std::bind(&ConfigServer::_onWifiState, this, std::placeholders::_1));
    _server.on("/wifi/stations.json", std::bind(&ConfigServer::_onWifiScan, this, std::placeholders::_1));
    insertMenuCb("Security", "/ewc/security", "menu_security", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, HTML_EWC_SECURITY, FPSTR(PROGMEM_CONFIG_TEXT_HTML)));
    _server.on("/ewc/security.json", std::bind(&ConfigServer::_onSecurityGet, this, std::placeholders::_1));
    _server.on("/ewc/secsave", std::bind(&ConfigServer::_onSecuritySave, this, std::placeholders::_1));
    insertMenuCb("Info", "/ewc/info", "menu_info", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, HTML_EWC_INFO, FPSTR(PROGMEM_CONFIG_TEXT_HTML)));
    _server.on("/ewc/info.json", std::bind(&ConfigServer::_onGetInfo, this, std::placeholders::_1));
    _server.on("/ewc/config", std::bind(&ConfigServer::_sendFileContent, this, std::placeholders::_1, FPSTR(CONFIG_FILENAME), FPSTR(PROGMEM_CONFIG_APPLICATION_JS)));
    _server.on("/fwlink", std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, HTML_WIFI_SETUP, FPSTR(PROGMEM_CONFIG_TEXT_HTML))).setFilter(ON_AP_FILTER);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    _server.on("/favicon.ico", std::bind(&ConfigServer::_sendFileContent, this, std::placeholders::_1, "/favicon.ico", "image/x-icon"));
    _server.onNotFound (std::bind(&ConfigServer::_onNotFound,this,std::placeholders::_1));
    _server.begin(); // Web server start
}

void ConfigServer::_startAP() {
    I::get().logger() << endl;
    _msConfigPortalStart = millis();
    I::get().logger() << F("[EWC CS]: Configuring access point... ") << _config.paramAPName << endl;
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
    I::get().logger() << F("[EWC CS]: AP IP address: ") << _ap_address << endl;
    /* Setup the DNS server redirecting all the domains to the apIP */
    _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer.start(DNS_PORT, "*", _ap_address);
    I::get().logger() << F("[EWC CS]: HTTP server started") << endl;
}

void ConfigServer::_connect(const char* ssid, const char* pass)
{
    I::get().logger() << F("[EWC CS]: Connecting as wifi client... ssid: '") << ssid << "', pass: '" << pass << "'" << endl;
    _msConnectStart = millis();
    // check if we've got static_ip settings, if we do, use those.
    if (_sta_static_ip) {
        I::get().logger() << F("[EWC CS]: Custom STA IP/GW/Subnet/DNS") << endl;
        WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns1, _sta_static_dns2);
        I::get().logger() << F("[EWC CS]:   Local IP: ") << WiFi.localIP() << endl;
    }
    if (ssid != nullptr && strlen(ssid) > 0) {
        //trying to fix connection in progress hanging
        I::get().logger() << F("[EWC CS]: Try to connect with new credentials with SSID: ") << ssid << endl;
        WiFi.disconnect(false);
        WiFi.begin(ssid, pass);
    } else {
        I::get().logger() << F("[EWC CS]: Try to connect with saved credentials for SSID: ") << WiFi.SSID() << endl;
        WiFi.begin();
    }
}

#if defined(ESP8266)
void ConfigServer::_wifiOnStationModeConnected(const WiFiEventStationModeConnected& event)
{
    I::get().logger() << F("[EWC CS]: _wifiOnStationModeConnected: ") << event.ssid << endl;
    _config.setBootMode(BootMode::NORMAL);
}

void ConfigServer::_wifiOnStationModeDisconnected(const WiFiEventStationModeDisconnected& event)
{
    I::get().logger() << F("✘ [EWC CS]: _wifiOnStationModeDisconnected: ") << event.ssid << endl;
}

void ConfigServer::_wifiOnStationModeAuthModeChanged(const WiFiEventStationModeAuthModeChanged& event)
{
    I::get().logger() << F("[EWC CS]: _wifiOnStationModeAuthModeChanged: ") << event.newMode << endl;
}
void ConfigServer::_wifiOnStationModeGotIP(const WiFiEventStationModeGotIP& event)
{
    I::get().logger() << F("[EWC CS]: _wifiOnStationModeGotIP: ") << event.ip << endl;
}
void ConfigServer::_wifiOnStationModeDHCPTimeout()
{
    I::get().logger() << F("✘ [EWC CS]: _wifiOnStationModeDHCPTimeout: ") << endl;
}
void ConfigServer::_wifiOnSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected& event)
{
    I::get().logger() << F("[EWC CS]: _wifiOnSoftAPModeStationConnected: ") << endl;
}
void ConfigServer::_wifiOnSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& event)
{
    I::get().logger() << F("[EWC CS]: _wifiOnSoftAPModeStationDisconnected: ") << endl;
}
#else
// TODO: events for ESP32
#endif
void ConfigServer::insertMenu(const char* name, const char* uri, const char* entry_id, bool visible, int position)
{
    MenuItem item;
    item.name = name;
    item.link = uri;
    item.entry_id = entry_id;
    item.visible = visible;
    if (position < 0) {
        position = _menu.size() + position;
        if (position < 0) {
            position = 0;
        }
    }
    if (position == 255 || position >= (int)_menu.size()) {
        _menu.push_back(item);
    } else {
        _menu.insert(_menu.begin() + position, item);
    }
}

void ConfigServer::insertMenuCb(const char* name, const char* uri, const char* entry_id, ArRequestHandlerFunction onRequest, bool visible, int position)
{
    insertMenu(name, uri, entry_id, visible, position);
    _server.on(uri, onRequest);
}

void ConfigServer::insertMenuP(const char* name, const char* uri, const char* entry_id, PGM_P content, const String& contentType, bool visible, int position)
{
    insertMenuCb(name, uri, entry_id, std::bind(&ConfigServer::sendContentP, this, std::placeholders::_1, content, contentType), visible, position);
}

void ConfigServer::insertMenuNoAuthP(const char* name, const char* uri, const char* entry_id, PGM_P content, const String& contentType, bool visible, int position)
{
    insertMenuCb(name, uri, entry_id, std::bind(&ConfigServer::_sendContentNoAuthP, this, std::placeholders::_1, content, contentType), visible, position);
}

String ConfigServer::_token_WIFI_MODE() {
  PGM_P wifiMode;
  switch (WiFi.getMode()) {
  case WIFI_OFF:
    wifiMode = PSTR("OFF");
    break;
  case WIFI_STA:
    wifiMode = PSTR("STA");
    break;
  case WIFI_AP:
    wifiMode = PSTR("AP");
    break;
  case WIFI_AP_STA:
    wifiMode = PSTR("AP_STA");
    break;
#ifdef ARDUINO_ARCH_ESP32
  case WIFI_MODE_MAX:
    wifiMode = PSTR("MAX");
    break;
#endif
  default:
    wifiMode = PSTR("experiment");
  }
  return String(FPSTR(wifiMode));
}

/**
 *  Convert MAC address in uint8_t array to Sting XX:XX:XX:XX:XX:XX format.
 *  @param  mac   Array of MAC address 6 bytes.
 *  @return MAC address string in XX:XX:XX:XX:XX:XX format.
 */
String ConfigServer::_toMACAddressString(const uint8_t mac[]) {
  String  macAddr = String("");
  for (uint8_t i = 0; i < 6; i++) {
    char buf[3];
    sprintf(buf, "%02X", mac[i]);
    macAddr += buf;
    if (i < 5)
      macAddr += ':';
  }
  return macAddr;
}

void ConfigServer::_onSecurityGet(AsyncWebServerRequest *request)
{
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC CS]: ESP heap: _onSecurityGet: " << ESP.getFreeHeap() << endl;
    DynamicJsonDocument jsonDoc(512);
    JsonObject json = jsonDoc.to<JsonObject>();
    _config.fillJson(jsonDoc);
    I::get().logger() << "[EWC CS]: _onSecurityGet MEMUSAGE: " << json.memoryUsage() << endl;
    I::get().logger() << "[EWC CS]: ESP heap: _onSecurityGet: " << ESP.getFreeHeap() << endl;
    String output;
    serializeJson(json, output);
    I::get().logger() << "[EWC CS]: ESP heap: _onSecurityGet: " << ESP.getFreeHeap() << endl;
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::_onSecuritySave(AsyncWebServerRequest *request)
{
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC CS]: ESP heap: _onSecuritySave: " << ESP.getFreeHeap() << endl;
    if (request->hasArg("apname")) {
        _config.paramAPName = request->arg("apname");
    }
    if (request->hasArg("appass")) {
        _config.setAPPass(request->arg("appass"));
    }
    if (request->hasArg("ap_always_on")) {
        _config.paramAPStartAlways = request->arg("ap_always_on").equals("true");
    }
    if (request->hasArg("basic_auth")) {
        _config.paramBasicAuth = request->arg("basic_auth").equals("true");
    }
    if (request->hasArg("httpuser")) {
        _config.paramHttpUser = request->arg("httpuser");
    }
    if (request->hasArg("httppass")) {
        _config.paramHttpPassword = request->arg("httppass");
    }
    I::get().logger() << "[EWC CS]: ESP heap: _onSecuritySave: " << ESP.getFreeHeap() << endl;
    sendPageSuccess(request, "Security save", "/ewc/security", "Save success! Please, restart to apply AP changes!");
}

void ConfigServer::_onGetInfo(AsyncWebServerRequest *request)
{
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC CS]: ESP heap: _onGetInfo: " << ESP.getFreeHeap() << endl;
    DynamicJsonDocument jsonDoc(512);
    JsonObject json = jsonDoc.to<JsonObject>();
    json["version"] = _version;
  	json["estab_ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String(F("N/A"));
	json["wifi_mode"] = _token_WIFI_MODE();
	json["wifi_status"] = String(WiFi.status());
	json["local_ip"] = WiFi.localIP().toString();
	json["gateway"] = WiFi.gatewayIP().toString();
	json["netwask"] = WiFi.subnetMask().toString();
	json["softap_ip"] = WiFi.softAPIP().toString();
    uint8_t macAddress[6];
    WiFi.softAPmacAddress(macAddress);
	json["ap_mac"] = ConfigServer::_toMACAddressString(macAddress);
    WiFi.macAddress(macAddress);
	json["sta_mac"] = ConfigServer::_toMACAddressString(macAddress);
	json["channel"] = String(WiFi.channel());
    int32_t dBm = WiFi.RSSI();
	json["dbm"] = dBm == 31 ? String(F("N/A")) : String(dBm);
	json["chip_id"] = String(_config.getChipId());
	json["cpu_freq"] =String(ESP.getCpuFreqMHz());
#if defined(ARDUINO_ARCH_ESP8266)
	json["flash_size"] = String(ESP.getFlashChipRealSize());
#elif defined(ARDUINO_ARCH_ESP32)
    json["flash_size"] = String(spi_flash_get_chip_size());
#endif
	json["free_heap"] = String(ESP.getFreeHeap());
    I::get().logger() << "[EWC CS]: _onGetInfo MEMUSAGE: " << json.memoryUsage() << endl;
    String output;
    serializeJson(json, output);
    I::get().logger() << "[EWC CS]: ESP heap: _onGetInfo: " << ESP.getFreeHeap() << endl;
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::_onWiFiConnect(AsyncWebServerRequest *request)
{
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC CS]: ESP heap: _onWiFiConnect: " << ESP.getFreeHeap() << endl;
    for (size_t idx = 0; idx < request->args(); idx++) {
        I::get().logger() << "[EWC CS]: " << idx << ": " << request->argName(idx) << request->arg(idx) << endl;
    }
    const char* ssid = request->arg("ssid").c_str();
    const char* pass = request->arg("passphrase").c_str();
    if (request->hasArg("staip")) {
        I::get().logger() << F("[EWC CS]: static ip: ") << request->arg("staip") << endl;
        //_sta_static_ip.fromString(request->arg("ip"));
        String ip = request->arg("staip");
        _optionalIPFromString(&_sta_static_ip, ip.c_str());
    }
    if (request->hasArg("gtway")) {
        I::get().logger() << F("[EWC CS]: static gateway") << request->arg("gtway") << endl;
        String gw = request->arg("gtway");
        _optionalIPFromString(&_sta_static_gw, gw.c_str());
    }
    if (request->hasArg("ntmsk")) {
        I::get().logger() << F("[EWC CS]: static netmask") << request->arg("ntmsk") << endl;
        String sn = request->arg("ntmsk");
        _optionalIPFromString(&_sta_static_sn, sn.c_str());
    }
    if (request->hasArg("dns1")) {
        I::get().logger() << F("[EWC CS]: static DNS 1") << request->arg("dns1") << endl;
        String dns1 = request->arg("dns1");
        _optionalIPFromString(&_sta_static_dns1, dns1.c_str());
    }
    if (request->hasArg("dns2")) {
        I::get().logger() << F("[EWC CS]: static DNS 2") << request->arg("dns2") << endl;
        String dns2 = request->arg("dns2");
        _optionalIPFromString(&_sta_static_dns2, dns2.c_str());
    }
    I::get().logger() << "[EWC CS]: ESP heap: _onWiFiConnect: " << ESP.getFreeHeap() << endl;
    request->send_P(200, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_WIFI_CONNECT);
    _connect(ssid, pass);
    //HTML_WIFI_CONNECT, FPSTR(PROGMEM_CONFIG_TEXT_HTML)
    // AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
    // response->addHeader("Location", "/wifi/connecting");
    // request->send ( response);
}

void ConfigServer::_onWiFiDisconnect(AsyncWebServerRequest *request)
{
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC CS]: ESP heap: _onWiFiDisconnect: " << ESP.getFreeHeap() << endl;
    AsyncWebServerResponse *response = request->beginResponse(302,"text/plain","");
    response->addHeader("Location", "/wifi/setup");
    request->send ( response);
    I::get().logger() << "[EWC CS]: ESP heap: _onWiFiDisconnect: " << ESP.getFreeHeap() << endl;
    WiFi.disconnect(false);
}


String ConfigServer::_token_STATION_STATUS(bool failedOnly)
{
    wifi_status_t result = _station_status();
    _logger << F("[EWC CS]: Station status for token: ") << result << endl;
    PGM_P wlStatusSymbol = "";
    if (!failedOnly || result > EWC_STATION_GOT_IP) {
        wlStatusSymbol = wlStatusSymbols[result];
    }
    return String(FPSTR(wlStatusSymbol));
}

wifi_status_t ConfigServer::_station_status()
{
    wifi_status_t result = EWC_STATION_IDLE;
#if defined(ARDUINO_ARCH_ESP8266)
    station_status_t status = wifi_station_get_connect_status();
    switch (status) {
        case STATION_IDLE:
            result = EWC_STATION_IDLE;
            break;
        case STATION_CONNECTING:
            result = EWC_STATION_CONNECTING;
            break;
        case STATION_WRONG_PASSWORD:
            result = EWC_STATION_WRONG_PASSWORD;
            break;
        case STATION_NO_AP_FOUND:
            result = EWC_STATION_NO_AP_FOUND;
            break;
        case STATION_CONNECT_FAIL:
            result = EWC_STATION_CONNECT_FAIL;
            break;
        case STATION_GOT_IP:
            result = EWC_STATION_GOT_IP;
            break;
#elif defined(ARDUINO_ARCH_ESP32)
    switch (WiFi.status()) {
        case WL_IDLE_STATUS:
            result = EWC_STATION_IDLE;
            break;
        case WL_NO_SSID_AVAIL:
            result = EWC_STATION_NO_AP_FOUND;
            break;
        case WL_SCAN_COMPLETED:
            result = EWC_STATION_SCAN_COMPLETED;
            break;
        case WL_CONNECTED:
            result = EWC_STATION_CONNECTED;
            break;
        case WL_CONNECT_FAILED:
            result = EWC_STATION_CONNECT_FAIL;
            break;
        case WL_CONNECTION_LOST:
            result = EWC_STATION_CONNECTION_LOST;
            break;
        case WL_DISCONNECTED:
            result = EWC_STATION_DISCONNECTED;
            break;
        case WL_NO_SHIELD:
            result = EWC_NO_SHIELD;
            break;
#endif
    }
    return result;
}

void ConfigServer::_sendMenu(AsyncWebServerRequest *request) {
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC CS]: ESP heap: _sendMenu: " << ESP.getFreeHeap() << endl;
    DynamicJsonDocument jsonDoc(2048);
    JsonObject json = jsonDoc.to<JsonObject>();
    json["brand"] = _brand;
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
    I::get().logger() << "[EWC CS]: ESP heap: _sendMenu: " << ESP.getFreeHeap() << endl;
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::_onWifiState(AsyncWebServerRequest *request)
{
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC CS]: ESP heap: _onWifiState: " << ESP.getFreeHeap() << endl;
    DynamicJsonDocument jsonDoc(2048);
    JsonObject json = jsonDoc.to<JsonObject>();
    json["ssid"] = WiFi.SSID();
    json["connected"] = WiFi.status() == WL_CONNECTED;
    String reason = _token_STATION_STATUS(true);
    json["failed"] = reason.length() > 0;
    json["reason"] = reason;
    String output;
    serializeJson(json, output);
    I::get().logger() << "[EWC CS]: ESP heap: _onWifiState: " << ESP.getFreeHeap() << endl;
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
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC CS]: ESP heap: _onWifiScan: " << ESP.getFreeHeap() << endl;
    _startWiFiScan();
    int n = WiFi.scanComplete();
    DynamicJsonDocument jsonDoc(2048);
    JsonObject json = jsonDoc.to<JsonObject>();
    if (n == WIFI_SCAN_FAILED) {
        I::get().logger() << F("[EWC CS]: scanNetworks returned: WIFI_SCAN_FAILED!") << endl;
        _wiFiState2Json(json, true, true, "scanNetworks returned: WIFI_SCAN_FAILED!");
	  } else if(n == WIFI_SCAN_RUNNING) {
        I::get().logger() << F("[EWC CS]: scanNetworks returned: WIFI_SCAN_RUNNING!") << endl;
        _wiFiState2Json(json, false, false, "");
	  } else if(n < 0) {
        I::get().logger() << F("[EWC CS]: scanNetworks failed with unknown error code!") << endl;
        _wiFiState2Json(json, true, true, "scanNetworks failed with unknown error code!");
	  } else if (n == 0) {
        I::get().logger() << F("✘ [EWC CS]: No networks found") << endl;
        _wiFiState2Json(json, true, true, "No networks found");
    }
    JsonArray networks = json.createNestedArray("networks");
    if (n > 0) {
        _wiFiState2Json(json, true, false, "");
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
        I::get().logger() << "[EWC CS]: ESP heap: _onWifiScan: " << ESP.getFreeHeap() << endl;
        request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
        I::get().logger() << "[EWC CS]: ESP heap: _onWifiScan: " << ESP.getFreeHeap() << endl;
        //WiFi.scanDelete();
        return;
    } else if (n == -2) {
        _startWiFiScan(true);
        _wiFiState2Json(json, false, false, "");
    }
    String output;
    serializeJson(json, output);
    I::get().logger() << "[EWC CS]: ESP heap: _onWifiScan: " << ESP.getFreeHeap() << endl;
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
                wifi_status_t st = _station_status();
                if (st == 0 || st > EWC_STATION_GOT_IP) {
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

void ConfigServer::setBrand(const char* brand, const char* version)
{
    _brand = String(brand);
    _version = String(version);
}

void ConfigServer::sendPageSuccess(AsyncWebServerRequest *request, String title, String redirectUrl, String summary, String details, int timeout)
{
    I::get().logger() << "[EWC CS]: sendPageSuccess " << request->url() << ":" << ESP.getFreeHeap() << endl;
    I::get().logger() << "[EWC CS]: sendPageSuccess " << summary << ":" << details << endl;
    String result(FPSTR(EWC_PAGE_SUCCESS));
    result.replace("{{TIMEOUT}}", String(timeout));
    result.replace("{{REDIRECT}}", redirectUrl);
    result.replace("{{TITLE}}", title);
    result.replace("{{SUMMARY}}", summary);
    result.replace("{{DETAILS}}", details);
    request->send(200, FPSTR(PROGMEM_CONFIG_TEXT_HTML), result);
    I::get().logger() << "[EWC CS]: sendPageSuccess " << request->url() << ":" << ESP.getFreeHeap() << endl;
}

void ConfigServer::sendPageFailed(AsyncWebServerRequest *request, String title, String redirectUrl, String summary, String details, int timeout)
{
    String result(FPSTR(EWC_PAGE_FAIL));
    result.replace("{{TIMEOUT}}", String(timeout));
    result.replace("{{REDIRECT}}", redirectUrl);
    result.replace("{{TITLE}}", title);
    result.replace("{{SUMMARY}}", summary);
    result.replace("{{DETAILS}}", details);
    request->send(200, FPSTR(PROGMEM_CONFIG_TEXT_HTML), result);
}

void ConfigServer::_sendFileContent(AsyncWebServerRequest *request, const String& filename, const String& contentType)
{
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << F("[EWC CS]: Handle sendFileContent: ") << filename << endl;
    File reqFile = LittleFS.open(filename, "r");
    if (reqFile && reqFile.isFile()) {
        request->send(LittleFS, filename, contentType);
        reqFile.close();
        return;
    }
    _onNotFound(request);
}

void ConfigServer::sendContentP(AsyncWebServerRequest *request, PGM_P content, const String& contentType)
{
    if (!isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC CS]: send content P" << request->url() << ":" << ESP.getFreeHeap() << endl;
    request->send_P(200, contentType.c_str(), content);
    I::get().logger() << "[EWC CS]: finished " << request->url() << ":" << ESP.getFreeHeap() << endl;
}

void ConfigServer::_sendContentNoAuthP(AsyncWebServerRequest *request, PGM_P content, const String& contentType)
{
    I::get().logger() << "[EWC CS]: send content P" << request->url() << ":" << ESP.getFreeHeap() << endl;
    request->send_P(200, contentType.c_str(), content);
    I::get().logger() << "[EWC CS]: finished " << request->url() << ":" << ESP.getFreeHeap() << endl;
}

void ConfigServer::_onNotFound(AsyncWebServerRequest *request) {
    I::get().logger() << F("[EWC CS]: Handle not found") << endl;
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
        I::get().logger() << F("[EWC CS]: Request host is not an IP: ") << request->host() << endl;
        I::get().logger() << F("[EWC CS]: Request redirected to captive portal: ") << toStringIp(request->client()->localIP()) << endl;
        AsyncWebServerResponse *response = request->beginResponse(302,"text/plain","");
        response->addHeader("Location", String("http://") + toStringIp(request->client()->localIP()));
        request->send ( response);
        return true;
    }
    return false;
}

void ConfigServer::_startWiFiScan(bool force)
{
    if (force || millis() - _msWifiScanStart > WIFI_SCAN_DELAY) {
        WiFi.scanNetworks(true, true);
        _msWifiScanStart = millis();
    }
}

bool ConfigServer::isAuthenticated(AsyncWebServerRequest *request)
{
    return ! (_config.paramBasicAuth && !request->authenticate(_config.paramHttpUser.c_str(), _config.paramHttpPassword.c_str()));
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