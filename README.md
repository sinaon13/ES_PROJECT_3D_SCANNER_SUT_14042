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

### Permanent Session Storage & Archiving
Every scan session creates its own unique timestamped folder so **no projects, models, or images are ever overwritten or deleted**:
- **Session Photos:** `E:\3D\photos\scan_<timestamp>\photo_0001.jpg`, etc.
- **Session Models:** `E:\3D\models\scan_<timestamp>\texturedMesh.obj`
- **Session Projects:** `E:\3D\RealityScanProject\scan_<timestamp>\scan_<timestamp>.rsc`

### High Quality RealityScan Automation
When all photos are received, the server executes RealityScan CLI with maximum reconstruction quality flags:
```cmd
E:\Epic Games\RealityScan_2.2\RealityScan.exe -newScene -addFolder "E:\3D\photos\scan_<timestamp>" -align -downscale=1 -calculateHighModel -calculateTexture -textureSize=4096 -saveProject "E:\3D\RealityScanProject\scan_<timestamp>\scan_<timestamp>.rsc" -exportModel "Model 1" "E:\3D\models\scan_<timestamp>\texturedMesh.obj" -save -quit
```

---

## 2. Flashing the ESP32-S3 Firmware

1. Open `esp32_firmware/esp32_firmware.ino` in the Arduino IDE.
2. Configure your Wi-Fi Hotspot credentials at the top of the file.
3. In **Tools**, select your board (`ESP32-S3 Dev Module` or `YD-ESP32-S3-N16R8`).
4. **CRITICAL PSRAM SETTING**: In **Tools -> PSRAM**, select **`OPI PSRAM`** (or `Enabled`).
5. Upload the sketch.

---

## 3. How to Perform a Scan

1. Connect your smartphone to the same **2.4GHz Wi-Fi Hotspot** (`192.168.137.x`).
2. Open the ESP32's IP address in your mobile browser.
3. **Camera Features:**
   - **Normal Camera Selection:** Automatically filters out ultra-wide lenses and selects your phone's primary back camera.
   - **1-Tap Lens Switching:** Tap the **"📷 Normal Lens"** button on the crop bar at any time to cycle between available rear camera lenses.
   - **Flash / Torch Toggle:** Tap the **"⚡ Flash: OFF"** button to turn on your smartphone's LED Flashlight for improved photogrammetry lighting.
4. **High Quality Image Capture:** Photos are captured at up to **1 MB** high quality (compressed only if the raw JPEG exceeds 1 MB).
5. **Crop & Configure:**
   - Drag and resize the crop box over the live camera preview.
   - Set the rotation **Degree**, **PWM Speed**, **Total Photos**, and **Stabilization Delay**.
6. **Start Scan:** Tap **START SCAN** to begin the automated turntable photogrammetry loop.
