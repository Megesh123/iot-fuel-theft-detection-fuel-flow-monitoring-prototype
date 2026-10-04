/**********************
 * BUTTON HANDLER
 * 5-button HMI: MENU, UP, DOWN, BACK, OK
 * Supports debounce, press, release, long press
 **********************/

#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <Arduino.h>

// Button identifiers
enum ButtonId
{
    BTN_MENU = 0,
    BTN_UP,
    BTN_DOWN,
    BTN_BACK,
    BTN_OK,
    BTN_COUNT // Total number of buttons
};

// Button events
enum ButtonEvent
{
    BTN_EVENT_NONE = 0,
    BTN_EVENT_PRESS,
    BTN_EVENT_RELEASE,
    BTN_EVENT_LONG_PRESS
};

class ButtonHandler
{
public:
    void begin();
    void update(); // Call in loop

    ButtonEvent getEvent(ButtonId id); // Get and clear event for button
    bool isPressed(ButtonId id);       // Check if currently pressed
    bool isMenuHeldOnBoot();           // Check MENU held during boot

private:
    struct ButtonState
    {
        uint8_t pin;
        bool currentState;
        bool lastState;
        bool pressed;
        uint32_t lastDebounceTime;
        uint32_t pressStartTime;
        bool longPressTriggered;
        ButtonEvent event;
    };

    ButtonState _buttons[BTN_COUNT];
    uint32_t _debounceMs;
    uint32_t _longPressMs;
};

extern ButtonHandler buttons;

#endif