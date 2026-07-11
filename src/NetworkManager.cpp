#include "NetworkManager.h"

#include <WiFi.h>

#include "Config.h"

NetworkManager::NetworkManager(PubSubClient& mqttClient) : mqttClient_(mqttClient) {}

void NetworkManager::connect() {
  WiFi.begin(Config::WifiSsid, Config::WifiPassword);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

  mqttClient_.setServer(Config::MqttServer, Config::MqttPort);
  while (!mqttClient_.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient_.connect(Config::MqttClientId, Config::MqttUser, Config::MqttPassword)) {
      Serial.println("connected");
    } else {
      Serial.printf("MQTT connection failed: %d\n", mqttClient_.state());
      delay(2000);
    }
  }
}

void NetworkManager::update() {
  mqttClient_.loop();
}
