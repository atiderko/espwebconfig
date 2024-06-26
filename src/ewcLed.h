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

#ifndef EWC_LED_H
#define EWC_LED_H

#include <Arduino.h>

namespace EWC
{

  typedef enum
  {
    LED_GREEN = 0,
    LED_RED,
    LED_GREEN_RED,
    LED_ORANGE
  } twin_led_mode_t;

  class Led
  {
  public:
    explicit Led(const uint8_t active = LOW, const uint8_t portGreen = 13, const uint8_t portRed = 12)
        : _portGreen(portGreen), _portRed(portRed), _signalOn(active)
    {
      _enabled = false;
      _active = false;
      _greenOn = false;
      _redOn = false;
    }
    ~Led() { stop(); }
    void loop();

    void init(bool enable, const uint8_t active = LOW, const uint8_t portGreen = 13, const uint8_t portRed = 12);
    void enable(bool enable);
    bool enabled() { return _enabled; }
    bool active() { return _active; }
    /**
     * Start LED cycle
     * @param mode      Which LED's are on
     * @param cycle     Cycle time in [ms]
     * @param duration  Duty cycle in [ms]
     */
    void start(const twin_led_mode_t mode, const uint32_t cycle, const uint32_t duration, const uint32_t max_cycle_count = 0);
    void stop(void);

  protected:
    uint32_t _cycle = 500;       //< Cycle time in [ms]
    uint32_t _duration = 100;    //< Pulse width in [ms]
    uint32_t _maxCycleCount = 0; //< after this count of cycles the timer will be disabled
    uint32_t _cycleCount = 0;    //< counter for current cycle counts
    bool _enabled;               //< True if enabled by enable(...)
    void setCycle(const uint32_t cycle) { _cycle = cycle; }
    void setDuration(const uint32_t duration);
    void start(void);

  private:
    void _onPeriod();
    void _onPulse();
    void _greenSwitch(bool state);
    void _redSwitch(bool state);
    bool _active;               //< True if LED was started, false if stopped
    bool _greenOn;              //< True if green led is enabled
    bool _redOn;                //< True if red led is enabled
    uint8_t _portGreen;         //< Port to output signal for green LED
    uint8_t _portRed;           //< Port to output signal for red LED
    uint8_t _signalOn;          //< Signal to turn on led
    twin_led_mode_t _mode;      //< Mode describes which LEDs should be used
    unsigned long _nextCycleTs; //< Time if the LED should be turned on
    unsigned long _nextOffTs;   //< Time if the LED should be turned off
  };

}
#endif
