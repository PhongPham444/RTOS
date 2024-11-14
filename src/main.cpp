#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
#include <LiquidCrystal_I2C.h>
#include <HCSR04.h>
#include <WiFi.h>
#include <WebServer.h>  
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"



#define WLAN_SSID "RD-SEAI_2.4G"
#define WLAN_PASS ""

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "phongpham"
#define AIO_KEY         ""

WiFiClient client;
WebServer server(80);
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
Adafruit_MQTT_Publish solid = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/solid");
Adafruit_MQTT_Publish humi = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humi");
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");
Adafruit_MQTT_Publish light = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/light");
Adafruit_MQTT_Publish pir = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/pir");
Adafruit_MQTT_Publish dis = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/distance");

// Define your tasks here
void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndRelay(void *pvParameters);
void TaskLightAndLED(void *pvParameters);
void TaskDetectMotion(void *pvParameters);
void TaskLCDDisplay(void *pvParameters);
void TaskMQTT(void *pvParameters);
void TaskServer(void *pvParameters);

//Define your components here
Adafruit_NeoPixel pixels3(4, 8, NEO_GRB + NEO_KHZ800);
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);
UltraSonicDistanceSensor distance(18, 21);

bool controlLed = false;
bool controlRelay = false;
bool controlFan = false;

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(10000);  // wait 10 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

