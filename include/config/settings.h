/**********************
 * SYSTEM SETTINGS & CONSTANTS
 * Default values, timing, and configuration structures
 **********************/

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <Arduino.h>

// ==================== Version ====================
#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_PREFIX "FTMS" // Fuel Tank Monitoring System

// ==================== Timing (milliseconds) ====================
#define WIFI_CONNECT_TIMEOUT 30000   // 30 seconds WiFi timeout
#define MQTT_RECONNECT_INTERVAL 5000 // 5 seconds between MQTT reconnect attempts
#define SENSOR_READ_INTERVAL 1000    // Read sensors every 1 second
#define DISPLAY_UPDATE_INTERVAL 500  // Update display every 500ms
#define MQTT_PUBLISH_INTERVAL 10000  // Publish data every 10 seconds
#define BUTTON_DEBOUNCE_MS 50        // Button debounce time
#define BUTTON_LONG_PRESS_MS 3000    // Long press threshold
#define POWER_CHECK_INTERVAL 100     // Check power every 100ms
#define MENU_TIMEOUT_MS 30000        // Menu auto-exit after 30s inactivity

// ==================== Fuel Flow Calibration ====================
#define DEFAULT_PULSES_PER_LITER 450.0f // Default calibration (adjust for your sensor)
#define FLOW_CALC_INTERVAL 1000         // Calculate flow rate every 1 second

// ==================== Fuel Level Calibration ====================
#define FUEL_LEVEL_SAMPLES 64     // ADC averaging samples
#define FUEL_LEVEL_MIN_RAW 0      // ADC value at empty tank
#define FUEL_LEVEL_MAX_RAW 4095   // ADC value at full tank
#define FUEL_TANK_CAPACITY 100.0f // Tank capacity in liters

// ==================== Fuel Theft Detection ====================
#define FUEL_THEFT_THRESHOLD 5.0f      // Sudden drop percentage threshold
#define FUEL_THEFT_CHECK_INTERVAL 5000 // Check every 5 seconds

// ==================== EEPROM Layout ====================
#define EEPROM_CONFIG_START 0x0000 // Config data start address
#define EEPROM_CONFIG_SIZE 512     // Max config size in bytes
#define EEPROM_DATA_START 0x0200   // Runtime data start address
#define EEPROM_MAGIC_BYTE 0xA5     // Magic byte for valid config check
#define EEPROM_PAGE_SIZE 64        // 24LC256 page size

// ==================== Configuration Structure ====================
struct DeviceConfig
{
    uint8_t magic;               // Magic byte for validation
    char wifiSSID[33];           // WiFi SSID (max 32 chars + null)
    char wifiPassword[65];       // WiFi password (max 64 chars + null)
    char mqttServer[65];         // MQTT server address
    uint16_t mqttPort;           // MQTT port
    char mqttUsername[33];       // MQTT username
    char mqttPassword[65];       // MQTT password
    char deviceId[33];           // Device ID / name
    char mqttTopicPublish[65];   // MQTT publish topic
    char mqttTopicSubscribe[65]; // MQTT subscribe topic
    float pulsesPerLiter;        // Fuel flow calibration
    float tankCapacity;          // Tank capacity (liters)
    uint16_t fuelLevelMinRaw;    // ADC raw at empty
    uint16_t fuelLevelMaxRaw;    // ADC raw at full
    uint8_t use4G;               // 0 = WiFi only, 1 = 4G fallback
    uint16_t publishInterval;    // MQTT publish interval (seconds)
    uint8_t reserved[32];        // Reserved for future use
    uint16_t crc;                // CRC16 checksum
};

// ==================== System State ====================
enum SystemMode
{
    MODE_INIT,
    MODE_BLE_CONFIG,
    MODE_NORMAL,
    MODE_MENU,
    MODE_ERROR
};

enum ConnectivityType
{
    CONN_NONE,
    CONN_WIFI,
    CONN_4G
};

// ==================== Runtime Data ====================
// Add Max Flow Rate threshold (e.g., 20.0 Liters per hour max expected)
#define MAX_ENGINE_FLOW_RATE_LPH   20.0f  

struct SensorData {
    float    fuelFlowRate;
    float    fuelTotalVolume;
    float    fuelLevelPercent;
    float    fuelLevelLiters;
    bool     fuelTheftDetected;
    bool     isEngineOn;          // NEW: Ignition status
    bool     valveClosed;         // NEW: Solenoid valve cut-off status
    String   timestamp;
};

#endif // __SETTINGS_H__