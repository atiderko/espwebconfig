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
#include "generated/accessSetupHTML.h"
#include "generated/ewcFailHTML.h"
#include "generated/ewcSuccessHTML.h"
#include "generated/loggingSetupHTML.h"
#include "generated/webBaseCSS.h"
#include "generated/webPostloadJS.h"
#include "generated/webPreJS.h"
#include "generated/webTableCSS.h"
#include "generated/webWifiiconsCSS.h"
#include "generated/webWifiJS.h"
#include "generated/wifiStateHTML.h"
#include "generated/wifiSetupHTML.h"

/**
 *  An actual reset function dependent on the architecture
 */
#if defined(ARDUINO_ARCH_ESP8266)
#define SET_HOSTNAME(x) \
  do                    \
  {                     \
    WiFi.hostname(x);   \
  } while (0)
#elif defined(ARDUINO_ARCH_ESP32)
#define SET_HOSTNAME(x)  \
  do                     \
  {                      \
    WiFi.setHostname(x); \
  } while (0)
#endif

using namespace EWC;

InterfaceData I::_interface; // need to define the static variable

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
    "NO_SHIELD"};

#if defined(ESP8266)
WiFiEventHandler p1;
WiFiEventHandler p2;
// WiFiEventHandler p3;
// WiFiEventHandler p4;
// WiFiEventHandler p5;
WiFiEventHandler p6;
WiFiEventHandler p7;
#endif

ConfigServer::ConfigServer(uint16_t port)
    : _server(port),
      _brand("ESP Web Config")
{

  I::get()._server = this;
  I::get()._config = &_config;
  I::get()._configFS = &_configFS;
  I::get()._time = &_time;
  I::get()._logger = &_logger;
  I::get()._led = &_led;
  _publicConfig = true;
  _brandUri = "/";
  _connected_wifi = false;
  _ap_disabled_after_timeout = false;
  _disconnect_state = 0;
  _disconnect_reason = "";
  _softAPClientCount = 0;
  _configFS.addConfig(_config);
  _configFS.addConfig(_time);
}

