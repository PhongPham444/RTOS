#pragma once

#include <PubSubClient.h>

class NetworkManager {
 public:
  explicit NetworkManager(PubSubClient& mqttClient);

  void connect();
  void update();

 private:
  PubSubClient& mqttClient_;
};
