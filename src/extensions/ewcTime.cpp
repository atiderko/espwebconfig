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

#include "ewcTime.h"
#include "ewcConfigServer.h"
#include <ArduinoJSON.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include "generated/timeSetupHTML.h"

using namespace EWC;

Time::Time() : ConfigInterface("time")
{
    _ntpAvailable = false;
    _ntpInitialized = false;
}

Time::~Time()
{
}

void Time::setup(JsonDocument& config, bool resetConfig)
{
    settimeofday_cb([this](void) { _callbackTimeSet(); });
    I::get().logger() << "[EWC Time] setup" << endl;
    _init_params();
    _fromJson(config);
    EWC::I::get().server().insertMenuP("Time", "/time/setup", "menu_time", HTML_TIME_SETUP, FPSTR(PROGMEM_CONFIG_TEXT_HTML), true, -1);
    EWC::I::get().server().webserver().on("/time/config.json", std::bind(&Time::_onTimeConfig, this, std::placeholders::_1));
    EWC::I::get().server().webserver().on("/time/save", std::bind(&Time::_onTimeSave, this, std::placeholders::_1));
}

void Time::loop()
{
    if (!paramManually && !_ntpInitialized && WiFi.status() == WL_CONNECTED) {
        // Sync our clock to NTP
        configTime(TIMEMAP[paramTimezone-1][1] * 3600, TIMEMAP[paramTimezone-1][0] * 3600, "0.europe.pool.ntp.org", "pool.ntp.org", "time.nist.gov");
        _ntpInitialized = true;
    }
}

void Time::fillJson(JsonDocument& config)
{
    config["time"]["timezone"] = paramTimezone;
    config["time"]["manually"] = paramManually;
    config["time"]["mdate"] = paramDate;
    config["time"]["mtime"] = paramTime;
    config["time"]["dnd_enabled"] = paramDndEnabled;
    config["time"]["dnd_from"] = paramDndFrom;
    config["time"]["dnd_to"] = paramDndTo;
    config["time"]["current"] = str();
}

void Time::_init_params()
{
    paramTimezone = 31;
    paramManually = false;
    paramDate = "21.10.2020";
    paramTime = "21:00";
    paramDndEnabled = false;
    paramDndFrom = "22:00";
    paramDndTo = "06:00";
}

void Time::_fromJson(JsonDocument& doc)
{
    JsonVariant jsonTimezone = doc["time"]["timezone"];
    if (!jsonTimezone.isNull()) {
        int tz = jsonTimezone.as<int>();
        if (tz > 0 && tz <= 82) {
            if (paramTimezone != tz) {
                paramTimezone = tz;
                _ntpInitialized = false;
            }
        }
    }
    JsonVariant jsonDate = doc["time"]["mdate"];
    if (!jsonDate.isNull()) {
        paramDate = jsonDate.as<String>();
    }
    JsonVariant jsonTime = doc["time"]["mtime"];
    if (!jsonTime.isNull()) {
        paramTime = jsonTime.as<String>();
    }
    JsonVariant jsonManually = doc["time"]["manually"];
    if (!jsonManually.isNull()) {
        paramManually = jsonManually.as<bool>();
        // TODO set time
    }
    JsonVariant jsonDndEnabled = doc["time"]["dnd_enabled"];
    if (!jsonDndEnabled.isNull()) {
        paramDndEnabled = jsonDndEnabled.as<bool>();
    }
    JsonVariant jsonDndFrom = doc["time"]["dnd_from"];
    if (!jsonDndFrom.isNull()) {
        paramDndFrom = jsonDndFrom.as<String>();
    }
    JsonVariant jsonDndTo = doc["time"]["dnd_to"];
    if (!jsonDndTo.isNull()) {
        paramDndTo = jsonDndTo.as<String>();
    }
}

void Time::_callbackTimeSet(void)
{
    _ntpAvailable = true;
}

void Time::_onTimeConfig(AsyncWebServerRequest *request)
{
    if (!I::get().server().isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC time]: ESP heap: _onTimeConfig: " << ESP.getFreeHeap() << endl;
    DynamicJsonDocument jsonDoc(1024);
    fillJson(jsonDoc);
    String output;
    serializeJson(jsonDoc, output);
    I::get().logger() << "[EWC time]: ESP heap: _onTimeConfig: " << ESP.getFreeHeap() << endl;
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void Time::_onTimeSave(AsyncWebServerRequest *request)
{
    if (!I::get().server().isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    I::get().logger() << "[EWC time]: ESP heap: _onTimeSave: " << ESP.getFreeHeap() << endl;
    for (size_t i = 0; i < request->params(); i++) {
        I::get().logger() << "  " << request->argName(i) << ": " << request->arg(i) << endl;
    }
    DynamicJsonDocument config(1024);
    if (request->hasArg("timezone") && !request->arg("timezone").isEmpty()) {
        config["time"]["timezone"] = request->arg("timezone").toInt();
    }

    if (request->hasArg("manually")) {
        bool manually = request->arg("manually").equals("true");
        config["time"]["manually"] = manually;
        if (manually) {
            if (request->hasArg("mdate")) {
                config["time"]["mdate"] = request->arg("mdate");
            }
            if (request->hasArg("mtime")) {
                config["time"]["mtime"] = request->arg("mtime");
            }
        }
    }
    if (request->hasArg("dnd_enabled")) {
        bool dnd = request->arg("dnd_enabled").equals("true");
        config["time"]["dnd_enabled"] = dnd;
        if (dnd) {
            if (request->hasArg("dnd_from")) {
                config["time"]["dnd_from"] = request->arg("dnd_from");
            }
            if (request->hasArg("dnd_to")) {
                config["time"]["dnd_to"]  = request->arg("dnd_to");
            }
        }
    }
    I::get().logger() << "[EWC time]: ESP heap: _onBbsState: " << ESP.getFreeHeap() << endl;
    _fromJson(config);
    I::get().configFS().save();
    String details;
    serializeJsonPretty(config["time"], details);
    I::get().server().sendPageSuccess(request, "EWC Time save", "/", "Save success!", "<pre id=\"json\">" + details + "</pre>");
}


void Time::setLocalTime(String& date, String& time) {
    // TODO
    // setTime(hour, minute, second, day, month, year);
    // RTC.set(now());
}

time_t Time::currentTime()
{
    time_t rawtime;
    time(&rawtime);
    rawtime += TIMEMAP[paramTimezone-1][1] * 3600 + TIMEMAP[paramTimezone-1][0] * 3600;
    return rawtime;
}

String Time::str(long offsetSeconds)
{
    time_t rawtime = currentTime();
    rawtime += offsetSeconds;
    char buffer [80];
    strftime(buffer, 80, "%FT%T", gmtime(&rawtime));
    return String(buffer);
}

bool Time::isDisturb(long offsetSeconds)
{
    if (paramDndEnabled) {
        time_t ctime = currentTime();
        time_t timeFrom = _timeToSec(paramDndFrom);
        time_t timeTo = _timeToSec(paramDndTo);
        ctime += offsetSeconds;
        return (timeFrom <= ctime) && (ctime <= timeTo);
    }
    return false;
}

time_t Time::_timeToSec(String& hmTime)
{
    int hour, minute;
    sscanf(hmTime.c_str(), "%d:%d", &hour, &minute);
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    timeinfo->tm_hour = hour;
    timeinfo->tm_min = minute;
    return mktime(timeinfo);
}