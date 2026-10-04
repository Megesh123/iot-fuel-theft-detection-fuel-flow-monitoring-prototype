/**********************
 * POWER MONITOR IMPLEMENTATION
 **********************/

#include "power/PowerMonitor.h"
#include "config/pins.h"
#include "config/settings.h"

PowerMonitor powerMonitor;

void PowerMonitor::begin(uint8_t pin, bool useAnalog, uint16_t threshold)
{
    _pin = pin;
    _useAnalog = useAnalog;
    _threshold = threshold;
    _powerGood = true;
    _powerFailTriggered = false;
    _callback = nullptr;
    _lastCheckTime = 0;

    if (_useAnalog)
    {
        pinMode(_pin, INPUT);
        analogReadResolution(12);
    }
    else
    {
        pinMode(_pin, INPUT_PULLUP);
    }

    Serial.println("[POWER] Monitor initialized on pin " + String(_pin));
}

void PowerMonitor::update()
{
    uint32_t now = millis();
    if (now - _lastCheckTime < POWER_CHECK_INTERVAL)
        return;
    _lastCheckTime = now;

    bool currentPower;

    if (_useAnalog)
    {
        uint16_t adcVal = analogRead(_pin);
        currentPower = (adcVal >= _threshold);
    }
    else
    {
        // Digital mode: HIGH = power good, LOW = power failing
        currentPower = (digitalRead(_pin) == HIGH);
    }

    if (!currentPower && _powerGood && !_powerFailTriggered)
    {
        // Power just dropped
        _powerGood = false;
        _powerFailTriggered = true;
        Serial.println("[POWER] *** POWER FAIL DETECTED ***");

        if (_callback)
        {
            _callback();
        }
    }
    else if (currentPower && !_powerGood)
    {
        // Power restored
        _powerGood = true;
        _powerFailTriggered = false;
        Serial.println("[POWER] Power restored");
    }
}

bool PowerMonitor::isPowerGood()
{
    return _powerGood;
}

bool PowerMonitor::isPowerFailing()
{
    return _powerFailTriggered;
}

void PowerMonitor::setCallback(PowerFailCallback callback)
{
    _callback = callback;
}