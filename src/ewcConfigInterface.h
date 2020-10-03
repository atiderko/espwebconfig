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
#ifndef EWC_CONFIG_INTERFACE_H
#define EWC_CONFIG_INTERFACE_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "ewcInterface.h"


namespace EWC {

typedef std::function<bool(const String& value)> Validator;


class ConfigInterface {
public:
    ConfigInterface(String name) { _name = name; }
    virtual ~ConfigInterface() {}
    /** This method is called on setup config server. **/
    virtual void setup(JsonDocument& config, bool resetConfig=false) = 0;
    /** On configuration save the ConfigFS requests each ConfigInterafe to fill the JSON object with parameter to save. **/
    virtual void fillJson(JsonDocument& config) = 0;

    /** Name of this configuration interface. Currently only used for comparisation configuration interfaces. **/
    const String& name() { return _name; }
    bool operator== (const ConfigInterface &other) { return (_name.compareTo(other._name) == 0); }

protected:
    String _name;
};
};

#endif
