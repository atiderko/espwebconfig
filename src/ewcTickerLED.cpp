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
  pinMode(_portGreen, OUTPUT);
  pinMode(_portRed, OUTPUT);
  this->enable(enable);
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
    _nextCycleTs = 0;
    _nextOffTs = 0;
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
  _active = true;
  // enable the led. Further cycle or duty switches are triggered in the loop().
  _onPeriod();
}

void TickerLed::stop(void)
{
  if (_active)
  {
    I::get().logger() << "[EWC LED] stop" << endl;
  }
  _active = false;
  digitalWrite(_portGreen, !_signalOn);
  digitalWrite(_portRed, !_signalOn);
  _greenOn = false;
  _redOn = false;
}

void TickerLed::loop()
{
  if (_active)
  {
    unsigned long now = millis();
    if (now >= _nextOffTs)
    {
      _onPulse();
    }
    if (now >= _nextCycleTs)
    {
      _onPeriod();
    }
  }
}

/**
 * Turn on the LED.
 */
void TickerLed::_onPeriod()
{
  switch (_mode)
  {
  case LED_GREEN:
    _greenSwitch(true);
    break;
  case LED_RED:
    _redSwitch(true);
    break;
  case LED_GREEN_RED:
    _greenSwitch(true);
    break;
  case LED_ORANGE:
    _greenSwitch(true);
    _redSwitch(true);
    break;
  default:
    break;
  }
  if (_maxCycleCount == 0 || _cycleCount < _maxCycleCount)
  {
    _nextCycleTs = millis() + _cycle;
    _nextOffTs = millis() + _duration;
  }
  else
  {
    stop();
  }
  _cycleCount += 1;
}

/**
 * Turn off the led or switch to another color.
 */
void TickerLed::_onPulse()
{
  switch (_mode)
  {
  case LED_GREEN:
    _greenSwitch(false);
    break;
  case LED_RED:
    _redSwitch(false);
    break;
  case LED_GREEN_RED:
    if (_greenOn)
    {
      _greenSwitch(false);
      _redSwitch(true);
      _nextOffTs = millis() + _duration;
    }
    else if (_redOn)
    {
      _redSwitch(false);
    }
    break;
  case LED_ORANGE:
    _greenSwitch(false);
    _redSwitch(false);
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
