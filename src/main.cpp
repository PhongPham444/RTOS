#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "Config.h"
#include "NetworkManager.h"
#include "SensorReader.h"

HardwareSerial rs485Serial(1);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
NetworkManager networkManager(mqttClient);
SensorReader sensorReader(rs485Serial, mqttClient);

void sensorTask(void*) {
  while (true) {
    sensorReader.readAndPublish();
    vTaskDelay(pdMS_TO_TICKS(Config::SensorReadIntervalMs));
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  sensorReader.begin();
  networkManager.connect();
  xTaskCreate(sensorTask, "SensorTask", 4096, nullptr, 1, nullptr);
}

void loop() {
  networkManager.update();
  yield();
}
