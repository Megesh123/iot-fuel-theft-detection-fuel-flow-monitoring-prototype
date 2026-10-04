/**********************
 * FUEL FLOW SENSOR IMPLEMENTATION
 **********************/

#include "fuel/FuelFlow.h"
#include "config/pins.h"
#include "config/settings.h"

FuelFlowSensor fuelFlow;
FuelFlowSensor *FuelFlowSensor::_instance = nullptr;

void IRAM_ATTR FuelFlowSensor::_pulseISR()
{
    if (_instance)
    {
        _instance->_pulseCount++;
    }
}

void FuelFlowSensor::begin(uint8_t pin, float pulsesPerLiter)
{
    _pin = pin;
    _pulsesPerLiter = pulsesPerLiter;
    _pulseCount = 0;
    _lastPulseCount = 0;
    _lastCalcTime = millis();
    _flowRate = 0.0f;
    _totalVolume = 0.0f;

    _instance = this;

    pinMode(_pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_pin), _pulseISR, RISING);

    Serial.println("[FUEL_FLOW] Initialized on pin " + String(_pin));
    Serial.println("[FUEL_FLOW] Calibration: " + String(_pulsesPerLiter) + " pulses/L");
}

void FuelFlowSensor::update()
{
    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - _lastCalcTime;

    if (elapsed >= FLOW_CALC_INTERVAL)
    {
        // Disable interrupts briefly to read volatile counter
        noInterrupts();
        uint32_t currentCount = _pulseCount;
        interrupts();

        uint32_t pulseDelta = currentCount - _lastPulseCount;

        // Calculate flow rate in L/h
        // pulseDelta in elapsed ms -> convert to per hour
        float liters = (float)pulseDelta / _pulsesPerLiter;
        _flowRate = (liters / (float)elapsed) * 3600000.0f; // Convert ms to hours

        // Accumulate total volume
        _totalVolume += liters;

        _lastPulseCount = currentCount;
        _lastCalcTime = currentTime;
    }
}

void FuelFlowSensor::reset()
{
    noInterrupts();
    _pulseCount = 0;
    interrupts();
    _lastPulseCount = 0;
    _totalVolume = 0.0f;
    _flowRate = 0.0f;
    Serial.println("[FUEL_FLOW] Counters reset");
}

void FuelFlowSensor::setCalibration(float pulsesPerLiter)
{
    _pulsesPerLiter = pulsesPerLiter;
    Serial.println("[FUEL_FLOW] Calibration set to " + String(_pulsesPerLiter) + " pulses/L");
}

float FuelFlowSensor::getFlowRate()
{
    return _flowRate;
}

float FuelFlowSensor::getTotalVolume()
{
    return _totalVolume;
}

uint32_t FuelFlowSensor::getTotalPulses()
{
    noInterrupts();
    uint32_t count = _pulseCount;
    interrupts();
    return count;
}
// In FuelFlow.cpp
void FuelFlowSensor::setValvePin(uint8_t relayPin, uint8_t ignitionPin)
{
    _relayPin = relayPin;
    _ignitionPin = ignitionPin;
    _valveClosed = false;

    pinMode(_relayPin, OUTPUT);
    digitalWrite(_relayPin, LOW); // Valve OPEN by default

    pinMode(_ignitionPin, INPUT_PULLDOWN);
}

void FuelFlowSensor::closeValve()
{
    digitalWrite(_relayPin, HIGH); // Trigger relay -> Close valve
    _valveClosed = true;
    Serial.println("[SAFETY] FUEL VALVE CLOSED AUTOMATICALLY!");
}

void FuelFlowSensor::openValve()
{
    digitalWrite(_relayPin, LOW); // De-energize relay -> Open valve
    _valveClosed = false;
    Serial.println("[SAFETY] FUEL VALVE OPENED");
}

bool FuelFlowSensor::isValveClosed()
{
    return _valveClosed;
}

void FuelFlowSensor::checkFlowAnomalies(float currentFlowLPH, float maxAllowedLPH)
{
    bool isEngineRunning = (digitalRead(_ignitionPin) == HIGH);

    // RULE 1: Engine is OFF, but fuel is flowing out > 0.5 L/h
    if (!isEngineRunning && currentFlowLPH > 0.5f)
    {
        Serial.println("[ALERT] THEFT DETECTED: Engine OFF but fuel flowing!");
        closeValve();
    }

    // RULE 2: Engine is ON, but flow exceeds maximum engine capacity
    if (isEngineRunning && currentFlowLPH > maxAllowedLPH)
    {
        Serial.println("[ALERT] OVER-FLOW DETECTED: Fuel line leak or siphoning!");
        closeValve();
    }
}