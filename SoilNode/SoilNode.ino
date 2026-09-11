/*
  ============================================================
  SoilNode.ino
  WSN Model for Greenhouse Monitoring — Soil Moisture Node
  ============================================================
  Board: ESP32-C6 (DF-Robot Beetle Mini Dev Board)
  Sensor: Capacitive/Analog Soil Moisture Sensor
  Power: 18650 Li-ion Battery + TP4056 charger
  Wake control: Capacitive touch switch (ON/OFF)

  Behavior:
    1. Wake from deep sleep
    2. Read soil moisture (analog)
    3. Send reading to Environment (Gateway) Node via ESP-NOW
    4. Go back to deep sleep to conserve battery

  Replace GATEWAY_MAC[] below with the actual MAC address of
  your Environment Node before uploading.
  ============================================================
*/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>

// ---------- CONFIGURATION ----------
#define SOIL_SENSOR_PIN     A0        // Analog pin for soil moisture sensor
#define NODE_ID              1        // Unique ID for this node
#define SLEEP_DURATION_US   (30ULL * 1000000ULL)  // 30 seconds (adjust as needed)

// MAC address of the Environment (Gateway) Node — UPDATE THIS
uint8_t GATEWAY_MAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

// ---------- DATA STRUCTURE ----------
// Must match the struct used on the Environment Node
typedef struct struct_message {
  int nodeId;
  char nodeType[10];   // "SOIL" or "GAS"
  float value1;        // primary reading (soil moisture % or gas ppm)
  float value2;        // unused for soil node, reserved for future sensors
} struct_message;

struct_message soilData;

// ---------- ESP-NOW SEND CALLBACK ----------
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ---------- READ SOIL MOISTURE ----------
float readSoilMoisture() {
  int raw = analogRead(SOIL_SENSOR_PIN);
  // Calibrate these values to your specific sensor (dry vs. wet readings)
  int dryValue = 3000;   // raw ADC value in dry air
  int wetValue = 1200;   // raw ADC value fully submerged in water

  float percentage = map(raw, dryValue, wetValue, 0, 100);
  percentage = constrain(percentage, 0, 100);
  return percentage;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Set WiFi to station mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    goToSleep();
    return;
  }

  esp_now_register_send_cb(onDataSent);

  // Register the gateway as a peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, GATEWAY_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    goToSleep();
    return;
  }

  // Read sensor and populate struct
  soilData.nodeId = NODE_ID;
  strcpy(soilData.nodeType, "SOIL");
  soilData.value1 = readSoilMoisture();
  soilData.value2 = 0.0;

  Serial.print("Soil Moisture: ");
  Serial.print(soilData.value1);
  Serial.println("%");

  // Send data
  esp_err_t result = esp_now_send(GATEWAY_MAC, (uint8_t *)&soilData, sizeof(soilData));
  if (result != ESP_OK) {
    Serial.println("Error sending data");
  }

  delay(200); // allow time for send callback to complete
  goToSleep();
}

void goToSleep() {
  Serial.println("Going to deep sleep...");
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
  esp_deep_sleep_start();
}

void loop() {
  // Not used — device sleeps after setup()
}
