/**********************
 * MENU SYSTEM
 * 5-button navigation menu displayed on OLED
 **********************/

#ifndef __MENU_SYSTEM_H__
#define __MENU_SYSTEM_H__

#include <Arduino.h>
#include "../config/settings.h"

// Menu screens
enum MenuScreen
{
    MENU_MAIN = 0,
    MENU_SENSOR_INFO,
    MENU_NETWORK_INFO,
    MENU_CALIBRATION,
    MENU_SYSTEM,
    MENU_RESET_TOTAL,
    MENU_RESET_THEFT,
    MENU_REBOOT,
    MENU_FACTORY_RESET,
    MENU_SCREEN_COUNT
};

// Forward declaration
class OLEDDisplay;

class MenuSystem
{
public:
    void begin();
    void update(); // Process button input

    bool isActive();   // Menu currently displayed
    void activate();   // Enter menu
    void deactivate(); // Exit menu

    MenuScreen getCurrentScreen();

    // Action callbacks
    typedef void (*MenuActionCallback)(MenuScreen action);
    void setActionCallback(MenuActionCallback callback);

private:
    bool _active;
    MenuScreen _currentScreen;
    uint8_t _selectedIndex;
    uint8_t _scrollOffset;
    uint32_t _lastActivity; // For timeout

    MenuActionCallback _actionCallback;

    void _navigateUp();
    void _navigateDown();
    void _selectItem();
    void _goBack();
    void _renderMenu();

    // Menu item definitions
    static const char *_mainMenuItems[];
    static const uint8_t _mainMenuCount;
};

extern MenuSystem menuSystem;

#endif