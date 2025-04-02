#include <Arduino.h>
#include <RadioLib.h>  // RadioLib by Jan Gromes v7.1.2
#include <ArduinoJson.h>

// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON  // must be before "#include <heltec_unofficial.h>"

// Uncomment this if you have Wireless Stick v3
// #define HELTEC_WIRELESS_STICK

// creates 'radio', 'display' and 'button' instances
#include <heltec_unofficial.h>  // Heltec_ESP32_LoRa_v3 by Rop Gonggrijp v0.9.2

SX1262 controller(radio);

void setup() {
  heltec_setup();
  Serial.begin(9600);

  display.println("Receiver Init");
  delay(2000);

  // Initialize Radio
  display.print("Radio ");
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    display.println("OK");
  } else {
    display.printf("fail, code: %i\n", state);
    while (true) { delay(10); }  // Halt if initialization fails
  }

  controller.begin();
  controller.setFrequency(915.0);
}

void loop() {
  heltec_loop();
  Serial.println("[SX1262] Waiting for incoming transmission...");

  String receivedData;
  int state = controller.receive(receivedData);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Received Data: " + receivedData);
    display.clear();
    display.println("Received Data:");
    display.println(receivedData);
    display.display();

    // Parse JSON data
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, receivedData);
    if (!error) {
      float tempC = doc["Temp"]["DegreesC"];
      float tempF = doc["Temp"]["DegreesF"];
      float humidity = doc["Humidity"];

      Serial.printf("Temp: %.1fC (%.1fF), Humidity: %.1f%%\n", tempC, tempF, humidity);
      display.clear();
      display.printf("Temp: %.1fC\nHum: %.1f%%\n", tempC, humidity);
      display.display();
    } else {
      Serial.println("JSON Parse Failed!");
    }
  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.println("Receive Timeout");
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    Serial.println("CRC Error");
  } else {
    Serial.printf("Receive Failed, Code: %i\n", state);
  }
}
