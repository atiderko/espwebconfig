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

Based on:
 *  @file   AutoConnect.cpp
 *  @author hieromon@gmail.com
 

**************************************************************/

#include "ewcTickerLED.h"
#include "ewcInterface.h"

using namespace EWC;

void TickerLed::enable(bool enable, const uint8_t port, const uint8_t active, const uint32_t cycle, uint32_t duration)
{
    I::get().logger() << "[EWC LED] enable" << endl;
    _port = port;
    _turnOn = active;
    _cycle = cycle;
    setDuration(duration);
    _enabled = enable;
    if (!enable) {
      stop();
    }
}

/**
 * Start ticker cycle
 * @param cycle Cycle time in [ms]
 * @param duty  Duty cycle in [ms]
 */
void TickerLed::start(const uint32_t cycle, const uint32_t duration) {
    if (_enabled) {

        I::get().logger() << "[EWC LED] start: " << cycle << ", duration: " << duration  << endl;
        _cycle = cycle;
        if (duration <= _cycle) {
            _duration = duration;
        }
        start();
    } else {
        I::get().logger() << "✘ [EWC LED] not enabled, skip start"  << endl;
    }
}

/**
 * Start ticker cycle
 */
void TickerLed::start(void)
{
    pinMode(_port, OUTPUT);
    if (_period == nullptr) {
        _period = new Ticker();
        _pulse = new Ticker();
    }
    _pulse->detach();
    _period->attach_ms<TickerLed*>(_cycle, TickerLed::_onPeriod, this);
    _active = true;
}

void TickerLed::stop(void) 
{
    if (_period != nullptr) {
        _period->detach();
        _pulse->detach();
        digitalWrite(_port, !_turnOn);
        if (_active) {
            I::get().logger() << "[EWC LED] stopped"  << endl;
        }
        delete _period;
        delete _pulse;
        _period = nullptr;
        _pulse = nullptr;
    }
    _active = false;
}

/**
 * Turn on the flicker signal and reserves a ticker to turn off the
 * signal. This behavior will perform every cycle to generate the
 * pseudo-PWM signal.
 * If the function is registered, call the callback function at the
 * end of one cycle.
 * @param  t  Its own address
 */
void TickerLed::_onPeriod(TickerLed* t) {
    digitalWrite(t->_port, t->_turnOn);
    t->_pulse->once_ms<TickerLed*>(t->_duration, TickerLed::_onPulse, t);
    if (t->_callback) {
        t->_callback();
    }
}

/**
 * Turn off the flicker signal
 * @param  t  Its own address
 */
void TickerLed::_onPulse(TickerLed* t) {
    digitalWrite(t->_port, !(t->_turnOn));
}
