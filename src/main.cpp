#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
#include <LiquidCrystal_I2C.h>
#include <HCSR04.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"



#define WLAN_SSID "Mua Hoa No"
#define WLAN_PASS "trongtachtraxinh"

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
  String message = "<html><body>";
  message += "<h1>Sensor Data</h1>";
  message += "<p>Temperature: " + String(dht20.getTemperature()) + " &deg;C</p>";
  message += "<p>Humidity: " + String(dht20.getHumidity()) + " %</p>";
  message += "<p>Soil Moisture: " + String(analogRead(G1)) + "</p>";
  message += "<p>Light Intensity: " + String(analogRead(G2)) + "</p>";
  message += "<p>PIR Value: " + String(analogRead(G3)) + "</p>";
  message += "<p>Distance: " + String(distance.measureDistanceCm()) + " cm</p>";
  
  message += "<button onclick=\"location.href='/update'\">Update</button>";
  
  message += "</body></html>";

  server.send(200, "text/html", message);
}
void handleUpdate() {
  String message = "<html><body>";
  message += "<h1>Sensor Data</h1>";
  message += "<p>Temperature: " + String(dht20.getTemperature()) + " &deg;C</p>";
  message += "<p>Humidity: " + String(dht20.getHumidity()) + " %</p>";
  message += "<p>Soil Moisture: " + String(analogRead(G1)) + "</p>";
  message += "<p>Light Intensity: " + String(analogRead(G2)) + "</p>";
  message += "<p>PIR Value: " + String(analogRead(G3)) + "</p>";
  message += "<p>Distance: " + String(distance.measureDistanceCm()) + " cm</p>";
  
  message += "<button onclick=\"location.href='/update'\">Update</button>";
  
  message += "</body></html>";

  server.send(200, "text/html", message);
}
void setupServer() {
  server.on("/", handleRoot);
  server.on("/update", handleUpdate);
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

  pinMode(GPIO_NUM_45, OUTPUT);

  while(1) {                          
    digitalWrite(GPIO_NUM_45, HIGH);  // turn the LED ON
    vTaskDelay(1000);
    digitalWrite(GPIO_NUM_45, LOW);  // turn the LED OFF
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
    
    if(analogRead(G1) > 500){
      digitalWrite(G6, LOW);
    }
    if(analogRead(G1) < 50){
      digitalWrite(G6, HIGH);
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

        // Control fan speed based on distance
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
        vTaskDelay(1000);
    }
}

void TaskLightAndLED(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  while(1) {                          
    Serial.println("Task Light and LED");
    Serial.println(analogRead(G2));
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
}