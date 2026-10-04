# IoT Fuel Theft Detection & Fuel Flow Monitoring Prototype

## Overview

This repository contains a preliminary ESP32-S3 based demonstration project prepared to demonstrate a possible technical approach for an IoT-based fuel-flow monitoring and fuel-level anomaly detection system.

The project demonstrates embedded firmware architecture, sensor integration, local user interface, device provisioning, persistent configuration, wireless communication and 4G connectivity concepts.

## Demonstration Features

- ESP32-S3 based embedded controller
- Fuel-flow sensor monitoring
- Fuel-level sensor monitoring
- Fuel-flow and fuel-level processing
- Preliminary abnormal-flow / fuel-level anomaly detection
- Ignition / vehicle status input concept
- Solenoid valve control concept
- OLED display
- 5-button local interface
- RTC
- EEPROM configuration storage
- BLE device provisioning
- Wi-Fi connectivity
- MQTT communication
- 4G communication / fallback concept
- Power-fail detection and data-save concept

## Device Provisioning Flow

A new device can enter provisioning mode by holding the MENU button during power-up.

```text
Power ON
   |
   v
MENU button detected?
   |
   +---- YES ----> BLE Provisioning
   |                   |
   |                   v
   |             Receive Configuration
   |                   |
   |                   v
   |              Validate Data
   |                   |
   |                   v
   |             Store in EEPROM
   |                   |
   |                   v
   |                Restart
   |
   +---- NO -----> Normal Startup
                       |
                       v
                 Load Configuration
                       |
                       v
                  Wi-Fi / 4G
                       |
                       v
                      MQTT
