# AutoShade – Automated Blind Controller (ESP32)

A solar-powered, temperature-driven motorised blind controller built on the ESP32-C6. The device automatically opens and closes window blinds based on room temperature, and can also be controlled manually via physical buttons or remotely over Wi-Fi.

This was developed as part of the KD5080 Electrical Product Development module at Northumbria University (2024–2025) by team AutoShade LLC.

---

## How It Works

The ESP32 reads room temperature from a DHT11/BME280 sensor and drives a continuous-rotation servo motor to open or close blinds. The system operates in three modes:

- **Idle** – displays temperature and blind position on the OLED; waits for input
- **Manual control** – Up/Down buttons move the blinds; releasing the button stops movement mid-way and saves the partial position
- **Calibration mode** – hold the calibrate button for 2 seconds to enter; use the Up/Down buttons to set the full open and close travel times, which are saved to EEPROM and persist across reboots

Remote control is available over Wi-Fi via a simple HTTP web server:
- `GET /up` – move blinds up
- `GET /down` – move blinds down
- `GET /temperature` – returns current temperature as JSON

If Wi-Fi is unavailable, the device falls back to offline/manual mode automatically.

---

## Hardware

| Component | Part |
|---|---|
| Microcontroller | ESP32-C6 Mini Development Board |
| Motor | TowerPro MG996R 360° Continuous Servo |
| Temperature sensor | DHT11 (or BME280 at address `0x76`) |
| Display | 0.96" SSD1306 OLED (128×64, I2C) |
| Battery | 3.7V 1200mAh LiPo |
| Solar charging | DFR0264 Solar LiPo Charger |
| Voltage boost | Adafruit MiniBoost 5V (TPS61023) |
| Solar panel | Voltaic P122 5V 0.3W |

---

## Wiring

| ESP32 Pin | Connected To |
|---|---|
| GPIO 18 | Servo signal wire |
| GPIO 4 | Up button (other leg to GND) |
| GPIO 5 | Down button (other leg to GND) |
| GPIO 19 | Calibrate button (other leg to GND) |
| GPIO 21 (SDA) | OLED SDA / BME280 SDA |
| GPIO 22 (SCL) | OLED SCL / BME280 SCL |
| 3.3V | OLED VCC, DHT11 VCC |
| 5V | Servo VCC (from MiniBoost output) |
| GND | All component grounds (shared) |

> ⚠️ The servo draws significant current — power it from the 5V MiniBoost output, **not** directly from the ESP32's 3.3V pin. Make sure all grounds are connected together.

---

## Library Dependencies

Install these via the Arduino IDE Library Manager (**Sketch → Include Library → Manage Libraries**):

| Library | Install Name |
|---|---|
| ESP32Servo | `ESP32Servo` |
| Adafruit SSD1306 | `Adafruit SSD1306` |
| Adafruit GFX | `Adafruit GFX Library` |
| Adafruit Unified Sensor | `Adafruit Unified Sensor` |
| Adafruit BME280 | `Adafruit BME280 Library` |
| ESPAsyncWebServer | Install manually from [GitHub](https://github.com/me-no-dev/ESPAsyncWebServer) |
| AsyncTCP (dependency) | Install manually from [GitHub](https://github.com/me-no-dev/AsyncTCP) |
| EEPROM | Built into Arduino ESP32 core |
| Wire | Built into Arduino ESP32 core |
| WiFi | Built into Arduino ESP32 core |

---

## Flashing with Arduino IDE

1. **Install the ESP32 board package**
   - Open Arduino IDE and go to **File → Preferences**
   - Add this URL to *Additional Boards Manager URLs*:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to **Tools → Board → Boards Manager**, search for `esp32` and install the Espressif package

2. **Select the correct board**
   - **Tools → Board → ESP32 Arduino → ESP32C6 Dev Module**

3. **Configure Wi-Fi credentials**
   - Open the sketch and update these two lines near the top:
     ```cpp
     const char* ssid = "YourNetworkName";
     const char* password = "YourPassword";
     ```

4. **Connect and flash**
   - Connect the ESP32 via USB-C
   - Select the correct port under **Tools → Port**
   - Click **Upload**

5. **Open Serial Monitor** (115200 baud) to confirm Wi-Fi connection and see the device's local IP address

6. **Test remote control**
   - Visit `http://<device-ip>/temperature` in a browser to confirm it's working

---

## Calibration

The first time you use the device (or after changing blinds), you need to calibrate the travel time:

1. Hold the **Calibrate button** for more than 2 seconds — the OLED will show *"Calibration Mode"*
2. Hold the **Down button** until the blind is fully closed, then press **Calibrate** to save
3. Hold the **Up button** until the blind is fully open, then press **Calibrate** to save
4. The OLED will show *"Calibration Done!"* — values are saved to EEPROM and will persist after reboot

---

## File Structure

```
/
├── autoshade_esp32.ino   # Main sketch
└── README.md
```

---

## Known Limitations

- Blind position is estimated by timing, not measured by an encoder
- Button input uses polling rather than hardware interrupts
- Web interface is basic HTTP endpoints — no HTML UI
- No OTA (over-the-air) update support yet

---

## License

This project was developed for academic purposes at Northumbria University. Feel free to use or adapt it for your own projects.