void ConfigServer::setup()
{
  I::get().logger() << F("[EWC CS]: setup configFS") << endl;
  _configFS.setup();
  WiFi.setAutoConnect(false);
  if (_configFS.resetDetected())
  {
    WiFi.disconnect(true);
  }
  // Set host name
  if (_config.paramHostname.length())
    SET_HOSTNAME(_config.paramHostname.c_str());

  I::get().logger() << F("[EWC CS]: setup wifi events") << endl;
  WiFi.setAutoReconnect(false);
#if defined(ESP8266)
  p1 = WiFi.onStationModeConnected(std::bind(&ConfigServer::_wifiOnStationModeConnected, this, std::placeholders::_1));
  p2 = WiFi.onStationModeDisconnected(std::bind(&ConfigServer::_wifiOnStationModeDisconnected, this, std::placeholders::_1));
  // p3 = WiFi.onStationModeAuthModeChanged(std::bind(&ConfigServer::_wifiOnStationModeAuthModeChanged, this, std::placeholders::_1));
  // p4 = WiFi.onStationModeGotIP(std::bind(&ConfigServer::_wifiOnStationModeGotIP, this, std::placeholders::_1));
  // p5 = WiFi.onStationModeDHCPTimeout(std::bind(&ConfigServer::_wifiOnStationModeDHCPTimeout, this));
  p6 = WiFi.onSoftAPModeStationConnected(std::bind(&ConfigServer::_wifiOnSoftAPModeStationConnected, this, std::placeholders::_1));
  p7 = WiFi.onSoftAPModeStationDisconnected(std::bind(&ConfigServer::_wifiOnSoftAPModeStationDisconnected, this, std::placeholders::_1));
#else
  // events for ESP32
  WiFi.onEvent(std::bind(&ConfigServer::_wifiOnStationModeConnected, this, std::placeholders::_1, std::placeholders::_2), WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(std::bind(&ConfigServer::_wifiOnStationModeDisconnected, this, std::placeholders::_1, std::placeholders::_2), WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(std::bind(&ConfigServer::_wifiOnSoftAPModeStationConnected, this, std::placeholders::_1, std::placeholders::_2), WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent(std::bind(&ConfigServer::_wifiOnSoftAPModeStationDisconnected, this, std::placeholders::_1, std::placeholders::_2), WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
#endif
  I::get().logger() << F("[EWC CS]: configure urls") << endl;
  if (!_config.paramWifiDisabled)
  {
    if (_config.paramAPStartAlways || _config.getBootMode() == BootMode::CONFIGURATION || _config.getBootMode() == BootMode::STANDALONE)
    {
      WiFi.mode(WIFI_AP_STA);
      _startAP();
    }
    else if (_config.getBootMode() != BootMode::STANDALONE)
    {
      WiFi.mode(WIFI_STA);
      delay(500);
    }
    _connect();
    _startWiFiScan();
  }
  else
  {
    WiFi.mode(WIFI_OFF);
  }

  I::get().logger() << "CSS_WEB_BASE_GZIP size: " << sizeof(CSS_WEB_BASE_GZIP) << endl;
  // _server.reset(); do we need this?
  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  _server.on("/menu.json", std::bind(&ConfigServer::_sendMenu, this, &_server));
  _server.on("/css/base.css", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_TEXT_CSS), CSS_WEB_BASE_GZIP, sizeof(CSS_WEB_BASE_GZIP)));
  _server.on("/css/table.css", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_TEXT_CSS), CSS_WEB_TABLE_GZIP, sizeof(CSS_WEB_TABLE_GZIP)));
  _server.on("/css/wifiicons.css", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_TEXT_CSS), CSS_WEB_WIFIICONS_GZIP, sizeof(CSS_WEB_WIFIICONS_GZIP)));
  _server.on("/js/postload.js", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_APPLICATION_JS), JS_WEB_POSTLOAD_GZIP, sizeof(JS_WEB_POSTLOAD_GZIP)));
  _server.on("/js/pre.js", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_APPLICATION_JS), JS_WEB_PRE_GZIP, sizeof(JS_WEB_PRE_GZIP)));
  _server.on("/js/wifi.js", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_APPLICATION_JS), JS_WEB_WIFI_GZIP, sizeof(JS_WEB_WIFI_GZIP)));
  insertMenuCb("WiFi", "/wifi/setup", "menu_wifi", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_WIFI_SETUP_GZIP, sizeof(HTML_WIFI_SETUP_GZIP)));
  _server.on("/wifi/disconnect", std::bind(&ConfigServer::_onWiFiDisconnect, this, &_server)); //.setFilter(ON_AP_FILTER);
  _server.on("/wifi/config/save", std::bind(&ConfigServer::_onWiFiConnect, this, &_server));
  _server.on("/wifi/state.html", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_WIFI_SETUP_GZIP, sizeof(HTML_WIFI_SETUP_GZIP)));
  _server.on("/wifi/state.json", std::bind(&ConfigServer::_onWifiState, this, &_server));
  _server.on("/wifi/stations.json", std::bind(&ConfigServer::_onWifiScan, this, &_server));
  insertMenuCb("Access", "/access/setup", "menu_access", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_ACCESS_SETUP_GZIP, sizeof(HTML_ACCESS_SETUP_GZIP)));
  _server.on("/access/config.json", std::bind(&ConfigServer::_onAccessGet, this, &_server));
  _server.on("/access/config/save", std::bind(&ConfigServer::_onAccessSave, this, &_server));
  insertMenuCb("Logging", "/logging/setup", "menu_access", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_LOGGING_SETUP_GZIP, sizeof(HTML_LOGGING_SETUP_GZIP)));
  _server.on("/logging/config.json", std::bind(&ConfigServer::_onLoggingGet, this, &_server));
  _server.on("/logging/enable", std::bind(&ConfigServer::_onLoggingEnable, this, &_server));
  insertMenuCb("Info", "/ewc/info", "menu_info", std::bind(&ConfigServer::sendContentG, this, &_server, FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_EWC_INFO_GZIP, sizeof(HTML_EWC_INFO_GZIP)));
  _server.on("/ewc/info.json", std::bind(&ConfigServer::_onGetInfo, this, &_server));
  if (_publicConfig)
  {
    _server.on("/config.json", std::bind(&ConfigServer::_sendFileContent, this, &_server, FPSTR(PROGMEM_CONFIG_APPLICATION_JS), FPSTR(CONFIG_FILENAME)));
  }
  _server.on("/device/delete", std::bind(&ConfigServer::_onDeviceReset, this, &_server));
  _server.on("/device/restart", std::bind(&ConfigServer::_onDeviceRestart, this, &_server));
  _server.on("/favicon.ico", std::bind(&ConfigServer::_sendFileContent, this, &_server, "image/x-icon", "/favicon.ico"));
  _server.onNotFound(std::bind(&ConfigServer::_onNotFound, this, &_server));
  _server.begin(); // Web server start
}

void ConfigServer::_startAP()
{
  // Start LED with AP_STA
  _led.start(LED_ORANGE, 1000, 100);
  I::get().logger() << endl;
  _msConfigPortalStart = _config.paramAPStartAlways ? 0 : millis();
  I::get().logger() << F("[EWC CS]: Configuring access point... ") << _config.paramAPName << endl;
  // optional soft ip config
  //  if (_ap_static_ip) {
  //      DEBUG_WM(F("Custom AP IP/GW/Subnet"));
  //      WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn);
  //  }
  if (!_config.getAPPass().isEmpty())
  {
    WiFi.softAP(_config.paramAPName.c_str(), _config.getAPPass().c_str());
  }
  else
  {
    WiFi.softAP(_config.paramAPName.c_str());
  }
  delay(500); // Without delay I've seen the IP address blank
  _ap_address = WiFi.softAPIP();
  I::get().logger() << F("[EWC CS]: AP IP address: ") << _ap_address << endl;
  // Setup the DNS server redirecting all the domains to the apIP
  _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  _dnsServer.start(DNS_PORT, "*", _ap_address);
  I::get().logger() << F("[EWC CS]: HTTP server started") << endl;
}

