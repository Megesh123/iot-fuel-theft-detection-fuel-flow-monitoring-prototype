/**********************
 * BLUETOOTH COMMUNICATION HEADER
 * Defines BLE interface for device configuration and data exchange
 **********************/

#ifndef __BLUETOOTH_SCANNERH__
#define __BLUETOOTH_SCANNERH__

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

/**********************
 * BLUETOOTH CONFIGURATION
 **********************/
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Connection status enum
enum BluetoothStatus
{
    BLUETOOTH_DISCONNECTED,
    BLUETOOTH_CONNECTING,
    BLUETOOTH_CONNECTED,
    BLUETOOTH_ERROR
};

// External BLE objects
extern BLEServer *pServer;
extern BLECharacteristic *pCharacteristic;

// Forward declaration
class MyServerCallbacks;

// Bluetooth processing class
class BLUETOOTHPROCESS
{
public:
    void enable();
    void send(String _message);
    String read();
    void name(String deviceName);
    void connect();
    void disconnect();
    BluetoothStatus status();
    bool isMessageAvailable();

    friend class MyServerCallbacks;

private:
    void _setBluetoothName();

    String _customName = "";
    bool _isConnected = false;
    bool _isAdvertising = false;
};

extern BLUETOOTHPROCESS Bluetooth;

#endif