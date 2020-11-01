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
    # define WebServer ESP8266WebServer
#elif defined(ESP32)
    #include "WiFi.h"
    #include "WebServer.h"
#endif

#include <Arduino.h>
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()
// #include <ESPAsyncWebServer.h>
#include "../ewcConfigInterface.h"


namespace EWC {

    const float PROGMEM TZMAP[82][2] {
        {0, -12},  // 1 International Date Line West
        {0, -11},  // 2 Midway Island, Samoa
        {0, -10},  // 3 Hawaii
        {1, -9},   // 4 Alaska
        {1, -8},   // 5 Pacific Time (US & Canada)
        {1, -8},   // 6 Tijuana, Baja California
        {0, -7},   // 7 Arizona
        {1, -7},   // 8 Chihuahua, La Paz, Mazatlan
        {1, -7},   // 9 Mountain Time (US & Canada)
        {0, -6},   // 10 Central America
        {1, -6},   // Central Time (US & Canada)
        {1, -6},   // Guadalajara, Mexico City, Monterrey
        {0, -6},   // Saskatchewan
        {0, -5},   // Bogota, Lima, Quito, Rio Branco
        {1, -5},   // Eastern Time (US & Canada)
        {1, -5},   // Indiana (East)
        {1, -4},   // Atlantic Time (Canada)
        {0, -4},   // Caracas, La Paz
        {0, -4},   // Manaus
        {1, -4},   // 20 Santiago
        {1, -3.5},   // Newfoundland
        {1, -3},   // Brasilia
        {0, -3},   // Buenos Aires, Georgetown
        {1, -3},   // Greenland
        {1, -3},   // Montevideo
        {1, -2},   // Mid-Atlantic
        {0, -1},   // Cape Verde Is.
        {1, -1},   // Azores
        {0, 0},   // Casablanca, Monrovia, Reykjavik
        {1, 0},   // 30 Greenwich Mean Time : Dublin, Edinburgh, Lisbon, London
        {1, 1},   // Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna
        {1, 1},   // Belgrade, Bratislava, Budapest, Ljubljana, Prague
        {1, 1},   // Brussels, Copenhagen, Madrid, Paris
        {1, 1},   // Sarajevo, Skopje, Warsaw, Zagreb
        {1, 1},   // West Central Africa
        {1, 2},   // Amman
        {1, 2},   // Athens, Bucharest, Istanbul
        {1, 2},   // Beirut
        {1, 2},   // Cairo
        {0, 2},   // 40 Harare, Pretoria
        {1, 2},   // Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius
        {1, 2},   // Jerusalem
        {1, 2},   // Minsk
        {1, 2},   // Windhoek
        {0, 3},   // Kuwait, Riyadh, Baghdad
        {1, 3},   // Moscow, St. Petersburg, Volgograd
        {0, 3},   // Nairobi
        {0, 3},   // Tbilisi
        {1, 3.5},   // Tehran
        {0, 4},   // 50 Abu Dhabi, Muscat
        {1, 4},   // Baku
        {1, 4},   // Yerevan
        {0, 4.5},   // Kabul
        {1, 5},   // Yekaterinburg
        {0, 5},   // Islamabad, Karachi, Tashkent
        {0, 5.5},   // Sri Jayawardenapura
        {0, 5.5},   // Chennai, Kolkata, Mumbai, New Delhi
        {0, 5.75},   // Kathmandu
        {1, 6},   // Almaty, Novosibirsk
        {0, 6},   // 60 Astana, Dhaka
        {0, 6.5},   // Yangon (Rangoon)
        {0, 7},   // Bangkok, Hanoi, Jakarta
        {1, 7},   // Krasnoyarsk
        {0, 8},   // Beijing, Chongqing, Hong Kong, Urumqi
        {0, 8},   // Kuala Lumpur, Singapore
        {0, 8},   // Irkutsk, Ulaan Bataar
        {0, 8},   // Perth
        {0, 8},   // Taipei
        {0, 9},   // Osaka, Sapporo, Tokyo
        {0, 9},   // 70 Seoul
        {1, 9},   // Yakutsk
        {0, 9.5},   // Adelaide
        {0, 9.5},   // Darwin
        {0, 10},   // Brisbane
        {1, 10},   // Canberra, Melbourne, Sydney
        {1, 10},   // Hobart
        {0, 10},   // Guam, Port Moresby
        {1, 10},   // Vladivostok
        {1, 11},   // Magadan, Solomon Is., New Caledonia
        {1, 12},   // 80 Auckland, Wellington
        {0, 12},   // Fiji, Kamchatka, Marshall Is.
        {0, 13}   // Nuku'alofa
    };

class Time : public ConfigInterface
{
public:
    Time();
    ~Time();

    /** === ConfigInterface Methods === **/    
    void setup(JsonDocument& config, bool resetConfig=false);
    void fillJson(JsonDocument& config);

    bool ntpAvailable() { return _ntpAvailable; }
    void setLocalTime(String& date, String& time);
    /** Current time as string.
     * param offsetSeconds: Offset in seconds to now **/
    String str(time_t offsetSeconds=0);
    time_t currentTime();
    bool dndEnabled();
    /** Checks if current time (shifted by offsetSeconds) is in Do not Disturb period. **/
    bool isDisturb(time_t offsetSeconds=0);
    /** If current time (shifted by offsetSeconds) is in Do not Disturb period
     * returns the count of seconds until end of DnD period (included offsetSeconds). **/
    time_t shiftDisturb(time_t offsetSeconds=0);
    const String& dndTo();

protected:
    bool _ntpAvailable;
    time_t _manuallOffset;  //< used for manually time

    /** === Parameter === **/
    int _paramTimezone;
    bool _paramManually;
    String _paramDate;
    String _paramTime;
    bool _paramDndEnabled;
    String _paramDndFrom;
    String _paramDndTo;

    void _initParams();
    void _fromJson(JsonDocument& config);
    void _callbackTimeSet(void);
    void _onTimeConfig(WebServer* request);
    void _onTimeSave(WebServer* request);

    time_t _dndToMin(String& hmTime);
    boolean _summertimeEU(int year, byte month, byte day, byte hour, byte tzHours);
};

};
#endif
