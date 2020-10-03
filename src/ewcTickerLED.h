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

#ifndef EWC_TICKER_H
#define EWC_TICKER_H

#include <Arduino.h>
#include <Ticker.h>

#ifndef LED_BUILTIN
// Native pin for the arduino if LED_BUILTIN no defined
#define LED_BUILTIN 2
#endif

namespace EWC {

class TickerLed {
 public:
	explicit TickerLed(const uint8_t port = LED_BUILTIN, const uint8_t active = LOW, const uint32_t cycle = 0, uint32_t duration = 0) 
    : _cycle(cycle), _duration(duration), _port(port), _turnOn(active), _callback(nullptr) {
    _enabled = false;
    _active = false;
    setDuration(_duration);
  }
  ~TickerLed() { stop(); }

  void enable(bool enable, const uint8_t port = LED_BUILTIN, const uint8_t active = LOW, const uint32_t cycle = 0, uint32_t duration = 0);
  bool enabled() { return _enabled; }
  bool active() { return _active; }
  typedef std::function<void(void)> Callback_ft;
  void start(const uint32_t cycle, const uint32_t duration);
  // void start(const uint32_t cycle, const uint8_t width) { start(cycle, (uint32_t)((cycle * width) >> 8)); }
  void stop(void);
  void onPeriod(Callback_ft cb) { _callback = cb ;}

 protected:
  Ticker    _period;        //< Ticker for flicking cycle
  Ticker    _pulse;         //< Ticker for pulse width generating
  uint32_t  _cycle;         //< Cycle time in [ms]
  uint32_t  _duration;      //< Pulse width in [ms]
  bool _enabled;            //< True if enabled by enable(...)
  void setCycle(const uint32_t cycle) { _cycle = cycle; }
  void setDuration(const uint32_t duration) { _duration = duration <= _cycle ? duration : _duration; }
  void start(void);

 private:
  static void _onPeriod(TickerLed* t);
  static void _onPulse(TickerLed* t);
  bool _active;             //< True if ticker was started, false if stopped
  uint8_t     _port;        //< Port to output signal
  uint8_t     _turnOn;      //< Signal to turn on
  Callback_ft _callback;    //< An exit by every cycle
};

}
#endif
