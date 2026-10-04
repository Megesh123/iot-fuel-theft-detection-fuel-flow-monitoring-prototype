/**********************
 * EXTERNAL EEPROM STORAGE IMPLEMENTATION
 **********************/

#include "storage/EEPROMStorage.h"

EEPROMStorage eepromStorage;

void EEPROMStorage::begin(TwoWire &wire, uint8_t address)
{
    _wire = &wire;
    _address = address;

    if (isReady())
    {
        Serial.println("[EEPROM] 24LC256 detected at 0x" + String(_address, HEX));
    }
    else
    {
        Serial.println("[EEPROM] ERROR: 24LC256 not found at 0x" + String(_address, HEX));
    }
}

bool EEPROMStorage::isReady()
{
    _wire->beginTransmission(_address);
    return (_wire->endTransmission() == 0);
}

bool EEPROMStorage::saveConfig(const DeviceConfig &config)
{
    DeviceConfig configCopy = config;
    configCopy.magic = EEPROM_MAGIC_BYTE;

    // Calculate CRC over entire struct except the CRC field itself
    uint16_t dataLen = sizeof(DeviceConfig) - sizeof(uint16_t);
    configCopy.crc = _calculateCRC((uint8_t *)&configCopy, dataLen);

    // Write to EEPROM
    writeBlock(EEPROM_CONFIG_START, (uint8_t *)&configCopy, sizeof(DeviceConfig));

    // Verify write
    DeviceConfig verify;
    readBlock(EEPROM_CONFIG_START, (uint8_t *)&verify, sizeof(DeviceConfig));

    bool success = (memcmp(&configCopy, &verify, sizeof(DeviceConfig)) == 0);

    if (success)
        Serial.println("[EEPROM] Configuration saved successfully");
    else
        Serial.println("[EEPROM] ERROR: Configuration verification failed");

    return success;
}

bool EEPROMStorage::loadConfig(DeviceConfig &config)
{
    readBlock(EEPROM_CONFIG_START, (uint8_t *)&config, sizeof(DeviceConfig));

    // Check magic byte
    if (config.magic != EEPROM_MAGIC_BYTE)
    {
        Serial.println("[EEPROM] No valid configuration found (magic byte mismatch)");
        return false;
    }

    // Verify CRC
    uint16_t dataLen = sizeof(DeviceConfig) - sizeof(uint16_t);
    uint16_t calculatedCRC = _calculateCRC((uint8_t *)&config, dataLen);

    if (calculatedCRC != config.crc)
    {
        Serial.println("[EEPROM] Configuration CRC mismatch - data corrupt");
        return false;
    }

    Serial.println("[EEPROM] Configuration loaded successfully");
    Serial.println("[EEPROM] Device ID: " + String(config.deviceId));
    return true;
}

bool EEPROMStorage::isConfigValid()
{
    DeviceConfig temp;
    return loadConfig(temp);
}

void EEPROMStorage::eraseConfig()
{
    uint8_t zeros[EEPROM_CONFIG_SIZE];
    memset(zeros, 0xFF, EEPROM_CONFIG_SIZE);
    writeBlock(EEPROM_CONFIG_START, zeros, EEPROM_CONFIG_SIZE);
    Serial.println("[EEPROM] Configuration erased");
}

void EEPROMStorage::loadDefaults(DeviceConfig &config)
{
    memset(&config, 0, sizeof(DeviceConfig));

    config.magic = EEPROM_MAGIC_BYTE;
    strncpy(config.wifiSSID, "", sizeof(config.wifiSSID));
    strncpy(config.wifiPassword, "", sizeof(config.wifiPassword));
    strncpy(config.mqttServer, "broker.hivemq.com", sizeof(config.mqttServer));
    config.mqttPort = 1883;
    strncpy(config.mqttUsername, "", sizeof(config.mqttUsername));
    strncpy(config.mqttPassword, "", sizeof(config.mqttPassword));
    strncpy(config.deviceId, "FTMS_DEFAULT", sizeof(config.deviceId));
    strncpy(config.mqttTopicPublish, "ftms/data", sizeof(config.mqttTopicPublish));
    strncpy(config.mqttTopicSubscribe, "ftms/cmd", sizeof(config.mqttTopicSubscribe));
    config.pulsesPerLiter = DEFAULT_PULSES_PER_LITER;
    config.tankCapacity = FUEL_TANK_CAPACITY;
    config.fuelLevelMinRaw = FUEL_LEVEL_MIN_RAW;
    config.fuelLevelMaxRaw = FUEL_LEVEL_MAX_RAW;
    config.use4G = 1;
    config.publishInterval = MQTT_PUBLISH_INTERVAL / 1000; // Store in seconds

    Serial.println("[EEPROM] Default configuration loaded");
}

