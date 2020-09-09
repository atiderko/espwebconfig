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
#include "ewcConfigInterface.h"
#include "config.h"


using namespace EWC;

// void ConfigInterface::_addChanged(const char* name, const String& value)
// {
//     // if (_config != NULL) {
//     BBSI::get().config().addChanged(name, value);
//     // }
// }
/*
bool ConfigInterface::update_setting(HomieSetting<bool>& setting, const HomieRange& range, const String& value) {
    bool nval = str2bool(value.c_str(), "false", "true");
    bool valid = setting.validate(nval);
    bool equal = nval == setting.get();
    if (valid && !equal) {
        setting.set(nval);
        _addChanged(setting.getName(), value);
        ETCI::get().logger() << F("✔ '") << setting.getName() << F("' updated to ") << setting.get() << endl;
    } else if (equal) {
        ETCI::get().logger() << F("✖ '") << setting.getName() << F("' is already set to ") << value << endl;
    } else {
        ETCI::get().logger() << F("✖ '") << setting.getName() << F("' can not apply invalid value: ") << value << endl;
    }
    return true;
}

bool ConfigInterface::update_setting(HomieSetting<long>& setting, const HomieRange& range, const String& value) {
    long nval = value.toInt();
    bool valid = setting.validate(nval);
    bool equal = nval == setting.get();
    if (valid && !equal) {
        setting.set(nval);
        _addChanged(setting.getName(), value);
        ETCI::get().logger() << F("✔ '") << setting.getName() << F("' updated to ") << setting.get() << endl;
    } else if (equal) {
        ETCI::get().logger() << F("✖ '") << setting.getName() << F("' is already set to ") << value << endl;
    } else {
        ETCI::get().logger() << F("✖ '") << setting.getName() << F("' can not apply invalid value: ") << value << endl;
    }
    return true;
}

bool ConfigInterface::update_setting(HomieSetting<double>& setting, const HomieRange& range, const String& value) {
    double nval = value.toDouble();
    bool valid = setting.validate(nval);
    bool equal = nval == setting.get();
    if (valid && !equal) {
        setting.set(nval);
        _addChanged(setting.getName(), value);
        ETCI::get().logger() << F("✔ '") << setting.getName() << F("' updated to ") << setting.get() << endl;
    } else if (equal) {
        ETCI::get().logger() << F("✖ '") << setting.getName() << F("' is already set to ") << value << endl;
    } else {
        ETCI::get().logger() << F("✖ '") << setting.getName() << F("' can not apply invalid value: ") << value << endl;
    }
    return true;
}

bool ConfigInterface::update_setting(HomieSetting<const char*>& setting, const HomieRange& range, const String& value) {
    const char* nval = value.c_str();
    bool valid = setting.validate(nval);
    bool equal = nval == setting.get();
    if (valid && !equal) {
        setting.set(nval);
        _addChanged(setting.getName(), value);
        ETCI::get().logger() << F("✔ '") << setting.getName() << F("' updated to ") << setting.get() << endl;
    } else if (equal) {
        ETCI::get().logger() << F("✖ '") << setting.getName() << F("' is already set to ") << value << endl;
    } else {
        ETCI::get().logger() << F("✖ '") << setting.getName() << F("' can not apply invalid value: ") << value << endl;
    }
    return true;
}
*/
// bool ConfigInterface::get_bit(unsigned int var, unsigned int bit)
// {
//     return (var & (1 << bit)) != 0;
// }

// void ConfigInterface::setBit(unsigned int var, unsigned int bit, bool value)
// {
//     if (value) {
//         var &= (1 << bit);
//     } else {
//         var &= ~(1 << bit);
//     }
// }
