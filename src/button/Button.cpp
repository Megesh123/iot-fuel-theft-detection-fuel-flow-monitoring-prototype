/**********************
 * BUTTON HANDLER IMPLEMENTATION
 **********************/

#include "button/Button.h"
#include "config/pins.h"
#include "config/settings.h"

ButtonHandler buttons;

void ButtonHandler::begin()
{
    _debounceMs = BUTTON_DEBOUNCE_MS;
    _longPressMs = BUTTON_LONG_PRESS_MS;

    // Define pin mapping
    uint8_t pins[BTN_COUNT] = {
        PIN_BTN_MENU,
        PIN_BTN_UP,
        PIN_BTN_DOWN,
        PIN_BTN_BACK,
        PIN_BTN_OK};

    for (int i = 0; i < BTN_COUNT; i++)
    {
        _buttons[i].pin = pins[i];
        _buttons[i].currentState = HIGH;
        _buttons[i].lastState = HIGH;
        _buttons[i].pressed = false;
        _buttons[i].lastDebounceTime = 0;
        _buttons[i].pressStartTime = 0;
        _buttons[i].longPressTriggered = false;
        _buttons[i].event = BTN_EVENT_NONE;

        pinMode(_buttons[i].pin, INPUT_PULLUP); // Active LOW
    }

    Serial.println("[BUTTONS] Initialized 5 buttons");
}

void ButtonHandler::update()
{
    uint32_t now = millis();

    for (int i = 0; i < BTN_COUNT; i++)
    {
        bool reading = digitalRead(_buttons[i].pin);

        // Debounce
        if (reading != _buttons[i].lastState)
        {
            _buttons[i].lastDebounceTime = now;
        }

        if ((now - _buttons[i].lastDebounceTime) > _debounceMs)
        {
            // State has been stable long enough
            if (reading != _buttons[i].currentState)
            {
                _buttons[i].currentState = reading;

                if (_buttons[i].currentState == LOW) // Button pressed (active LOW)
                {
                    _buttons[i].pressed = true;
                    _buttons[i].pressStartTime = now;
                    _buttons[i].longPressTriggered = false;
                    _buttons[i].event = BTN_EVENT_PRESS;
                }
                else // Button released
                {
                    _buttons[i].pressed = false;
                    if (!_buttons[i].longPressTriggered)
                    {
                        _buttons[i].event = BTN_EVENT_RELEASE;
                    }
                }
            }

            // Check for long press while held
            if (_buttons[i].pressed && !_buttons[i].longPressTriggered)
            {
                if ((now - _buttons[i].pressStartTime) >= _longPressMs)
                {
                    _buttons[i].longPressTriggered = true;
                    _buttons[i].event = BTN_EVENT_LONG_PRESS;
                }
            }
        }

        _buttons[i].lastState = reading;
    }
}

ButtonEvent ButtonHandler::getEvent(ButtonId id)
{
    if (id >= BTN_COUNT)
        return BTN_EVENT_NONE;

    ButtonEvent evt = _buttons[id].event;
    _buttons[id].event = BTN_EVENT_NONE; // Clear after reading
    return evt;
}

bool ButtonHandler::isPressed(ButtonId id)
{
    if (id >= BTN_COUNT)
        return false;
    return _buttons[id].pressed;
}

bool ButtonHandler::isMenuHeldOnBoot()
{
    // Check if MENU button is held LOW at boot time
    // Call this early in setup() before begin()
    pinMode(PIN_BTN_MENU, INPUT_PULLUP);
    delay(100); // Let pullup settle

    bool held = (digitalRead(PIN_BTN_MENU) == LOW);
    if (held)
    {
        Serial.println("[BUTTONS] MENU button held during boot - entering BLE config mode");
    }
    return held;
}