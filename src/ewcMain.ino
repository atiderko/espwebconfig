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
/**
#include <Arduino.h>
#include "ewcConfigServer.h"
#include "extensions/ewcUpdater.h"
#include "extensions/ewcTime.h"
#include "extensions/ewcMqtt.h"

EWC::ConfigServer server;
EWC::Updater updater_;
EWC::Time time_;
EWC::Mqtt mqtt_;

void setup() {
    Serial.begin(115200);
    Serial.println();
    // optional: add page for OTA updates
    EWC::I::get().configFS().addConfig(updater_);
    // optional: add page to setup time
    EWC::I::get().configFS().addConfig(time_);
    // optional: add page for MQTT settings and async client
    EWC::I::get().configFS().addConfig(mqtt_);
    // optional: update brand and version
    server.setBrand("EWC", "1.0.1");
    // optional: enable LED for wifi state
    EWC::I::get().led().enable(true, LED_BUILTIN, LOW);
    // start webserver
	server.setup();
}


void loop() {
    // process dns requests and connection state
    server.loop();
    delay(1);
}
**/

#include <Arduino.h>
#include <ewcConfigServer.h>
#include <ewcLogger.h>
#include <extensions/ewcTime.h>

using namespace EWC;
ConfigServer server;
Time ewcTime;
bool timePrinted = false;

void setup() {
    Serial.begin(115200);
    // add time configuration
    I::get().configFS().addConfig(ewcTime);
    // start webserver
	server.setup();
}


void loop() {
    // process dns requests and connection state AP/STA
    server.loop();
    if (WiFi.status() == WL_CONNECTED) {
        if (ewcTime.ntpAvailable() && !timePrinted) {
            timePrinted = true;
            // print current time
            I::get().logger() << "Current time:" << ewcTime.str() << endl;
            // or current time in seconds
            I::get().logger() << "  as seconds:" << ewcTime.currentTime() << endl;
        }
    } else {
        // or if not yet connected
    }
    delay(1000);
}