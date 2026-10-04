/**********************
 * OLED DISPLAY IMPLEMENTATION
 **********************/

#include "display/Display.h"
#include "config/pins.h"

OLEDDisplay oledDisplay;

void OLEDDisplay::begin(TwoWire &wire, uint8_t address)
{
    _display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &wire, -1);

    _available = _display->begin(SSD1306_SWITCHCAPVCC, address);

    if (_available)
    {
        _display->clearDisplay();
        _display->setTextColor(SSD1306_WHITE);
        _display->setTextSize(1);
        _display->display();
        Serial.println("[DISPLAY] SSD1306 initialized");
    }
    else
    {
        Serial.println("[DISPLAY] ERROR: SSD1306 not found");
    }
}

void OLEDDisplay::clear()
{
    if (!_available)
        return;
    _display->clearDisplay();
}

void OLEDDisplay::update()
{
    if (!_available)
        return;
    _display->display();
}

void OLEDDisplay::showSplash(const char *version)
{
    if (!_available)
        return;
    _display->clearDisplay();

    _display->setTextSize(2);
    _display->setCursor(10, 5);
    _display->print("FUEL MON");

    _display->setTextSize(1);
    _display->setCursor(30, 30);
    _display->print("IoT System");

    _display->setCursor(35, 50);
    _display->print("v");
    _display->print(version);

    _display->display();
}

void OLEDDisplay::showBLEConfig(const char *deviceName, BluetoothStatus bleStatus)
{
    if (!_available)
        return;
    _display->clearDisplay();

    _display->setTextSize(1);
    _display->setCursor(0, 0);
    _display->print("== BLE CONFIG MODE ==");

    _display->setCursor(0, 16);
    _display->print("Name:");
    _display->setCursor(0, 26);
    _display->print(deviceName);

    _display->setCursor(0, 42);
    _display->print("Status: ");
    switch (bleStatus)
    {
    case BLUETOOTH_CONNECTING:
        _display->print("Advertising");
        break;
    case BLUETOOTH_CONNECTED:
        _display->print("Connected!");
        break;
    default:
        _display->print("Waiting...");
        break;
    }

    _display->setCursor(0, 56);
    _display->print("Connect via app");

    _display->display();
}

void OLEDDisplay::showNormalStatus(const SensorData &data, ConnectivityType conn,
                                   bool mqttConnected)
{
    if (!_available)
        return;
    _display->clearDisplay();

    // Title bar
    _display->setTextSize(1);
    _display->setCursor(0, 0);
    _display->print("FUEL MONITOR");

    // Connection indicator (top right)
    _display->setCursor(90, 0);
    if (conn == CONN_WIFI)
        _display->print(mqttConnected ? "WiFi+" : "WiFi");
    else if (conn == CONN_4G)
        _display->print(mqttConnected ? "4G+" : "4G");
    else
        _display->print("OFF");

    _display->drawLine(0, 10, 127, 10, SSD1306_WHITE);

    // Fuel Level
    _display->setCursor(0, 14);
    _display->print("Level: ");
    _display->print(data.fuelLevelPercent, 1);
    _display->print("% (");
    _display->print(data.fuelLevelLiters, 1);
    _display->print("L)");

    // Draw fuel level bar
    _display->drawRect(0, 25, 128, 8, SSD1306_WHITE);
    int barWidth = (int)(data.fuelLevelPercent * 1.24f); // 124 pixels inner
    barWidth = constrain(barWidth, 0, 124);
    _display->fillRect(2, 27, barWidth, 4, SSD1306_WHITE);

    // Flow Rate
    _display->setCursor(0, 36);
    _display->print("Flow: ");
    _display->print(data.fuelFlowRate, 2);
    _display->print(" L/h");

    // Total Volume
    _display->setCursor(0, 46);
    _display->print("Total: ");
    _display->print(data.fuelTotalVolume, 2);
    _display->print(" L");

    // Time / Theft alert
    _display->setCursor(0, 56);
    if (data.fuelTheftDetected)
    {
        _display->print("!! THEFT ALERT !!");
    }
    else
    {
        _display->print(data.timestamp);
    }

    _display->display();
}

void OLEDDisplay::showError(const char *errorMsg)
{
    if (!_available)
        return;
    _display->clearDisplay();

    _display->setTextSize(2);
    _display->setCursor(20, 5);
    _display->print("ERROR");

    _display->setTextSize(1);
    _display->setCursor(0, 30);
    _display->print(errorMsg);

    _display->setCursor(0, 50);
    _display->print("Check connections");

    _display->display();
}

void OLEDDisplay::showMessage(const char *line1, const char *line2, const char *line3)
{
    if (!_available)
        return;
    _display->clearDisplay();

    _display->setTextSize(1);
    if (line1)
    {
        _display->setCursor(0, 10);
        _display->print(line1);
    }
    if (line2)
    {
        _display->setCursor(0, 28);
        _display->print(line2);
    }
    if (line3)
    {
        _display->setCursor(0, 46);
        _display->print(line3);
    }

    _display->display();
}

void OLEDDisplay::showMenu(const char *title, const char **items, uint8_t itemCount,
                           uint8_t selectedIndex, uint8_t scrollOffset)
{
    if (!_available)
        return;
    _display->clearDisplay();

    // Title
    _display->setTextSize(1);
    _display->setCursor(0, 0);
    _display->print("= ");
    _display->print(title);
    _display->print(" =");
    _display->drawLine(0, 10, 127, 10, SSD1306_WHITE);

    // Menu items (max 5 visible)
    uint8_t maxVisible = 5;
    for (uint8_t i = 0; i < maxVisible && (scrollOffset + i) < itemCount; i++)
    {
        uint8_t itemIdx = scrollOffset + i;
        uint8_t y = 14 + (i * 10);

        if (itemIdx == selectedIndex)
        {
            _display->fillRect(0, y - 1, 128, 10, SSD1306_WHITE);
            _display->setTextColor(SSD1306_BLACK);
        }
        else
        {
            _display->setTextColor(SSD1306_WHITE);
        }

        _display->setCursor(4, y);
        _display->print(items[itemIdx]);
    }

    _display->setTextColor(SSD1306_WHITE); // Reset
    _display->display();
}

void OLEDDisplay::showPowerFailWarning()
{
    if (!_available)
        return;
    _display->clearDisplay();

    _display->setTextSize(2);
    _display->setCursor(5, 10);
    _display->print("POWER LOW");

    _display->setTextSize(1);
    _display->setCursor(10, 40);
    _display->print("Saving data...");

    _display->display();
}

bool OLEDDisplay::isAvailable()
{
    return _available;
}