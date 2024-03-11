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
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#include <WiFiUdp.h>
#endif
#include "lwip/apps/sntp.h"
#include "generated/timeSetupHTML.h"

using namespace EWC;

Time::Time() : ConfigInterface("time")
{
  _paramManually = false;
  _paramNtpEnabled = false;
  _manualOffset = 0;
}

Time::~Time()
{
}

void Time::setup(JsonDocument &config, bool resetConfig)
{
  I::get().logger() << F("[EWC Time] setup") << endl;
  _initParams();
  _fromJson(config);
  EWC::I::get().server().insertMenuG("Time", "/time/setup", "menu_time", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_TIME_SETUP_GZIP, sizeof(HTML_TIME_SETUP_GZIP), true, 0);
  EWC::I::get().server().webServer().on("/time/config.json", std::bind(&Time::_onTimeConfig, this, &EWC::I::get().server().webServer()));
  EWC::I::get().server().webServer().on("/time/config/save", std::bind(&Time::_onTimeSave, this, &EWC::I::get().server().webServer()));
  _setupTime();
}

void Time::_setupTime()
{
  if (!_paramManually)
  {
    // Sync our clock to NTP
    I::get().logger() << F("[EWC Time] sync to ntp server...") << endl;
    // configTime(TZMAP[_paramTimezone - 1][1] * 3600, TZMAP[_paramTimezone - 1][0] * 3600, "0.europe.pool.ntp.org", "pool.ntp.org", "time.nist.gov");
    // configTime(0, 0, "0.europe.pool.ntp.org", "pool.ntp.org", "time.nist.gov");
  }
  else
  {
    I::get().logger() << F("[EWC Time] current time: ") << str() << endl;
  }
}

void Time::fillJson(JsonDocument &config)
{
  config["time"]["timezone"] = _paramTimezone;
  config["time"]["ntp_enabled"] = _paramNtpEnabled;
  config["time"]["manually"] = _paramManually;
  config["time"]["mdate"] = _paramDate;
  config["time"]["mtime"] = _paramTime;
  config["time"]["dnd_enabled"] = _paramDndEnabled;
  config["time"]["dnd_from"] = _paramDndFrom;
  config["time"]["dnd_to"] = _paramDndTo;
  config["time"]["current"] = str();
}

void Time::_initParams()
{
  _paramTimezone = 31;
  _zoneOffset = TZMAP[_paramTimezone - 1][1] * 3600;
  _dstOffset = TZMAP[_paramTimezone - 1][0] * 3600;
  _paramNtpEnabled = false;
  _paramManually = false;
  _paramDate = "21.10.2020";
  _paramTime = "21:00";
  _paramDndEnabled = false;
  _paramDndFrom = "22:00";
  _paramDndTo = "06:00";
}

void Time::_fromJson(JsonDocument &config)
{
  JsonVariant jv = config["time"]["timezone"];
  if (!jv.isNull())
  {
    int tz = jv.as<int>();
    if (tz > 0 && tz <= 82)
    {
      if (_paramTimezone != tz)
      {
        _paramTimezone = tz;
        _zoneOffset = TZMAP[_paramTimezone - 1][1] * 3600;
        _dstOffset = TZMAP[_paramTimezone - 1][0] * 3600;
      }
    }
  }
  jv = config["time"]["mdate"];
  if (!jv.isNull())
  {
    _paramDate = jv.as<String>();
  }
  jv = config["time"]["mtime"];
  if (!jv.isNull())
  {
    _paramTime = jv.as<String>();
  }
  bool ntpEnabled = false;
  jv = config["time"]["ntp_enabled"];
  if (!jv.isNull())
  {
    ntpEnabled = jv.as<bool>();
  }
  _paramNtpEnabled = ntpEnabled;
  if (_paramNtpEnabled) {
    configTime(0, 0, "0.europe.pool.ntp.org", "pool.ntp.org", "time.nist.gov");
  } else {
    sntp_stop();
  }
  bool manually = false;
  jv = config["time"]["manually"];
  if (!jv.isNull())
  {
    manually = jv.as<bool>();
    // set time
    setLocalTime(_paramDate, _paramTime);
  }
  _paramManually = manually;
  jv = config["time"]["dnd_enabled"];
  if (!jv.isNull())
  {
    _paramDndEnabled = jv.as<bool>();
  }
  jv = config["time"]["dnd_from"];
  if (!jv.isNull())
  {
    _paramDndFrom = jv.as<String>();
  }
  jv = config["time"]["dnd_to"];
  if (!jv.isNull())
  {
    _paramDndTo = jv.as<String>();
  }
  _setupTime();
}

void Time::_onTimeConfig(WebServer *request)
{
  if (!I::get().server().isAuthenticated(request))
  {
    return request->requestAuthentication();
  }
  JsonDocument jsonDoc;
  fillJson(jsonDoc);
  String output;
  serializeJson(jsonDoc, output);
  request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}

