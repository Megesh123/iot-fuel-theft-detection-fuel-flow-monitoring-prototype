/**********************
 * MQTT MANAGER IMPLEMENTATION
 **********************/

#include "network/MQTTManager.h"

MQTTManager mqttManager;
MQTTManager *MQTTManager::_instance = nullptr;

void MQTTManager::begin(const char *server, uint16_t port,
                        const char *username, const char *password,
                        const char *clientId)
{
    _server = String(server);
    _port = port;
    _username = String(username);
    _password = String(password);
    _clientId = String(clientId);
    _userCallback = nullptr;
    _lastReconnectAttempt = 0;
    _initialized = false;
    _instance = this;

    if (_server.length() == 0)
    {
        Serial.println("[MQTT] No server configured");
        return;
    }

    _mqttClient.setClient(_wifiClient);
    _mqttClient.setServer(_server.c_str(), _port);
    _mqttClient.setCallback(_mqttCallback);
    _mqttClient.setBufferSize(512);

    _initialized = true;
    Serial.println("[MQTT] Configured: " + _server + ":" + String(_port));
}

bool MQTTManager::connect()
{
    if (!_initialized)
        return false;

    Serial.println("[MQTT] Connecting...");

    bool result;
    if (_username.length() > 0)
    {
        result = _mqttClient.connect(_clientId.c_str(),
                                     _username.c_str(),
                                     _password.c_str());
    }
    else
    {
        result = _mqttClient.connect(_clientId.c_str());
    }

    if (result)
    {
        Serial.println("[MQTT] Connected!");

        // Re-subscribe if topic was set
        if (_subscribeTopic.length() > 0)
        {
            _mqttClient.subscribe(_subscribeTopic.c_str());
            Serial.println("[MQTT] Subscribed to: " + _subscribeTopic);
        }
    }
    else
    {
        Serial.println("[MQTT] Connection failed, rc=" + String(_mqttClient.state()));
    }

    return result;
}

void MQTTManager::disconnect()
{
    if (_mqttClient.connected())
    {
        _mqttClient.disconnect();
        Serial.println("[MQTT] Disconnected");
    }
}

void MQTTManager::update()
{
    if (!_initialized)
        return;

    if (!_mqttClient.connected())
    {
        uint32_t now = millis();
        if (now - _lastReconnectAttempt > MQTT_RECONNECT_INTERVAL)
        {
            _lastReconnectAttempt = now;
            connect();
        }
    }
    else
    {
        _mqttClient.loop();
    }
}

bool MQTTManager::publish(const char *topic, const char *payload, bool retained)
{
    if (!_mqttClient.connected())
        return false;

    bool result = _mqttClient.publish(topic, payload, retained);
    if (result)
        Serial.println("[MQTT] Published to " + String(topic));
    else
        Serial.println("[MQTT] Publish failed");

    return result;
}

bool MQTTManager::subscribe(const char *topic)
{
    _subscribeTopic = String(topic);

    if (_mqttClient.connected())
    {
        return _mqttClient.subscribe(topic);
    }
    return false;
}

void MQTTManager::setCallback(MQTTMessageCallback callback)
{
    _userCallback = callback;
}

bool MQTTManager::isConnected()
{
    return _mqttClient.connected();
}

void MQTTManager::_mqttCallback(char *topic, byte *payload, unsigned int length)
{
    // Convert payload to string
    char msg[length + 1];
    memcpy(msg, payload, length);
    msg[length] = '\0';

    Serial.println("[MQTT] Received [" + String(topic) + "]: " + String(msg));

    // Forward to user callback
    if (_instance && _instance->_userCallback)
    {
        _instance->_userCallback(topic, msg);
    }
}