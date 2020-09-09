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
#include "ewcRTC.h"
#include "ewcInterface.h"
#include "ewcLogger.h"

using namespace EWC;

RTC::RTC(uint8_t start, uint8_t max)
{
    _max = max;
    _idxCurrent = start;
}

RTC::~RTC()
{
}

void RTC::reset(uint32_t flag)
{
    for (uint8_t idx = 0; idx < _max; idx++) {
        write(idx, flag);
    }
}

uint8_t RTC::get()
{
    if (_idxCurrent < _max) {
        uint8_t result = _idxCurrent;
        _idxCurrent++; 
        ETCI::get().logger() << "[RTC] get new index: " << result << endl;
        return result;
    }
    ETCI::get().logger() << "✘ [RTC] failed create new index, maximum reached!" << endl;
    return 0;
}

uint32_t RTC::read(uint8_t address)
{
    uint32_t read_flag;
    ESP.rtcUserMemoryRead(address, &read_flag, sizeof(read_flag));
    ETCI::get().logger() << "[RTC] read " << read_flag <<  " from " << address << endl;
    return read_flag;
}

void RTC::write(uint8_t address, uint32_t flag)
{
    ETCI::get().logger() << "[RTC] write " << flag <<  " to " << address << endl;
    ESP.rtcUserMemoryWrite(address, &flag, sizeof(flag));
}
