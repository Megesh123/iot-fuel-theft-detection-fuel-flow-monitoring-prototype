/**********************
 * MENU SYSTEM IMPLEMENTATION
 **********************/

#include "menu/MenuSystem.h"
#include "button/Button.h"
#include "display/Display.h"
#include "fuel/FuelFlow.h"
#include "fuel/FuelLevel.h"
#include "network/WiFiManager.h"
#include "network/MQTTManager.h"
#include "rtc/RTCModule.h"
#include "config/settings.h"

MenuSystem menuSystem;

const char *MenuSystem::_mainMenuItems[] = {
    "Sensor Info",
    "Network Info",
    "Calibration",
    "System",
    "Reset Fuel Total",
    "Reset Theft Alert",
    "Reboot Device",
    "Factory Reset",
    "Exit"};
const uint8_t MenuSystem::_mainMenuCount = 9;

void MenuSystem::begin()
{
    _active = false;
    _currentScreen = MENU_MAIN;
    _selectedIndex = 0;
    _scrollOffset = 0;
    _lastActivity = 0;
    _actionCallback = nullptr;

    Serial.println("[MENU] System initialized");
}

void MenuSystem::update()
{
    if (!_active)
        return;

    // Check timeout
    if (millis() - _lastActivity > MENU_TIMEOUT_MS)
    {
        Serial.println("[MENU] Timeout - exiting menu");
        deactivate();
        return;
    }

    // Process button events
    ButtonEvent evtUp = buttons.getEvent(BTN_UP);
    ButtonEvent evtDown = buttons.getEvent(BTN_DOWN);
    ButtonEvent evtOk = buttons.getEvent(BTN_OK);
    ButtonEvent evtBack = buttons.getEvent(BTN_BACK);
    ButtonEvent evtMenu = buttons.getEvent(BTN_MENU);

    bool activity = false;

    if (evtUp == BTN_EVENT_PRESS || evtUp == BTN_EVENT_RELEASE)
    {
        _navigateUp();
        activity = true;
    }
    if (evtDown == BTN_EVENT_PRESS || evtDown == BTN_EVENT_RELEASE)
    {
        _navigateDown();
        activity = true;
    }
    if (evtOk == BTN_EVENT_PRESS || evtOk == BTN_EVENT_RELEASE)
    {
        _selectItem();
        activity = true;
    }
    if (evtBack == BTN_EVENT_PRESS || evtBack == BTN_EVENT_RELEASE ||
        evtMenu == BTN_EVENT_PRESS)
    {
        _goBack();
        activity = true;
    }

    if (activity)
    {
        _lastActivity = millis();
        _renderMenu();
    }
}

bool MenuSystem::isActive()
{
    return _active;
}

void MenuSystem::activate()
{
    _active = true;
    _currentScreen = MENU_MAIN;
    _selectedIndex = 0;
    _scrollOffset = 0;
    _lastActivity = millis();
    _renderMenu();
    Serial.println("[MENU] Activated");
}

void MenuSystem::deactivate()
{
    _active = false;
    _currentScreen = MENU_MAIN;
    Serial.println("[MENU] Deactivated");
}

MenuScreen MenuSystem::getCurrentScreen()
{
    return _currentScreen;
}

void MenuSystem::setActionCallback(MenuActionCallback callback)
{
    _actionCallback = callback;
}

void MenuSystem::_navigateUp()
{
    if (_selectedIndex > 0)
    {
        _selectedIndex--;
        if (_selectedIndex < _scrollOffset)
            _scrollOffset = _selectedIndex;
    }
}

void MenuSystem::_navigateDown()
{
    uint8_t maxIdx;

    switch (_currentScreen)
    {
    case MENU_MAIN:
        maxIdx = _mainMenuCount - 1;
        break;
    default:
        maxIdx = 0;
        break;
    }

    if (_selectedIndex < maxIdx)
    {
        _selectedIndex++;
        if (_selectedIndex >= _scrollOffset + 5)
            _scrollOffset = _selectedIndex - 4;
    }
}

void MenuSystem::_selectItem()
{
    if (_currentScreen == MENU_MAIN)
    {
        switch (_selectedIndex)
        {
        case 0: // Sensor Info
        {
            String line1 = "Flow: " + String(fuelFlow.getFlowRate(), 2) + " L/h";
            String line2 = "Total: " + String(fuelFlow.getTotalVolume(), 2) + " L";
            String line3 = "Level: " + String(fuelLevel.getPercent(), 1) + "% (" +
                           String(fuelLevel.getLiters(), 1) + "L)";
            oledDisplay.showMessage(line1.c_str(), line2.c_str(), line3.c_str());
            delay(3000);
            break;
        }
        case 1: // Network Info
        {
            String line1 = "WiFi: " + String(wifiManager.isConnected() ? "Connected" : "Disconnected");
            String line2 = "IP: " + wifiManager.getIP();
            String line3 = "MQTT: " + String(mqttManager.isConnected() ? "Connected" : "Disconnected");
            oledDisplay.showMessage(line1.c_str(), line2.c_str(), line3.c_str());
            delay(3000);
            break;
        }
        case 2: // Calibration
            oledDisplay.showMessage("Calibration", "Set via BLE app", "Connect & configure");
            delay(3000);
            break;
        case 3: // System
        {
            String line1 = "FW: v" + String(FIRMWARE_VERSION);
            String line2 = "Time: " + rtcModule.getTimeString();
            String line3 = "Uptime: " + String(millis() / 1000) + "s";
            oledDisplay.showMessage(line1.c_str(), line2.c_str(), line3.c_str());
            delay(3000);
            break;
        }
        case 4: // Reset Fuel Total
            if (_actionCallback)
                _actionCallback(MENU_RESET_TOTAL);
            oledDisplay.showMessage("Fuel total", "RESET!", nullptr);
            delay(2000);
            break;
        case 5: // Reset Theft Alert
            if (_actionCallback)
                _actionCallback(MENU_RESET_THEFT);
            oledDisplay.showMessage("Theft alert", "CLEARED!", nullptr);
            delay(2000);
            break;
        case 6: // Reboot
            oledDisplay.showMessage("Rebooting...", nullptr, nullptr);
            delay(1000);
            if (_actionCallback)
                _actionCallback(MENU_REBOOT);
            break;
        case 7: // Factory Reset
            oledDisplay.showMessage("FACTORY RESET", "All data erased!", "Rebooting...");
            delay(2000);
            if (_actionCallback)
                _actionCallback(MENU_FACTORY_RESET);
            break;
        case 8: // Exit
            deactivate();
            return;
        }
    }
}

void MenuSystem::_goBack()
{
    if (_currentScreen == MENU_MAIN)
    {
        deactivate();
    }
    else
    {
        _currentScreen = MENU_MAIN;
        _selectedIndex = 0;
        _scrollOffset = 0;
    }
}

void MenuSystem::_renderMenu()
{
    switch (_currentScreen)
    {
    case MENU_MAIN:
        oledDisplay.showMenu("MAIN MENU", _mainMenuItems, _mainMenuCount,
                             _selectedIndex, _scrollOffset);
        break;
    default:
        break;
    }
}