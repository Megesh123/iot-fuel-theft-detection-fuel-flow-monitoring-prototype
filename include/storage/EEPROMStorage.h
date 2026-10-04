/**********************
 * EXTERNAL EEPROM STORAGE (24LC256)
 * I2C EEPROM for persistent configuration storage
 * Stores WiFi, MQTT, calibration data with CRC validation
 **********************/

#ifndef __EEPROM_STORAGE_H__
#define __EEPROM_STORAGE_H__

#include <Arduino.h>
#include <Wire.h>
#include "../config/settings.h"

class EEPROMStorage
{
public:
    void begin(TwoWire &wire = Wire, uint8_t address = 0x50);
    bool isReady(); // Check if EEPROM responds

    // Configuration management
    bool saveConfig(const DeviceConfig &config); // Save with CRC
    bool loadConfig(DeviceConfig &config);       // Load and verify CRC
    bool isConfigValid();                        // Check if stored config is valid
    void eraseConfig();                          // Erase configuration area
    void loadDefaults(DeviceConfig &config);     // Fill with default values

    // Raw read/write
    void writeByte(uint16_t address, uint8_t data);
    uint8_t readByte(uint16_t address);
    void writeBlock(uint16_t address, const uint8_t *data, uint16_t length);
    void readBlock(uint16_t address, uint8_t *data, uint16_t length);

private:
    TwoWire *_wire;
    uint8_t _address;

    uint16_t _calculateCRC(const uint8_t *data, uint16_t length);
    void _waitForWrite(); // Poll for write completion
};

extern EEPROMStorage eepromStorage;

#endif