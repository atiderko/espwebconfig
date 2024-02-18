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
#ifdef ESP8266
    #include "ewcRtc.h"
#endif
#include "ewcLogger.h"

using namespace EWC;

#ifdef ESP32
void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");

            Serial.print(file.name());
            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);

            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");

            Serial.print(file.size());
            time_t t = file.getLastWrite();
            struct tm *tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
        }
        file = root.openNextFile();
    }
}
#endif

ConfigFS::ConfigFS(String filename)
{
    _filename = filename;
    // initialize utc addresses in sutup method
    _resetUtcAddress = 0;
    _resetDetected = false;
}

ConfigFS::~ConfigFS()
{
    _cfgInterfaces.clear();
}

void ConfigFS::setup()
{
    I::get().logger() << F("[EWC ConfigFS]: setup config container") << endl;
#ifdef ESP8266
    _resetUtcAddress = I::get().rtc().get();
#endif
    I::get().logger() << F("[EWC ConfigFS]: initialize LittleFS") << endl;
#ifdef ESP8266
    LittleFS.begin();
    Dir root = LittleFS.openDir("/");
    while(root.next()) {
        I::get().logger() << "  found file: " << root.fileName() << endl;
    }
#else
    LittleFS.begin(true);
    listDir(LittleFS , "/", 3);
    // File root = LittleFS.open("/");
    // while (root.openNextFile()) {
    //     I::get().logger() << "  found file: " << root.name() << endl;
    // }
#endif
#ifdef ESP8266
    I::get().logger() << F("[EWC ConfigFS]: read reset flag") << endl;
    RTC &rtc = I::get().rtc();
    if (rtc.read(_resetUtcAddress) == RESET_FLAG) {
        // double reset press detected
        _resetDetected = true;
        // reset configuration
        I::get().logger() << F("\n[EWC ConfigFS]: RESET detected, remove configuration") << endl;
        // TODO: delete configuration file
    }
    rtc.write(_resetUtcAddress, RESET_FLAG);
    delay(1000);
    I::get().logger() << F("[EWC ConfigFS]: clear reset flag") << endl;
    rtc.write(_resetUtcAddress, RESET_FLAG_CLEAR);
#endif
    I::get().logger() << F("[EWC ConfigFS]: Load configuration from ") << _filename << endl;
    File cfgFile = LittleFS.open(_filename, "r");
    if (cfgFile && !cfgFile.isDirectory())
    {
#ifdef ESP8266
        I::get().logger() << F("[EWC ConfigFS]: file open: ") << cfgFile.name() << endl;
#else
        I::get().logger() << F("[EWC ConfigFS]: file open: ") << cfgFile.path() << ", name: " << cfgFile.name() << endl;
#endif
        JsonDocument jsonDoc;
        deserializeJson(jsonDoc, cfgFile);
        // add sub configurations
        I::get().logger() << F("[EWC ConfigFS]: Load subconfigurations, count: ") << _cfgInterfaces.size() << endl;
        for (std::size_t i = 0; i < _cfgInterfaces.size(); ++i)
        {
            I::get().logger() << F("[EWC ConfigFS]:  load [") << i << F("]: ") << _cfgInterfaces[i]->name() << endl;
            _cfgInterfaces[i]->setup(jsonDoc, _resetDetected);
        }
    }
#ifdef ESP8266
    // reset RTC
    if (_resetDetected) {
        I::get().rtc().reset();
    }
#endif
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
    I::get().logger() << F("[EWC ConfigFS]: save subconfigurations, count: ") << _cfgInterfaces.size() << endl;
    JsonDocument doc;
    for(std::size_t i = 0; i < _cfgInterfaces.size(); ++i) {
        I::get().logger() << F("[EWC ConfigFS]:  add [") << i << F("]: ") << _cfgInterfaces[i]->name() << endl;
        _cfgInterfaces[i]->fillJson(doc);
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
