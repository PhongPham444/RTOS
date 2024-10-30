#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <DHT20.h>
#include <LiquidCrystal_I2C.h>;

#include <WiFi.h>

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"



#define WLAN_SSID "RD-SEAI_2.4G"
#define WLAN_PASS ""

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    ""
#define AIO_KEY         ""

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);
//Adafruit_MQTT_Client mqtt(&client, OHS_SERVER, OHS_SERVERPORT, OHS_USERNAME, OHS_USERNAME, OHS_KEY);
//Adafruit_MQTT_Client mqtt(&client, BKTK_SERVER, BKTK_SERVERPORT, BKTK_USERNAME, BKTK_USERNAME, BKTK_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
Adafruit_MQTT_Publish solid = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/solid");
Adafruit_MQTT_Publish humi = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humi");
Adafruit_MQTT_Publish temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temp");
Adafruit_MQTT_Publish light = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/light");

// Define your tasks here
void TaskBlink(void *pvParameters);
void TaskTemperatureHumidity(void *pvParameters);
void TaskSoilMoistureAndRelay(void *pvParameters);
void TaskLightAndLED(void *pvParameters);

//Define your components here
Adafruit_NeoPixel pixels3(4, G8, NEO_GRB + NEO_KHZ800);
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);

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

void setup() {

  // Initialize serial communication at 115200 bits per second:
  Serial.begin(115200); 


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

  // slider.setCallback(slidercallback);
  // onoffbutton.setCallback(onoffcallback);
  // mqtt.subscribe(&slider);
  // mqtt.subscribe(&onoffbutton);

  
  xTaskCreate( TaskBlink, "Task Blink" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskTemperatureHumidity, "Task Temperature" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskSoilMoistureAndRelay, "Task Soild Relay" ,2048  ,NULL  ,2 , NULL);
  xTaskCreate( TaskLightAndLED, "Task Light LED" ,2048  ,NULL  ,2 , NULL);
  
  
  //Now the task scheduler is automatically started.
  Serial.printf("Basic Multi Threading Arduino Example\n");
  
  
  
}
// int pubCount = 0;
// void loop() {
//   MQTT_connect();
//   mqtt.processPackets(10000);
//   if(! mqtt.ping()) {
//     mqtt.disconnect();
//   }
// }



/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


uint32_t x=0;
void TaskBlink(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  // initialize digital LED_BUILTIN on pin 13 as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  

  while(1) {                          
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED ON
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED OFF
    delay(1000);
    // if (sensory.publish(x++)) {
    //   Serial.println(F("Published successfully!!"));
    // }
  }
}


void TaskTemperatureHumidity(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  while(1) {                          
    Serial.println("Task Temperature and Humidity");
    dht20.read();
    Serial.println(dht20.getTemperature());
    Serial.println(dht20.getHumidity());
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(dht20.getTemperature());
    lcd.setCursor(0, 1);
    lcd.print(dht20.getHumidity());

    delay(5000);
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
    delay(1000);
  }
}


void TaskLightAndLED(void *pvParameters) {  // This is a task.
  //uint32_t blink_delay = *((uint32_t *)pvParameters);

  while(1) {                          
    Serial.println("Task Light and LED");
    Serial.println(analogRead(G2));
    if(analogRead(G2) < 350){
      pixels3.setPixelColor(0, pixels3.Color(255,255,255));
      pixels3.setPixelColor(1, pixels3.Color(255,255,255));
      pixels3.setPixelColor(2, pixels3.Color(255,255,255));
      pixels3.setPixelColor(3, pixels3.Color(255,255,255));
      pixels3.show();
    }
    if(analogRead(G2) > 550){
      pixels3.setPixelColor(0, pixels3.Color(0,0,0));
      pixels3.setPixelColor(1, pixels3.Color(0,0,0));
      pixels3.setPixelColor(2, pixels3.Color(0,0,0));
      pixels3.setPixelColor(3, pixels3.Color(0,0,0));
      pixels3.show();
    }
    delay(1000);
  }
}
void loop() {
    MQTT_connect();  // Kết nối đến MQTT
    mqtt.processPackets(10000);  // Xử lý dữ liệu từ server MQTT
    if(!mqtt.ping()) {  // Kiểm tra kết nối
        mqtt.disconnect();
    }

    // Chuyển đổi kết quả analogRead() sang kiểu uint32_t
    uint32_t soilMoisture = analogRead(G1);
    uint32_t lightLevel = analogRead(G2);

    // Publish các dữ liệu từ cảm biến
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

    delay(5000); // Đợi một chút trước khi gửi dữ liệu tiếp theo
}

