/**
 * This is a bridge from LoRA to MQTT
 */

#include <ArduinoJson.h>
#include "arduino-secrets.h"  // https://www.andreagrandi.it/posts/how-to-safely-store-arduino-secrets/
#include <WiFi.h>
#include <PubSubClient.h>

// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON  // must be before "#include <heltec_unofficial.h>"
#include <heltec_unofficial.h>

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
#define TRANSMIT_POWER 0

String rxdata;
volatile bool rxFlag = false;
long counter = 0;
uint64_t last_tx = 0;
uint64_t tx_time;
uint64_t minimum_pause;

#define base_topic "norseiot/lora-demo"
const char* temp_f_topic = base_topic "/temperature/f";
const char* temp_c_topic = base_topic "/temperature/c";
const char* humidity_topic = base_topic "/humidity";
#undef base_topic

WiFiClient espClient;
PubSubClient client(espClient);

String getClientId() {
  return "esp32-client-" + String(WiFi.macAddress());
}

void rx() {
  rxFlag = true;
}

void setup() {
  heltec_setup();

  /******* MQTT Stuff **********/

  // Connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    both.println("Connecting to WiFi..");
  }
  //connecting to a mqtt broker
  both.println("Connected to the Wi-Fi network");

  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected()) {
    String client_id = getClientId();
    both.printf("The client %s is connecting to the public MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str())) {
      both.println("Public EMQX MQTT broker connected");
    } else {
      both.print("failed with state ");
      both.print(client.state());
      delay(2000);
    }
  }

  /******* LoRA Stuff **********/

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

void loop() {
  heltec_loop();
  client.loop();

  // If a packet was received, display it and the RSSI and SNR
  if (rxFlag) {
    rxFlag = false;
    heltec_led(50);  // 50% bright
    radio.readData(rxdata);
    if (_radiolib_status == RADIOLIB_ERR_NONE) {
      both.printf("RX [%s]\n", rxdata.c_str());
      both.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
      both.printf("  SNR: %.2f dB\n", radio.getSNR());
    }
    processLoRaData(rxdata);
    heltec_led(0);  // off
    RADIOLIB_OR_HALT(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
  }
}

/**
 * Take the JSON data transmitted over LoRA
 */
void processLoRaData(String rawData) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, rawData);
  if (!error) {
    String client_id = getClientId();
    String message = client_id + " has been on for " + String(millis()) + " milliseconds!";

    float tempC = doc["Temp"]["DegreesC"];
    float tempF = doc["Temp"]["DegreesF"];
    float humidity = doc["Humidity"];
    client.publish(temp_f_topic, String(tempF).c_str());
    client.publish(temp_c_topic, String(tempC).c_str());
    client.publish(humidity_topic, String(humidity).c_str());

    both.printf("Recieved Data: %.2fC %.2fF %.2f\n", tempC, tempF, humidity);
  }
}