// ==================== Raw I/O ====================

void EEPROMStorage::writeByte(uint16_t address, uint8_t data)
{
    _wire->beginTransmission(_address);
    _wire->write((uint8_t)(address >> 8));   // MSB address
    _wire->write((uint8_t)(address & 0xFF)); // LSB address
    _wire->write(data);
    _wire->endTransmission();
    _waitForWrite();
}

uint8_t EEPROMStorage::readByte(uint16_t address)
{
    _wire->beginTransmission(_address);
    _wire->write((uint8_t)(address >> 8));
    _wire->write((uint8_t)(address & 0xFF));
    _wire->endTransmission();
    _wire->requestFrom(_address, (uint8_t)1);
    if (_wire->available())
        return _wire->read();
    return 0xFF;
}

void EEPROMStorage::writeBlock(uint16_t address, const uint8_t *data, uint16_t length)
{
    uint16_t bytesWritten = 0;

    while (bytesWritten < length)
    {
        // Calculate bytes remaining in current page
        uint16_t currentAddr = address + bytesWritten;
        uint16_t pageOffset = currentAddr % EEPROM_PAGE_SIZE;
        uint16_t bytesInPage = EEPROM_PAGE_SIZE - pageOffset;
        uint16_t bytesToWrite = min((uint16_t)(length - bytesWritten), bytesInPage);

        // Limit to Wire buffer (usually 32 bytes minus 2 for address)
        bytesToWrite = min(bytesToWrite, (uint16_t)30);

        _wire->beginTransmission(_address);
        _wire->write((uint8_t)(currentAddr >> 8));
        _wire->write((uint8_t)(currentAddr & 0xFF));

        for (uint16_t i = 0; i < bytesToWrite; i++)
        {
            _wire->write(data[bytesWritten + i]);
        }

        _wire->endTransmission();
        _waitForWrite();

        bytesWritten += bytesToWrite;
    }
}

void EEPROMStorage::readBlock(uint16_t address, uint8_t *data, uint16_t length)
{
    uint16_t bytesRead = 0;

    while (bytesRead < length)
    {
        uint16_t currentAddr = address + bytesRead;
        uint16_t bytesToRead = min((uint16_t)(length - bytesRead), (uint16_t)30);

        _wire->beginTransmission(_address);
        _wire->write((uint8_t)(currentAddr >> 8));
        _wire->write((uint8_t)(currentAddr & 0xFF));
        _wire->endTransmission();

        _wire->requestFrom(_address, (uint8_t)bytesToRead);

        for (uint16_t i = 0; i < bytesToRead && _wire->available(); i++)
        {
            data[bytesRead + i] = _wire->read();
        }

        bytesRead += bytesToRead;
    }
}

// ==================== Private Methods ====================

uint16_t EEPROMStorage::_calculateCRC(const uint8_t *data, uint16_t length)
{
    // CRC-16/CCITT-FALSE
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= ((uint16_t)data[i] << 8);
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

void EEPROMStorage::_waitForWrite()
{
    // Poll ACK - EEPROM won't ACK during write cycle (~5ms)
    uint32_t start = millis();
    while (millis() - start < 10)
    {
        _wire->beginTransmission(_address);
        if (_wire->endTransmission() == 0)
            return; // Write complete
        delay(1);
    }
}