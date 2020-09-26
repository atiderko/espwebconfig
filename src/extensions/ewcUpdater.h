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

#ifndef EWC_UPDATER_H
#define EWC_UPDATER_H

#include <Arduino.h>
#include "stdlib_noniso.h"

#if defined(ESP8266)
    #include "ESP8266WiFi.h"
    #include "ESPAsyncTCP.h"
    #include "flash_hal.h"
    #include "FS.h"
#elif defined(ESP32)
    #include "WiFi.h"
    #include "AsyncTCP.h"
    #include "Update.h"
    #include "esp_int_wdt.h"
    #include "esp_task_wdt.h"
#endif

#include <ESPAsyncWebServer.h>
#include "ewcUpdater.h"

#include "../ewcConfigInterface.h"

namespace EWC {

class Updater: public ConfigInterface {

public:
    Updater();
    ~Updater();
    /** === ConfigInterface Methods === **/    
    void setup(JsonDocument& config, bool resetConfig=false);
    void loop();
    void fillJson(JsonDocument& config);

protected:
    bool _shouldReboot;
    void _onUpdate(AsyncWebServerRequest* request);
    void _onUpdateUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void _onUpdateInfo(AsyncWebServerRequest* request);
    void _initParams();
    void _fromJson(JsonDocument& config);
};

};
#endif