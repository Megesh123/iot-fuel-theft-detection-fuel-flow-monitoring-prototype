/******************************************************************************
 * IoT FUEL TANK MONITORING SYSTEM
 * Main application entry point
 *
 * Features:
 * - Fuel flow rate measurement (Hall effect pulse sensor)
 * - Fuel level monitoring (analog sensor)
 * - BLE configuration mode (hold MENU on boot)
 * - WiFi + MQTT cloud connectivity
 * - 4G cellular fallback (A7676G)
 * - OLED display with menu system
 * - External EEPROM persistent storage (24LC256)
 * - DS3231 RTC for timestamps
 * - Power fail detection with emergency save
 * - Fuel theft detection
 ******************************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>

// Project modules
#include "config/pins.h"
#include "config/settings.h"
#include "bluetooth/BLEScanner.h"
#include "fuel/FuelFlow.h"
#include "fuel/FuelLevel.h"
#include "button/Button.h"
#include "storage/EEPROMStorage.h"
#include "display/Display.h"
#include "rtc/RTCModule.h"
#include "network/WiFiManager.h"
#include "network/MQTTManager.h"
#include "network/CellularManager.h"
#include "menu/MenuSystem.h"
#include "power/PowerMonitor.h"

// ==================== Global State ====================
SystemMode systemMode = MODE_INIT;
ConnectivityType connectivity = CONN_NONE;
DeviceConfig deviceConfig;
SensorData sensorData;

// ==================== Timing Variables ====================
uint32_t lastSensorRead = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastMqttPublish = 0;

// ==================== Function Declarations ====================
void enterBLEConfigMode();
void processBLEConfig();
void enterNormalMode();
void readSensors();
void updateDisplay();
void publishData();
void buildJsonPayload(char *buffer, size_t bufferSize);
void onMQTTMessage(const char *topic, const char *payload);
void onMenuAction(MenuScreen action);
void onPowerFail();
void emergencySave();

// ==================== SETUP ====================
void setup()
{
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========================================");
  Serial.println("  IoT Fuel Tank Monitoring System");
  Serial.println("  Firmware v" FIRMWARE_VERSION);
  Serial.println("========================================\n");

  // Initialize I2C bus
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000); // 400kHz I2C

  // Initialize buttons first (need MENU check)
  buttons.begin();

  // Initialize display
  oledDisplay.begin(Wire, OLED_I2C_ADDR);
  oledDisplay.showSplash(FIRMWARE_VERSION);
  delay(2000);

  // Initialize EEPROM
  eepromStorage.begin(Wire, EEPROM_I2C_ADDR);

  // Initialize RTC
  rtcModule.begin(Wire);

  // Initialize power monitor
  powerMonitor.begin(PIN_POWER_FAIL, true, 2048); // Analog mode
  powerMonitor.setCallback(onPowerFail);

  // ==================== Check Boot Mode ====================
  // Check if MENU button was held during power-on
  bool bleConfigMode = buttons.isMenuHeldOnBoot();

  if (bleConfigMode)
  {
    // ---- BLE CONFIGURATION MODE ----
    enterBLEConfigMode();
  }
  else
  {
    // ---- NORMAL STARTUP ----
    // Load configuration from EEPROM
    if (eepromStorage.loadConfig(deviceConfig))
    {
      Serial.println("[MAIN] Valid configuration loaded from EEPROM");
      enterNormalMode();
    }
    else
    {
      Serial.println("[MAIN] No valid configuration - entering BLE config mode");
      eepromStorage.loadDefaults(deviceConfig);
      enterBLEConfigMode();
    }
  }
}

// ==================== LOOP ====================
void loop()
{
  uint32_t now = millis();

  // Always update buttons and power monitor
  buttons.update();
  powerMonitor.update();

  switch (systemMode)
  {
  // ==================== BLE CONFIG MODE ====================
  case MODE_BLE_CONFIG:
  {
    processBLEConfig();

    // Update display with BLE status
    static uint32_t lastBLEDisplayUpdate = 0;
    if (now - lastBLEDisplayUpdate > 1000)
    {
      oledDisplay.showBLEConfig(DEVICE_PREFIX, Bluetooth.status());
      lastBLEDisplayUpdate = now;
    }
    break;
  }

  // ==================== NORMAL OPERATION ====================
  case MODE_NORMAL:
  {
    // Check MENU button for menu activation
    ButtonEvent menuEvt = buttons.getEvent(BTN_MENU);
    if (menuEvt == BTN_EVENT_PRESS && !menuSystem.isActive())
    {
      menuSystem.activate();
      systemMode = MODE_MENU;
      break;
    }

    // Read sensors periodically
    if (now - lastSensorRead >= SENSOR_READ_INTERVAL)
    {
      readSensors();
      lastSensorRead = now;
    }

    // Update display
    if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL)
    {
      updateDisplay();
      lastDisplayUpdate = now;
    }

    // Publish data via MQTT
    uint32_t publishInterval = (uint32_t)deviceConfig.publishInterval * 1000;
    if (publishInterval == 0)
      publishInterval = MQTT_PUBLISH_INTERVAL;

    if (now - lastMqttPublish >= publishInterval)
    {
      publishData();
      lastMqttPublish = now;
    }

    // Update network connections
    wifiManager.update();
    if (connectivity == CONN_WIFI && wifiManager.isConnected())
    {
      mqttManager.update();
    }
    else if (connectivity == CONN_4G)
    {
      cellularManager.update();
    }

    // Check WiFi reconnection if lost
    if (connectivity == CONN_WIFI && !wifiManager.isConnected())
    {
      Serial.println("[MAIN] WiFi lost, attempting reconnect...");
      WiFiState wState = wifiManager.connect(deviceConfig.wifiSSID,
                                             deviceConfig.wifiPassword,
                                             10000);
      if (wState != WIFI_STATE_CONNECTED && deviceConfig.use4G)
      {
        Serial.println("[MAIN] WiFi failed, switching to 4G");
        if (cellularManager.initialize())
        {
          cellularManager.connectMQTT(deviceConfig.mqttServer,
                                      deviceConfig.mqttPort,
                                      deviceConfig.deviceId,
                                      deviceConfig.mqttUsername,
                                      deviceConfig.mqttPassword);
          connectivity = CONN_4G;
        }
      }
    }
    break;
  }

  // ==================== MENU MODE ====================
  case MODE_MENU:
  {
    menuSystem.update();

    if (!menuSystem.isActive())
    {
      systemMode = MODE_NORMAL;
    }

    // Still read sensors in background
    if (now - lastSensorRead >= SENSOR_READ_INTERVAL)
    {
      readSensors();
      lastSensorRead = now;
    }
    break;
  }

  // ==================== ERROR MODE ====================
  case MODE_ERROR:
  {
    static uint32_t lastErrorDisplay = 0;
    if (now - lastErrorDisplay > 2000)
    {
      oledDisplay.showError("System Error");
      lastErrorDisplay = now;
    }

    // Allow reboot via OK button
    if (buttons.getEvent(BTN_OK) == BTN_EVENT_LONG_PRESS)
    {
      ESP.restart();
    }
    break;
  }

  default:
    break;
  }
}

// ==================== BLE CONFIGURATION MODE ====================
void enterBLEConfigMode()
{
  Serial.println("[MAIN] Entering BLE Configuration Mode");
  systemMode = MODE_BLE_CONFIG;

  Bluetooth.name(String(DEVICE_PREFIX) + "_Setup");
  Bluetooth.enable();

  oledDisplay.showBLEConfig(DEVICE_PREFIX, Bluetooth.status());
}

void processBLEConfig()
{
  if (!Bluetooth.isMessageAvailable())
    return;

  String message = Bluetooth.read();
  Serial.println("[BLE_CFG] Received: " + message);

  // Parse JSON configuration
  // Expected format: {"ssid":"xxx","pass":"xxx","mqtt_srv":"xxx","mqtt_port":1883,...}
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error)
  {
    Serial.println("[BLE_CFG] JSON parse error: " + String(error.c_str()));
    Bluetooth.send("*ERROR:INVALID_JSON#");
    return;
  }

  // Extract configuration fields
  bool configChanged = false;

  if (doc.containsKey("ssid"))
  {
    strncpy(deviceConfig.wifiSSID, doc["ssid"].as<const char *>(),
            sizeof(deviceConfig.wifiSSID) - 1);
    configChanged = true;
  }
  if (doc.containsKey("pass"))
  {
    strncpy(deviceConfig.wifiPassword, doc["pass"].as<const char *>(),
            sizeof(deviceConfig.wifiPassword) - 1);
    configChanged = true;
  }
  if (doc.containsKey("mqtt_srv"))
  {
    strncpy(deviceConfig.mqttServer, doc["mqtt_srv"].as<const char *>(),
            sizeof(deviceConfig.mqttServer) - 1);
    configChanged = true;
  }
  if (doc.containsKey("mqtt_port"))
  {
    deviceConfig.mqttPort = doc["mqtt_port"].as<uint16_t>();
    configChanged = true;
  }
  if (doc.containsKey("mqtt_user"))
  {
    strncpy(deviceConfig.mqttUsername, doc["mqtt_user"].as<const char *>(),
            sizeof(deviceConfig.mqttUsername) - 1);
    configChanged = true;
  }
  if (doc.containsKey("mqtt_pass"))
  {
    strncpy(deviceConfig.mqttPassword, doc["mqtt_pass"].as<const char *>(),
            sizeof(deviceConfig.mqttPassword) - 1);
    configChanged = true;
  }
  if (doc.containsKey("device_id"))
  {
    strncpy(deviceConfig.deviceId, doc["device_id"].as<const char *>(),
            sizeof(deviceConfig.deviceId) - 1);
    configChanged = true;
  }
  if (doc.containsKey("pub_topic"))
  {
    strncpy(deviceConfig.mqttTopicPublish, doc["pub_topic"].as<const char *>(),
            sizeof(deviceConfig.mqttTopicPublish) - 1);
    configChanged = true;
  }
  if (doc.containsKey("sub_topic"))
  {
    strncpy(deviceConfig.mqttTopicSubscribe, doc["sub_topic"].as<const char *>(),
            sizeof(deviceConfig.mqttTopicSubscribe) - 1);
    configChanged = true;
  }
  if (doc.containsKey("pulses_per_l"))
  {
    deviceConfig.pulsesPerLiter = doc["pulses_per_l"].as<float>();
    configChanged = true;
  }
  if (doc.containsKey("tank_cap"))
  {
    deviceConfig.tankCapacity = doc["tank_cap"].as<float>();
    configChanged = true;
  }
  if (doc.containsKey("level_min"))
  {
    deviceConfig.fuelLevelMinRaw = doc["level_min"].as<uint16_t>();
    configChanged = true;
  }
  if (doc.containsKey("level_max"))
  {
    deviceConfig.fuelLevelMaxRaw = doc["level_max"].as<uint16_t>();
    configChanged = true;
  }
  if (doc.containsKey("use_4g"))
  {
    deviceConfig.use4G = doc["use_4g"].as<uint8_t>();
    configChanged = true;
  }
  if (doc.containsKey("pub_interval"))
  {
    deviceConfig.publishInterval = doc["pub_interval"].as<uint16_t>();
    configChanged = true;
  }

  // Set RTC time if provided
  if (doc.containsKey("rtc_time"))
  {
    const char *timeStr = doc["rtc_time"].as<const char *>();
    rtcModule.setFromString(timeStr);
    Serial.println("[BLE_CFG] RTC time set: " + String(timeStr));
  }

  // Save configuration
  if (configChanged)
  {
    if (eepromStorage.saveConfig(deviceConfig))
    {
      Serial.println("[BLE_CFG] Configuration saved to EEPROM");
      Bluetooth.send("*OK:CONFIG_SAVED#");

      oledDisplay.showMessage("Config Saved!", "Rebooting in 3s...", nullptr);
      delay(3000);

      // Reboot to apply new configuration
      ESP.restart();
    }
    else
    {
      Bluetooth.send("*ERROR:SAVE_FAILED#");
      oledDisplay.showMessage("Save FAILED!", "Try again", nullptr);
    }
  }
  else
  {
    Bluetooth.send("*ERROR:NO_DATA#");
  }
}

// ==================== NORMAL MODE INITIALIZATION ====================
void enterNormalMode()
{
  Serial.println("[MAIN] Entering Normal Operation Mode");
  systemMode = MODE_NORMAL;

  // Show connecting screen
  oledDisplay.showMessage("Starting...", "Connecting to WiFi", nullptr);

  // Initialize sensors
  fuelFlow.begin(PIN_FUEL_FLOW, deviceConfig.pulsesPerLiter);
  fuelLevel.begin(PIN_FUEL_LEVEL, deviceConfig.fuelLevelMinRaw,
                  deviceConfig.fuelLevelMaxRaw, deviceConfig.tankCapacity);

  // Initialize menu
  menuSystem.begin();
  menuSystem.setActionCallback(onMenuAction);

  // ---- Connect to WiFi ----
  wifiManager.begin();
  WiFiState wifiState = wifiManager.connect(deviceConfig.wifiSSID,
                                            deviceConfig.wifiPassword,
                                            WIFI_CONNECT_TIMEOUT);

  if (wifiState == WIFI_STATE_CONNECTED)
  {
    connectivity = CONN_WIFI;
    Serial.println("[MAIN] WiFi connected, starting MQTT");

    // Initialize and connect MQTT over WiFi
    mqttManager.begin(deviceConfig.mqttServer, deviceConfig.mqttPort,
                      deviceConfig.mqttUsername, deviceConfig.mqttPassword,
                      deviceConfig.deviceId);
    mqttManager.setCallback(onMQTTMessage);
    mqttManager.subscribe(deviceConfig.mqttTopicSubscribe);
    mqttManager.connect();
  }
  else if (deviceConfig.use4G)
  {
    // WiFi failed - try 4G
    Serial.println("[MAIN] WiFi failed, trying 4G...");
    oledDisplay.showMessage("WiFi failed", "Trying 4G...", nullptr);

    cellularManager.begin(PIN_4G_RX, PIN_4G_TX, PIN_4G_PWR);

    if (cellularManager.initialize())
    {
      connectivity = CONN_4G;
      cellularManager.connectMQTT(deviceConfig.mqttServer,
                                  deviceConfig.mqttPort,
                                  deviceConfig.deviceId,
                                  deviceConfig.mqttUsername,
                                  deviceConfig.mqttPassword);
      Serial.println("[MAIN] 4G connected");
    }
    else
    {
      Serial.println("[MAIN] 4G initialization failed");
      connectivity = CONN_NONE;
    }
  }
  else
  {
    Serial.println("[MAIN] No connectivity available (4G disabled)");
    connectivity = CONN_NONE;
  }

  Serial.println("[MAIN] Normal mode initialized, connectivity: " +
                 String(connectivity == CONN_WIFI ? "WiFi" : connectivity == CONN_4G ? "4G"
                                                                                     : "None"));
}

// ==================== SENSOR READING ====================
void readSensors()
{
  // Update fuel flow
  fuelFlow.update();
  sensorData.fuelFlowRate = fuelFlow.getFlowRate();
  sensorData.fuelTotalVolume = fuelFlow.getTotalVolume();
  sensorData.isEngineOn = (digitalRead(PIN_IGNITION_SENSE) == HIGH);

  // Actively check for flow anomalies and cut off if necessary
  fuelFlow.checkFlowAnomalies(sensorData.fuelFlowRate, MAX_ENGINE_FLOW_RATE_LPH);
  sensorData.valveClosed = fuelFlow.isValveClosed();

  // Update fuel level
  fuelLevel.update();
  sensorData.fuelLevelPercent = fuelLevel.getPercent();
  sensorData.fuelLevelLiters = fuelLevel.getLiters();

  // Check theft
  sensorData.fuelTheftDetected = fuelLevel.checkTheft(FUEL_THEFT_THRESHOLD);

  // Get timestamp
  sensorData.timestamp = rtcModule.getTimestamp();
}

// ==================== DISPLAY UPDATE ====================
void updateDisplay()
{
  if (menuSystem.isActive())
    return; // Don't overwrite menu

  bool mqttConn = false;
  if (connectivity == CONN_WIFI)
    mqttConn = mqttManager.isConnected();
  else if (connectivity == CONN_4G)
    mqttConn = cellularManager.isMQTTConnected();

  oledDisplay.showNormalStatus(sensorData, connectivity, mqttConn);
}

// ==================== DATA PUBLISHING ====================
void publishData()
{
  char jsonBuffer[512];
  buildJsonPayload(jsonBuffer, sizeof(jsonBuffer));

  if (connectivity == CONN_WIFI && mqttManager.isConnected())
  {
    mqttManager.publish(deviceConfig.mqttTopicPublish, jsonBuffer);
  }
  else if (connectivity == CONN_4G && cellularManager.isMQTTConnected())
  {
    cellularManager.publishMQTT(deviceConfig.mqttTopicPublish, jsonBuffer);
  }
  else
  {
    Serial.println("[MAIN] Cannot publish - no connection");
  }
}

void buildJsonPayload(char *buffer, size_t bufferSize)
{
  StaticJsonDocument<512> doc;

  doc["device_id"] = deviceConfig.deviceId;
  doc["timestamp"] = sensorData.timestamp;
  doc["fuel_flow_rate"] = round(sensorData.fuelFlowRate * 100.0) / 100.0;
  doc["fuel_total"] = round(sensorData.fuelTotalVolume * 100.0) / 100.0;
  doc["fuel_level_pct"] = round(sensorData.fuelLevelPercent * 10.0) / 10.0;
  doc["fuel_level_l"] = round(sensorData.fuelLevelLiters * 10.0) / 10.0;
  doc["theft_alert"] = sensorData.fuelTheftDetected;
  doc["fw_version"] = FIRMWARE_VERSION;

  if (connectivity == CONN_WIFI)
  {
    doc["rssi"] = wifiManager.getRSSI();
    doc["conn"] = "wifi";
  }
  else if (connectivity == CONN_4G)
  {
    doc["conn"] = "4g";
  }

  serializeJson(doc, buffer, bufferSize);
}

// ==================== MQTT MESSAGE CALLBACK ====================
void onMQTTMessage(const char *topic, const char *payload)
{
  Serial.println("[MAIN] MQTT command received: " + String(payload));

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error)
    return;

  // Handle remote commands
  if (doc.containsKey("cmd"))
  {
    String cmd = doc["cmd"].as<String>();

    if (cmd == "reset_total")
    {
      fuelFlow.reset();
      Serial.println("[MAIN] Fuel total reset via MQTT");
    }
    else if (cmd == "reset_theft")
    {
      fuelLevel.resetTheftAlert();
      sensorData.fuelTheftDetected = false;
      Serial.println("[MAIN] Theft alert reset via MQTT");
    }
    else if (cmd == "reboot")
    {
      Serial.println("[MAIN] Reboot requested via MQTT");
      delay(1000);
      ESP.restart();
    }
    else if (cmd == "status")
    {
      // Force immediate publish
      publishData();
    }
  }

  // Handle RTC time sync
  if (doc.containsKey("set_time"))
  {
    const char *timeStr = doc["set_time"].as<const char *>();
    rtcModule.setFromString(timeStr);
  }
}

// ==================== MENU ACTION CALLBACK ====================
void onMenuAction(MenuScreen action)
{
  switch (action)
  {
  case MENU_RESET_TOTAL:
    fuelFlow.reset();
    Serial.println("[MAIN] Fuel total reset via menu");
    break;

  case MENU_RESET_THEFT:
    fuelLevel.resetTheftAlert();
    sensorData.fuelTheftDetected = false;
    Serial.println("[MAIN] Theft alert reset via menu");
    break;

  case MENU_REBOOT:
    Serial.println("[MAIN] Rebooting...");
    delay(500);
    ESP.restart();
    break;

  case MENU_FACTORY_RESET:
    Serial.println("[MAIN] Factory reset!");
    eepromStorage.eraseConfig();
    delay(1000);
    ESP.restart();
    break;

  default:
    break;
  }
}

// ==================== POWER FAIL HANDLER ====================
void onPowerFail()
{
  Serial.println("[MAIN] *** POWER FAILURE - EMERGENCY SAVE ***");
  oledDisplay.showPowerFailWarning();
  emergencySave();
}

void emergencySave()
{
  // Save runtime data to EEPROM at data area
  // This writes the current fuel total so it persists across power cycles
  struct RuntimeData
  {
    float totalVolume;
    float lastLevelPercent;
    uint32_t timestamp; // millis at save time
    uint8_t valid;
  };

  RuntimeData data;
  data.totalVolume = fuelFlow.getTotalVolume();
  data.lastLevelPercent = fuelLevel.getPercent();
  data.timestamp = millis();
  data.valid = 0xBB; // Marker for valid runtime data

  eepromStorage.writeBlock(EEPROM_DATA_START, (uint8_t *)&data, sizeof(RuntimeData));
  Serial.println("[MAIN] Emergency data saved: total=" + String(data.totalVolume) + "L");
}