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

#ifndef EWC_CONFIG_h
#define EWC_CONFIG_h

#include <Arduino.h>
#include "ewcConfigInterface.h"


namespace EWC {

const char VERSION[] PROGMEM = "0.1.0";
const char DEFAULT_HTTP_USER[] = "admin";
const char DEFAULT_HTTP_PASSWORD[] = "";

enum BootMode {
    NORMAL = 0x10101010,
    STANDALONE = 0x12121212,
    CONFIGURATION = 0x02020202
};


class Config : public ConfigInterface
{
public:
    Config();
    ~Config();

    void save();

    String& version() { return _version; }

    /** === ConfigInterface Methods === **/    
    void setup(JsonDocument& doc, bool resetConfig=false);
    void loop();
    void json(JsonDocument& doc);

    /** === BOOT mode handling  === **/
    void setBootMode(BootMode mode);
    BootMode getBootMode() { return _bootMode; }
    bool wifiDisabled() { return _bootMode == BootMode::STANDALONE; }

    /** === Parameter === **/
    const String& getVersion() { return _version; }
    String paramAPName;
    const String& getAPPass() { return _paramAPPass; }
    void setAPPass(String pass);
    bool paramAPStartAlways;
    bool paramWifiDisabled;
    String paramHttpUser;
    String paramHttpPassword;

protected:
    String _version;
    uint8_t _bootmodeUtcAddress;
    BootMode _bootMode;

    String _paramAPPass;

    void _initParams();
    void _fromJson(JsonDocument& doc);
    uint32_t _getChipId();
};

};
#endif