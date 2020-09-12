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
#include "ewcRtc.h"
#include "ewcLogger.h"


using namespace EWC;

Config::Config() : ConfigInterface("ewc")
{
    // initialize utc addresses in sutup method
    _bootmodeUtcAddress = 0;
    _bootMode = BootMode::CONFIGURATION;
    _initParams();
}

Config::~Config()
{
}

void Config::setup(JsonDocument& doc, bool resetConfig)
{
    I::get().logger() << F("[EWC Config]: setup EWC config") << endl;
    _bootmodeUtcAddress = I::get().rtc().get();
    if (resetConfig) {
        I::get().logger() << F("[EWC Config]: reset -> set bootmode to CONFIGURATION") << endl;
        setBootMode(BootMode::CONFIGURATION);
    } else {
        I::get().logger() << F("[EWC Config]: read boot mode flag from ") << _bootmodeUtcAddress << endl;
        // read boot mode
        uint32_t boot_mode = I::get().rtc().read(_bootmodeUtcAddress);
        if (boot_mode == BootMode::NORMAL) {
            I::get().logger() << F("[EWC Config]: boot flag is NORMAL: ") << boot_mode << endl;
            _bootMode = BootMode::NORMAL;
        } else if (boot_mode == BootMode::CONFIGURATION) {
            I::get().logger() << F("[EWC Config]: boot flag is CONFIGURATION") << boot_mode << endl;
            _bootMode = BootMode::CONFIGURATION;
        } else if (boot_mode == BootMode::STANDALONE) {
            I::get().logger() << F("[EWC Config]: boot flag is STANDALONE") << boot_mode << endl;
            _bootMode = BootMode::STANDALONE;
        } else {
            I::get().logger() << F("[EWC Config]: reset boot flag to ") << _bootMode << endl;
            I::get().rtc().write(_bootmodeUtcAddress, _bootMode);
        }
    }
    I::get().logger() << F("[EWC Config]: read configuration") << endl;
    if (resetConfig) {
        I::get().logger() << F("[EWC Config]: reset -> use default configuration") << endl;
        _initParams();
    } else {
        _fromJson(doc);
    }
}


void Config::loop()
{
}

void Config::json(JsonDocument& doc)
{
    JsonVariant etcJson = doc["ewc"];
    if (etcJson.isNull()) {
        etcJson = doc.createNestedObject("ewc");
    }
    etcJson["apname"] = paramAPName;
    etcJson["appass"] = _paramAPPass;
    etcJson["ap_start_always"] = paramAPStartAlways;
    etcJson["wifi_disabled"] = paramWifiDisabled;
    etcJson["basic_auth"] = paramBasicAuth;
    etcJson["httpuser"] = paramHttpUser;
    etcJson["httppass"] = paramHttpPassword;
}

void Config::_fromJson(JsonDocument& doc)
{
    JsonVariant jsonWifiDisabled = doc["ewc"]["wifi_disabled"];
    if (!jsonWifiDisabled.isNull()) {
        paramWifiDisabled = jsonWifiDisabled.as<bool>();
    }
    JsonVariant jsonApName = doc["ewc"]["apname"];
    if (!jsonApName.isNull()) {
        paramAPName = jsonApName.as<String>();
    }
    JsonVariant jsonApPass = doc["ewc"]["appass"];
    if (!jsonApPass.isNull()) {
        setAPPass(jsonApPass.as<String>());
    }
    JsonVariant jsonApStartAlways = doc["ewc"]["ap_start_always"];
    if (!jsonApStartAlways.isNull()) {
        paramAPStartAlways = jsonApStartAlways.as<bool>();
    }
    JsonVariant jsonBasicAuth = doc["ewc"]["basic_auth"];
    if (!jsonBasicAuth.isNull()) {
        paramBasicAuth = jsonBasicAuth.as<bool>();
    }
    JsonVariant jsonHttpUser = doc["ewc"]["httpuser"];
    if (!jsonHttpUser.isNull()) {
        paramHttpUser = jsonHttpUser.as<String>();
    }
    JsonVariant jsonHttpPass = doc["ewc"]["httppass"];
    if (!jsonHttpPass.isNull()) {
        paramHttpPassword = jsonHttpPass.as<String>();
    }
}

// ConfigInterface* Config::sub_config(String name) {
//     if (_cfgInterfaces.find(name) != _cfgInterfaces.end())
//         return _cfgInterfaces[name];
//     return NULL;
// }

void Config::setBootMode(BootMode mode)
{
    if (mode != getBootMode()) {
        I::get().logger() << F("[EWC Config]: write bootmode ") << (uint32_t)mode << " to: " << _bootmodeUtcAddress << endl;
        I::get().rtc().write(_bootmodeUtcAddress, mode);
    }
}

void Config::setAPPass(String pass)
{
    if (!pass.isEmpty()) {
        if (pass.length() < 8 || pass.length() > 63) {
            // fail passphrase to short or long!
            I::get().logger() << F("✘ [EWC Config]: Invalid AccessPoint password. Ignoring") << endl;
        } else {
            _paramAPPass = pass;
            I::get().logger() << F("[EWC Config]: Set new AccessPoit password to ") << _paramAPPass << endl;
        }
    } else {
        _paramAPPass = pass;
        I::get().logger() << F("[EWC Config]: Clear AccessPoit password") << endl;
    }
}

void Config::_initParams()
{
    paramAPName = String("ewc-") + String(getChipId());
    _paramAPPass = "";
    paramAPStartAlways = false;
    paramWifiDisabled = false;
    paramBasicAuth = false;
    paramHttpUser = DEFAULT_HTTP_USER;
    paramHttpPassword = DEFAULT_HTTP_PASSWORD;
}

uint32_t Config::getChipId() {
#if defined(ARDUINO_ARCH_ESP8266)
  return ESP.getChipId();
#elif defined(ARDUINO_ARCH_ESP32)
  uint64_t  chipId;
  chipId = ESP.getEfuseMac();
  return (uint32_t)(chipId >> 32);
#endif
}