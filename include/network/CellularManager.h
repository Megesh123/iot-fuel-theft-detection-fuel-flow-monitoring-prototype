/**********************
 * CELLULAR MANAGER (A7676G via AIS_IoT_4G)
 * 4G backup connectivity when WiFi unavailable
 **********************/

#ifndef __CELLULAR_MANAGER_H__
#define __CELLULAR_MANAGER_H__

#include <Arduino.h>

// Forward declare - library may not always be available
#define CELLULAR_BAUD_RATE 115200

enum CellularState
{
    CELL_STATE_OFF,
    CELL_STATE_INITIALIZING,
    CELL_STATE_READY,
    CELL_STATE_CONNECTED,
    CELL_STATE_ERROR
};

class CellularManager
{
public:
    void begin(uint8_t rxPin, uint8_t txPin, uint8_t pwrPin = 0);
    bool initialize(); // Power on and initialize module
    void update();     // Call in loop

    bool connectMQTT(const char *server, uint16_t port,
                     const char *clientId,
                     const char *username = nullptr,
                     const char *password = nullptr);
    bool publishMQTT(const char *topic, const char *payload);
    bool subscribeMQTT(const char *topic);
    void disconnectMQTT();

    bool sendHTTPS(const char *url, const char *payload);

    CellularState getState();
    bool isMQTTConnected();
    String getSignalStrength();

private:
    HardwareSerial *_serial;
    uint8_t _rxPin;
    uint8_t _txPin;
    uint8_t _pwrPin;
    CellularState _state;
    bool _mqttConnected;

    bool _sendATCommand(const char *cmd, const char *expectedResponse,
                        uint32_t timeoutMs = 5000);
    String _readResponse(uint32_t timeoutMs = 2000);
    void _powerOn();
};

extern CellularManager cellularManager;

#endif