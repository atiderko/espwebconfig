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
  class Logger : public Print
  {

  public:
    Logger();
    virtual size_t write(uint8_t character);
    virtual size_t write(const uint8_t *buffer, size_t size);
    void setLogging(bool enable);

  private:
    void setPrinter(Print *printer);

    bool _loggingEnabled;
    Print *_printer;
  };
};

#endif