void ConfigServer::_connect(const char *ssid, const char *pass)
{
  I::get().logger() << F("[EWC CS]: Connecting as wifi client... ssid: '") << ssid << "', pass: '" << pass << "'" << endl;
  _msConnectStart = millis();
  _disconnect_state = 0;
  _disconnect_reason = "";
  // check if we've got static_ip settings, if we do, use those.
  if (_sta_static_ip)
  {
    I::get().logger() << F("[EWC CS]: Custom STA IP/GW/Subnet/DNS") << endl;
    WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn, _sta_static_dns1, _sta_static_dns2);
    I::get().logger() << F("[EWC CS]:   Local IP: ") << WiFi.localIP() << endl;
  }
  if (ssid != nullptr && strlen(ssid) > 0)
  {
    // trying to fix connection in progress hanging
    I::get().logger() << F("[EWC CS]: Try to connect with new credentials with SSID: ") << ssid << endl;
    WiFi.disconnect(false);
    WiFi.begin(ssid, pass);
  }
  else
  {
    I::get().logger() << F("[EWC CS]: Try to connect with saved credentials for SSID: ") << WiFi.SSID() << endl;
    WiFi.disconnect(false);
    WiFi.begin();
  }
  // Start LED according to the WiFi condition if LED is available.
  if (WiFi.status() != WL_CONNECTED)
  {
    _led.start(LED_GREEN, 1000, 256);
  }
}

#ifdef ESP8266
void ConfigServer::_wifiOnStationModeConnected(const WiFiEventStationModeConnected &event)
{
  I::get().logger() << F("[EWC CS]: _wifiOnStationModeConnected: ") << event.ssid << endl;
  if (!_config.paramAPStartAlways)
  {
    _config.setBootMode(BootMode::NORMAL);
  }
  _ap_disabled_after_timeout = false;
  _disconnect_state = 0;
}

void ConfigServer::_wifiOnStationModeDisconnected(const WiFiEventStationModeDisconnected &event)
{
  I::get().logger() << F("✘ [EWC CS]: _wifiOnStationModeDisconnected: ") << event.ssid << ", code: " << event.reason << endl;
  switch (event.reason)
  {
  case WIFI_DISCONNECT_REASON_NO_AP_FOUND:
  {
    _disconnect_state = event.reason;
    _disconnect_reason = "No AP found";
    break;
  }
  case WIFI_DISCONNECT_REASON_AUTH_EXPIRE:
  case WIFI_DISCONNECT_REASON_AUTH_FAIL:
  {
    _disconnect_state = event.reason;
    _disconnect_reason = "Authentication failed";
    break;
  }
  default:
  {
    _disconnect_state = event.reason;
    if (event.reason > 0)
    {
      _disconnect_reason = "Failed, error: " + String(event.reason);
    }
  }
  }
  // set time if we should reconnect
  if (_reconnectTs == 0)
  {
    I::get().logger() << F("[EWC CS]: start reconnect Timer in 7 sec") << endl;
    _reconnectTs = millis() + 7000;
  }
}

// void ConfigServer::_wifiOnStationModeAuthModeChanged(const WiFiEventStationModeAuthModeChanged& event)
// {
//     I::get().logger() << F("[EWC CS]: _wifiOnStationModeAuthModeChanged: ") << event.newMode << endl;
// }
// void ConfigServer::_wifiOnStationModeGotIP(const WiFiEventStationModeGotIP& event)
// {
//     I::get().logger() << F("[EWC CS]: _wifiOnStationModeGotIP: ") << event.ip << endl;
// }
// void ConfigServer::_wifiOnStationModeDHCPTimeout()
// {
//     I::get().logger() << F("✘ [EWC CS]: _wifiOnStationModeDHCPTimeout: ") << endl;
// }
void ConfigServer::_wifiOnSoftAPModeStationConnected(const WiFiEventSoftAPModeStationConnected &event)
{
  I::get().logger() << F("[EWC CS]: _wifiOnSoftAPModeStationConnected: ") << endl;
  _softAPClientCount++;
}
void ConfigServer::_wifiOnSoftAPModeStationDisconnected(const WiFiEventSoftAPModeStationDisconnected &event)
{
  I::get().logger() << F("[EWC CS]: _wifiOnSoftAPModeStationDisconnected: ") << endl;
  _softAPClientCount--;
  if (WiFi.status() == WL_CONNECTED && !_config.paramAPStartAlways)
  {
    I::get().logger() << F("[EWC CS]: disable AP after successfully connected") << endl;
    WiFi.mode(WIFI_STA);
  }
}
#else
void ConfigServer::_wifiOnStationModeConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  I::get().logger() << F("[EWC CS]: _wifiOnStationModeConnected: ") << String(info.wifi_sta_connected.ssid, sizeof(info.wifi_sta_connected.ssid)) << endl;
  _config.setBootMode(BootMode::NORMAL);
  _ap_disabled_after_timeout = false;
  _disconnect_state = 0;
}

