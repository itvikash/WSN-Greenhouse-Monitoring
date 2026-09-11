/*
  ============================================================
  GasNode.ino
  WSN Model for Greenhouse Monitoring — Gas Sensor Node
  ============================================================
  Board: ESP32-C6 (DF-Robot Beetle Mini Dev Board)
  Sensor: MQ-Series Gas Sensor
  Power: 18650 Li-ion Battery + TP4056 charger
  Wake control: Capacitive touch switch (ON/OFF)

  Behavior:
    1. Wake from deep sleep
    2. Allow brief warm-up time for MQ sensor
    3. Read gas concentration (analog, converted to approx. ppm)
    4. Send reading to Environment (Gateway) Node via ESP-NOW
    5. Go back to deep sleep to conserve battery

  Replace GATEWAY_MAC[] below with the actual MAC address of
  your Environment Node before uploading.
  ============================================================
*/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_sleep.h>

// ---------- CONFIGURATION ----------
#define GAS_SENSOR_PIN       A0       // Analog pin for MQ gas sensor
#define NODE_ID               2       // Unique ID for this node
#define WARMUP_TIME_MS      2000      // MQ sensors need a short warm-up before a stable reading
#define SLEEP_DURATION_US   (30ULL * 1000000ULL)  // 30 seconds (adjust as needed)

// MAC address of the Environment (Gateway) Node — UPDATE THIS
uint8_t GATEWAY_MAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

// ---------- DATA STRUCTURE ----------
// Must match the struct used on the Environment Node
typedef struct struct_message {
  int nodeId;
  char nodeType[10];   // "SOIL" or "GAS"
  float value1;        // primary reading (soil moisture % or gas ppm)
  float value2;        // unused for gas node, reserved for future sensors
} struct_message;

struct_message gasData;

// ---------- ESP-NOW SEND CALLBACK ----------
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ---------- READ GAS CONCENTRATION ----------
float readGasConcentration() {
  int raw = analogRead(GAS_SENSOR_PIN);
  // Simple linear approximation — replace with a proper MQ calibration
  // curve (Rs/Ro ratio) for accurate ppm values in your application.
  float ppm = map(raw, 0, 4095, 0, 10000);
  return ppm;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // MQ sensors need a brief warm-up for a stable reading
  delay(WARMUP_TIME_MS);

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
  gasData.nodeId = NODE_ID;
  strcpy(gasData.nodeType, "GAS");
  gasData.value1 = readGasConcentration();
  gasData.value2 = 0.0;

  Serial.print("Gas Concentration: ");
  Serial.print(gasData.value1);
  Serial.println(" ppm");

  // Send data
  esp_err_t result = esp_now_send(GATEWAY_MAC, (uint8_t *)&gasData, sizeof(gasData));
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
