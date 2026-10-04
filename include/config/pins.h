/**********************
 * PIN DEFINITIONS (Optimized for ESP32-S3)
 **********************/

#ifndef __PINS_H__
#define __PINS_H__

// ==================== I2C Bus ====================
#define PIN_I2C_SDA 8 // Standard S3 I2C SDA
#define PIN_I2C_SCL 9 // Standard S3 I2C SCL

// ==================== Fuel Sensors ====================
#define PIN_FUEL_FLOW 13  // Pulse input (Interrupt capable)
#define PIN_FUEL_LEVEL 14 // Analog input (Must be ADC1: GPIO 1-10 on S3)

// ==================== Buttons (Active LOW) ====================
#define PIN_BTN_MENU 38
#define PIN_BTN_UP 39
#define PIN_BTN_DOWN 40
#define PIN_BTN_BACK 41
#define PIN_BTN_OK 42

// Add Ignition Sense and Valve Control Pins for ESP32-S3
#define PIN_IGNITION_SENSE 12   // Input: 12V Ignition converted to 3.3V (HIGH = Engine ON)
#define PIN_FUEL_VALVE_RELAY 15 // Output: Controls Solenoid Valve Relay (HIGH = Close Valve)

// ==================== 4G Module (A7676G) UART ====================
#define PIN_4G_TX 17  // ESP32-S3 TX -> 4G RX
#define PIN_4G_RX 18  // ESP32-S3 RX -> 4G TX
#define PIN_4G_PWR 21 // 4G module power key

// ==================== Power Monitoring ====================
#define PIN_POWER_FAIL 6 // 3.3V monitoring input (Analog ADC1 or Digital)

// ==================== I2C Addresses ====================
#define EEPROM_I2C_ADDR 0x50 // 24LC256 default address
#define OLED_I2C_ADDR 0x3C   // SSD1306 default address

#endif // __PINS_H__