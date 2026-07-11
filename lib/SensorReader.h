#pragma once

#include <Arduino.h>
#include <PubSubClient.h>

class SensorReader {
 public:
  SensorReader(HardwareSerial& rs485Serial, PubSubClient& mqttClient);

  void begin();
  void readAndPublish();

 private:
  bool sendCommand(const uint8_t* command, size_t commandSize, uint8_t* response, size_t responseSize);
  void publishReadings(float windDirection, float windSpeed, float rainLevel);

  HardwareSerial& rs485Serial_;
  PubSubClient& mqttClient_;
};
