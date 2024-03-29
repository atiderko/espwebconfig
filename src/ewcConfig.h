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

namespace EWC
{

  const char VERSION[] PROGMEM = "0.1.0";
  const char DEFAULT_HTTP_USER[] = "admin";
  const char DEFAULT_HTTP_PASSWORD[] = "";
  const unsigned long WIFI_SCAN_DELAY = 10000;
  const char BOOT_MODE_FILENAME[] PROGMEM = "/boot.mode";

  enum BootMode
  {
    NORMAL = 1,
    STANDALONE = 2,
    CONFIGURATION = 3
  };

  class Config : public ConfigInterface
  {
  public:
    Config();
    ~Config();

    /** === ConfigInterface Methods === **/
    void setup(JsonDocument &config, bool resetConfig = false);
    void fillJson(JsonDocument &config);

    /** === BOOT mode handling  === **/
    void setBootMode(BootMode mode, bool forceWrite = false);
    BootMode getBootMode() { return _bootMode; }
    bool wifiDisabled() { return _bootMode == BootMode::STANDALONE; }

    /** === Parameter === **/
    String paramAPName;
    const String &getAPPass() { return _paramAPPass; }
    void setAPPass(String pass);
    String paramDeviceName;
    bool paramAPStartAlways;
    bool paramWifiDisabled;
    bool paramBasicAuth;
    String paramHttpUser;
    String paramHttpPassword;
    String paramHostname;
    String paramLanguage;
    String getChipId();
    bool disableLogSetting = false;

  protected:
    BootMode _bootMode;

    String _paramAPPass;

    void _initParams();
    void _fromJson(JsonDocument &doc);
  };

};
#endif
