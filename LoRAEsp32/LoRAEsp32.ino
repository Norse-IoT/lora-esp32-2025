/**
 * Send and receive LoRa-modulation packets with a sequence number, showing RSSI
 * and SNR for received packets on the little display.
 *
 * Note that while this send and received using LoRa modulation, it does not do
 * LoRaWAN. For that, see the LoRaWAN_TTN example.
 *
 * This works on the stick, but the output on the screen gets cut off.
*/

#include <ArduinoJson.h>
#include <DHTesp.h>

#define DHTPIN 6  // GPIO48 for DHT sensor
DHTesp dht;

// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON  // must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>

// Pause between transmited packets in seconds.
// Set to zero to only transmit a packet when pressing the user button
// Will not exceed 1% duty cycle, even if you set a lower value.
#define PAUSE 5

// Frequency in MHz. Keep the decimal point to designate float.
// Check your own rules and regulations to see what is legal where you are.
#define FREQUENCY 866.3  // for Europe
// #define FREQUENCY           905.2       // for US

// LoRa bandwidth. Keep the decimal point to designate float.
// Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
#define BANDWIDTH 250.0

// Number from 5 to 12. Higher means slower but higher "processor gain",
// meaning (in nutshell) longer range and more robust against interference.
#define SPREADING_FACTOR 9

// Transmit power in dBm. 0 dBm = 1 mW, enough for tabletop-testing. This value can be
// set anywhere between -9 dBm (0.125 mW) to 22 dBm (158 mW). Note that the maximum ERP
// (which is what your antenna maximally radiates) on the EU ISM band is 25 mW, and that
// transmissting without an antenna can damage your hardware.
#define TRANSMIT_POWER 12

String rxdata;
volatile bool rxFlag = false;
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
uint64_t minimum_pause;

void setup() {
  heltec_setup();
  dht.setup(DHTPIN, DHTesp::DHT22);  // Initialize DHT sensor

  both.println("Radio init");
  RADIOLIB_OR_HALT(radio.begin());
  // Set the callback function for received packets
  radio.setDio1Action(rx);
  // Set radio parameters
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  RADIOLIB_OR_HALT(radio.setSpreadingFactor(SPREADING_FACTOR));
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  RADIOLIB_OR_HALT(radio.setOutputPower(TRANSMIT_POWER));
  // Start receiving
  RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
}

float celsiusToFahrenheit(float celsius) {
  return (celsius * 9.0 / 5.0) + 32.0;
}

void loop() {
  heltec_loop();

  bool tx_legal = millis() > last_tx + minimum_pause;
  // Transmit a packet every PAUSE seconds or when the button is pressed
  if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 1000)) || button.isSingleClick()) {
    // In case of button click, tell user to wait
    if (!tx_legal) {
      both.printf("Legal limit, wait %i sec.\n", (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
      return;
    }
    both.printf("TX [%s] ", String(counter).c_str());
    radio.clearDio1Action();
    heltec_led(50);  // 50% brightness is plenty for this LED
    tx_time = millis();
    String data = getStringData();
    both.println(data);
    RADIOLIB(radio.transmit(data.c_str()));
    tx_time = millis() - tx_time;
    heltec_led(0);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("OK (%i ms)\n", (int)tx_time);
    } else {
      both.printf("fail (%i)\n", _radiolib_status);
    }
    // Maximum 1% duty cycle
    minimum_pause = tx_time * 100;
    last_tx = millis();
    radio.setDio1Action(rx);
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }

  // If a packet was received, display it and the RSSI and SNR
  if (rxFlag) {
    rxFlag = false;
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX [%s]\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
      both.printf("  SNR: %.2f dB\n", radio.getSNR());
    }
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

// Can't do Serial or display things here, takes too much time for the interrupt
void rx() {
  rxFlag = true;
}

String getStringData() {
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
  return data;
}

// // Board: Heltec WiFi LoRa 32 (V3)

// #include <Arduino.h>
// #include <RadioLib.h>  // RadioLib by Jan Gromes v7.1.2
// #include <DHTesp.h>
// #include <ArduinoJson.h>

// // Turns the 'PRG' button into the power button, long press is off
// #define HELTEC_POWER_BUTTON  // must be before "#include <heltec_unofficial.h>"

// // Uncomment this if you have Wireless Stick v3
// // #define HELTEC_WIRELESS_STICK

// // creates 'radio', 'display' and 'button' instances
// #include <heltec_unofficial.h>  // Heltec_ESP32_LoRa_v3 by Rop Gonggrijp v0.9.2

// #define DHTPIN 6  // GPIO48 for DHT sensor
// DHTesp dht;

// #define COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

// SX1262 controller(radio);
// uint8_t data[32] = { 0 };

// #define FREQUENCY 905.2  // for US
// #define BANDWIDTH 250.0

// void setup() {
//   heltec_setup();
//   dht.setup(DHTPIN, DHTesp::DHT22);  // Initialize DHT sensor
//   Serial.begin(9600);

//   for (size_t i = 0; i < 3; i++) {
//     display.print(".");
//     delay(1000);
//   }
//   display.println();

//   display.println("Zack Says Hi!!");
//   delay(2000);

//   // Radio
//   display.print("Radio ");
//   int state = radio.begin();
//   if (state == RADIOLIB_ERR_NONE) {
//     display.println("OK");
//   } else {
//     display.printf("fail, code: %i\n", state);
//   }

//   // Battery
//   float vbat = heltec_vbat();
//   display.printf("Vbat: %.2fV (%d%%)\n", vbat, heltec_battery_percent(vbat));

//   controller.begin();
//   both.printf("Frequency: %.2f MHz\n", FREQUENCY);
//   RADIOLIB_OR_HALT(radio.setFrequency(FREQUENCY));
//   both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
//   RADIOLIB_OR_HALT(radio.setBandwidth(BANDWIDTH));
//   controller.setFrequency(FREQUENCY);
//   controller.setBandwidth(BANDWIDTH);
//   // controller.setOutputPower(22);
// }


// void loop() {
//   heltec_loop();

//   display.clear();
//   display.printf("Temp: %.1fC\nHum: %.1f%%\n", dht.getTemperature(), dht.getHumidity());
//   display.display();

//   // Create a DynamicJsonDocument with a capacity of 1024 bytes
//   DynamicJsonDocument payload(1024);

//   // Create JSON data
//   JsonObject temp = payload.createNestedObject("Temp");
//   float degreesC = dht.getTemperature();
//   temp["DegreesC"] = degreesC;
//   temp["DegreesF"] = celsiusToFahrenheit(degreesC);
//   payload["Humidity"] = dht.getHumidity();

//   String data;
//   serializeJson(payload, data);
//   Serial.println(data);

//   int state = controller.transmit(data, data.length());
//   Serial.printf("Transmit State: %d\n", state);
//   delay(5000);  // Delay before next read
// }
