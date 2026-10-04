/**********************
 * CELLULAR MANAGER IMPLEMENTATION
 * Uses AT commands for A7676G module
 * Integrates with AIS_IoT_4G library concepts
 **********************/

#include "network/CellularManager.h"
#include "config/pins.h"

CellularManager cellularManager;

void CellularManager::begin(uint8_t rxPin, uint8_t txPin, uint8_t pwrPin)
{
    _rxPin = rxPin;
    _txPin = txPin;
    _pwrPin = pwrPin;
    _state = CELL_STATE_OFF;
    _mqttConnected = false;

    _serial = &Serial2;
    _serial->begin(CELLULAR_BAUD_RATE, SERIAL_8N1, _rxPin, _txPin);

    if (_pwrPin > 0)
    {
        pinMode(_pwrPin, OUTPUT);
        digitalWrite(_pwrPin, LOW);
    }

    Serial.println("[4G] Module configured on Serial2");
}

bool CellularManager::initialize()
{
    Serial.println("[4G] Initializing module...");
    _state = CELL_STATE_INITIALIZING;

    // Power on if pin configured
    if (_pwrPin > 0)
    {
        _powerOn();
    }

    delay(3000); // Wait for module boot

    // Basic AT check
    if (!_sendATCommand("AT", "OK", 3000))
    {
        Serial.println("[4G] Module not responding");
        _state = CELL_STATE_ERROR;
        return false;
    }

    // Disable echo
    _sendATCommand("ATE0", "OK", 2000);

    // Check SIM
    if (!_sendATCommand("AT+CPIN?", "READY", 5000))
    {
        Serial.println("[4G] SIM card not ready");
        _state = CELL_STATE_ERROR;
        return false;
    }

    // Check network registration
    delay(2000);
    if (!_sendATCommand("AT+CREG?", "0,1", 10000) &&
        !_sendATCommand("AT+CREG?", "0,5", 10000))
    {
        Serial.println("[4G] Network not registered");
        _state = CELL_STATE_ERROR;
        return false;
    }

    // Check signal
    _sendATCommand("AT+CSQ", "OK", 3000);

    // Setup PDP context for data
    _sendATCommand("AT+CGATT=1", "OK", 10000);
    _sendATCommand("AT+CGACT=1,1", "OK", 10000);

    _state = CELL_STATE_READY;
    Serial.println("[4G] Module ready");
    return true;
}

void CellularManager::update()
{
    // Read any unsolicited messages from module
    while (_serial->available())
    {
        String line = _serial->readStringUntil('\n');
        line.trim();
        if (line.length() > 0)
        {
            Serial.println("[4G] URCMsg: " + line);

            // Check for MQTT disconnect notification
            if (line.indexOf("CMQTTDISC") >= 0 || line.indexOf("CMQTTCONNLOST") >= 0)
            {
                _mqttConnected = false;
            }
        }
    }
}

bool CellularManager::connectMQTT(const char *server, uint16_t port,
                                  const char *clientId,
                                  const char *username,
                                  const char *password)
{
    if (_state != CELL_STATE_READY && _state != CELL_STATE_CONNECTED)
        return false;

    Serial.println("[4G] Connecting MQTT via 4G...");

    // Start MQTT service
    _sendATCommand("AT+CMQTTSTART", "OK", 5000);
    delay(1000);

    // Acquire client
    String cmd = "AT+CMQTTACCQ=0,\"" + String(clientId) + "\"";
    _sendATCommand(cmd.c_str(), "OK", 5000);

    // Connect to broker
    cmd = "AT+CMQTTCONNECT=0,\"tcp://" + String(server) + ":" + String(port) + "\",60,1";
    if (username && strlen(username) > 0)
    {
        cmd += ",\"" + String(username) + "\",\"" + String(password ? password : "") + "\"";
    }

    if (_sendATCommand(cmd.c_str(), "OK", 15000))
    {
        _mqttConnected = true;
        _state = CELL_STATE_CONNECTED;
        Serial.println("[4G] MQTT connected via 4G");
        return true;
    }

    Serial.println("[4G] MQTT connection failed via 4G");
    return false;
}

