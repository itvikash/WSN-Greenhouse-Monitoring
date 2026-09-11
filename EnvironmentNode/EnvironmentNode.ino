/*
  ============================================================
  EnvironmentNode.ino
  WSN Model for Greenhouse Monitoring — Environment (Gateway) Node
  ============================================================
  Board: ESP32-C6 (DF-Robot Beetle Mini Dev Board)
  Sensors: AHT20 (temperature & humidity), BH1750 (light intensity)
  Display: OLED (I2C, SSD1306)
  Power: USB or larger battery (always-on gateway)

  Role:
    - Central hub of the WSN
    - Receives data from Soil Node and Gas Node via ESP-NOW
    - Reads its own local sensors (temp, humidity, light)
    - Displays combined data on OLED (cycling screens)
    - Uploads combined data to Blynk IoT platform via WiFi

  Required libraries (install via Arduino Library Manager):
    - Adafruit AHTX0
    - BH1750
    - Adafruit SSD1306 + Adafruit GFX
    - Blynk (Blynk.Edgent or Blynk IoT library, v1.x)

  Before uploading:
    - Fill in WIFI_SSID / WIFI_PASSWORD
    - Fill in BLYNK_AUTH_TOKEN / BLYNK_TEMPLATE_ID / BLYNK_TEMPLATE_NAME
    - Note this board's MAC address and use it as GATEWAY_MAC
      in SoilNode.ino and GasNode.ino
  ============================================================
*/

#define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Greenhouse Monitoring System"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BlynkSimpleEsp32.h>

// ---------- WIFI / BLYNK CREDENTIALS ----------
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// ---------- OLED CONFIG ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- SENSOR OBJECTS ----------
Adafruit_AHTX0 aht;
BH1750 lightMeter;

// ---------- ESP-NOW DATA STRUCTURE ----------
// Must match the struct used on SoilNode.ino / GasNode.ino
typedef struct struct_message {
  int nodeId;
  char nodeType[10];   // "SOIL" or "GAS"
  float value1;        // soil moisture % or gas ppm
  float value2;        // reserved
} struct_message;

struct_message incomingData;

// ---------- SHARED STATE ----------
float soilMoisture = -1;
float gasConcentration = -1;
float envTemp = 0;
float envHumidity = 0;
float envLight = 0;

unsigned long lastLocalReadMs = 0;
const unsigned long LOCAL_READ_INTERVAL_MS = 5000;   // read local sensors every 5s

unsigned long lastDisplayMs = 0;
const unsigned long DISPLAY_CYCLE_MS = 3000;          // switch OLED screen every 3s
int displayScreen = 0;

// ---------- ESP-NOW RECEIVE CALLBACK ----------
void onDataReceive(const uint8_t *mac, const uint8_t *incomingBytes, int len) {
  memcpy(&incomingData, incomingBytes, sizeof(incomingData));

  if (strcmp(incomingData.nodeType, "SOIL") == 0) {
    soilMoisture = incomingData.value1;
    Blynk.virtualWrite(V0, soilMoisture);   // Virtual pin for soil moisture
    Serial.print("Received Soil Moisture: ");
    Serial.println(soilMoisture);
  } else if (strcmp(incomingData.nodeType, "GAS") == 0) {
    gasConcentration = incomingData.value1;
    Blynk.virtualWrite(V1, gasConcentration); // Virtual pin for gas concentration
    Serial.print("Received Gas Concentration: ");
    Serial.println(gasConcentration);
  }
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin();

  // --- OLED init ---
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Greenhouse Monitor");
  display.println("Starting...");
  display.display();

  // --- Sensor init ---
  if (!aht.begin()) {
    Serial.println("AHT20 not found");
  }
  if (!lightMeter.begin()) {
    Serial.println("BH1750 not found");
  }

  // --- WiFi + Blynk ---
  WiFi.mode(WIFI_STA);
  Serial.print("This node's MAC address: ");
  Serial.println(WiFi.macAddress());  // Use this MAC in SoilNode.ino / GasNode.ino

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // --- ESP-NOW init (after WiFi.mode, before WiFi fully connects is fine) ---
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
  } else {
    esp_now_register_recv_cb(onDataReceive);
  }
}

// ---------- READ LOCAL ENVIRONMENT SENSORS ----------
void readLocalSensors() {
  sensors_event_t humidityEvent, tempEvent;
  aht.getEvent(&humidityEvent, &tempEvent);

  envTemp = tempEvent.temperature;
  envHumidity = humidityEvent.relative_humidity;
  envLight = lightMeter.readLightLevel();

  Blynk.virtualWrite(V2, envTemp);
  Blynk.virtualWrite(V3, envHumidity);
  Blynk.virtualWrite(V4, envLight);

  Serial.printf("Temp: %.2f C | Humidity: %.2f%% | Light: %.2f lx\n",
                envTemp, envHumidity, envLight);
}

// ---------- OLED DISPLAY ----------
void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);

  switch (displayScreen) {
    case 0:
      display.println("ENVIRONMENT");
      display.print("Temp: "); display.println(envTemp);
      display.print("Hum : "); display.println(envHumidity);
      display.print("Lux : "); display.println(envLight);
      break;
    case 1:
      display.println("SOIL NODE");
      display.print("Moisture: ");
      display.print(soilMoisture >= 0 ? String(soilMoisture) : "N/A");
      display.println(" %");
      break;
    case 2:
      display.println("GAS NODE");
      display.print("Gas: ");
      display.print(gasConcentration >= 0 ? String(gasConcentration) : "N/A");
      display.println(" ppm");
      break;
  }

  display.display();
  displayScreen = (displayScreen + 1) % 3;
}

// ---------- MAIN LOOP ----------
void loop() {
  Blynk.run();

  unsigned long now = millis();

  if (now - lastLocalReadMs >= LOCAL_READ_INTERVAL_MS) {
    lastLocalReadMs = now;
    readLocalSensors();
  }

  if (now - lastDisplayMs >= DISPLAY_CYCLE_MS) {
    lastDisplayMs = now;
    updateDisplay();
  }
}
