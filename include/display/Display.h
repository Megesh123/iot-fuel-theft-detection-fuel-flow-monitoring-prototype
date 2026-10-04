/**********************
 * OLED DISPLAY (SSD1306 128x64)
 * I2C OLED display for status and menu
 **********************/

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "config/settings.h"
#include "bluetooth/BLEScanner.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

class OLEDDisplay
{
public:
    void begin(TwoWire &wire = Wire, uint8_t address = 0x3C);
    void clear();
    void update(); // Push buffer to display

    // Status screens
    void showSplash(const char *version);
    void showBLEConfig(const char *deviceName, BluetoothStatus bleStatus);
    void showNormalStatus(const SensorData &data, ConnectivityType conn, bool mqttConnected);
    void showError(const char *errorMsg);
    void showMessage(const char *line1, const char *line2 = nullptr, const char *line3 = nullptr);

    // Menu display
    void showMenu(const char *title, const char **items, uint8_t itemCount,
                  uint8_t selectedIndex, uint8_t scrollOffset);

    // Power fail warning
    void showPowerFailWarning();

    bool isAvailable();

private:
    Adafruit_SSD1306 *_display;
    bool _available;
};

extern OLEDDisplay oledDisplay;

#endif