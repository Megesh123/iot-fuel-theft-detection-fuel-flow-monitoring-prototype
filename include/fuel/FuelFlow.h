/**********************
 * FUEL FLOW SENSOR
 * Hall effect pulse-based flow meter
 * Measures flow rate (L/h) and total consumption (L)
 **********************/

#ifndef __FUEL_FLOW_H__
#define __FUEL_FLOW_H__

#include <Arduino.h>

class FuelFlowSensor
{
public:
    void begin(uint8_t pin, float pulsesPerLiter = 450.0f);
    void update(); // Call in loop to calculate flow rate
    void reset();  // Reset total volume counter
    void setCalibration(float pulsesPerLiter);

    float getFlowRate();       // Returns L/h
    float getTotalVolume();    // Returns total liters
    uint32_t getTotalPulses(); // Returns raw pulse count

    void setValvePin(uint8_t relayPin, uint8_t ignitionPin);
    void closeValve();
    void openValve();
    bool isValveClosed();
    void checkFlowAnomalies(float currentFlowLPH, float maxAllowedLPH);

private:
    uint8_t _pin;
    float _pulsesPerLiter;

    volatile uint32_t _pulseCount; // ISR pulse counter
    uint32_t _lastPulseCount;      // Previous count for rate calc
    uint32_t _lastCalcTime;        // Last rate calculation time

    float _flowRate;    // Current flow rate (L/h)
    float _totalVolume; // Total consumed (L)

    static void IRAM_ATTR _pulseISR(); // Interrupt service routine
    static FuelFlowSensor *_instance;  // Static instance for ISR
    uint8_t _relayPin;
    uint8_t _ignitionPin;
    bool _valveClosed;
};

extern FuelFlowSensor fuelFlow;

#endif