#include "SensorReader.h"

#include <ArduinoJson.h>

#include "Config.h"

namespace {
constexpr size_t kResponseSize = 7;
constexpr size_t kSensorCount = 3;

const uint8_t kSensorCommands[kSensorCount][8] = {
    {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0xD5, 0xCA},
    {0x02, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x39},
    {0x41, 0x03, 0x00, 0x00, 0x00, 0x01, 0x8A, 0xCA},
};
}  // namespace

SensorReader::SensorReader(HardwareSerial& rs485Serial, PubSubClient& mqttClient)
    : rs485Serial_(rs485Serial), mqttClient_(mqttClient) {}

void SensorReader::begin() {
  rs485Serial_.begin(9600, SERIAL_8N1, Config::Rs485ReceivePin, Config::Rs485TransmitPin);
}

void SensorReader::readAndPublish() {
  float windSpeed = 0;
  float windDirection = 0;
  float rainLevel = 0;

  uint8_t response[kResponseSize];
  for (size_t sensorIndex = 0; sensorIndex < kSensorCount; ++sensorIndex) {
    memset(response, 0, sizeof(response));
    const bool receivedResponse = sendCommand(
        kSensorCommands[sensorIndex], sizeof(kSensorCommands[sensorIndex]), response, sizeof(response));

    if (!receivedResponse || response[1] != 0x03) {
      Serial.printf("Sensor read failed for sensor %u\n", static_cast<unsigned>(sensorIndex));
    } else {
      const float value = ((response[3] << 8) | response[4]) / 10.0F;
      switch (sensorIndex) {
        case 0:
          windDirection = value;
          break;
        case 1:
          windSpeed = value;
          break;
        case 2:
          rainLevel = value;
          break;
      }
    }

    delay(Config::Rs485ResponseDelayMs);
  }

  Serial.printf("Wind direction: %.1f degrees\n", windDirection);
  Serial.printf("Wind speed: %.1f m/s\n", windSpeed);
  Serial.printf("Rainfall: %.1f mm\n", rainLevel);
  publishReadings(windDirection, windSpeed, rainLevel);
}

bool SensorReader::sendCommand(const uint8_t* command, size_t commandSize, uint8_t* response, size_t responseSize) {
  rs485Serial_.write(command, commandSize);
  rs485Serial_.flush();
  delay(Config::Rs485ResponseDelayMs);

  if (rs485Serial_.available() < responseSize) {
    Serial.println("RS485 response did not contain enough data");
    return false;
  }

  rs485Serial_.readBytes(response, responseSize);
  return true;
}

void SensorReader::publishReadings(float windDirection, float windSpeed, float rainLevel) {
  if (!mqttClient_.connected()) {
    return;
  }

  JsonDocument document;
  document["WinDirection"] = windDirection;
  document["WinSpeed"] = windSpeed;
  document["Rain"] = rainLevel;

  String payload;
  serializeJson(document, payload);
  mqttClient_.publish("v1/devices/me/telemetry", payload.c_str());
  mqttClient_.publish("v1/devices/me/attributes", payload.c_str());
}
