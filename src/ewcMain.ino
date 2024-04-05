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

#include <Arduino.h>
#include "ewcConfigServer.h"
#include "extensions/ewcUpdater.h"
#include "extensions/ewcTime.h"
#include "extensions/ewcMqtt.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

EWC::ConfigServer server;
EWC::Updater updater_;
EWC::Time time_;
EWC::Mqtt mqtt_;
bool timePrinted;
using namespace EWC;

void setup()
{
  EWC::I::get().logger().setBaudRate(115200);
  EWC::I::get().logger().setLogging(true);
  // optional: enable LED for wifi state and reset detection support.
  // place it on top
  EWC::I::get().led().init(true, LOW, LED_BUILTIN);
  // optional: add page for OTA updates
  EWC::I::get().configFS().addConfig(updater_);
  // optional: add page to setup time
  EWC::I::get().configFS().addConfig(time_);
  // optional: add page for MQTT settings
  EWC::I::get().configFS().addConfig(mqtt_);
  // optional: update brand and version
  server.setBrand("EWC", "1.0.1");
  // start web server
  server.setup();
}

void loop()
{
  // process web and dns requests
  server.loop();
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!timePrinted)
    {
      I::get().logger() << "Check for NTP..." << endl;
      if (time_.timeAvailable())
      {
        timePrinted = true;
        // print current time
        I::get().logger() << "Current time:" << time_.str() << endl;
        // or current time in seconds
        I::get().logger() << "  as seconds:" << time_.currentTime() << endl;
      }
    }
  }
  else
  {
    // or if not yet connected
  }
  delay(1);
}
