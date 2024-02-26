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

#include <LittleFS.h>
#include "ewcConfig.h"
#ifdef ESP8266
#include "ewcRtc.h"
#endif
#include "ewcLogger.h"

using namespace EWC;

Config::Config() : ConfigInterface("ewc")
{
  // initialize utc addresses in setup method
  _bootModeUtcAddress = 0;
  _bootMode = BootMode::CONFIGURATION;
  _initParams();
}

Config::~Config()
{
}

void Config::setup(JsonDocument &config, bool resetConfig)
{
#ifdef ESP8266
  I::get().logger() << F("[EWC Config]: setup EWC config") << endl;
  _bootModeUtcAddress = I::get().rtc().get();
  if (resetConfig)
  {
    I::get().logger() << F("[EWC Config]: reset -> set boot mode to CONFIGURATION") << endl;
    setBootMode(BootMode::CONFIGURATION);
  }
  else
  {
    I::get().logger() << F("[EWC Config]: read boot mode flag from ") << _bootModeUtcAddress << endl;
    // read boot mode
    uint32_t boot_mode = I::get().rtc().read(_bootModeUtcAddress);
    if (boot_mode == BootMode::NORMAL)
    {
      I::get().logger() << F("[EWC Config]: boot flag is NORMAL: ") << boot_mode << endl;
      _bootMode = BootMode::NORMAL;
    }
    else if (boot_mode == BootMode::CONFIGURATION)
    {
      I::get().logger() << F("[EWC Config]: boot flag is CONFIGURATION") << boot_mode << endl;
      _bootMode = BootMode::CONFIGURATION;
    }
    else if (boot_mode == BootMode::STANDALONE)
    {
      I::get().logger() << F("[EWC Config]: boot flag is STANDALONE") << boot_mode << endl;
      _bootMode = BootMode::STANDALONE;
    }
    else
    {
      I::get().logger() << F("[EWC Config]: reset boot flag to ") << _bootMode << endl;
      I::get().rtc().write(_bootModeUtcAddress, _bootMode);
    }
  }
#endif
  I::get().logger() << F("[EWC Config]: read configuration") << endl;
  if (resetConfig)
  {
    I::get().logger() << F("[EWC Config]: reset -> use default configuration") << endl;
    _initParams();
  }
  else
  {
    _fromJson(config);
  }
}

void Config::fillJson(JsonDocument &config)
{
  config["ewc"]["enable_serial_log"] = I::get().logger().enabled();
  config["ewc"]["dev_name"] = paramDeviceName;
  config["ewc"]["apName"] = paramAPName;
  config["ewc"]["apPass"] = _paramAPPass;
  config["ewc"]["ap_start_always"] = paramAPStartAlways;
  config["ewc"]["wifi_disabled"] = paramWifiDisabled;
  config["ewc"]["basic_auth"] = paramBasicAuth;
  config["ewc"]["httpUser"] = paramHttpUser;
  config["ewc"]["httpPass"] = paramHttpPassword;
  config["ewc"]["hostname"] = paramHostname;
}

void Config::_fromJson(JsonDocument &doc)
{
  JsonVariant jv = doc["ewc"]["wifi_disabled"];
  if (!jv.isNull())
  {
    paramWifiDisabled = jv.as<bool>();
  }
  jv = doc["ewc"]["dev_name"];
  if (!jv.isNull())
  {
    paramDeviceName = jv.as<String>();
  }
  jv = doc["ewc"]["enable_serial_log"];
  if (!jv.isNull())
  {
    I::get().logger().setLogging(jv.as<bool>());
  }
  jv = doc["ewc"]["apName"];
  if (!jv.isNull())
  {
    paramAPName = jv.as<String>();
  }
  jv = doc["ewc"]["apPass"];
  if (!jv.isNull())
  {
    setAPPass(jv.as<String>());
  }
  jv = doc["ewc"]["ap_start_always"];
  if (!jv.isNull())
  {
    paramAPStartAlways = jv.as<bool>();
  }
  jv = doc["ewc"]["basic_auth"];
  if (!jv.isNull())
  {
    paramBasicAuth = jv.as<bool>();
  }
  jv = doc["ewc"]["httpUser"];
  if (!jv.isNull())
  {
    paramHttpUser = jv.as<String>();
  }
  jv = doc["ewc"]["httpPass"];
  if (!jv.isNull())
  {
    paramHttpPassword = jv.as<String>();
  }
  jv = doc["ewc"]["hostname"];
  if (!jv.isNull())
  {
    paramHostname = jv.as<String>();
  }
}

// ConfigInterface* Config::sub_config(String name) {
//     if (_cfgInterfaces.find(name) != _cfgInterfaces.end())
//         return _cfgInterfaces[name];
//     return NULL;
// }

void Config::setBootMode(BootMode mode)
{
  if (mode != getBootMode())
  {
    I::get().logger() << F("[EWC Config]: write boot mode ") << (uint32_t)mode << " to: " << _bootModeUtcAddress << endl;
#ifdef ESP8266
    I::get().rtc().write(_bootModeUtcAddress, mode);
#endif
  }
}

void Config::setAPPass(String pass)
{
  if (!pass.isEmpty())
  {
    if (pass.length() < 8 || pass.length() > 63)
    {
      // fail passphrase to short or long!
      I::get().logger() << F("✘ [EWC Config]: Invalid AccessPoint password. Ignoring") << endl;
    }
    else
    {
      _paramAPPass = pass;
      I::get().logger() << F("[EWC Config]: Set new AccessPoint password to ") << _paramAPPass << endl;
    }
  }
  else
  {
    _paramAPPass = pass;
    I::get().logger() << F("[EWC Config]: Clear AccessPoint password") << endl;
  }
}

void Config::_initParams()
{
  paramDeviceName = String("ewc-") + getChipId();
  paramAPName = String("ewc-") + getChipId();
  _paramAPPass = "";
  paramAPStartAlways = false;
  paramWifiDisabled = false;
  paramBasicAuth = false;
  paramHttpUser = DEFAULT_HTTP_USER;
  paramHttpPassword = DEFAULT_HTTP_PASSWORD;
  paramHostname = paramAPName;
  paramLanguage = "en";
}

String Config::getChipId()
{
  String id = "";
#if defined(ARDUINO_ARCH_ESP8266)
  id = String(ESP.getChipId(), HEX);
#elif defined(ARDUINO_ARCH_ESP32)
  id = String((uint32_t)ESP.getEfuseMac(), HEX);
#endif
  id.toUpperCase();
  return id;
}
