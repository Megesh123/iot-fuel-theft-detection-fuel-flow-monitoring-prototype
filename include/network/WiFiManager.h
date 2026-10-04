/**********************
 * WiFi MANAGER
 * Handles WiFi connection with timeout and reconnection
 **********************/

#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <Arduino.h>
#include <WiFi.h>

enum WiFiState
{
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_FAILED
};

class WiFiManager
{
public:
    void begin();
    WiFiState connect(const char *ssid, const char *password, uint32_t timeoutMs = 30000);
    void disconnect();
    void update(); // Check connection status periodically

    WiFiState getState();
    bool isConnected();
    String getIP();
    int getRSSI();

private:
    WiFiState _state;
    String _ssid;
    String _password;
    uint32_t _connectStartTime;
    uint32_t _timeoutMs;
    uint32_t _lastReconnectAttempt;
};

extern WiFiManager wifiManager;

#endif