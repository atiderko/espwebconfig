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

void Logger::disableMutex()
{
  _useMutex = false;
  deleteLockGuard();
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
  deleteLockGuard();
}

size_t Logger::write(uint8_t character)
{
  size_t result = 0;
  if (_loggingEnabled)
  {
    result = _printer->write(character);
  }
  deleteLockGuard();
  return result;
}

size_t Logger::write(const uint8_t *buffer, size_t size)
{
  size_t result = 0;
  if (_loggingEnabled)
  {
    result = _printer->write(buffer, size);
  }
  deleteLockGuard();
  return result;
}

void Logger::setBaudRate(uint32_t baudRate)
{
  _baudRate = baudRate;
  deleteLockGuard();
}

void Logger::timePrefix(bool enable)
{
  _timePrefix = enable;
  deleteLockGuard();
}

void Logger::setTimeStr(String timeStr)
{
  if (_loggingEnabled)
  {
    _timeStr = timeStr;
    _newLine = true;
  }
}

void Logger::startLock()
{
  if (_loggingEnabled && _useMutex)
  {
    std::lock_guard<std::mutex> *lock = new std::lock_guard<std::mutex>(_mutex);
    _lockGuard = lock;
  }
}

bool Logger::enabled()
{
  deleteLockGuard();
  return _loggingEnabled;
}

void Logger::deleteLockGuard()
{
  if (_lockGuard != NULL)
  {
    delete _lockGuard;
    _lockGuard = NULL;
  }
}