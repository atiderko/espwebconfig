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
#include <LittleFS.h>
#include "ewcConfigFS.h"
#include "ewcRtc.h"
#include "ewcLogger.h"


using namespace EWC;

ConfigFS::ConfigFS(String filename)
{
    Serial.println("create config");
    _filename = filename;
    // initialize utc addresses in sutup method
    _resetUtcAddress = 0;
    _resetDetected = false;
    Serial.println("ok");
}

ConfigFS::~ConfigFS()
{
    _cfgInterfaces.clear();
}

void ConfigFS::setup()
{
    ETCI::get().logger() << F("[CC]: setup config container") << endl;
    _resetUtcAddress = ETCI::get().rtc().get();
    ETCI::get().logger() << F("[CC]: initialize LittleFS") << endl;
    LittleFS.begin();
    Dir root = LittleFS.openDir("");
    while(root.next()) {
        ETCI::get().logger() << "  found file: " << root.fileName() << endl;
    }
    ETCI::get().logger() << F("[CC]: read reset flag") << endl;
    RTC& rtc = ETCI::get().rtc();
    if (rtc.read(_resetUtcAddress) == RESET_FLAG) {
        // double reset press detected
        _resetDetected = true;
        // reset configuration
        ETCI::get().logger() << F("\n[CC]: RESET detected, remove configuration") << endl;
        // TODO: delete configuration file
    }
    rtc.write(_resetUtcAddress, RESET_FLAG);
    delay(1000);
    ETCI::get().logger() << F("[CC]: clear reset flag") << endl;
    rtc.write(_resetUtcAddress, RESET_FLAG_CLEAR);
    ETCI::get().logger() << F("[CC]: Load configuration from ") << _filename << endl;
    File cfgFile = LittleFS.open(_filename, "r");
    size_t fsize = 1024;
    if (cfgFile && cfgFile.isFile()) {
        fsize = cfgFile.size();
    }
    DynamicJsonDocument jsondoc(fsize);
    if (cfgFile && cfgFile.isFile()) {
        deserializeJson(jsondoc, cfgFile);
    }
    // reset RTC
    if (_resetDetected) {
        ETCI::get().rtc().reset();
    }
    // add sub configurations
    ETCI::get().logger() << F("[CC]: Load subconfigurations, count: ") << _cfgInterfaces.size() << endl;
    for(std::size_t i = 0; i < _cfgInterfaces.size(); ++i) {
        ETCI::get().logger() << F("[CC]:  load [") << i << F("]: ") << _cfgInterfaces[i]->name() << endl;
        _cfgInterfaces[i]->setup(jsondoc, _resetDetected);
    }
}


void ConfigFS::loop()
{
}

// ConfigInterface* Config::sub_config(String name) {
//     if (_cfgInterfaces.find(name) != _cfgInterfaces.end())
//         return _cfgInterfaces[name];
//     return NULL;
// }

void ConfigFS::save()
{
    _tsLastSave = millis();
    ETCI::get().logger() << F("[CC]: save subconfigurations, count: ") << _cfgInterfaces.size() << endl;
    DynamicJsonDocument doc(2048);
    for(std::size_t i = 0; i < _cfgInterfaces.size(); ++i) {
        ETCI::get().logger() << F("[CC]:  add [") << i << F("]: ") << _cfgInterfaces[i]->name() << endl;
        _cfgInterfaces[i]->json(doc);
    }
    // Open file for writing
    File file = LittleFS.open(_filename, "w");
    // Write a prettified JSON document to the file
    serializeJsonPretty(doc, file);
}

void ConfigFS::addConfig(ConfigInterface &config)
{
    // TODO: add check to avoid add a config twice
    _cfgInterfaces.push_back(&config);
}
