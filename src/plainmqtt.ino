#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include "WEMOS_DHT12.h"
#include "config.h"
#include <MQTT.h>

WiFiClientSecure net;
MQTTClient client(1024);

unsigned long lastMillis = 0;

// This is an instance of the sensor reading library
DHT12 dht12;
bool state = false;

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
  if(payload = "toggle"){
    if(state == false){
      digitalWrite(LED_BUILTIN, HIGH);
      state = !state;
    }else{
      digitalWrite(LED_BUILTIN, LOW);
      state = !state;
    }
    Serial.println("got toggle");  
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

  // publish sensor readings roughly every minute.
  if (millis() - lastMillis > 60000) {
    if(dht12.get()==0){
      float h = dht12.cTemp;
      float t = dht12.humidity;
      DynamicJsonDocument doc(300);
      doc["Array"]["temp"] = t;
      doc["Array"]["humd"] = h;
      String jsonStr;
      serializeJson(doc, Serial);
      Serial.println("sent: " + jsonStr);
      
      lastMillis = millis();
      client.publish(TOPICPUB, jsonStr);
    }
  }
}
