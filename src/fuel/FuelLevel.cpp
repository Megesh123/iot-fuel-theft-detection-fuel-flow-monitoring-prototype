/**********************
 * FUEL LEVEL SENSOR IMPLEMENTATION
 **********************/

#include "fuel/FuelLevel.h"
#include "config/pins.h"
#include "config/settings.h"

FuelLevelSensor fuelLevel;

void FuelLevelSensor::begin(uint8_t pin, uint16_t minRaw, uint16_t maxRaw,
                            float tankCapacity)
{
    _pin = pin;
    _minRaw = minRaw;
    _maxRaw = maxRaw;
    _tankCapacity = tankCapacity;
    _rawValue = 0;
    _percent = 0.0f;
    _liters = 0.0f;
    _previousPercent = 0.0f;
    _theftDetected = false;
    _lastTheftCheck = millis();

    // Configure ADC
    analogReadResolution(12);       // 12-bit ADC (0-4095)
    analogSetAttenuation(ADC_11db); // Full range 0-3.3V
    pinMode(_pin, INPUT);

    // Initial reading
    update();
    _previousPercent = _percent;

    Serial.println("[FUEL_LEVEL] Initialized on pin " + String(_pin));
    Serial.println("[FUEL_LEVEL] Range: " + String(_minRaw) + " - " + String(_maxRaw));
    Serial.println("[FUEL_LEVEL] Tank capacity: " + String(_tankCapacity) + " L");
}

void FuelLevelSensor::update()
{
    _rawValue = _readFiltered(FUEL_LEVEL_SAMPLES);

    // Map raw ADC to percentage (constrained 0-100%)
    if (_maxRaw > _minRaw)
    {
        _percent = ((float)(_rawValue - _minRaw) / (float)(_maxRaw - _minRaw)) * 100.0f;
    }
    else
    {
        // Inverted sensor (full tank = low resistance = low ADC)
        _percent = ((float)(_minRaw - _rawValue) / (float)(_minRaw - _maxRaw)) * 100.0f;
    }

    _percent = constrain(_percent, 0.0f, 100.0f);
    _liters = (_percent / 100.0f) * _tankCapacity;
}

void FuelLevelSensor::setCalibration(uint16_t minRaw, uint16_t maxRaw, float tankCapacity)
{
    _minRaw = minRaw;
    _maxRaw = maxRaw;
    _tankCapacity = tankCapacity;
    Serial.println("[FUEL_LEVEL] Calibration updated");
}

float FuelLevelSensor::getPercent()
{
    return _percent;
}

float FuelLevelSensor::getLiters()
{
    return _liters;
}

uint16_t FuelLevelSensor::getRawValue()
{
    return _rawValue;
}

bool FuelLevelSensor::checkTheft(float thresholdPercent)
{
    uint32_t now = millis();
    if (now - _lastTheftCheck >= FUEL_THEFT_CHECK_INTERVAL)
    {
        float drop = _previousPercent - _percent;

        if (drop >= thresholdPercent && _previousPercent > 10.0f)
        {
            // Significant sudden drop detected (and tank wasn't nearly empty)
            _theftDetected = true;
            Serial.println("[FUEL_LEVEL] THEFT DETECTED! Drop: " + String(drop) + "%");
        }

        _previousPercent = _percent;
        _lastTheftCheck = now;
    }

    return _theftDetected;
}

void FuelLevelSensor::resetTheftAlert()
{
    _theftDetected = false;
    _previousPercent = _percent;
}

uint16_t FuelLevelSensor::_readFiltered(uint8_t samples)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; i++)
    {
        sum += analogRead(_pin);
        delayMicroseconds(100); // Small delay between readings
    }
    return (uint16_t)(sum / samples);
}