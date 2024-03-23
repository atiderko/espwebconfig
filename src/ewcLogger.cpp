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
#include "ewcLogger.h"

using namespace EWC;

Logger::Logger()
    : _printer(&Serial)
{
}

void Logger::setLogging(bool enable)
{
  _loggingEnabled = enable;
  if (!enable)
  {
    if (Serial.available())
    {
      _baudRate = Serial.baudRate();
      Serial.end();
    }
  }
  else
  {
    Serial.begin(_baudRate);
  }
}

size_t Logger::write(uint8_t character)
{
  size_t result = 0;
  if (_loggingEnabled)
  {
    result = _printer->write(character);
  }
  return result;
}

size_t Logger::write(const uint8_t *buffer, size_t size)
{
  size_t result = 0;
  if (_loggingEnabled)
  {
    result = _printer->write(buffer, size);
  }
  return result;
}

void Logger::setBaudRate(uint32_t baudRate)
{
  _baudRate = baudRate;
}

void Logger::timePrefix(bool enable)
{
  _timePrefix = enable;
}

void Logger::setTimeStr(String timeStr)
{
  if (_loggingEnabled)
  {
    _timeStr = timeStr;
    _newLine = true;
  }
}

bool Logger::enabled()
{
  return _loggingEnabled;
}
