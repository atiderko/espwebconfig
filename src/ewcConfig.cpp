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
    Serial.println("create config");
    // initialize utc addresses in sutup method
    _bootmodeUtcAddress = 0;
    _version = "not-set";
    _bootMode = BootMode::CONFIGURATION;
    _initParams();
    Serial.println("ok");
}

Config::~Config()
{
}

void Config::setup(JsonDocument& doc, bool resetConfig)
{
    ETCI::get().logger() << F("[EWC Config]: setup EWC config") << endl;
    _bootmodeUtcAddress = ETCI::get().rtc().get();
    if (resetConfig) {
        ETCI::get().logger() << F("[EWC Config]: reset -> set bootmode to CONFIGURATION") << endl;
        setBootMode(BootMode::CONFIGURATION);
    } else {
        ETCI::get().logger() << F("[EWC Config]: read boot mode flag") << endl;
        // read boot mode
        uint32_t boot_mode = ETCI::get().rtc().read(_bootmodeUtcAddress);
        Serial.println(boot_mode);
        if (boot_mode == BootMode::NORMAL) {
            ETCI::get().logger() << F("[EWC Config]: boot flag is NORMAL") << endl;
            _bootMode = BootMode::NORMAL;
        } else if (boot_mode == BootMode::CONFIGURATION) {
            ETCI::get().logger() << F("[EWC Config]: boot flag is CONFIGURATION") << endl;
            _bootMode = BootMode::CONFIGURATION;
        } else if (boot_mode == BootMode::STANDALONE) {
            ETCI::get().logger() << F("[EWC Config]: boot flag is STANDALONE") << endl;
            _bootMode = BootMode::STANDALONE;
        } else {
            ETCI::get().logger() << F("[EWC Config]: reset boot flag to ") << _bootMode << endl;
            ETCI::get().rtc().write(_bootmodeUtcAddress, _bootMode);
        }
    }
    ETCI::get().logger() << F("[EWC Config]: read configuration") << endl;
    if (resetConfig) {
        ETCI::get().logger() << F("[EWC Config]: reset -> use default configuration") << endl;
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
    etcJson["version"] = _version;
    etcJson["ap_name"] = paramAPName;
    etcJson["ap_pass"] = _paramAPPass;
    etcJson["ap_start_always"] = paramAPStartAlways;
    etcJson["wifi_disabled"] = paramWifiDisabled;
    etcJson["http_user"] = paramHttpUser;
    etcJson["http_pass"] = paramHttpPassword;
}

void Config::_fromJson(JsonDocument& doc)
{
    JsonVariant jsonWifiDisabled = doc["ewc"]["wifi_disabled"];
    if (!jsonWifiDisabled.isNull()) {
        paramWifiDisabled = jsonWifiDisabled.as<bool>();
    }
    JsonVariant jsonApName = doc["ewc"]["ap_name"];
    if (!jsonApName.isNull()) {
        paramAPName = jsonApName.as<String>();
    }
    JsonVariant jsonApPass = doc["ewc"]["ap_pass"];
    if (!jsonApPass.isNull()) {
        setAPPass(jsonApPass.as<String>());
    }
    JsonVariant jsonApStartAlways = doc["ewc"]["ap_start_always"];
    if (!jsonApStartAlways.isNull()) {
        paramAPStartAlways = jsonApStartAlways.as<bool>();
    }
    JsonVariant jsonHttpUser = doc["ewc"]["http_user"];
    if (!jsonHttpUser.isNull()) {
        paramHttpUser = jsonHttpUser.as<String>();
    }
    JsonVariant jsonHttpPass = doc["ewc"]["http_pass"];
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
    ETCI::get().rtc().write(_bootmodeUtcAddress, mode);
}

void Config::setAPPass(String pass)
{
    if (!pass.isEmpty()) {
        if (pass.length() < 8 || pass.length() > 63) {
            // fail passphrase to short or long!
            ETCI::get().logger() << F("Invalid AccessPoint password. Ignoring") << endl;
        } else {
            _paramAPPass = pass;
            ETCI::get().logger() << F("Set new AccessPoit password to ") << _paramAPPass << endl;
        }
    } else {
        _paramAPPass = pass;
        ETCI::get().logger() << F("Clear AccessPoit password") << endl;
    }
}

void Config::_initParams()
{
    paramAPName = String("ewc-") + String(_getChipId());
    _paramAPPass = "";
    paramAPStartAlways = false;
    paramWifiDisabled = false;
    paramHttpUser = DEFAULT_HTTP_USER;
    paramHttpPassword = DEFAULT_HTTP_PASSWORD;
}

uint32_t Config::_getChipId() {
#if defined(ARDUINO_ARCH_ESP8266)
  return ESP.getChipId();
#elif defined(ARDUINO_ARCH_ESP32)
  uint64_t  chipId;
  chipId = ESP.getEfuseMac();
  return (uint32_t)(chipId >> 32);
#endif
}