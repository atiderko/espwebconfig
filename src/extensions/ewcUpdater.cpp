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

#include <ESPAsyncWebServer.h>
#include "ewcUpdater.h"
#include "ewcConfigServer.h"
#include "ewcInterface.h"
#include "generated/ewcUpdateHTML.h"

using namespace EWC;

Updater::Updater() : ConfigInterface("updater")
{
    _shouldReboot = false;
}

Updater::~Updater()
{
}

void Updater::setup(JsonDocument& config, bool resetConfig)
{
    I::get().logger() << "[EWC Updater] setup" << endl;
    _initParams();
    _fromJson(config);
    I::get().server().insertMenuP("Update", "/ewc/update", "menu_update", HTML_EWC_UPDATE, FPSTR(PROGMEM_CONFIG_TEXT_HTML), true, 0);
    I::get().server().webserver().on("/ewc/update.json", std::bind(&Updater::_onUpdateInfo, this, std::placeholders::_1));
    I::get().server().webserver().on("/ewc/updatefw", HTTP_POST, std::bind(&Updater::_onUpdate, this, std::placeholders::_1),
        std::bind(&Updater::_onUpdateUpload, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
        std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
}

void Updater::loop()
{
    if (_shouldReboot) {
        yield();
        delay(2000);
        yield();
        #if defined(ESP8266)
            ESP.restart();
        #elif defined(ESP32)
            // ESP32 will commit sucide
            esp_task_wdt_init(1,true);
            esp_task_wdt_add(NULL);
            while(true);
        #endif
    }

}

void Updater::fillJson(JsonDocument& config)
{
}

void Updater::_initParams()
{
}

void Updater::_fromJson(JsonDocument& config)
{
}


void Updater::_onUpdate(AsyncWebServerRequest *request)
{
    if (!I::get().server().isAuthenticated(request)) {
        return request->requestAuthentication();
    }
    _shouldReboot = !Update.hasError();
    I::get().logger() << F("[EWC Updater] should restart after update: ") << _shouldReboot << endl;
    if (_shouldReboot) {
        I::get().server().sendPageSuccess(request, "Update successful", "Update successful!", "/ewc/update", "restarting device...");
    } else {
        I::get().server().sendPageFailed(request, "Update Failed", "Update Failed with error " + String(Update.getError()), "/ewc/update");
    }
}

void Updater::_onUpdateUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index){
        I::get().logger() << F("[EWC Updater] Update Start: ") << filename << endl;
        Update.runAsync(true);
        if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
            I::get().logger() << F("✘ [EWC Updater] update error: ") << Update.getError() << endl;
        }
    }
    if (!Update.hasError()) {
        if (Update.write(data, len) != len){
            I::get().logger() << F("✘ [EWC Updater] update error: ") << Update.getError() << endl;
        }
    }
    if (final) {
        if (Update.end(true)) {
            I::get().logger() << F("✔ [EWC Updater] Update Success: ") << index+len << "B" << endl;
        } else {
            I::get().logger() << F("✘ [EWC Updater] update error: ") << Update.getError() << endl;
        }
    }
}

void Updater::_onUpdateInfo(AsyncWebServerRequest *request)
{
    if (!I::get().server().isAuthenticated(request)) {
        return request->requestAuthentication();
    }
//    I::get().logger() << "[EWC time]: ESP heap: _onUpdateInfo: " << ESP.getFreeHeap() << endl;
    DynamicJsonDocument jsonDoc(512);
    fillJson(jsonDoc);
    jsonDoc["update"]["version"] = I::get().server().version();
    String output;
    serializeJson(jsonDoc, output);
//    I::get().logger() << "[EWC time]: ESP heap: _onUpdateInfo: " << ESP.getFreeHeap() << endl;
    request->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}