void Time::_onTimeSave(WebServer *request)
{
  if (!I::get().server().isAuthenticated(request))
  {
    return request->requestAuthentication();
  }
  JsonDocument config;
  if (request->hasArg("timezone") && !request->arg("timezone").isEmpty())
  {
    config["time"]["timezone"] = request->arg("timezone").toInt();
  }

  if (request->hasArg("manually"))
  {
    bool manually = request->arg("manually").equals("true");
    config["time"]["manually"] = manually;
    if (manually)
    {
      if (request->hasArg("mdate"))
      {
        config["time"]["mdate"] = request->arg("mdate");
      }
      if (request->hasArg("mtime"))
      {
        config["time"]["mtime"] = request->arg("mtime");
      }
    }
  }
  if (request->hasArg("dnd_enabled"))
  {
    bool dnd = request->arg("dnd_enabled").equals("true");
    config["time"]["dnd_enabled"] = dnd;
    if (dnd)
    {
      if (request->hasArg("dnd_from"))
      {
        config["time"]["dnd_from"] = request->arg("dnd_from");
      }
      if (request->hasArg("dnd_to"))
      {
        config["time"]["dnd_to"] = request->arg("dnd_to");
      }
    }
  }
  _fromJson(config);
  I::get().configFS().save();
  String details;
  serializeJsonPretty(config["time"], details);
  I::get().server().sendPageSuccess(request, "EWC Time save", "Save successful!", "/time/setup", "<pre id=\"json\">" + details + "</pre>");
}

bool Time::timeAvailable()
{
  struct tm timeInfo;
  time_t now;
  time(&now);
  localtime_r(&now, &timeInfo);
  if (timeInfo.tm_year > (2016 - 1900))
  {
    return true;
  }
  return false;
}

void Time::setLocalTime(uint64_t unixtime)
{
  _manualOffset = unixtime - millis() / 1000;
}

void Time::setLocalTime(String &date, String &time)
{
  struct tm ti;
  sscanf(date.c_str(), "%d.%d.%d", &ti.tm_mday, &ti.tm_mon, &ti.tm_year);
  sscanf(time.c_str(), "%d:%d", &ti.tm_hour, &ti.tm_min);
  time_t t = mktime(&ti);
  _manualOffset = t - millis() / 1000 - _zoneOffset;
}

time_t Time::currentTime()
{
  if (_paramManually || !isNtpEnabled())
  {
    return millis() / 1000 + _manualOffset + _zoneOffset;
  }
  time_t rawTime;
  time(&rawTime);
  struct tm *timeInfo;
  timeInfo = localtime(&rawTime);
  time_t dst = 0;
  if (timeInfo->tm_isdst > 0)
  {
    dst = _dstOffset;
  }
  rawTime += _zoneOffset + dst;
  return rawTime;
}

String Time::str(time_t offsetSeconds)
{
  time_t rawTime = currentTime();
  rawTime += offsetSeconds;
  char buffer[80];
  strftime(buffer, 80, "%FT%T", gmtime(&rawTime));

  return String(buffer);
}

bool Time::dndEnabled()
{
  return _paramDndEnabled;
}

bool Time::isDisturb(time_t offsetSeconds)
{
  if (_paramDndEnabled)
  {
    time_t r = shiftDisturb(offsetSeconds);
    return r > offsetSeconds;
  }
  return false;
}

time_t Time::shiftDisturb(time_t offsetSeconds)
{
  if (_paramDndEnabled)
  {
    time_t m24 = 24 * 60;
    time_t ctime = currentTime();
    struct tm *timeInfo;
    timeInfo = gmtime(&ctime);
    time_t cmin = timeInfo->tm_hour * 60 + timeInfo->tm_min;
    time_t minFrom = _dndToMin(_paramDndFrom);
    time_t minTo = _dndToMin(_paramDndTo);
    cmin += offsetSeconds / 60;
    if (cmin > m24)
    {
      cmin -= m24;
    }
    time_t result = 0;
    if (minFrom > minTo)
    {
      // overflow on 24:00
      if (cmin >= minFrom)
      {
        result = m24 - cmin + minTo;
      }
      else if (cmin <= minTo)
      {
        result = minTo - cmin;
      }
    }
    else
    {
      result = minTo - cmin;
    }
    return result * 60 + offsetSeconds; // back to seconds
  }
  return offsetSeconds;
}

const String &Time::dndTo()
{
  return _paramDndTo;
}

time_t Time::_dndToMin(String &hmTime)
{
  int hours, minutes;
  sscanf(hmTime.c_str(), "%d:%d", &hours, &minutes);
  return hours * 60 + minutes;
}

boolean Time::_summertimeEU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
  if (month < 3 || month > 10)
    return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
  if (month > 3 && month < 10)
    return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
  if ((month == 3 && (hour + 24 * day) >= (1 + tzHours + 24 * (31 - (5 * year / 4 + 4) % 7))) || (month == 10 && (hour + 24 * day) < (1 + tzHours + 24 * (31 - (5 * year / 4 + 1) % 7))))
    return true;
  else
    return false;
}