void handleRoot() {
    String html = R"rawliteral(
      <html>
      <head>
          <title>Control Panel</title>
          <meta charset="UTF-8">
          <style>
              body {
                  display: flex;
                  justify-content: center;
                  align-items: center;
                  height: 100vh;
                  margin: 0;
                  font-family: Arial, sans-serif;
                  background-color: #f2f2f2;
              }
              .container {
                  text-align: center;
                  background-color: #ffffff;
                  padding: 20px;
                  border-radius: 10px;
                  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
              }
              .content {
                  display: flex;
                  flex-direction: row;
                  justify-content: space-between;
                  margin-top: 20px;
                  gap: 20px; /* Tạo khoảng cách giữa các cột */
              }
              .column {
                  width: 45%;
                  padding: 10px;
              }
              h1 {
                  color: #333;
                  margin-bottom: 0;
              }
              .button {
                  display: block;
                  width: 150px;
                  padding: 15px;
                  margin: 10px auto;
                  font-size: 16px;
                  color: #ffffff;
                  border: none;
                  border-radius: 5px;
                  cursor: pointer;
                  transition: background-color 0.3s;
              }
              .button.red { background-color: #F44336; }
              .button.blue { background-color: #2196F3; }
              .button.green { background-color: #4CAF50; }
              .button.off { background-color: #9E9E9E; }
              .button.relay { background-color: #FF9800; }
              .button:hover { opacity: 0.8; }
              .slider-container {
                  margin-top: 20px;
                  display: flex;
                  flex-direction: column;
                  align-items: center;
              }
              .slider {
                  width: 100%;
                  max-width: 300px;
              }
              .data-container {
                  display: flex;
                  flex-direction: column;
                  align-items: flex-start;
                  margin-top: 20px;
                  padding: 10px;
                  background-color: #f9f9f9;
                  border: 1px solid #ddd;
                  border-radius: 5px;
                  width: 100%;
              }
              .data-item {
                  display: flex;
                  justify-content: space-between;
                  width: 100%;
                  font-size: 18px;
                  padding: 5px 0;
              }
              .label {
                  text-align: left;
              }
              .value {
                  text-align: right;
              }
          </style>
          <script>
              function toggleDevice(device) {
                  fetch('/' + device)
                      .then(response => response.text())
                      .then(data => console.log(device + ' toggled', data))
                      .catch(error => console.error('Error:', error));
              }

              function setFanSpeed(value) {
                  fetch('/setFanSpeed?speed=' + value)
                      .then(response => response.text())
                      .then(data => {
                          console.log('Fan speed set to', value);
                          document.getElementById("fanSpeedValue").textContent = value;
                      })
                      .catch(error => console.error('Error:', error));
              }

              function updateSensorData() {
                  fetch('/update')
                      .then(response => response.json())
                      .then(data => {
                          document.getElementById("temperature").textContent = data.temperature + " °C";
                          document.getElementById("humidity").textContent = data.humidity + " %";
                          document.getElementById("soilMoisture").textContent = data.soilMoisture;
                          document.getElementById("lightIntensity").textContent = data.lightIntensity;
                          document.getElementById("pirValue").textContent = data.pirValue;
                          document.getElementById("distance").textContent = data.distance;
                      })
                      .catch(error => console.error('Error:', error));
              }

              setInterval(updateSensorData, 1000);
          </script>
      </head>
      <body>
          <div class="container">
              <h1>Control Panel</h1>
              <div class="content">
                  <div class="column">
                      <div class="data-container">
                          <div class="data-item"><span class="label">Temperature:</span> <span id="temperature" class="value">-- °C</span></div>
                          <div class="data-item"><span class="label">Humidity:</span> <span id="humidity" class="value">-- %</span></div>
                          <div class="data-item"><span class="label">Soil Moisture:</span> <span id="soilMoisture" class="value">--</span></div>
                          <div class="data-item"><span class="label">Light Intensity:</span> <span id="lightIntensity" class="value">--</span></div>
                          <div class="data-item"><span class="label">PIR Sensor:</span> <span id="pirValue" class="value">--</span></div>
                          <div class="data-item"><span class="label">Distance:</span> <span id="distance" class="value">--</span></div>
                      </div>
                      <div class="slider-container">
                          <label for="fanSpeed">Fan Speed:</label>
                          <input type="range" id="fanSpeed" class="slider" min="0" max="255" value="128" onchange="setFanSpeed(this.value)">
                          <span id="fanSpeedValue">128</span>
                      </div>
                  </div>
                  <div class="column">
                      <button class="button red" onclick="toggleDevice('redLED')">Red LED</button>
                      <button class="button blue" onclick="toggleDevice('blueLED')">Blue LED</button>
                      <button class="button green" onclick="toggleDevice('greenLED')">Green LED</button>
                      <button class="button off" onclick="toggleDevice('off')">Turn Off All LEDs</button>
                      <button class="button relay" onclick="toggleDevice('relay')">Toggle Relay</button>
                  </div>
              </div>
          </div>
      </body>
      </html>
  )rawliteral";


    server.send(200, "text/html", html);
}

void handleUpdate() {
    String json = "{";
    json += "\"temperature\":" + String(dht20.getTemperature()) + ",";
    json += "\"humidity\":" + String(dht20.getHumidity()) + ",";
    json += "\"soilMoisture\":" + String(analogRead(G1)) + ",";
    json += "\"lightIntensity\":" + String(analogRead(G2)) + ",";
    json += "\"pirValue\":" + String(analogRead(G3)) + ",";
    json += "\"distance\":" + String(distance.measureDistanceCm());
    json += "}";

    server.send(200, "application/json", json);
}

void handleToggleRedLED() {
    controlLed = true;
    pixels3.setPixelColor(0, pixels3.Color(255, 0, 0));
    pixels3.setPixelColor(1, pixels3.Color(255, 0, 0));
    pixels3.setPixelColor(2, pixels3.Color(255, 0, 0));
    pixels3.setPixelColor(3, pixels3.Color(255, 0, 0));
    pixels3.show();
    server.send(200, "text/plain", "Red LED turned on");
}
void handleToggleBlueLED() {
    controlLed = true;
    pixels3.setPixelColor(0, pixels3.Color(0, 0, 255));
    pixels3.setPixelColor(1, pixels3.Color(0, 0, 255));
    pixels3.setPixelColor(2, pixels3.Color(0, 0, 255));
    pixels3.setPixelColor(3, pixels3.Color(0, 0, 255));
    pixels3.show();
    server.send(200, "text/plain", "Blue LED turned on");
}
void handleToggleGreenLED() {
    controlLed = true;
    pixels3.setPixelColor(0, pixels3.Color(0, 255, 0));
    pixels3.setPixelColor(1, pixels3.Color(0, 255, 0));
    pixels3.setPixelColor(2, pixels3.Color(0, 255, 0));
    pixels3.setPixelColor(3, pixels3.Color(0, 255, 0));
    pixels3.show();
    server.send(200, "text/plain", "Green LED turned on");
}
void handleTurnOffLED() {
    controlLed = false;
    for (int i = 0; i < 4; i++) {
        pixels3.setPixelColor(i, pixels3.Color(0, 0, 0)); // Turn off all LEDs
    }
    pixels3.show();
    server.send(200, "text/plain", "All LEDs turned off");
}

void handleSetFanSpeed() {
    String speedStr = server.arg("speed");
    int speed = speedStr.toInt();
    analogWrite(GPIO_NUM_10, speed);
    server.send(200, "text/plain", "Fan speed set to " + String(speed));
    if(speed == 0){
        controlFan = false;
    }
    else controlFan = true;
}

void handleToggleRelay() {
    controlRelay = !controlRelay;
    digitalWrite(G6, !digitalRead(G6));
    server.send(200, "text/plain", "Relay toggled");
}

void setupServer() {
    server.on("/", handleRoot);
    server.on("/redLED", handleToggleRedLED);
    server.on("/blueLED", handleToggleBlueLED);
    server.on("/greenLED", handleToggleGreenLED);
    server.on("/off", handleTurnOffLED);
    server.on("/setFanSpeed", handleSetFanSpeed);
    server.on("/relay", handleToggleRelay);
    server.on("/update", handleUpdate);  // Endpoint to update data
    server.begin();
}

void setup() {
  Serial.begin(9600); 
  Wire.begin(GPIO_NUM_11,GPIO_NUM_12);

  dht20.begin();
  lcd.begin();
  pixels3.begin();


  WiFi.begin(WLAN_SSID, WLAN_PASS);
  delay(2000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  setupServer();
  xTaskCreate( TaskBlink, "Task Blink" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskTemperatureHumidity, "Task Temperature" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskSoilMoistureAndRelay, "Task Soild Relay" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskLightAndLED, "Task Light LED" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskDetectMotion, "Task Detect Motion" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskLCDDisplay, "Task LCD Display", 2048, NULL, 2, NULL);

  xTaskCreate(TaskMQTT, "Task MQTT", 4096, NULL, 1, NULL);
  xTaskCreate(TaskServer, "Task Server", 4096, NULL, 1, NULL);
  
  Serial.printf("Basic Multi Threading Arduino Example\n");
}

  /*--------------------------------------------------*/
  /*---------------------- Tasks ---------------------*/
  /*--------------------------------------------------*/

uint32_t x=0;
void TaskBlink(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  pinMode(GPIO_NUM_48, OUTPUT);

  while(1) {                          
    digitalWrite(GPIO_NUM_48, HIGH);  // turn the LED ON
    vTaskDelay(1000);
    digitalWrite(GPIO_NUM_48, LOW);  // turn the LED OFF
    vTaskDelay(1000);
  }
}

void TaskTemperatureHumidity(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  while(1) {                          
    Serial.println("Task Temperature and Humidity");

    dht20.read();
    Serial.println(dht20.getTemperature());
    Serial.println(dht20.getHumidity());
    vTaskDelay(5000);
  }
}

void TaskSoilMoistureAndRelay(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  pinMode(G6, OUTPUT);

  while(1) {                          
    Serial.println("Task Soild and Relay");
    Serial.println(analogRead(G1));
    if(!controlRelay){
        if(analogRead(G1) > 500){
        digitalWrite(G6, LOW);
        }
        if(analogRead(G1) < 50){
        digitalWrite(G6, HIGH);
        }
    }
    vTaskDelay(1000);
  }
}

void TaskDetectMotion(void *pvParameters) {
    //uint32_t blink_delay = *((uint32_t *)pvParameters);

    pinMode(GPIO_NUM_10, OUTPUT);

    while(1) {                          
        Serial.println("Task detect motion");
        int motion_value = analogRead(G3);
        Serial.println(motion_value);
        Serial.println("Task distance");
        int distance_cm = distance.measureDistanceCm();
        Serial.println(distance_cm);
        if(!controlFan){
            if (motion_value> 0){
            if (distance_cm < 20) {  // Close range
                analogWrite(GPIO_NUM_10, 128);  // Low speed (50% duty cycle)
            } 
            else if (distance_cm >= 20 && distance_cm < 50) {  // Medium range
                analogWrite(GPIO_NUM_10, 200);  // Medium speed (around 80% duty cycle)
            } 
            else {  // Far range
                analogWrite(GPIO_NUM_10, 255);  // High speed (100% duty cycle)
            }
            }
            else analogWrite(GPIO_NUM_10, 0);
        }
        vTaskDelay(1000);
    }
}

void TaskLightAndLED(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  while(1) {                          
    Serial.println("Task Light and LED");
    Serial.println(analogRead(G2));
    if(!controlLed){
        if(analogRead(G2) < 750){
        pixels3.setPixelColor(0, pixels3.Color(255,255,255));
        pixels3.setPixelColor(1, pixels3.Color(255,255,255));
        pixels3.setPixelColor(2, pixels3.Color(255,255,255));
        pixels3.setPixelColor(3, pixels3.Color(255,255,255));
        pixels3.show();
        }
        if(analogRead(G2) > 1500){
        pixels3.setPixelColor(0, pixels3.Color(0,0,0));
        pixels3.setPixelColor(1, pixels3.Color(0,0,0));
        pixels3.setPixelColor(2, pixels3.Color(0,0,0));
        pixels3.setPixelColor(3, pixels3.Color(0,0,0));
        pixels3.show();
        }
    }
    vTaskDelay(1000);
  }
}

void TaskLCDDisplay(void *pvParameters) {
    while(1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("T: ");
        lcd.print(dht20.getTemperature());
        lcd.print(" ");
        lcd.print("H: ");
        lcd.print(dht20.getHumidity());

        lcd.setCursor(0, 1);
        lcd.print("S: ");
        lcd.print(analogRead(G1));
        lcd.print(" ");
        lcd.print("D: ");
        lcd.print(distance.measureDistanceCm());
        vTaskDelay(1000);
    }
}

void TaskMQTT(void *pvParameters) {
    while (1) {
        MQTT_connect();
        mqtt.processPackets(10000);
        if (!mqtt.ping()) {
            mqtt.disconnect();
        }

        uint32_t soilMoisture = analogRead(G1);
        uint32_t lightLevel = analogRead(G2);
        uint32_t pirLevel = analogRead(G3);
        uint32_t distanceLevel = distance.measureDistanceCm();

        if (temp.publish(dht20.getTemperature())) {
            Serial.println(F("Published temperature!"));
        }
        if (humi.publish(dht20.getHumidity())) {
            Serial.println(F("Published humidity!"));
        }
        if (solid.publish(soilMoisture)) {
            Serial.println(F("Published soil moisture!"));
        }
        if (light.publish(lightLevel)) {
            Serial.println(F("Published light intensity!"));
        }
        if (pir.publish(pirLevel)) {
            Serial.println(F("Published pir value!"));
        }
        if (dis.publish(distanceLevel)) {
            Serial.println(F("Published distance !"));
        }

        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void TaskServer(void *pvParameters) {
    while (1) {
        server.handleClient();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void loop() {
    // Serial.println(WiFi.localIP());
    // delay(3000);
}