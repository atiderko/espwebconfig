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

#include "ewcTickerLed.h"
#include "ewcInterface.h"

using namespace EWC;

void TickerLed::init(bool enable, const uint8_t active, const uint8_t portGreen, const uint8_t portRed)
{
  I::get().logger() << "[EWC LED] init ports green=" << portGreen << ", red=" << portRed << endl;
  _portGreen = portGreen;
  _portRed = portRed;
  _signalOn = active;
  _enabled = enable;
  pinMode(_portGreen, OUTPUT);
  pinMode(_portRed, OUTPUT);
  if (!enable)
  {
    stop();
  }
}

void TickerLed::enable(bool enable)
{
  I::get().logger() << "[EWC LED] enable " << enable << endl;
  _enabled = enable;
  if (!enable)
  {
    stop();
  }
}

void TickerLed::setDuration(const uint32_t duration)
{
  _duration = duration;
  if (duration >= _cycle)
  {
    _duration = _cycle / 2;
  }
  if (_mode == LED_GREEN_RED)
  {
    if (_duration > _cycle / 2)
    {
      _duration = _cycle / 2;
    }
  }
}

void TickerLed::start(const ticker_twin_led_mode_t mode, const uint32_t cycle, const uint32_t duration, const uint32_t maxCycleCount)
{
  if (_enabled)
  {

    I::get().logger() << "[EWC LED] start in mode: " << mode << ", cycle: " << cycle << ", duration: " << duration << endl;
    _maxCycleCount = maxCycleCount;
    _cycleCount = 0;
    _cycle = cycle;
    _mode = mode;
    setDuration(duration);
    start();
  }
  else
  {
    I::get().logger() << "✘ [EWC LED] not enabled, skip start" << endl;
  }
}

/**
 * Start ticker cycle
 */
void TickerLed::start(void)
{
  stop();
  if (_period == nullptr)
  {
    _period = new Ticker();
    _pulse = new Ticker();
  }
  _pulse->detach();
  // we have to start first cycle manually
  _onPeriod(this);
  // further cycles are triggered by a ticker timer
  _period->attach_ms<TickerLed *>(_cycle, TickerLed::_onPeriod, this);
  _active = true;
}

void TickerLed::stop(void)
{
  if (_period != nullptr)
  {
    _period->detach();
    delete _period;
    _period = nullptr;
    if (_pulse != nullptr)
    {
      _pulse->detach();
      delete _pulse;
      _pulse = nullptr;
    }
    digitalWrite(_portGreen, !_signalOn);
    digitalWrite(_portRed, !_signalOn);
    _greenOn = false;
    _redOn = false;
    if (_active)
    {
      I::get().logger() << "[EWC LED] stopped" << endl;
    }
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
void TickerLed::_onPeriod(TickerLed *t)
{
  switch (t->_mode)
  {
  case LED_GREEN:
    t->_greenSwitch(true);
    break;
  case LED_RED:
    t->_redSwitch(true);
    break;
  case LED_GREEN_RED:
    t->_greenSwitch(true);
    break;
  case LED_ORANGE:
    t->_greenSwitch(true);
    t->_redSwitch(true);
    break;
  default:
    break;
  }
  if (t->_maxCycleCount == 0 || t->_cycleCount < t->_maxCycleCount)
  {
    t->_pulse->once_ms<TickerLed *>(t->_duration, TickerLed::_onPulse, t);
  }
  else
  {
    t->stop();
  }
  t->_cycleCount += 1;
}

/**
 * Turn off the flicker signal
 * @param  t  Its own address
 */
void TickerLed::_onPulse(TickerLed *t)
{
  switch (t->_mode)
  {
  case LED_GREEN:
    t->_greenSwitch(false);
    break;
  case LED_RED:
    t->_redSwitch(false);
    break;
  case LED_GREEN_RED:
    if (t->_greenOn)
    {
      t->_greenSwitch(false);
      t->_redSwitch(true);
      t->_pulse->once_ms<TickerLed *>(t->_duration, TickerLed::_onPulse, t);
    }
    else if (t->_redOn)
    {
      t->_redSwitch(false);
    }
    break;
  case LED_ORANGE:
    t->_greenSwitch(false);
    t->_redSwitch(false);
    break;
  default:
    break;
  }
}

void TickerLed::_greenSwitch(bool state)
{
  if (state)
  {
    digitalWrite(_portGreen, _signalOn);
  }
  else
  {
    digitalWrite(_portGreen, !_signalOn);
  }
  _greenOn = state;
}

void TickerLed::_redSwitch(bool state)
{
  if (state)
  {
    digitalWrite(_portRed, _signalOn);
  }
  else
  {
    digitalWrite(_portRed, !_signalOn);
  }
  _redOn = state;
}
