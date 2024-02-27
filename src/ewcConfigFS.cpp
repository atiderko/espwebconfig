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
#include "ewcTickerLED.h"
#include "ewcLogger.h"

using namespace EWC;

#ifdef ESP32
void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  I::get().logger() << F("Listing directory: ") << dirname << endl;

  File root = fs.open(dirname);
  if (!root)
  {
    I::get().logger() << F(" - failed to open directory") << endl;
    return;
  }
  if (!root.isDirectory())
  {
    I::get().logger() << F(" - not a directory") << endl;
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      I::get().logger() << F(" DIR: ") << file.name() << endl;
      // time_t t = file.getLastWrite();
      // struct tm *tmStruct = localtime(&t);
      // Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmStruct->tm_year) + 1900, (tmStruct->tm_mon) + 1, tmStruct->tm_mday, tmStruct->tm_hour, tmStruct->tm_min, tmStruct->tm_sec);

      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      I::get().logger() << F("  FILE: ") << file.name() << "  SIZE: " << file.size() << endl;

      // time_t t = file.getLastWrite();
      // struct tm *tmStruct = localtime(&t);
      // Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmStruct->tm_year) + 1900, (tmStruct->tm_mon) + 1, tmStruct->tm_mday, tmStruct->tm_hour, tmStruct->tm_min, tmStruct->tm_sec);
    }
    file = root.openNextFile();
  }
}
#endif

ConfigFS::ConfigFS(String filename)
{
  _filename = filename;
  _resetDetected = false;
  _resetFileRemoved = false;
}

ConfigFS::~ConfigFS()
{
  _cfgInterfaces.clear();
}

void ConfigFS::setup()
{
  String resetFileName = RESET_FILENAME;
  I::get().logger() << F("[EWC ConfigFS]: setup config container") << endl;
  I::get().logger() << F("[EWC ConfigFS]: initialize LittleFS") << endl;
#ifdef ESP8266
  LittleFS.begin();
  Dir root = LittleFS.openDir("/");
  while (root.next())
  {
    I::get().logger() << "  found file: " << root.fileName() << endl;
  }
#else
  LittleFS.begin(true);
  listDir(LittleFS, "/", 3);
  // File root = LittleFS.open("/");
  // while (root.openNextFile()) {
  //     I::get().logger() << "  found file: " << root.name() << endl;
  // }
#endif
  String resetContent;
  File resetFile = LittleFS.open(resetFileName, "r");
  if (resetFile && !resetFile.isDirectory())
  {
    I::get().logger() << F("[EWC ConfigFS]: file open: ") << resetFile.name() << endl;
    resetContent = resetFile.readString();
    if (resetContent.compareTo("1") == 0)
    {
      I::get().logger() << F("[EWC ConfigFS]: update reset file") << endl;
      resetFile.close();
      resetFile = LittleFS.open(resetFileName, "w");
      resetFile.write('2');
      resetFile.close();
      I::get().led().start(2000, 2000);
      I::get().logger() << F("[EWC ConfigFS]: wait for reset") << endl;
      delay(2000);
    }
    else if (resetContent.compareTo("2") == 0)
    {
      resetFile.close();
      I::get().logger() << F("\n[EWC ConfigFS]: RESET detected, remove configuration") << endl;
      I::get().led().start(100, 50);
      _resetDetected = true;
      // delete configuration file
      LittleFS.remove(_filename);
      // delete reset file
      LittleFS.remove(resetFileName);
      delay(2000);
    }
  }
  else
  {
    I::get().logger() << F("[EWC ConfigFS]: create reset file") << endl;
    resetFile = LittleFS.open(resetFileName, "w");
    resetFile.write('1');
    resetFile.close();
    I::get().led().start(2000, 2000);
    I::get().logger() << F("[EWC ConfigFS]: wait for reset") << endl;
    delay(2000);
  }
  I::get().logger() << F("[EWC ConfigFS]: Load configuration from ") << _filename << endl;
  JsonDocument jsonDoc;
  File cfgFile = LittleFS.open(_filename, "r");
  if (cfgFile && !cfgFile.isDirectory())
  {
    I::get().logger() << F("[EWC ConfigFS]: file open: ") << cfgFile.name() << endl;
    deserializeJson(jsonDoc, cfgFile);
  }
  // add sub configurations
  I::get().logger() << F("[EWC ConfigFS]: Load sub-configurations, count: ") << _cfgInterfaces.size() << endl;
  for (std::size_t i = 0; i < _cfgInterfaces.size(); ++i)
  {
    I::get().logger() << F("[EWC ConfigFS]:  load [") << i << F("]: ") << _cfgInterfaces[i]->name() << endl;
    _cfgInterfaces[i]->setup(jsonDoc, false);
    // _cfgInterfaces[i]->setup(jsonDoc, _resetDetected);
  }
}

void ConfigFS::loop()
{
  if (!_resetFileRemoved)
  {
    _resetFileRemoved = true;
    I::get().logger() << F("[EWC ConfigFS]: delete reset file") << endl;
    String resetFileName = RESET_FILENAME;
    LittleFS.remove(resetFileName);
  }
}

// ConfigInterface* Config::sub_config(String name) {
//     if (_cfgInterfaces.find(name) != _cfgInterfaces.end())
//         return _cfgInterfaces[name];
//     return NULL;
// }

void ConfigFS::save()
{
  I::get().logger() << F("[EWC ConfigFS]: save sub-configurations, count: ") << _cfgInterfaces.size() << endl;
  JsonDocument doc;
  for (std::size_t i = 0; i < _cfgInterfaces.size(); ++i)
  {
    I::get().logger() << F("[EWC ConfigFS]:  add [") << i << F("]: ") << _cfgInterfaces[i]->name() << endl;
    _cfgInterfaces[i]->fillJson(doc);
  }
  // Open file for writing
  File file = LittleFS.open(_filename, "w");
  // Write a prettified JSON document to the file
  serializeJsonPretty(doc, file);
  if (file)
    file.close();
}

void ConfigFS::addConfig(ConfigInterface &config)
{
  // TODO: add check to avoid add a config twice
  I::get().logger() << F("[EWC ConfigFS]:  add: ") << config.name() << endl;
  _cfgInterfaces.push_back(&config);
}

void ConfigFS::deleteFile()
{
  I::get().logger() << F("[EWC ConfigFS]: reset configuration file") << endl;
  LittleFS.remove(_filename);
}

String ConfigFS::readFrom(String fileName)
{
  File file = LittleFS.open(fileName, "r");
  if (file)
  {
    String data = file.readString();
    file.close();
    return data;
  }
  return "";
}

bool ConfigFS::saveTo(String fileName, String data)
{
  File file = LittleFS.open(fileName, "w");
  if (file)
  {
    for (unsigned int i = 0; i < data.length(); i++)
    {
      file.write(data[i]);
    }
    file.close();
    return true;
  }
  return false;
}