bool CellularManager::publishMQTT(const char *topic, const char *payload)
{
    if (!_mqttConnected)
        return false;

    // Set topic
    String cmd = "AT+CMQTTTOPIC=0," + String(strlen(topic));
    _sendATCommand(cmd.c_str(), ">", 3000);
    _serial->print(topic);
    delay(500);

    // Set payload
    cmd = "AT+CMQTTPAYLOAD=0," + String(strlen(payload));
    _sendATCommand(cmd.c_str(), ">", 3000);
    _serial->print(payload);
    delay(500);

    // Publish
    bool result = _sendATCommand("AT+CMQTTPUB=0,1,60", "OK", 5000);
    if (result)
        Serial.println("[4G] MQTT published to " + String(topic));

    return result;
}

bool CellularManager::subscribeMQTT(const char *topic)
{
    if (!_mqttConnected)
        return false;

    String cmd = "AT+CMQTTSUB=0," + String(strlen(topic)) + ",1";
    _sendATCommand(cmd.c_str(), ">", 3000);
    _serial->print(topic);

    return _sendATCommand("", "OK", 5000);
}

void CellularManager::disconnectMQTT()
{
    _sendATCommand("AT+CMQTTDISC=0,120", "OK", 5000);
    _sendATCommand("AT+CMQTTREL=0", "OK", 3000);
    _sendATCommand("AT+CMQTTSTOP", "OK", 3000);
    _mqttConnected = false;
}

bool CellularManager::sendHTTPS(const char *url, const char *payload)
{
    // Simplified HTTPS POST
    _sendATCommand("AT+HTTPINIT", "OK", 3000);

    String cmd = "AT+HTTPPARA=\"URL\",\"" + String(url) + "\"";
    _sendATCommand(cmd.c_str(), "OK", 3000);

    _sendATCommand("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK", 3000);

    cmd = "AT+HTTPDATA=" + String(strlen(payload)) + ",10000";
    _sendATCommand(cmd.c_str(), "DOWNLOAD", 5000);
    _serial->print(payload);
    delay(1000);

    bool result = _sendATCommand("AT+HTTPACTION=1", "200", 15000);
    _sendATCommand("AT+HTTPTERM", "OK", 3000);

    return result;
}

CellularState CellularManager::getState()
{
    return _state;
}

bool CellularManager::isMQTTConnected()
{
    return _mqttConnected;
}

String CellularManager::getSignalStrength()
{
    _serial->println("AT+CSQ");
    String response = _readResponse(3000);

    int idx = response.indexOf("+CSQ:");
    if (idx >= 0)
    {
        return response.substring(idx + 5, response.indexOf(",", idx));
    }
    return "N/A";
}

// ==================== Private Methods ====================

void CellularManager::_powerOn()
{
    Serial.println("[4G] Powering on module...");
    digitalWrite(_pwrPin, HIGH);
    delay(1000);
    digitalWrite(_pwrPin, LOW);
    delay(5000);
}

bool CellularManager::_sendATCommand(const char *cmd, const char *expectedResponse,
                                     uint32_t timeoutMs)
{
    if (strlen(cmd) > 0)
    {
        _serial->println(cmd);
        Serial.println("[4G] >> " + String(cmd));
    }

    String response = _readResponse(timeoutMs);
    Serial.println("[4G] << " + response);

    return (response.indexOf(expectedResponse) >= 0);
}

String CellularManager::_readResponse(uint32_t timeoutMs)
{
    String response = "";
    uint32_t start = millis();

    while (millis() - start < timeoutMs)
    {
        while (_serial->available())
        {
            char c = _serial->read();
            response += c;
        }

        if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0 ||
            response.indexOf(">") >= 0)
        {
            break;
        }

        delay(10);
    }

    response.trim();
    return response;
}