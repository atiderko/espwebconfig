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

**************************************************************

It is also possible to reset the configuration by double press
on reset. The idea is based on
https://github.com/datacute/DoubleResetDetector

**************************************************************/

#ifndef EWC_CONFIG_CONTAINER_h
#define EWC_CONFIG_CONTAINER_h

#include <Arduino.h>
#ifdef ESP32
#define USE_LittleFS
#include <vector>
#endif
#include "ewcConfigInterface.h"

namespace EWC
{

  const char RESET_FILENAME[] PROGMEM = "/reset.lock";
  const char CONFIG_FILENAME[] PROGMEM = "/ewc.json";

  class ConfigFS
  {
  public:
    ConfigFS(String filename = FPSTR(CONFIG_FILENAME));
    ~ConfigFS();

    void setup();
    void loop();
    void save();
    void deleteFile();
    /** === Configurations of modules implement ConfigInterface === **/
    void addConfig(ConfigInterface &config);
    // ConfigInterface* sub_config(String name);
    bool resetDetected() { return _resetDetected; }
    String readFrom(String fileName);
    bool saveTo(String fileName, String data);

  protected:
    bool _resetDetected;
    String _filename;
    std::vector<ConfigInterface *> _cfgInterfaces;
    bool _resetFileRemoved;
  };

};
#endif
