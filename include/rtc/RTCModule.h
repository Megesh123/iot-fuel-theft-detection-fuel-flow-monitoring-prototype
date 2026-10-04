/**********************
 * RTC MODULE (DS3231)
 * Real-time clock for timestamping
 **********************/

#ifndef __RTC_MODULE_H__
#define __RTC_MODULE_H__

#include <Arduino.h>
#include <Wire.h>

// DS3231 registers
#define DS3231_ADDR 0x68
#define DS3231_REG_SEC 0x00

struct DateTime
{
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t dayOfWeek;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

class RTCModule
{
public:
    void begin(TwoWire &wire = Wire);
    bool isAvailable();

    void setDateTime(uint16_t year, uint8_t month, uint8_t day,
                     uint8_t hour, uint8_t minute, uint8_t second);
    void setFromString(const char *dateTimeStr); // Format: "YYYY-MM-DD HH:MM:SS"

    DateTime getDateTime();
    String getTimestamp();  // Returns "YYYY-MM-DD HH:MM:SS"
    String getDateString(); // Returns "YYYY-MM-DD"
    String getTimeString(); // Returns "HH:MM:SS"

private:
    TwoWire *_wire;
    bool _available;

    uint8_t _bcdToDec(uint8_t val);
    uint8_t _decToBcd(uint8_t val);
};

extern RTCModule rtcModule;

#endif