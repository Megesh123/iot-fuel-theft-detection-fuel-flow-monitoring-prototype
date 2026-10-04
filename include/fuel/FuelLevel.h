/**********************
 * FUEL LEVEL SENSOR
 * Analog resistive/capacitive fuel level measurement
 * Returns percentage and liters based on calibration
 **********************/

#ifndef __FUEL_LEVEL_H__
#define __FUEL_LEVEL_H__

#include <Arduino.h>

class FuelLevelSensor
{
public:
    void begin(uint8_t pin, uint16_t minRaw = 0, uint16_t maxRaw = 4095,
               float tankCapacity = 100.0f);
    void update(); // Read and filter ADC
    void setCalibration(uint16_t minRaw, uint16_t maxRaw, float tankCapacity);

    float getPercent();     // Returns 0-100%
    float getLiters();      // Returns liters in tank
    uint16_t getRawValue(); // Returns filtered ADC value

    // Theft detection
    bool checkTheft(float thresholdPercent = 5.0f);
    void resetTheftAlert();

private:
    uint8_t _pin;
    uint16_t _minRaw;
    uint16_t _maxRaw;
    float _tankCapacity;

    uint16_t _rawValue; // Filtered ADC value
    float _percent;     // Current level percentage
    float _liters;      // Current level in liters

    // Theft detection
    float _previousPercent;
    bool _theftDetected;
    uint32_t _lastTheftCheck;

    // ADC filtering
    uint16_t _readFiltered(uint8_t samples = 64);
};

extern FuelLevelSensor fuelLevel;

#endif