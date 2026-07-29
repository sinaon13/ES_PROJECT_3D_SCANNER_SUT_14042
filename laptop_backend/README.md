# Laptop Backend Server (FastAPI + uv)

## Prerequisites
- **uv** package manager installed on Windows.
- **Windows 2.4GHz Wi-Fi Hotspot** active (`192.168.137.1`).
- **Epic Games RealityScan 2.2** installed at `E:\Epic Games\RealityScan_2.2\RealityScan.exe`.

## How to Run
Open your terminal in this directory and run:

```cmd
uv run server.py
```

`uv` will automatically download FastAPI & Uvicorn and launch the server on `http://192.168.137.1:8000`.

## API Endpoints
- `POST /session_start` : Clears `E:\3D\photos`, `E:\3D\models`, `E:\3D\RealityScanCache` and resets photo counter.
- `POST /upload_image` : Receives raw JPEG body from ESP32 and saves as `photo_0001.jpg`, etc. When expected photo count is reached, triggers RealityScan CLI.
- `POST /session_complete` : Manually triggers RealityScan CLI.
- `GET /status` : Diagnostics endpoint.
