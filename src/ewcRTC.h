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
#ifndef EWC_RTC_H
#define EWC_RTC_H

#include <Arduino.h>

namespace EWC {

class RTC {
public:
    RTC(uint8_t start=33, uint8_t max=127);
    ~RTC();
    void reset(uint32_t flag=0);
    /** Returns free address if available. 0 and 255 are invalid values. **/
    uint8_t get();
    uint32_t read(uint8_t address);
    void write(uint8_t address, uint32_t flag);

protected:
    uint8_t _max;
    uint8_t _idxCurrent;
};
}; // namespace

#endif
