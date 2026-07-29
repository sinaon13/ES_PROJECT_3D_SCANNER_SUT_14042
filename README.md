# 3D Turntable Scanner - Pro Automated Photogrammetry Pipeline

This project implements a **4-Node Star Topology** 3D photogrammetry scanning system using an **ESP32-S3** microcontroller as the central broker, a **smartphone camera**, an **L298N** turntable motor, and a **Windows Laptop** running **Epic Games RealityScan 2.2**.

---

## Architecture Overview (ESP32 Hub)

```mermaid
graph TD
    P[Smartphone Web App] <-->|HTTP GET /status<br/>POST /upload| E[ESP32-S3 Central Hub<br/>Wi-Fi Client 192.168.137.x]
    E -->|GPIO 4, 5, 6<br/>rotateMotor()| M[Turntable Motor<br/>L298N Driver]
    E -->|HTTP POST /upload_image<br/>Relayed JPEGs from PSRAM| L[Windows Laptop Server<br/>192.168.137.1:8000]
    L -->|Auto-Execute CLI| R[Epic Games RealityScan 2.2<br/>E:\Epic Games\RealityScan_2.2\RealityScan.exe]
```

- **ESP32-S3 Hub**: Connects as a client to the Laptop's 2.4GHz Wi-Fi Hotspot (`192.168.137.1`). It manages the scan state machine, drives the L298N motor, serves the interactive Phone Web App, buffers incoming JPEG photos in **8MB PSRAM**, and relays them to the laptop.
- **Strict Isolation**: No direct network connections occur between the phone and the laptop.

---

## Hardware & Pinout

| YD-ESP32-S3 Pin | L298N Pin | Function |
| :--- | :--- | :--- |
| **GPIO 4** *(Header P1, Pin 4)* | **IN1** | Motor Direction Control Pin 1 |
| **GPIO 5** *(Header P1, Pin 5)* | **IN2** | Motor Direction Control Pin 2 |
| **GPIO 6** *(Header P1, Pin 6)* | **ENA** | PWM Motor Speed Enable *(remove jumper cap)* |
| **GND** | **GND** | **Common Ground** *(mandatory shared ground)* |

---

## 1. Running the Windows Laptop Server

The laptop backend uses Python with **FastAPI** & **Uvicorn**, managed via **`uv`**.

1. Ensure your Windows 2.4GHz Wi-Fi Hotspot is active at IP address `192.168.137.1`.
2. Open a command prompt inside the `laptop_backend/` folder:
   ```cmd
   cd laptop_backend
   uv run server.py
   ```
   *`uv` will automatically install dependencies and launch the server on port `8000` without needing a manual virtual environment.*

### Automated RealityScan Paths
The server uses the exact unchangeable paths:
- **RealityScan Executable:** `E:\Epic Games\RealityScan_2.2\RealityScan.exe`
- **Photos Directory:** `E:\3D\photos`
- **Models Directory:** `E:\3D\models`
- **Project File:** `E:\3D\RealityScanProject\AutoScan.rsc`
- **Cache Directory:** `E:\3D\RealityScanCache`

---

## 2. Flashing the ESP32-S3 Firmware

1. Open `esp32_firmware/esp32_firmware.ino` in the Arduino IDE.
2. In `esp32_firmware.ino`, configure your Wi-Fi Hotspot SSID and password:
   ```cpp
   const char* WIFI_SSID     = "YOUR_HOTSPOT_SSID";
   const char* WIFI_PASSWORD = "YOUR_HOTSPOT_PASSWORD";
   ```
3. In **Tools**, select your board (`ESP32-S3 Dev Module` or `YD-ESP32-S3-N16R8`).
4. **CRITICAL PSRAM SETTING**: In **Tools -> PSRAM**, select **`OPI PSRAM`** (or `Enabled`).
   - This ensures the 2MB JPEG relay buffer is safely allocated in PSRAM without consuming DRAM.
5. Upload the sketch.

---

## 3. How to Perform a Scan

1. Connect your smartphone to the same **2.4GHz Wi-Fi Hotspot** (`192.168.137.x`).
2. Open the ESP32's IP address in your mobile browser (e.g., `http://192.168.137.15`).
3. **Crop & Configure:**
   - Adjust the interactive crop box over the live camera preview to frame your turntable object.
   - Set the rotation **Degree** (e.g. `10°`), **PWM Speed** (`0-255`), **Total Photos** (e.g. `36`), and **Stabilization Delay** (e.g. `1500 ms`).
4. **Start Scan:**
   - Press **START SCAN**.
   - The ESP32 will notify the laptop to automatically clear old models, photos, and cache from `E:\3D`.
   - The turntable will rotate by the specified degree, wait for stabilization, and command your phone to snap and upload the cropped photo.
   - The ESP32 buffers the JPEG in PSRAM and relays it to `192.168.137.1:8000/upload_image`.
5. **3D Reconstruction:**
   - Once all photos are captured and relayed, the laptop backend automatically spawns **RealityScan 2.2 CLI** in the background to generate and export your `.fbx` model to `E:\3D\models`.
