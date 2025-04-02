// Board: Heltec WiFi LoRa 32 (V3)

#include <Arduino.h>
#include <RadioLib.h>  // RadioLib by Jan Gromes v7.1.2
#include <DHTesp.h>
#include <ArduinoJson.h>

// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON  // must be before "#include <heltec_unofficial.h>"

// Uncomment this if you have Wireless Stick v3
// #define HELTEC_WIRELESS_STICK

// creates 'radio', 'display' and 'button' instances
#include <heltec_unofficial.h>  // Heltec_ESP32_LoRa_v3 by Rop Gonggrijp v0.9.2

#define DHTPIN 6  // GPIO48 for DHT sensor
DHTesp dht;

#define COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

SX1262 controller(radio);
uint8_t data[32] = { 0 };

void setup() {
  heltec_setup();
  dht.setup(DHTPIN, DHTesp::DHT22);  // Initialize DHT sensor

  for (size_t i = 0; i < 3; i++) {
    display.print(".");
    delay(1000);
  }
  display.println();

  display.println("Zack Says Hi!!");
  delay(2000);

  // Radio
  display.print("Radio ");
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    display.println("OK");
  } else {
    display.printf("fail, code: %i\n", state);
  }

  // Battery
  float vbat = heltec_vbat();
  display.printf("Vbat: %.2fV (%d%%)\n", vbat, heltec_battery_percent(vbat));

  controller.begin();
  controller.setFrequency(915.0);
  controller.setOutputPower(22);
}

float celsiusToFahrenheit(float celsius) {
    return (celsius * 9.0 / 5.0) + 32.0;
}

void loop() {
  heltec_loop();

  display.clear();
  display.printf("Temp: %.1fC\nHum: %.1f%%\n", dht.getTemperature(), dht.getHumidity());
  display.display();

  // Create a DynamicJsonDocument with a capacity of 1024 bytes
  DynamicJsonDocument payload(1024);

  // Create JSON data
  JsonObject temp = payload.createNestedObject("Temp");
  float degreesC = dht.getTemperature();
  temp["DegreesC"] = degreesC;
  temp["DegreesF"] = celsiusToFahrenheit(degreesC);
  payload["Humidity"] = dht.getHumidity();

  String data;
  serializeJson(payload, data);

  controller.transmit(data, data.length());
  delay(5000);  // Delay before next read
}
