# ESP8266 IoTHub through MQTT

Interested in connecting an ESP8266 to your Azure IoT Hub? This repo does exactly that. We send periodic data to the IoT Hub MQTT server containing temperature and humidity.

Our setup:
 - WEMOS D1 mini Pro
 - WEMOS DHT Shield V2.0.0
 - Access to WiFi

We simply connect the Shield and edit `src/config.h` to match your settings. Some libraries generate SAS tokens on the embedded system, however, we opted for generating them in advance.

Minimum settings to change in `src/config.h`
 - `SSID`
 - `PASS`
 - `CLIENTID`
 - `IOTNAME`
