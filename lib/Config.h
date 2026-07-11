#pragma once

#include <Arduino.h>

namespace Config {
constexpr char WifiSsid[] = "ACLAB";
constexpr char WifiPassword[] = "ACLAB2023";
constexpr char MqttServer[] = "app.coreiot.io";
constexpr uint16_t MqttPort = 1883;
constexpr char MqttClientId[] = "";
constexpr char MqttUser[] = "";
constexpr char MqttPassword[] = "";

constexpr int Rs485TransmitPin = GPIO_NUM_7;
constexpr int Rs485ReceivePin = GPIO_NUM_6;
constexpr uint32_t Rs485ResponseDelayMs = 200;
constexpr uint32_t SensorReadIntervalMs = 1000;
}  // namespace Config
