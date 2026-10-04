/**********************
 * POWER MONITOR
 * Detects 3.3V power failure for emergency data save
 **********************/

#ifndef __POWER_MONITOR_H__
#define __POWER_MONITOR_H__

#include <Arduino.h>

typedef void (*PowerFailCallback)();

class PowerMonitor
{
public:
    void begin(uint8_t pin, bool useAnalog = false, uint16_t threshold = 2048);
    void update(); // Call frequently in loop

    bool isPowerGood();
    bool isPowerFailing();
    void setCallback(PowerFailCallback callback);

private:
    uint8_t _pin;
    bool _useAnalog;
    uint16_t _threshold; // ADC threshold for analog mode
    bool _powerGood;
    bool _powerFailTriggered;

    PowerFailCallback _callback;
    uint32_t _lastCheckTime;
};

extern PowerMonitor powerMonitor;

#endif