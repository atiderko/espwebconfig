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
/** from homie Logger.hpp and StreamingOperator.hpp */

#ifndef EWC_LOGGER_H
#define EWC_LOGGER_H

#include <Arduino.h>

namespace EWC
{
  enum _EndLineCode
  {
    endl
  };

  /** Thread safe logger class. The lock is initialized with startLock().
   * The lock is released by every method of the logger, with the exception of the << operator and setTimeStr().
   */
  class Logger : public Print
  {

  public:
    Logger();
    void setBaudRate(uint32_t baudRate);
    void setLogging(bool enable);
    void timePrefix(bool enable);
    bool enabled();
    template <class T>
    inline Logger &operator<<(T arg)
    {
      if (_loggingEnabled)
      {
        if (_newLine)
        {
          _printer->print(_timeStr);
          _printer->print("| ");
          _newLine = false;
        }
        _printer->print(arg);
      }
      return *this;
    }
    inline Logger &operator<<(_EndLineCode arg)
    {
      if (_loggingEnabled)
      {
        _printer->println();
      }
      return *this;
    }
    // ----- used by ewcInterface -----
    void setTimeStr(String timeStr);

  private:
    virtual size_t write(uint8_t character);
    virtual size_t write(const uint8_t *buffer, size_t size);

    bool _loggingEnabled = false;
    bool _timePrefix = true;
    bool _newLine = true;
    uint32_t _baudRate = 115200;
    Print *_printer;
    String _timeStr;
  };
};

#endif