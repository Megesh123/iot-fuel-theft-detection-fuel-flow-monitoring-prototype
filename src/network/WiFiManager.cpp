/**********************
 * WiFi MANAGER IMPLEMENTATION
 **********************/

#include "network/WiFiManager.h"
#include "config/settings.h"

WiFiManager wifiManager;

void WiFiManager::begin()
{
    _state = WIFI_STATE_DISCONNECTED;
    _lastReconnectAttempt = 0;

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    Serial.println("[WiFi] Manager initialized");
}

WiFiState WiFiManager::connect(const char *ssid, const char *password, uint32_t timeoutMs)
{
    _ssid = String(ssid);
    _password = String(password);
    _timeoutMs = timeoutMs;

    if (_ssid.length() == 0)
    {
        Serial.println("[WiFi] No SSID configured");
        _state = WIFI_STATE_FAILED;
        return _state;
    }

    Serial.println("[WiFi] Connecting to: " + _ssid);
    _state = WIFI_STATE_CONNECTING;
    _connectStartTime = millis();

    WiFi.begin(ssid, password);

    // Blocking connect with timeout
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - _connectStartTime > _timeoutMs)
        {
            Serial.println("[WiFi] Connection timeout");
            _state = WIFI_STATE_FAILED;
            WiFi.disconnect();
            return _state;
        }
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("[WiFi] Connected! IP: " + WiFi.localIP().toString());
    _state = WIFI_STATE_CONNECTED;
    return _state;
}

void WiFiManager::disconnect()
{
    WiFi.disconnect();
    _state = WIFI_STATE_DISCONNECTED;
    Serial.println("[WiFi] Disconnected");
}

void WiFiManager::update()
{
    if (_state == WIFI_STATE_CONNECTED && WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[WiFi] Connection lost");
        _state = WIFI_STATE_DISCONNECTED;
    }
}

WiFiState WiFiManager::getState()
{
    return _state;
}

bool WiFiManager::isConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getIP()
{
    if (isConnected())
        return WiFi.localIP().toString();
    return "0.0.0.0";
}

int WiFiManager::getRSSI()
{
    if (isConnected())
        return WiFi.RSSI();
    return -100;
}