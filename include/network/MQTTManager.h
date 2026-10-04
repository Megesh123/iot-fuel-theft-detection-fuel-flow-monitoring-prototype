/**********************
 * MQTT MANAGER
 * Handles MQTT connection, publishing, subscribing
 * Works over WiFi (PubSubClient)
 **********************/

#ifndef __MQTT_MANAGER_H__
#define __MQTT_MANAGER_H__

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config/settings.h"

// Callback type for received messages
typedef void (*MQTTMessageCallback)(const char *topic, const char *payload);

class MQTTManager
{
public:
    void begin(const char *server, uint16_t port,
               const char *username, const char *password,
               const char *clientId);
    bool connect();
    void disconnect();
    void update(); // Call in loop - handles reconnection

    bool publish(const char *topic, const char *payload, bool retained = false);
    bool subscribe(const char *topic);
    void setCallback(MQTTMessageCallback callback);

    bool isConnected();

private:
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;

    String _server;
    uint16_t _port;
    String _username;
    String _password;
    String _clientId;
    String _subscribeTopic;

    MQTTMessageCallback _userCallback;

    uint32_t _lastReconnectAttempt;
    bool _initialized;

    static void _mqttCallback(char *topic, byte *payload, unsigned int length);
    static MQTTManager *_instance;
};

extern MQTTManager mqttManager;

#endif