void ConfigServer::_wifiOnStationModeDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  I::get().logger() << F("✘ [EWC CS]: _wifiOnStationModeDisconnected: ") << String(info.wifi_sta_disconnected.ssid, sizeof(info.wifi_sta_disconnected.ssid)) << ", code: " << info.wifi_sta_disconnected.reason << endl;
  switch (info.wifi_sta_disconnected.reason)
  {
  case wifi_err_reason_t::WIFI_REASON_NO_AP_FOUND:
  {
    _disconnect_state = info.wifi_sta_disconnected.reason;
    _disconnect_reason = "No AP found";
    break;
  }
  case wifi_err_reason_t::WIFI_REASON_AUTH_EXPIRE:
  {
    _disconnect_state = info.wifi_sta_disconnected.reason;
    _disconnect_reason = "Authentication failed";
    break;
  }
  default:
  {
    _disconnect_state = info.wifi_sta_disconnected.reason;
    if (info.wifi_sta_disconnected.reason > 0)
    {
      _disconnect_reason = "Failed, error: " + String(info.wifi_sta_disconnected.reason);
    }
  }
  }
  // set time if we should reconnect
  if (_reconnectTs == 0)
  {
    I::get().logger() << F("[EWC CS]: start reconnect Timer in 7 sec") << endl;
    _reconnectTs = millis() + 7000;
  }
}

// void ConfigServer::_wifiOnStationModeAuthModeChanged(const WiFiEventStationModeAuthModeChanged& event)
// {
//     I::get().logger() << F("[EWC CS]: _wifiOnStationModeAuthModeChanged: ") << event.newMode << endl;
// }
// void ConfigServer::_wifiOnStationModeGotIP(const WiFiEventStationModeGotIP& event)
// {
//     I::get().logger() << F("[EWC CS]: _wifiOnStationModeGotIP: ") << event.ip << endl;
// }
// void ConfigServer::_wifiOnStationModeDHCPTimeout()
// {
//     I::get().logger() << F("✘ [EWC CS]: _wifiOnStationModeDHCPTimeout: ") << endl;
// }
void ConfigServer::_wifiOnSoftAPModeStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  I::get().logger() << F("[EWC CS]: _wifiOnSoftAPModeStationConnected: ") << endl;
  _softAPClientCount++;
}
void ConfigServer::_wifiOnSoftAPModeStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  I::get().logger() << F("[EWC CS]: _wifiOnSoftAPModeStationDisconnected: ") << endl;
  _softAPClientCount--;
  if (WiFi.status() == WL_CONNECTED && !_config.paramAPStartAlways)
  {
    I::get().logger() << F("[EWC CS]: disable AP after successfully connected") << endl;
    WiFi.mode(WIFI_STA);
  }
}
#endif

void ConfigServer::insertMenu(const char *name, const char *uri, const char *entry_id, bool visible, int position)
{
  MenuItem item;
  item.name = name;
  item.link = uri;
  item.entry_id = entry_id;
  item.visible = visible;
  if (position < 0)
  {
    position = _menu.size() + position;
    if (position < 0)
    {
      position = 0;
    }
  }
  if (position == 255 || position >= (int)_menu.size())
  {
    I::get().logger() << F("[EWC CS]: append menu item: ") << name << endl;
    _menu.push_back(item);
  }
  else
  {
    I::get().logger() << F("[EWC CS]: insert menu item: ") << name << F(", at position: ") << position << endl;
    _menu.insert(_menu.begin() + position, item);
  }
}

void ConfigServer::insertMenuCb(const char *name, const char *uri, const char *entry_id, WebServerHandlerFunction onRequest, bool visible, int position)
{
  insertMenu(name, uri, entry_id, visible, position);
  _server.on(uri, HTTP_GET, onRequest);
}

void ConfigServer::insertMenuP(const char *name, const char *uri, const char *entry_id, const String &contentType, PGM_P content, bool visible, int position)
{
  insertMenuCb(name, uri, entry_id, std::bind(&ConfigServer::sendContentP, this, &_server, contentType, content), visible, position);
}

void ConfigServer::insertMenuG(const char *name, const char *uri, const char *entry_id, const String &contentType, const uint8_t *content, size_t len, bool visible, int position)
{
  insertMenuCb(name, uri, entry_id, std::bind(&ConfigServer::sendContentG, this, &_server, contentType, content, len), visible, position);
}

void ConfigServer::insertMenuNoAuthP(const char *name, const char *uri, const char *entry_id, const String &contentType, PGM_P content, bool visible, int position)
{
  insertMenuCb(name, uri, entry_id, std::bind(&ConfigServer::_sendContentNoAuthP, this, &_server, contentType, content), visible, position);
}

void ConfigServer::insertMenuNoAuthG(const char *name, const char *uri, const char *entry_id, const String &contentType, const uint8_t *content, size_t len, bool visible, int position)
{
  insertMenuCb(name, uri, entry_id, std::bind(&ConfigServer::_sendContentNoAuthG, this, &_server, contentType, content, len), visible, position);
}

