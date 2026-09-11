# ⚙️ Setup Guide

This guide walks you through configuring the three `.ino` files and getting the system running end-to-end.

---

## 1. Install Required Libraries

Open **Arduino IDE → Tools → Manage Libraries** and install:

| Library | Used in |
|---|---|
| ESP32 board package (by Espressif) | All nodes |
| Adafruit AHTX0 | EnvironmentNode.ino |
| BH1750 (by claws) | EnvironmentNode.ino |
| Adafruit GFX Library | EnvironmentNode.ino |
| Adafruit SSD1306 | EnvironmentNode.ino |
| Blynk (v1.x, "Blynk" by Volodymyr Shymanskyy) | EnvironmentNode.ino |

**Board Manager URL** (Arduino IDE → Preferences → Additional Board Manager URLs):
```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```
Then select **ESP32C6 Dev Module** under Tools → Board.

---

## 2. Set Up the Environment (Gateway) Node First

The gateway must be flashed **first** because the other two nodes need its MAC address.

1. Open `EnvironmentNode.ino`
2. Fill in your WiFi credentials:
   ```cpp
   char ssid[] = "YOUR_WIFI_SSID";
   char pass[] = "YOUR_WIFI_PASSWORD";
   ```
3. Create a project on [blynk.cloud](https://blynk.cloud) and fill in:
   ```cpp
   #define BLYNK_TEMPLATE_ID   "YOUR_TEMPLATE_ID"
   #define BLYNK_TEMPLATE_NAME "Greenhouse Monitoring System"
   #define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"
   ```
   These three values are shown in your Blynk.Cloud project under **Device Info**.
4. In Blynk, set up **Datastreams** matching the virtual pins used in the code:

   | Virtual Pin | Data | Type |
   |---|---|---|
   | V0 | Soil Moisture (%) | Double |
   | V1 | Gas Concentration (ppm) | Double |
   | V2 | Environment Temperature (°C) | Double |
   | V3 | Environment Humidity (%) | Double |
   | V4 | Environment Light (lx) | Double |

5. Upload `EnvironmentNode.ino` to the ESP32-C6 board.
6. Open **Serial Monitor** (115200 baud). On boot it prints:
   ```
   This node's MAC address: XX:XX:XX:XX:XX:XX
   ```
   **Copy this MAC address** — you'll need it in the next step.

---

## 3. Configure the Soil and Gas Nodes

1. Open `SoilNode.ino` and `GasNode.ino`
2. In both files, replace:
   ```cpp
   uint8_t GATEWAY_MAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};
   ```
   with the actual MAC address from Step 2, formatted as hex bytes. Example:
   if the printed MAC is `A4:CF:12:3B:9E:01`, write:
   ```cpp
   uint8_t GATEWAY_MAC[] = {0xA4, 0xCF, 0x12, 0x3B, 0x9E, 0x01};
   ```
3. Upload each sketch to its respective board.

---

## 4. Calibrate the Sensors

**Soil Moisture (`SoilNode.ino`):**
```cpp
int dryValue = 3000;   // raw ADC reading with sensor in dry air
int wetValue = 1200;   // raw ADC reading with sensor fully in water
```
Take actual readings from your sensor in both conditions and update these values for accurate percentages.

**Gas Sensor (`GasNode.ino`):**
The default code uses a simple linear ADC-to-ppm mapping. For accurate readings, refer to your specific MQ sensor's datasheet (e.g., MQ-2, MQ-135) and implement its Rs/Ro calibration curve instead.

---

## 5. Power On and Test

1. Power the Environment Node first (USB or battery) — confirm it connects to WiFi and Blynk (check Serial Monitor).
2. Power on the Soil Node and Gas Node.
3. Check the OLED display cycles through Environment / Soil / Gas screens.
4. Open your Blynk dashboard — values should update within a few seconds of each node waking up.

---

## Troubleshooting

| Issue | Likely Cause |
|---|---|
| Soil/Gas node data never appears | Wrong `GATEWAY_MAC` value, or nodes/gateway on different WiFi channels |
| OLED blank | Check I2C wiring (SDA/SCL) and address (default `0x3C`) |
| Blynk shows no data | Wrong auth token, or WiFi credentials incorrect |
| Gas readings unstable | Increase `WARMUP_TIME_MS` in `GasNode.ino` — MQ sensors need longer warm-up on first power-on |
