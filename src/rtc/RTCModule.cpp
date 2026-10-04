/**********************
 * RTC MODULE IMPLEMENTATION
 **********************/

#include "rtc/RTCModule.h"

RTCModule rtcModule;

void RTCModule::begin(TwoWire &wire)
{
    _wire = &wire;

    // Check if DS3231 responds
    _wire->beginTransmission(DS3231_ADDR);
    _available = (_wire->endTransmission() == 0);

    if (_available)
        Serial.println("[RTC] DS3231 detected");
    else
        Serial.println("[RTC] ERROR: DS3231 not found");
}

bool RTCModule::isAvailable()
{
    return _available;
}

void RTCModule::setDateTime(uint16_t year, uint8_t month, uint8_t day,
                            uint8_t hour, uint8_t minute, uint8_t second)
{
    if (!_available)
        return;

    _wire->beginTransmission(DS3231_ADDR);
    _wire->write(DS3231_REG_SEC);
    _wire->write(_decToBcd(second));
    _wire->write(_decToBcd(minute));
    _wire->write(_decToBcd(hour));
    _wire->write(_decToBcd(0)); // Day of week (unused)
    _wire->write(_decToBcd(day));
    _wire->write(_decToBcd(month));
    _wire->write(_decToBcd(year - 2000));
    _wire->endTransmission();

    Serial.println("[RTC] Time set to " + getTimestamp());
}

void RTCModule::setFromString(const char *dateTimeStr)
{
    // Expected format: "YYYY-MM-DD HH:MM:SS"
    if (strlen(dateTimeStr) < 19)
        return;

    uint16_t year = atoi(dateTimeStr);
    uint8_t month = atoi(dateTimeStr + 5);
    uint8_t day = atoi(dateTimeStr + 8);
    uint8_t hour = atoi(dateTimeStr + 11);
    uint8_t minute = atoi(dateTimeStr + 14);
    uint8_t second = atoi(dateTimeStr + 17);

    setDateTime(year, month, day, hour, minute, second);
}

DateTime RTCModule::getDateTime()
{
    DateTime dt = {0};

    if (!_available)
        return dt;

    _wire->beginTransmission(DS3231_ADDR);
    _wire->write(DS3231_REG_SEC);
    _wire->endTransmission();

    _wire->requestFrom((uint8_t)DS3231_ADDR, (uint8_t)7);

    if (_wire->available() >= 7)
    {
        dt.second = _bcdToDec(_wire->read() & 0x7F);
        dt.minute = _bcdToDec(_wire->read());
        dt.hour = _bcdToDec(_wire->read() & 0x3F);
        dt.dayOfWeek = _bcdToDec(_wire->read());
        dt.day = _bcdToDec(_wire->read());
        dt.month = _bcdToDec(_wire->read() & 0x1F);
        dt.year = _bcdToDec(_wire->read()) + 2000;
    }

    return dt;
}

String RTCModule::getTimestamp()
{
    DateTime dt = getDateTime();
    char buf[21];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
    return String(buf);
}

String RTCModule::getDateString()
{
    DateTime dt = getDateTime();
    char buf[12];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", dt.year, dt.month, dt.day);
    return String(buf);
}

String RTCModule::getTimeString()
{
    DateTime dt = getDateTime();
    char buf[10];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", dt.hour, dt.minute, dt.second);
    return String(buf);
}

uint8_t RTCModule::_bcdToDec(uint8_t val)
{
    return ((val / 16) * 10) + (val % 16);
}

uint8_t RTCModule::_decToBcd(uint8_t val)
{
    return ((val / 10) * 16) + (val % 10);
}