String ConfigServer::_token_WIFI_MODE()
{
  PGM_P wifiMode;
  switch (WiFi.getMode())
  {
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
String ConfigServer::_toMACAddressString(const uint8_t mac[])
{
  String macAddr = String("");
  for (uint8_t i = 0; i < 6; i++)
  {
    char buf[3];
    sprintf(buf, "%02X", mac[i]);
    macAddr += buf;
    if (i < 5)
      macAddr += ':';
  }
  return macAddr;
}

void ConfigServer::_onAccessGet(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  JsonObject json = jsonDoc.to<JsonObject>();
  _config.fillJson(jsonDoc);
  String output;
  serializeJson(json, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::_onAccessSave(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  // I::get().logger() << "[EWC CS]: handle args: " << webServer->args() << endl;
  // for (int i = 0; i < webServer->args(); i++) {
  //     I::get().logger() << "[EWC CS]:   " << webServer->argName(i) << ": " << webServer->arg(i) << endl;
  // }
  if (webServer->hasArg("dev_name"))
  {
    _config.paramDeviceName = webServer->arg("dev_name");
  }
  if (webServer->hasArg("apName"))
  {
    _config.paramAPName = webServer->arg("apName");
  }
  if (webServer->hasArg("apPass"))
  {
    _config.setAPPass(webServer->arg("apPass"));
  }
  if (webServer->hasArg("ap_start_always"))
  {
    _config.paramAPStartAlways = webServer->arg("ap_start_always").equals("true");
  }
  else
  {
    _config.paramAPStartAlways = false;
  }
  if (webServer->hasArg("basic_auth"))
  {
    _config.paramBasicAuth = webServer->arg("basic_auth").equals("true");
  }
  else
  {
    _config.paramBasicAuth = false;
  }
  if (webServer->hasArg("httpUser"))
  {
    _config.paramHttpUser = webServer->arg("httpUser");
  }
  if (webServer->hasArg("httpPass"))
  {
    _config.paramHttpPassword = webServer->arg("httpPass");
  }
  if (webServer->hasArg("hostname"))
  {
    _config.paramHostname = webServer->arg("hostname");
  }
  I::get().configFS().save();
  sendPageSuccess(webServer, "Security save", "Save successful! Please, restart to apply AP changes!", "/access/setup");
}

void ConfigServer::_onGetInfo(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  JsonObject json = jsonDoc.to<JsonObject>();
  json["version"] = _version;
  json["estab_ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String(F("N/A"));
  json["wifi_mode"] = _token_WIFI_MODE();
  json["wifi_status"] = String(WiFi.status());
  json["local_ip"] = WiFi.localIP().toString();
  json["gateway"] = WiFi.gatewayIP().toString();
  json["netmask"] = WiFi.subnetMask().toString();
  json["softApIp"] = WiFi.softAPIP().toString();
  uint8_t macAddress[6];
  WiFi.softAPmacAddress(macAddress);
  json["ap_mac"] = ConfigServer::_toMACAddressString(macAddress);
  WiFi.macAddress(macAddress);
  json["sta_mac"] = ConfigServer::_toMACAddressString(macAddress);
  json["channel"] = String(WiFi.channel());
  int32_t dBm = WiFi.RSSI();
  json["dbm"] = dBm == 31 ? String(F("N/A")) : String(dBm);
  json["chip_id"] = String(_config.getChipId());
  json["cpu_freq"] = String(ESP.getCpuFreqMHz());
#if defined(ARDUINO_ARCH_ESP8266)
  json["flash_size"] = String(ESP.getFlashChipRealSize());
#elif defined(ARDUINO_ARCH_ESP32)
  json["flash_size"] = String(spi_flash_get_chip_size());
#endif
  json["free_heap"] = String(ESP.getFreeHeap());
  String output;
  serializeJson(json, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::_onLoggingGet(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  JsonObject json = jsonDoc.to<JsonObject>();
  _config.fillJson(jsonDoc);
  String output;
  serializeJson(json, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::_onLoggingEnable(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  // I::get().logger() << "[EWC CS]: handle args: " << webServer->args() << endl;
  // for (int i = 0; i < webServer->args(); i++) {
  //     I::get().logger() << "[EWC CS]:   " << webServer->argName(i) << ": " << webServer->arg(i) << endl;
  // }
  if (webServer->hasArg("enable_serial_log"))
  {
    I::get().logger().setLogging(webServer->arg("enable_serial_log").equals("on"));
  }
  else
  {
    I::get().logger().setLogging(false);
  }
  I::get().configFS().save();
  sendRedirect(webServer, "/logging/setup");
}

void ConfigServer::_onWiFiConnect(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  for (int idx = 0; idx < webServer->args(); idx++)
  {
    I::get().logger() << "[EWC CS]: " << idx << ": " << webServer->argName(idx) << webServer->arg(idx) << endl;
  }
  if (webServer->hasArg("stationIP"))
  {
    I::get().logger() << F("[EWC CS]: static ip: ") << webServer->arg("stationIP") << endl;
    String ip = webServer->arg("stationIP");
    _optionalIPFromString(&_sta_static_ip, ip.c_str());
  }
  if (webServer->hasArg("gateway"))
  {
    I::get().logger() << F("[EWC CS]: static gateway") << webServer->arg("gateway") << endl;
    String gw = webServer->arg("gateway");
    _optionalIPFromString(&_sta_static_gw, gw.c_str());
  }
  if (webServer->hasArg("netmask"))
  {
    I::get().logger() << F("[EWC CS]: static netmask") << webServer->arg("netmask") << endl;
    String sn = webServer->arg("netmask");
    _optionalIPFromString(&_sta_static_sn, sn.c_str());
  }
  if (webServer->hasArg("dns1"))
  {
    I::get().logger() << F("[EWC CS]: static DNS 1") << webServer->arg("dns1") << endl;
    String dns1 = webServer->arg("dns1");
    _optionalIPFromString(&_sta_static_dns1, dns1.c_str());
  }
  if (webServer->hasArg("dns2"))
  {
    I::get().logger() << F("[EWC CS]: static DNS 2") << webServer->arg("dns2") << endl;
    String dns2 = webServer->arg("dns2");
    _optionalIPFromString(&_sta_static_dns2, dns2.c_str());
  }
  webServer->send(200, FPSTR(PROGMEM_CONFIG_TEXT_HTML), FPSTR(HTML_WIFI_STATE));
  // const char* ssid = webServer->arg("ssid").c_str();
  // const char* pass = webServer->arg("passphrase").c_str();
  _connect(webServer->arg("ssid").c_str(), webServer->arg("passphrase").c_str());
}

void ConfigServer::_onWiFiDisconnect(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  webServer->sendHeader("Location", "/wifi/setup");
  webServer->send(302, "text/plain", "");
  WiFi.disconnect(false);
}

void ConfigServer::_sendMenu(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  JsonDocument jsonDoc;
  jsonDoc["brand"] = _brand;
  jsonDoc["brandUri"] = _brandUri;
  jsonDoc["language"] = _config.paramLanguage;
  JsonArray elements = jsonDoc["elements"].to<JsonArray>();
  std::vector<MenuItem>::iterator it;
  for (it = _menu.begin(); it != _menu.end(); it++)
  {
    JsonObject jsonElements = elements.add<JsonObject>();
    jsonElements["name"] = it->name;
    jsonElements["id"] = it->entry_id;
    jsonElements["href"] = it->link;
    jsonElements["visible"] = it->visible;
  }
  String output;
  serializeJson(jsonDoc, output);
  I::get().logger() << "[EWC CS]: ESP heap: _sendMenu: " << ESP.getFreeHeap() << ", json overflowed: " << jsonDoc.overflowed() << endl;
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::_onWifiState(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  I::get().logger() << "[EWC CS]: report WifiState, connected: " << (WiFi.status() == WL_CONNECTED) << endl;
  JsonDocument jsonDoc;
  JsonObject json = jsonDoc.to<JsonObject>();
  json["ssid"] = WiFi.SSID();
  json["connected"] = WiFi.status() == WL_CONNECTED;
  json["failed"] = _disconnect_state > 0;
  json["reason"] = _disconnect_reason;
  if (WiFi.status() == WL_CONNECTED)
  {
    json["local_ip"] = WiFi.localIP().toString();
  }
  else
  {
    json["local_ip"] = "";
  }
  String output;
  serializeJson(json, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void _wiFiState2Json(JsonObject &json, bool finished, bool failed, const char *reason)
{
  json["finished"] = finished;
  json["failed"] = failed;
  json["reason"] = reason;
}

void ConfigServer::_onWifiScan(WebServer *webServer)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  _startWiFiScan();
  int n = WiFi.scanComplete();
  JsonDocument jsonDoc;
  JsonObject json = jsonDoc.to<JsonObject>();
  if (n == WIFI_SCAN_FAILED)
  {
    I::get().logger() << F("[EWC CS]: scanNetworks returned: WIFI_SCAN_FAILED!") << endl;
    _wiFiState2Json(json, true, true, "scanNetworks returned: WIFI_SCAN_FAILED!");
  }
  else if (n == WIFI_SCAN_RUNNING)
  {
    I::get().logger() << F("[EWC CS]: scanNetworks returned: WIFI_SCAN_RUNNING!") << endl;
    _wiFiState2Json(json, false, false, "");
  }
  else if (n < 0)
  {
    I::get().logger() << F("[EWC CS]: scanNetworks failed with unknown error code!") << endl;
    _wiFiState2Json(json, true, true, "scanNetworks failed with unknown error code!");
  }
  else if (n == 0)
  {
    I::get().logger() << F("✘ [EWC CS]: No networks found") << endl;
    _wiFiState2Json(json, true, true, "No networks found");
  }
  JsonArray networks = jsonDoc["networks"].to<JsonArray>();
  if (n > 0)
  {
    _wiFiState2Json(json, true, false, "");
    std::vector<String> ssids;
    String ssid;
    uint8_t encType;
    int32_t rssi;
    uint8_t *bssid;
    int32_t channel;
    bool isHidden;
    for (uint8_t i = 0; i < n; i++)
    {
#ifdef ESP8266
      bool result = WiFi.getNetworkInfo(i, ssid, encType, rssi, bssid, channel, isHidden);
#else
      bool result = WiFi.getNetworkInfo(i, ssid, encType, rssi, bssid, channel);
#endif
      // do not add the same station twice
      if (result && !ssid.isEmpty() && std::find(ssids.begin(), ssids.end(), ssid) == ssids.end())
      {
        JsonObject jsonNetwork = networks.add<JsonObject>();
        jsonNetwork["ssid"] = ssid;
#ifdef ESP8266
        jsonNetwork["encrypted"] = encType == ENC_TYPE_NONE ? false : true;
#else
        jsonNetwork["encrypted"] = encType == wifi_auth_mode_t::WIFI_AUTH_OPEN ? false : true;
#endif
        jsonNetwork["rssi"] = rssi;
        jsonNetwork["channel"] = channel;
        char mac[18] = {0};
        sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        jsonNetwork["bssid"] = mac;
        jsonNetwork["hidden"] = false;
        ssids.push_back(ssid);
      }
    }
    String output;
    serializeJson(json, output);
    webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
    // WiFi.scanDelete();
    return;
  }
  else if (n == -2)
  {
    _startWiFiScan(true);
    _wiFiState2Json(json, false, false, "");
  }
  String output;
  serializeJson(json, output);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void ConfigServer::loop()
{
  _led.loop();
  if (WiFi.getMode() == WIFI_AP_STA)
  {
    _dnsServer.processNextRequest();
  }
  _server.handleClient();
  if (!_config.paramWifiDisabled)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      if (_reconnectTs > 0 && millis() > _reconnectTs)
      {
        _reconnectTs = 0;
        I::get().logger() << F("[EWC CS]: reconnect to WiFi...") << endl;
        _connect();
      }
      // after _msConfigPortalTimeout the portal will be disabled
      // this can be enabled after restart or successful connect
      if (!_ap_disabled_after_timeout)
      {
        _connected_wifi = false;
        bool startAP = false;
        // check for timeout
        if (millis() - _msConnectStart > _msConnectTimeout)
        {
          startAP = true;
        }
        else
        {
          // check for errors
          if (_disconnect_state > 0)
          {
            startAP = true;
          }
          else if (WiFi.SSID().length() == 0)
          {
            // check for valid WiFi credentials
            startAP = true;
          }
        }
        if (startAP && !isAP())
        {
          if (millis() - _msConnectStart > _msConnectTimeout)
          {
            I::get().logger() << F("[EWC CS]: _msConnectTimeout=") << _msConnectTimeout << ", start AP" << endl;
          }

          if (_disconnect_state > 0)
          {
            I::get().logger() << F("[EWC CS]: _disconnect_state=") << _disconnect_state << ", start AP" << endl;
          }
          else if (WiFi.SSID().length() == 0)
          {
            I::get().logger() << F("[EWC CS]: SSID is empty, start AP") << endl;
          }
          // start AP
          I::get().logger() << F("[EWC CS]: change WiFi mode to AP_STA") << endl;
          WiFi.mode(WIFI_AP_STA);
          _startAP();
        }
        else if (_msConfigPortalTimeout > 0 && millis() - _msConfigPortalStart > _msConfigPortalTimeout)
        {
          if (_softAPClientCount <= 0)
          {
            _ap_disabled_after_timeout = true;
            if (WiFi.SSID().length() == 0)
            {
              I::get().logger() << F("✘ [EWC CS]: config portal timeout: disable WiFi, since no valid SSID available") << endl;
              WiFi.mode(WIFI_OFF);
              _led.stop();
            }
            else
            {
              I::get().logger() << F("✘ [EWC CS]: config portal timeout: disable AP") << endl;
              WiFi.mode(WIFI_STA);
              _led.start(LED_GREEN, 1000, 256);
            }
          }
        }
      }
    }
    else
    {
      if (!_connected_wifi)
      {
        _connected_wifi = true;
        _reconnectTs = 0;
        I::get().logger() << F("[EWC CS]: connected IP: ") << WiFi.localIP().toString() << endl;
        // Stop LED
        _led.stop();
      }
    }
  }
}

void ConfigServer::setBrand(const char *brand, const char *version)
{
  _brand = String(brand);
  _version = String(version);
}

void ConfigServer::sendContentP(WebServer *webServer, const String &contentType, PGM_P content)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  _sendContentNoAuthP(webServer, contentType, content);
}

void ConfigServer::sendContentG(WebServer *webServer, const String &contentType, const uint8_t *content, size_t len)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  _sendContentNoAuthG(webServer, contentType, content, len);
}

void ConfigServer::sendPageSuccess(WebServer *webServer, String title, String summary, String urlBack, String details, String nameBack, String urlForward, String nameForward)
{
  String result(FPSTR(HTML_EWC_SUCCESS));
  result.replace("{{TITLE}}", title);
  result.replace("{{SUMMARY}}", summary);
  result.replace("{{DETAILS}}", details);
  result.replace("{{BACK}}", urlBack);
  result.replace("{{BACK_NAME}}", nameBack);
  result.replace("{{FORWARD}}", urlForward);
  result.replace("{{FWD_NAME}}", nameForward);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_TEXT_HTML), result);
}

void ConfigServer::sendPageFailed(WebServer *webServer, String title, String summary, String urlBack, String details, String nameBack, String urlForward, String nameForward)
{
  String result(FPSTR(HTML_EWC_FAIL));
  result.replace("{{TITLE}}", title);
  result.replace("{{SUMMARY}}", summary);
  result.replace("{{DETAILS}}", details);
  result.replace("{{BACK}}", urlBack);
  result.replace("{{BACK_NAME}}", nameBack);
  result.replace("{{FORWARD}}", urlForward);
  result.replace("{{FWD_NAME}}", nameForward);
  webServer->send(200, FPSTR(PROGMEM_CONFIG_TEXT_HTML), result);
}

void ConfigServer::sendRedirect(WebServer *webServer, String url)
{
  webServer->sendHeader("Location", url);
  webServer->send(302, "text/plain", "");
}

void ConfigServer::_sendFileContent(WebServer *webServer, const String &contentType, const String &filename)
{
  if (!isAuthenticated(webServer))
  {
    return webServer->requestAuthentication();
  }
  I::get().logger() << F("[EWC CS]: Handle sendFileContent: ") << filename << endl;
  File reqFile = LittleFS.open(filename, "r");
  if (reqFile && !reqFile.isDirectory())
  {
    webServer->streamFile(reqFile, contentType);
    reqFile.close();
    return;
  }
  _onNotFound(webServer);
}

void ConfigServer::_sendContentNoAuthP(WebServer *webServer, const String &contentType, PGM_P content)
{
  if (ESP.getFreeHeap() <= strlen_P(content))
  {
    I::get().logger() << F("✘ [EWC CS]: Not enough memory to reply request ") << webServer->uri() << F("; free: ") << ESP.getFreeHeap() << F(", needed: ") << strlen_P(content) << endl;
    webServer->send(200, "text/plain", F("Not enough memory"));
  }
  else
  {
    webServer->send(200, contentType.c_str(), content);
  }
}

void ConfigServer::_sendContentNoAuthG(WebServer *webServer, const String &contentType, const uint8_t *content, size_t len)
{
  I::get().logger() << F("[EWC CS]: send content for ") << webServer->uri() << F("; free: ") << ESP.getFreeHeap() << F(", needed: ") << len << endl;
  if (ESP.getFreeHeap() <= 4000)
  {
    I::get().logger() << F("✘ [EWC CS]: Not enough memory to reply request ") << webServer->uri() << F("; free: ") << ESP.getFreeHeap() << F(", needed: ") << len << endl;
    sendRedirect(webServer, webServer->uri());
    webServer->send(406, "text/plain", F("Not enough memory"));
  }
  else
  {
    I::get().logger() << F("content type: ") << contentType << endl;
    webServer->sendHeader("Content-Encoding", "gzip");
    webServer->sendHeader("Content-Disposition", "inline");
#ifdef ESP8266
    webServer->send(200, contentType.c_str(), content, len);
#else
    webServer->send(200, contentType.c_str(), String(content, len));
#endif
  }
}

void ConfigServer::_onNotFound(WebServer *webServer)
{
  I::get().logger() << F("[EWC CS]: Handle not found") << endl;
  if (_captivePortal(webServer))
  { // If captive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer->uri();
  message += "\nMethod: ";
  message += (webServer->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer->args();
  message += "\n";
  for (int i = 0; i < webServer->args(); i++)
  {
    message += " " + webServer->argName(i) + ": " + webServer->arg(i) + "\n";
  }
  webServer->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  webServer->sendHeader("Pragma", "no-cache");
  webServer->sendHeader("Expires", "-1");
  webServer->sendHeader("Content-Length", String(message.length()));
  webServer->send(404, "text/plain", message);
}

void ConfigServer::_onDeviceReset(WebServer *webServer)
{
  I::get().logger() << F("[EWC CS]: delete config file") << endl;
  I::get().configFS().deleteFile();
  sendRedirect(webServer, "/");
  ESP.restart();
}

void ConfigServer::_onDeviceRestart(WebServer *webServer)
{
  I::get().logger() << F("[EWC CS]: restart by user request") << endl;
  sendRedirect(webServer, "/");
  ESP.restart();
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
bool ConfigServer::_captivePortal(WebServer *webServer)
{
  if (!_isIp(webServer->hostHeader()))
  {
    I::get().logger() << F("[EWC CS]: Request host is not an IP: ") << webServer->hostHeader() << endl;
    I::get().logger() << F("[EWC CS]: Request redirected to captive portal: ") << _toStringIp(webServer->client().localIP()) << endl;
    webServer->sendHeader("Location", String("http://") + _toStringIp(webServer->client().localIP()) + "/wifi/setup");
    webServer->send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    webServer->client().stop();             // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void ConfigServer::_startWiFiScan(bool force)
{
  if (force || millis() - _msWifiScanStart > WIFI_SCAN_DELAY)
  {
    WiFi.scanNetworks(true, true);
    _msWifiScanStart = millis();
  }
}

bool ConfigServer::isAuthenticated(WebServer *webServer)
{
  return !(_config.paramBasicAuth && !webServer->authenticate(_config.paramHttpUser.c_str(), _config.paramHttpPassword.c_str()));
}

/** Is this an IP? */
bool ConfigServer::_isIp(const String &str)
{
  for (unsigned int i = 0; i < str.length(); i++)
  {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9'))
    {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String ConfigServer::_toStringIp(const IPAddress &ip)
{
  String res = "";
  for (int i = 0; i < 3; i++)
  {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
