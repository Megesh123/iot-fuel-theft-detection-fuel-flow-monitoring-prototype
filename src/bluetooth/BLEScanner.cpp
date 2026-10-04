/**********************
 * BLUETOOTH COMMUNICATION IMPLEMENTATION
 **********************/

#include "bluetooth/BLEScanner.h"

BLUETOOTHPROCESS Bluetooth;

BLEServer *pServer = nullptr;
BLECharacteristic *pCharacteristic = nullptr;
String _clientid;
String _incomingData;
String __incomingDataAfterFilter;
bool _messageReceive = false;

/**********************
 * BLE SERVER CALLBACKS
 **********************/
class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param) override
    {
        Bluetooth._isConnected = true;
        Serial.println("[BLE] Client connected");
        pCharacteristic->setValue(_clientid.c_str());
        pCharacteristic->notify();
    }

    void onDisconnect(BLEServer *pServer) override
    {
        Bluetooth._isConnected = false;
        Serial.println("[BLE] Client disconnected");
        BLEAdvertising *pAdvertising = pServer->getAdvertising();
        pAdvertising->start();
        Bluetooth._isAdvertising = true;
    }
};

/**********************
 * BLE CHARACTERISTIC CALLBACKS
 **********************/
class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic) override
    {
        String value = String(pCharacteristic->getValue().c_str());
        _incomingData += value;

        if (_incomingData.startsWith("*") && _incomingData.endsWith("#"))
        {
            _incomingData = _incomingData.substring(1, _incomingData.length() - 1);
            _incomingData.trim();
            __incomingDataAfterFilter = _incomingData;
            _messageReceive = true;
            _incomingData = "";
        }
    }
};

void BLUETOOTHPROCESS::enable()
{
    _setBluetoothName();
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();
    connect();
}

void BLUETOOTHPROCESS::send(String _message)
{
    if (_isConnected)
    {
        pCharacteristic->setValue(_message.c_str());
        pCharacteristic->notify();
    }
}

String BLUETOOTHPROCESS::read()
{
    if (_messageReceive)
    {
        String msg = __incomingDataAfterFilter;
        __incomingDataAfterFilter = "";
        _messageReceive = false;
        return msg;
    }
    return "";
}

bool BLUETOOTHPROCESS::isMessageAvailable()
{
    return _messageReceive;
}

void BLUETOOTHPROCESS::name(String deviceName)
{
    _customName = deviceName;
}

void BLUETOOTHPROCESS::connect()
{
    if (pServer && !_isAdvertising)
    {
        BLEAdvertising *pAdvertising = pServer->getAdvertising();
        pAdvertising->start();
        _isAdvertising = true;
        _isConnected = false;
        Serial.println("[BLE] Advertising started");
    }
}

void BLUETOOTHPROCESS::disconnect()
{
    if (pServer)
    {
        BLEAdvertising *pAdvertising = pServer->getAdvertising();
        pAdvertising->stop();
        _isAdvertising = false;
        _isConnected = false;
        Serial.println("[BLE] Advertising stopped");
    }
}

BluetoothStatus BLUETOOTHPROCESS::status()
{
    if (_isConnected)
        return BLUETOOTH_CONNECTED;
    if (_isAdvertising)
        return BLUETOOTH_CONNECTING;
    return BLUETOOTH_DISCONNECTED;
}

void BLUETOOTHPROCESS::_setBluetoothName()
{
    uint64_t chipId = ESP.getEfuseMac();
    String uniqueId = String((uint32_t)(chipId >> 32), HEX);
    String chipIdStr = String((uint32_t)chipId, HEX);
    uniqueId.toUpperCase();
    chipIdStr.toUpperCase();
    _clientid = uniqueId + chipIdStr;

    String bluetoothName;
    if (_customName != "")
        bluetoothName = _customName;
    else
        bluetoothName = "FTMS_" + String(_clientid);

    BLEDevice::init(bluetoothName.c_str());
    BLEDevice::setMTU(512);
}