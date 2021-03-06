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
/** from homie Logget.hpp and StreamingOperator.hpp */
#ifndef EWC_STREAMING_OPERATOR_h
#define EWC_STREAMING_OPERATOR_h


#include <Arduino.h>

namespace EWC {

template<class T>
inline Print &operator <<(Print &stream, T arg)
{ stream.print(arg); return stream; }

enum _EndLineCode { endl };

inline Print &operator <<(Print &stream, _EndLineCode arg)
{ stream.println(); return stream; }

};

#endif