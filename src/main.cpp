#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ========== CẤU HÌNH ==========
const char* WIFI_SSID       = "ACLAB";
const char* WIFI_PASS       = "ACLAB2023";
const char* MQTT_SERVER     = "app.coreiot.io";
const uint16_t MQTT_PORT    = 1883;
const char* MQTT_CLIENT_ID  = "";
const char* MQTT_USER       = "";
const char* MQTT_PASS       = nullptr;

// RS485
#define TXD_RS485 GPIO_NUM_7
#define RXD_RS485 GPIO_NUM_6
const uint32_t DELAY_CONNECT = 200;    // ms giữa lệnh RS485
const uint32_t DELAY_TIME    = 1000;   // ms giữa chu kỳ đo

// ĐỐI TƯỢNG
HardwareSerial RS485Serial(1);
WiFiClient  wifiClient;
PubSubClient tb(wifiClient);

// ===== GỬI LỆNH RS485 =====
void sendRS485Command(const uint8_t *cmd, size_t cmdSize, uint8_t *resp, size_t respSize) {
  RS485Serial.write(cmd, cmdSize);
  RS485Serial.flush();
  delay(DELAY_CONNECT);

  if (RS485Serial.available() >= respSize) {
    RS485Serial.readBytes(resp, respSize);
    // Serial.print("[RS485] Phản hồi: ");
    // for (size_t i = 0; i < respSize; ++i) {
    //   Serial.printf("%02X ", resp[i]);
    // }
    // Serial.println();
  } else {
    Serial.println("[RS485] Không đủ dữ liệu trả về");
  }
}

// ===== ĐỌC CẢM BIẾN =====
void readSensors() {
  float windSpeed = 0;
  float windDir   = 0;
  float rainLevel = 0;

  uint8_t resp[7];

  const uint8_t cmds[][8] = {
    {0x01, 0x03, 0x00, 0x01, 0x00, 0x01, 0xD5, 0xCA}, // Wind Direction
    {0x02, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x39}, // Wind Speed
    {0x41, 0x03, 0x00, 0x00, 0x00, 0x01, 0x8A, 0xCA}  // Rain
  };

  for (size_t i = 0; i < 3; ++i) {
    memset(resp, 0, sizeof(resp));
    sendRS485Command(cmds[i], sizeof(cmds[i]), resp, sizeof(resp));

    if (resp[1] == 0x03) {
      float value = ((resp[3] << 8) | resp[4]) / 10.0;

      switch (i) {
        case 0: windDir = value; break;
        case 1: windSpeed = value; break;
        case 2: rainLevel = value; break;
      }
    } else {
      Serial.printf("[Sensor] Lỗi ở sensor %d\n", i);
    }

    delay(DELAY_CONNECT);
  }

  // ===== IN SERIAL =====
  Serial.printf("Wind Dir: %.1f °\n", windDir);
  Serial.printf("Wind Spd: %.1f m/s\n", windSpeed);
  Serial.printf("Rain: %.1f mm\n", rainLevel);

  // ===== GỬI MQTT =====
  if (tb.connected()) {
    DynamicJsonDocument doc(256);
    doc["WinDirection"] = windDir;
    doc["WinSpeed"]     = windSpeed;
    doc["Rain"]         = rainLevel;

    String payload;
    serializeJson(doc, payload);
    tb.publish("v1/devices/me/telemetry", payload.c_str());
    tb.publish("v1/devices/me/attributes", payload.c_str()); // Optional
  }
}

// ===== FREE RTOS TASK =====
void TaskSensor(void *pvParameters) {
  while (true) {
    readSensors();
    vTaskDelay(pdMS_TO_TICKS(DELAY_TIME));
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(500);

  RS485Serial.begin(9600, SERIAL_8N1, RXD_RS485, TXD_RS485);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());

  tb.setServer(MQTT_SERVER, MQTT_PORT);
  while (!tb.connected()) {
    Serial.print("Kết nối MQTT...");
    if (tb.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("Thành công");
    } else {
      Serial.printf("Lỗi %d\n", tb.state());
      delay(2000);
    }
  }

  xTaskCreate(TaskSensor, "TaskSensor", 4096, nullptr, 1, nullptr);
}

// ===== LOOP =====
void loop() {
  tb.loop();  // Duy trì MQTT
  yield();    // Tránh WDT
}
