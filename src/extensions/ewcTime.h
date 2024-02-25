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

#ifndef EWC_TIME_h
#define EWC_TIME_h

#if defined(ESP8266)
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#define WebServer ESP8266WebServer
#elif defined(ESP32)
#include "WiFi.h"
#include "WebServer.h"
#endif

#include <Arduino.h>
#include <time.h>     // time() ctime()
#include <sys/time.h> // struct timeval

// #include <ESPAsyncWebServer.h>
#include "../ewcConfigInterface.h"
#include "./ewcTimeZone.h"

namespace EWC
{
  class Time : public ConfigInterface
  {
  public:
    Time();
    ~Time();

    /** === ConfigInterface Methods === **/
    void setup(JsonDocument &config, bool resetConfig = false);
    void fillJson(JsonDocument &config);

    bool timeAvailable();
    void setLocalTime(String &date, String &time);
    /** Current time as string.
     * param offsetSeconds: Offset in seconds to now **/
    String str(time_t offsetSeconds = 0);
    time_t currentTime();
    bool dndEnabled();
    /** Checks if current time (shifted by offsetSeconds) is in Do not Disturb period. **/
    bool isDisturb(time_t offsetSeconds = 0);
    /** If current time (shifted by offsetSeconds) is in Do not Disturb period
     * returns the count of seconds until end of DnD period (included offsetSeconds). **/
    time_t shiftDisturb(time_t offsetSeconds = 0);
    const String &dndTo();

  protected:
    time_t _manualOffset; //< used for manually time

    /** === Parameter === **/
    int _paramTimezone;
    long _zoneOffset;
    long _dstOffset;
    bool _paramManually;
    String _paramDate;
    String _paramTime;
    bool _paramDndEnabled;
    String _paramDndFrom;
    String _paramDndTo;

    void _initParams();
    void _fromJson(JsonDocument &config);
    void _callbackTimeSet(void);
    void _onTimeConfig(WebServer *request);
    void _onTimeSave(WebServer *request);

    time_t _dndToMin(String &hmTime);
    boolean _summertimeEU(int year, byte month, byte day, byte hour, byte tzHours);

    void _setupTime();
  };

};
#endif
