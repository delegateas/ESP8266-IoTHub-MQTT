#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "WEMOS_DHT12.h"
#include "config.h"
#include <MQTT.h>

#define SEND_INTERVAL 30000
#define WARN_INTERVAL 30000
#define SEND_DATA 1

WiFiClientSecure net;
MQTTClient client(1024);

unsigned long lastMillis = 0;
unsigned long lastMillisWarn = 0;

// This is an instance of the sensor reading library
DHT12 dht12;
bool led_state = true;
bool sendData = true;
bool sendWarns = true;
float temp_threshold = 50;
float humd_threshold = 60;

float humd_threshold_increase = 0;

void sendSensorData();
void warnSensorData(char state[], float value);
void blink(int times, int delayBetween);

// Connect to wifi and then connect to MQTT
void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  net.setInsecure();

  Serial.print("\nconnecting...");
  while (!client.connect(CLIENTID,
                         USERNAME,
                         SAS)) {
    Serial.print("Retrying...\n");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe(TOPICSUB);
  dht12.get();
}

void messageReceived(String &topic, String &payload) {
  StaticJsonDocument<256> doc;
  // Deserialize JSON from MQTT
  deserializeJson(doc, payload);
  // We check "command" value of json
  if(doc["command"] == "toggle_light"){
    if(led_state == true){
      digitalWrite(LED_BUILTIN, HIGH);
      led_state = !led_state;
    }else{
      digitalWrite(LED_BUILTIN, LOW);
      led_state = !led_state;
    }
    Serial.println("got toggle LED");  
  }

  if(doc["command"] == "led_on"){
    led_state = true;
    digitalWrite(LED_BUILTIN, LOW);
  }

  if(doc["command"] == "led_off"){
    led_state = false;
    digitalWrite(LED_BUILTIN, HIGH);
  }
  
  if(doc["command"] == "toggle_data"){
    sendData = !sendData;
  }

  if(doc["command"] == "toggle_warn"){
    sendWarns = !sendWarns;
  }

  if(doc["command"] == "simulate_water_leakage"){
    humd_threshold_increase = 50;
  }

  if(doc["command"] == "stop_water_leakage"){
    humd_threshold_increase = 0;
  }

  if(doc["command"] == "send_data"){
    sendSensorData();
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(SSID, PASS);
  // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
  // The LED states for the builtin LED are switched LOW = on, HIGH = off
  led_state = true;
  digitalWrite(LED_BUILTIN, HIGH);
  
  // We connect on iothubs MQTT server on port 8883
  client.begin(IOTHOSTNAME, IOTPORT, net);
  // messageReceived is a callback on each message from our sub topic
  client.onMessage(messageReceived);
  connect();
  blink(1, 400);
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }
  
  if(dht12.get() == 0){
    // publish sensor readings roughly every minute if enabled with SEND_DATA define
    if (SEND_DATA && sendData && millis() - lastMillis > SEND_INTERVAL) {
      sendSensorData();
      blink(2, 400);
    }
    // Send warnings out if we're above temperature and humidity thresholds (Humidity can be simulated)
    if(sendWarns && dht12.cTemp >= temp_threshold && millis() - lastMillisWarn > WARN_INTERVAL){
      blink(3, 400);      
      sendSensorData();
      lastMillisWarn = millis();
    }
    
    if(sendWarns && (dht12.humidity + humd_threshold_increase) >= humd_threshold && millis() - lastMillisWarn > WARN_INTERVAL){
      // Do 3 blinks before sending data
      blink(3, 400);
      sendSensorData();
      lastMillisWarn = millis();
    }
  }
}

void sendSensorData(){
  float t = dht12.cTemp;
  float h = dht12.humidity + humd_threshold_increase;
  StaticJsonDocument<256> doc;
  doc["Temperature"] = t;
  doc["Humidity"] = h;
  String jsonStr;
  serializeJson(doc, jsonStr);
  Serial.println("sent " + jsonStr);
  
  lastMillis = millis();
  client.publish(TOPICPUB, jsonStr);
}


void warnSensorData(char state[], float value){
    StaticJsonDocument<256> doc;
    doc["status"] = state;
    doc["reading"] = value;
    String jsonStr;
    serializeJson(doc, jsonStr);
    Serial.println("sent warn " + jsonStr);
    
    lastMillisWarn = millis();
    client.publish(TOPICPUB, jsonStr);
}

void blink(int times, int delayBetween){
  for(int i = 0; i < times; i++)
  {
    digitalWrite(LED_BUILTIN, !led_state ? LOW : HIGH);
    delay(delayBetween);
    digitalWrite(LED_BUILTIN, led_state ? LOW : HIGH);
    delay(delayBetween);
  }
  delay(delayBetween);
  digitalWrite(LED_BUILTIN, led_state ? LOW : HIGH);
}