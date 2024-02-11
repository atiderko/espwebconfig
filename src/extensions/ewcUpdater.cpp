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

// #include <ESPAsyncWebServer.h>
#include "ewcUpdater.h"
#include "ewcConfigServer.h"
#include "ewcInterface.h"
#include "generated/ewcUpdateHTML.h"

using namespace EWC;

Updater::Updater() : ConfigInterface("updater")
{
    _shouldReboot = false;
    _tsReboot = 0;
}

Updater::~Updater()
{
}

void Updater::setup(JsonDocument& config, bool resetConfig)
{
    I::get().logger() << F("[EWC Updater] setup") << endl;
    _initParams();
    _fromJson(config);
    I::get().server().insertMenuG("Update", "/ewc/update", "menu_update", FPSTR(PROGMEM_CONFIG_TEXT_HTML), HTML_EWC_UPDATE_GZIP, sizeof(HTML_EWC_UPDATE_GZIP), true, 0);
    I::get().server().webserver().on("/ewc/update.json", std::bind(&Updater::_onUpdateInfo, this, &EWC::I::get().server().webserver()));
    I::get().server().webserver().on("/ewc/updatefw", HTTP_POST, std::bind(&Updater::_onUpdate, this, &EWC::I::get().server().webserver()),
                                                           std::bind(&Updater::_onUpdateUpload, this, &EWC::I::get().server().webserver()));
}

void Updater::loop()
{
    if (_shouldReboot && millis() - _tsReboot > 3000) {
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

void Updater::_onUpdate(WebServer* server)
{
    if (!I::get().server().isAuthenticated(server)) {
        return server->requestAuthentication();
    }
    _shouldReboot = !Update.hasError();
    I::get().logger() << F("[EWC Updater] should restart after update: ") << _shouldReboot << endl;
    if (_shouldReboot) {
        _tsReboot = millis();
        I::get().server().sendPageSuccess(server, "Update successful", "Update successful!", "/ewc/update", "restarting device...");
    } else {
        I::get().server().sendPageFailed(server, "Update Failed", "Update Failed with error " + String(Update.getError()), "/ewc/update");
    }
}

void Updater::_onUpdateUpload(WebServer* server)
{
    HTTPUpload& upload = server->upload();
    if (upload.status == UPLOAD_FILE_START) {
#ifdef ESP8266
        WiFiUDP::stopAll();
#endif
        I::get().logger() << F("[EWC Updater] Update Start: ") << upload.filename << endl;
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { // start with max available size
            I::get().logger() << F("✘ [EWC Updater] update error: ") << Update.getError() << endl;
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            I::get().logger() << F("✘ [EWC Updater] update error: ") << Update.getError() << endl;
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { // true to set the size to the current progress
            I::get().logger() << F("✔ [EWC Updater] Update Success: ") << upload.totalSize << "B\nRebooting..." << endl;
        } else {
            I::get().logger() << F("✘ [EWC Updater] update error: ") << Update.getError() << endl;
        }
    }
    yield();
}

void Updater::_onUpdateInfo(WebServer* server)
{
    if (!I::get().server().isAuthenticated(server)) {
        return server->requestAuthentication();
    }
    DynamicJsonDocument jsonDoc(512);
    fillJson(jsonDoc);
    jsonDoc["update"]["version"] = I::get().server().version();
    String output;
    serializeJson(jsonDoc, output);
    server->send(200, FPSTR(PROGMEM_CONFIG_APPLICATION_JSON), output);
}
