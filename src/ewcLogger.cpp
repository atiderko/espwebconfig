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
    : _loggingEnabled(false),
      _printer(&Serial)
{
}

void Logger::setLogging(bool enable, uint32_t baudRate)
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
    _baudRate = baudRate;
    Serial.begin(_baudRate);
  }
}

void Logger::setPrinter(Print *printer)
{
  _printer = printer;
}

size_t Logger::write(uint8_t character)
{
  if (_loggingEnabled)
    return _printer->write(character);
  return 0;
}

size_t Logger::write(const uint8_t *buffer, size_t size)
{
  if (_loggingEnabled)
    return _printer->write(buffer, size);
  return 0;
}
