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
#ifndef EWC_INTERFACE_H
#define EWC_INTERFACE_H

#include "Arduino.h"
#include "ewcLogger.h"
#include "ewcStreamingOperator.h"

namespace EWC {
class ConfigServer;
class Config;
class ConfigFS;
#if defined(ESP8266)
class RTC;
#endif
class TickerLed;

/** The objects of the interface are initialized by ConfigServer.
 * The interface is globally reachable through I::get(). **/
class InterfaceData {
  friend ConfigServer;

  public:
    InterfaceData() {}

    ConfigServer& server() { return *_server; }
    Config& config() { return *_config; }
    ConfigFS& configFS() { return *_configFS; }
    Logger& logger() { return *_logger; }
#if defined(ESP8266)
    RTC& rtc() { return *_rtc; }
#endif
    TickerLed& led() { return *_led; }

  private:
    ConfigServer* _server;
    Config* _config;
    ConfigFS* _configFS;
    Logger* _logger;
#if defined(ESP8266)
    RTC* _rtc;
#endif
    TickerLed* _led;
};


class I {
  public:
    static InterfaceData& get() { return _interface; }

  private:
    // Initialized in ewcConfigServer.cpp
    static InterfaceData _interface;
};


};

#endif
