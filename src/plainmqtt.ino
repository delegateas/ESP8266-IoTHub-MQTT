#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "WEMOS_DHT12.h"
#include "config.h"
#include <MQTT.h>

#define SEND_INTERVAL 60000
#define WARN_INTERVAL 30000

WiFiClientSecure net;
MQTTClient client(1024);

unsigned long lastMillis = 0;
unsigned long lastMillisWarn = 0;

// This is an instance of the sensor reading library
DHT12 dht12;
bool let_state = false;
bool sendData = true;
bool sendWarns = true;
float temp_threshold = 34;
float humd_threshold = 35;

void sendSensorData();
void warnSensorData(char state[], float value);

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
  // client.unsubscribe("/hello");
}

void messageReceived(String &topic, String &payload) {
  // For now we only support receiving a "toggle" message from cloud-to-device
  StaticJsonDocument<256> doc;
  deserializeJson(doc, payload);
  if(doc["command"] == "toggle_light"){
    if(let_state == false){
      digitalWrite(LED_BUILTIN, HIGH);
      let_state = !let_state;
    }else{
      digitalWrite(LED_BUILTIN, LOW);
      let_state = !let_state;
    }
    Serial.println("got toggle");  
  }
  
  if(doc["command"] == "toggle_data"){
    sendData = !sendData;
  }

  if(doc["command"] == "toggle_warn"){
    sendWarns = !sendWarns;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(SSID, PASS);
  // Initialize the LED_BUILTIN pin as an output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // We connect on iothubs MQTT server on port 8883
  client.begin(IOTHOSTNAME, IOTPORT, net);
  // messageReceived is a callback on each message from our sub topic
  client.onMessage(messageReceived);

  connect();
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }
  
  if(dht12.get() == 0){
    // publish sensor readings roughly every minute.
    if (sendData && millis() - lastMillis > SEND_INTERVAL) {
      sendSensorData();
    }

    // Send warnings out if we're above temperature and humidity thresholds
    if(sendWarns && dht12.cTemp >= temp_threshold && millis() - lastMillisWarn > WARN_INTERVAL){
      warnSensorData("-1", dht12.cTemp);
    }else if(sendWarns && dht12.humidity >= humd_threshold && millis() - lastMillisWarn > WARN_INTERVAL){
      warnSensorData("-2", dht12.humidity);
    }
  }

}

void sendSensorData(){
  float t = dht12.cTemp;
  float h = dht12.humidity;
  DynamicJsonDocument doc(300);
  doc["Body"]["status"] = "0";
  doc["Body"]["temp"] = t;
  doc["Body"]["humd"] = h;
  String jsonStr;
  serializeJson(doc, Serial);
  Serial.println(" sent" + jsonStr);
  
  lastMillis = millis();
  client.publish(TOPICPUB, jsonStr);
}


void warnSensorData(char state[], float value){
    DynamicJsonDocument doc(300);
    doc["Body"]["status"] = state;
    doc["Body"]["temp"] = value;
    String jsonStr;
    serializeJson(doc, Serial);
    Serial.println(" sent warn" + jsonStr);
    
    lastMillisWarn = millis();
    client.publish(TOPICPUB, jsonStr);
}