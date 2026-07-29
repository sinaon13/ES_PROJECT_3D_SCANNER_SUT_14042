#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>3D Turntable Scanner - Pro Controller</title>
  <style>
    :root {
      --bg-primary: #0a0e17;
      --bg-card: rgba(23, 30, 46, 0.75);
      --bg-card-hover: rgba(30, 40, 60, 0.85);
      --accent-cyan: #00f0ff;
      --accent-purple: #8a2be2;
      --accent-green: #00ff88;
      --accent-red: #ff3b30;
      --text-main: #f0f4f8;
      --text-muted: #94a3b8;
      --border-color: rgba(255, 255, 255, 0.12);
      --glass-shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
    }

    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      font-family: 'Segoe UI', system-ui, -apple-system, BlinkMacSystemFont, Roboto, sans-serif;
    }

    body {
      background-color: var(--bg-primary);
      background-image: 
        radial-gradient(circle at 15% 20%, rgba(0, 240, 255, 0.08) 0%, transparent 40%),
        radial-gradient(circle at 85% 80%, rgba(138, 43, 226, 0.12) 0%, transparent 40%);
      color: var(--text-main);
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 16px;
    }

    header {
      width: 100%;
      max-width: 640px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 16px;
      padding: 12px 20px;
      background: var(--bg-card);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border-radius: 16px;
      border: 1px solid var(--border-color);
      box-shadow: var(--glass-shadow);
    }

    .title-area h1 {
      font-size: 1.25rem;
      font-weight: 700;
      background: linear-gradient(90deg, var(--accent-cyan), #fff);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }

    .title-area p {
      font-size: 0.75rem;
      color: var(--text-muted);
    }

    .status-badge {
      padding: 6px 14px;
      border-radius: 20px;
      font-size: 0.75rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      background: rgba(0, 240, 255, 0.15);
      color: var(--accent-cyan);
      border: 1px solid rgba(0, 240, 255, 0.4);
      transition: all 0.3s ease;
    }

    .status-badge.scanning {
      background: rgba(0, 255, 136, 0.15);
      color: var(--accent-green);
      border-color: rgba(0, 255, 136, 0.4);
      animation: pulse 1.5s infinite;
    }

    .status-badge.aborted {
      background: rgba(255, 59, 48, 0.15);
      color: var(--accent-red);
      border-color: rgba(255, 59, 48, 0.4);
    }

    @keyframes pulse {
      0% { box-shadow: 0 0 0 0 rgba(0, 255, 136, 0.4); }
      70% { box-shadow: 0 0 0 10px rgba(0, 255, 136, 0); }
      100% { box-shadow: 0 0 0 0 rgba(0, 255, 136, 0); }
    }

    main {
      width: 100%;
      max-width: 640px;
      display: flex;
      flex-direction: column;
      gap: 16px;
    }

    /* CAMERA & CROP VIEWER */
    .viewport-card {
      position: relative;
      width: 100%;
      background: #000;
      border-radius: 20px;
      overflow: hidden;
      border: 1px solid var(--border-color);
      box-shadow: var(--glass-shadow);
      aspect-ratio: 4/3;
      display: flex;
      align-items: center;
      justify-content: center;
    }

    video {
      width: 100%;
      height: 100%;
      object-fit: contain;
      pointer-events: none;
    }

    /* Interactive Crop Overlay */
    #cropOverlay {
      position: absolute;
      top: 15%;
      left: 15%;
      width: 70%;
      height: 70%;
      border: 2px dashed var(--accent-cyan);
      box-shadow: 0 0 0 9999px rgba(0, 0, 0, 0.55);
      cursor: move;
      touch-action: none;
      display: flex;
      align-items: center;
      justify-content: center;
    }

    #cropOverlay::before {
      content: 'CROP AREA (DRAG / RESIZE)';
      font-size: 0.65rem;
      font-weight: 700;
      color: rgba(255, 255, 255, 0.65);
      letter-spacing: 1px;
      pointer-events: none;
    }

    .resize-handle {
      position: absolute;
      width: 24px;
      height: 24px;
      background: var(--accent-cyan);
      border-radius: 50%;
      border: 2px solid #fff;
    }

    .resize-se {
      bottom: -12px;
      right: -12px;
      cursor: se-resize;
    }

    /* CROP CONTROLS / PRESETS */
    .crop-bar {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 8px 16px;
      background: var(--bg-card);
      border-radius: 12px;
      border: 1px solid var(--border-color);
      font-size: 0.8rem;
      color: var(--text-muted);
    }

    .crop-bar button {
      background: rgba(255, 255, 255, 0.1);
      border: 1px solid var(--border-color);
      color: var(--text-main);
      padding: 4px 10px;
      border-radius: 6px;
      font-size: 0.75rem;
      cursor: pointer;
    }

    .crop-bar button:hover {
      background: rgba(0, 240, 255, 0.2);
    }

    /* PARAMS CARD */
    .params-card {
      background: var(--bg-card);
      backdrop-filter: blur(12px);
      -webkit-backdrop-filter: blur(12px);
      border-radius: 20px;
      border: 1px solid var(--border-color);
      padding: 20px;
      box-shadow: var(--glass-shadow);
    }

    .params-card h2 {
      font-size: 1rem;
      margin-bottom: 16px;
      color: var(--accent-cyan);
      display: flex;
      justify-content: space-between;
    }

    .param-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 16px;
    }

    .param-item {
      display: flex;
      flex-direction: column;
      gap: 6px;
    }

    .param-item label {
      font-size: 0.78rem;
      color: var(--text-muted);
      font-weight: 500;
    }

    .input-wrapper {
      display: flex;
      align-items: center;
      background: rgba(10, 14, 23, 0.8);
      border: 1px solid var(--border-color);
      border-radius: 10px;
      padding: 8px 12px;
      transition: border-color 0.2s;
    }

    .input-wrapper:focus-within {
      border-color: var(--accent-cyan);
    }

    .input-wrapper input {
      background: transparent;
      border: none;
      color: var(--text-main);
      font-size: 1rem;
      font-weight: 600;
      width: 100%;
      outline: none;
    }

    .unit {
      font-size: 0.75rem;
      color: var(--text-muted);
      margin-left: 4px;
    }

    /* ACTION BUTTONS */
    .actions-bar {
      display: flex;
      gap: 12px;
      margin-top: 20px;
    }

    .btn {
      flex: 1;
      padding: 16px;
      border-radius: 14px;
      border: none;
      font-size: 1rem;
      font-weight: 700;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      transition: all 0.25s ease;
      box-shadow: 0 4px 15px rgba(0, 0, 0, 0.3);
    }

    .btn-start {
      background: linear-gradient(135deg, #00f0ff, #0072ff);
      color: #000;
    }

    .btn-start:hover:not(:disabled) {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(0, 240, 255, 0.4);
    }

    .btn-abort {
      background: linear-gradient(135deg, #ff3b30, #c81d11);
      color: #fff;
    }

    .btn-abort:hover:not(:disabled) {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(255, 59, 48, 0.4);
    }

    .btn:disabled {
      opacity: 0.4;
      cursor: not-allowed;
      transform: none !important;
      box-shadow: none !important;
    }

    /* PROGRESS DASHBOARD */
    .progress-card {
      display: none;
      background: var(--bg-card);
      backdrop-filter: blur(12px);
      border-radius: 20px;
      border: 1px solid var(--border-color);
      padding: 20px;
      box-shadow: var(--glass-shadow);
      flex-direction: column;
      gap: 12px;
    }

    .progress-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    .progress-header span {
      font-size: 0.85rem;
      color: var(--text-muted);
    }

    .progress-header strong {
      color: var(--accent-cyan);
    }

    .progress-bar-bg {
      width: 100%;
      height: 10px;
      background: rgba(255, 255, 255, 0.1);
      border-radius: 10px;
      overflow: hidden;
    }

    .progress-bar-fill {
      width: 0%;
      height: 100%;
      background: linear-gradient(90deg, var(--accent-cyan), var(--accent-green));
      transition: width 0.3s ease;
    }

    .log-text {
      font-size: 0.78rem;
      color: var(--text-muted);
      text-align: center;
      min-height: 20px;
    }
  </style>
</head>
<body>

  <header>
    <div class="title-area">
      <h1>3D TURNTABLE SCANNER</h1>
      <p>ESP32-S3 Pro Photogrammetry Hub</p>
    </div>
    <div id="statusBadge" class="status-badge">IDLE</div>
  </header>

  <main>
    <!-- Live Camera & Cropping Viewport -->
    <div class="viewport-card" id="viewportCard">
      <video id="cameraPreview" autoplay playsinline muted></video>
      <div id="cropOverlay">
        <div class="resize-handle resize-se" id="resizeHandle"></div>
      </div>
    </div>

    <!-- Crop Bar Info & Reset -->
    <div class="crop-bar">
      <span id="cropInfo">Crop: 70% x 70% (Center)</span>
      <div style="display: flex; gap: 8px;">
        <button type="button" id="btnSwitchLens" onclick="switchCameraLens()" style="border-color: rgba(0, 240, 255, 0.4); color: var(--accent-cyan);">📷 Normal Lens</button>
        <button type="button" id="btnFlash" onclick="toggleFlash()" style="border-color: rgba(255, 215, 0, 0.4); color: #ffd700;">⚡ Flash: OFF</button>
        <button type="button" onclick="resetCrop()">Reset Box</button>
      </div>
    </div>

    <!-- Scan Parameters Card -->
    <div class="params-card" id="paramsCard">
      <h2>Scan Parameters <span style="font-size: 0.75rem; color: var(--text-muted);">L298N Turntable</span></h2>
      <div class="param-grid">
        <div class="param-item">
          <label for="inputDegree">Step Degree</label>
          <div class="input-wrapper">
            <input type="number" id="inputDegree" value="2" min="1" max="360">
            <span class="unit">°</span>
          </div>
        </div>

        <div class="param-item">
          <label for="inputSpeed">Motor PWM Speed</label>
          <div class="input-wrapper">
            <input type="number" id="inputSpeed" value="200" min="0" max="255">
            <span class="unit">0-255</span>
          </div>
        </div>

        <div class="param-item">
          <label for="inputPhotos">Total Photos</label>
          <div class="input-wrapper">
            <input type="number" id="inputPhotos" value="36" min="1" max="360">
            <span class="unit">qty</span>
          </div>
        </div>

        <div class="param-item">
          <label for="inputDelay">Stabilization Delay</label>
          <div class="input-wrapper">
            <input type="number" id="inputDelay" value="500" min="100" max="10000">
            <span class="unit">ms</span>
          </div>
        </div>
      </div>

      <div class="actions-bar">
        <button id="btnStart" class="btn btn-start" onclick="startScan()">
          START SCAN
        </button>
        <button id="btnAbort" class="btn btn-abort" onclick="abortScan()" disabled>
          ABORT
        </button>
      </div>
    </div>

    <!-- Live Scan Progress Dashboard -->
    <div class="progress-card" id="progressCard">
      <div class="progress-header">
        <span>Scan Progress</span>
        <strong id="progressCounter">0 / 36 Photos</strong>
      </div>
      <div class="progress-bar-bg">
        <div class="progress-bar-fill" id="progressBar"></div>
      </div>
      <div class="log-text" id="logText">Ready to start photogrammetry scan...</div>
    </div>
  </main>

  <!-- Offscreen canvas for cropping captured frames -->
  <canvas id="captureCanvas" style="display: none;"></canvas>

  <script>
    // Camera Stream Initialization
    const video = document.getElementById('cameraPreview');
    const cropOverlay = document.getElementById('cropOverlay');
    const resizeHandle = document.getElementById('resizeHandle');
    const viewportCard = document.getElementById('viewportCard');
    const cropInfo = document.getElementById('cropInfo');
    const captureCanvas = document.getElementById('captureCanvas');

    let videoStream = null;
    let scanActive = false;
    let totalPhotos = 36;
    let pollInterval = null;

    // Crop box percentages (0.0 to 1.0 relative to video view)
    let cropBox = { left: 0.15, top: 0.15, width: 0.70, height: 0.70 };

    let videoDevices = [];
    let currentDeviceIndex = 0;

    async function initCamera(preferredDeviceId = null) {
      try {
        if (!preferredDeviceId && videoDevices.length === 0) {
          // Get temporary stream to unlock full device labels
          const tempStream = await navigator.mediaDevices.getUserMedia({ video: { facingMode: 'environment' } });
          const devices = await navigator.mediaDevices.enumerateDevices();
          videoDevices = devices.filter(d => d.kind === 'videoinput');
          
          // Filter for rear cameras (avoid front/selfie cameras)
          let backCameras = videoDevices.filter(d => {
            const label = d.label.toLowerCase();
            return !label.includes('front') && !label.includes('user') && !label.includes('selfie');
          });
          if (backCameras.length === 0) backCameras = videoDevices;

          // Prefer standard/main back lens over ultra-wide / 0.5x / macro
          let mainCam = backCameras.find(d => {
            const label = d.label.toLowerCase();
            return !label.includes('ultra') && !label.includes('0.5') && !label.includes('macro');
          });
          if (!mainCam && backCameras.length > 0) mainCam = backCameras[0];
          if (mainCam) {
            preferredDeviceId = mainCam.deviceId;
            currentDeviceIndex = videoDevices.findIndex(d => d.deviceId === mainCam.deviceId);
          }
          tempStream.getTracks().forEach(t => t.stop());
        }

        const constraints = {
          video: preferredDeviceId ? {
            deviceId: { exact: preferredDeviceId },
            width: { ideal: 1920 },
            height: { ideal: 1080 }
          } : {
            facingMode: 'environment',
            width: { ideal: 1920 },
            height: { ideal: 1080 }
          },
          audio: false
        };

        if (videoStream) {
          videoStream.getTracks().forEach(t => t.stop());
        }

        const stream = await navigator.mediaDevices.getUserMedia(constraints);
        video.srcObject = stream;
        videoStream = stream;
        updateLensButtonText();
      } catch (err) {
        console.error("Camera access failed:", err);
        document.getElementById('logText').innerText = "Camera error: " + err.message;
      }
    }

    let torchState = false;

    function switchCameraLens() {
      if (videoDevices.length <= 1) {
        alert("Only one camera lens detected on this device.");
        return;
      }
      torchState = false;
      const flashBtn = document.getElementById('btnFlash');
      if (flashBtn) {
        flashBtn.innerText = "⚡ Flash: OFF";
        flashBtn.style.background = "transparent";
        flashBtn.style.color = "#ffd700";
      }
      currentDeviceIndex = (currentDeviceIndex + 1) % videoDevices.length;
      initCamera(videoDevices[currentDeviceIndex].deviceId);
    }

    function updateLensButtonText() {
      const btn = document.getElementById('btnSwitchLens');
      if (btn && videoDevices.length > 0) {
        const cam = videoDevices[currentDeviceIndex];
        let shortLabel = cam ? cam.label : "Default";
        if (shortLabel.length > 18) shortLabel = shortLabel.substring(0, 16) + "...";
        btn.innerText = "📷 " + shortLabel;
      }
    }

    async function toggleFlash() {
      if (!videoStream) return;
      const track = videoStream.getVideoTracks()[0];
      if (!track) return;
      try {
        torchState = !torchState;
        await track.applyConstraints({
          advanced: [{ torch: torchState }]
        });
        const btn = document.getElementById('btnFlash');
        if (btn) {
          btn.innerText = torchState ? "⚡ Flash: ON" : "⚡ Flash: OFF";
          btn.style.background = torchState ? "rgba(255, 215, 0, 0.25)" : "transparent";
          btn.style.color = torchState ? "#ffd700" : "var(--text-main)";
        }
      } catch (err) {
        console.warn("Torch not supported on this device/lens:", err);
        alert("Flash / Torch is not supported on the currently selected camera lens.");
        torchState = false;
      }
    }

    initCamera();

    function getDisplayedVideoRect() {
      const cw = viewportCard.clientWidth;
      const ch = viewportCard.clientHeight;
      const vw = video.videoWidth || 1920;
      const vh = video.videoHeight || 1080;

      const containerRatio = cw / ch;
      const videoRatio = vw / vh;

      let rw, rh, rx, ry;

      if (containerRatio > videoRatio) {
        // Pillarboxed (black bars on left and right)
        rh = ch;
        rw = ch * videoRatio;
        ry = 0;
        rx = (cw - rw) / 2;
      } else {
        // Letterboxed (black bars on top and bottom)
        rw = cw;
        rh = cw / videoRatio;
        rx = 0;
        ry = (ch - rh) / 2;
      }

      return { rw, rh, rx, ry, vw, vh };
    }

    function updateCropOverlay() {
      const { rw, rh, rx, ry } = getDisplayedVideoRect();
      const leftPx = rx + (cropBox.left * rw);
      const topPx = ry + (cropBox.top * rh);
      const widthPx = cropBox.width * rw;
      const heightPx = cropBox.height * rh;

      cropOverlay.style.left = leftPx + "px";
      cropOverlay.style.top = topPx + "px";
      cropOverlay.style.width = widthPx + "px";
      cropOverlay.style.height = heightPx + "px";
      cropInfo.innerText = `Crop: ${Math.round(cropBox.width * 100)}% x ${Math.round(cropBox.height * 100)}%`;
    }

    function resetCrop() {
      cropBox = { left: 0.05, top: 0.05, width: 0.90, height: 0.90 };
      updateCropOverlay();
    }

    window.addEventListener('resize', updateCropOverlay);
    setTimeout(updateCropOverlay, 500);
    setInterval(updateCropOverlay, 1000); // keep in sync once camera metadata loads

    // Interactive crop drag & resize
    let isDragging = false;
    let isResizing = false;
    let startX, startY, startLeft, startTop, startW, startH;

    cropOverlay.addEventListener('pointerdown', (e) => {
      if (e.target === resizeHandle) {
        isResizing = true;
      } else {
        isDragging = true;
      }
      startX = e.clientX;
      startY = e.clientY;
      startLeft = cropBox.left;
      startTop = cropBox.top;
      startW = cropBox.width;
      startH = cropBox.height;
      cropOverlay.setPointerCapture(e.pointerId);
    });

    cropOverlay.addEventListener('pointermove', (e) => {
      const { rw, rh } = getDisplayedVideoRect();

      if (isDragging) {
        let dx = (e.clientX - startX) / rw;
        let dy = (e.clientY - startY) / rh;
        let nl = Math.max(0, Math.min(1.0 - startW, startLeft + dx));
        let nt = Math.max(0, Math.min(1.0 - startH, startTop + dy));
        cropBox.left = nl;
        cropBox.top = nt;
        updateCropOverlay();
      } else if (isResizing) {
        let dx = (e.clientX - startX) / rw;
        let dy = (e.clientY - startY) / rh;
        let nw = Math.max(0.1, Math.min(1.0 - startLeft, startW + dx));
        let nh = Math.max(0.1, Math.min(1.0 - startTop, startH + dy));
        cropBox.width = nw;
        cropBox.height = nh;
        updateCropOverlay();
      }
    });

    cropOverlay.addEventListener('pointerup', () => {
      isDragging = false;
      isResizing = false;
    });

    // Capture & Crop Image (Up to 1MB high quality, compress only if >1MB)
    async function captureCroppedPhoto() {
      if (!video.videoWidth || !video.videoHeight) {
        throw new Error("Video not ready");
      }
      const vw = video.videoWidth;
      const vh = video.videoHeight;

      // Calculate actual pixel coords in source video stream
      const srcX = Math.round(cropBox.left * vw);
      const srcY = Math.round(cropBox.top * vh);
      const srcW = Math.round(cropBox.width * vw);
      const srcH = Math.round(cropBox.height * vh);

      captureCanvas.width = srcW;
      captureCanvas.height = srcH;

      const ctx = captureCanvas.getContext('2d');
      ctx.drawImage(video, srcX, srcY, srcW, srcH, 0, 0, srcW, srcH);

      const qualities = [1.00, 0.95, 0.90, 0.85, 0.80, 0.75];
      for (let q of qualities) {
        const blob = await new Promise((resolve) => {
          captureCanvas.toBlob(resolve, 'image/jpeg', q);
        });
        if (blob && blob.size <= 1000000) {
          return blob;
        }
      }
      return new Promise((resolve, reject) => {
        captureCanvas.toBlob((blob) => {
          if (blob) resolve(blob);
          else reject(new Error("Canvas blob error"));
        }, 'image/jpeg', 0.70);
      });
    }

    // Start Scan Workflow
    async function startScan() {
      const degree = parseInt(document.getElementById('inputDegree').value) || 10;
      const speed = parseInt(document.getElementById('inputSpeed').value) || 200;
      totalPhotos = parseInt(document.getElementById('inputPhotos').value) || 36;
      const delay = parseInt(document.getElementById('inputDelay').value) || 1500;

      const payload = {
        degree: degree,
        speed: speed,
        total_photos: totalPhotos,
        delay: delay
      };

      try {
        const resp = await fetch('/start_scan', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });

        if (resp.ok) {
          scanActive = true;
          document.getElementById('btnStart').disabled = true;
          document.getElementById('btnAbort').disabled = false;
          document.getElementById('progressCard').style.display = 'flex';
          document.getElementById('statusBadge').innerText = "SCANNING";
          document.getElementById('statusBadge').className = "status-badge scanning";
          document.getElementById('logText').innerText = "Scan session started. Rotating turntable...";
          
          startPolling();
        } else {
          alert("Failed to start scan on ESP32.");
        }
      } catch (err) {
        console.error("Start scan error:", err);
        alert("Network error communicating with ESP32.");
      }
    }

    // Abort Scan
    async function abortScan() {
      if (!confirm("Are you sure you want to abort the current scan?")) return;
      try {
        await fetch('/abort', { method: 'POST' });
      } catch (e) {
        console.warn("Abort signal sent:", e);
      }
      window.location.reload();
    }

    // Polling /status loop
    let lastHandledIndex = -1;

    function startPolling() {
      if (pollInterval) clearInterval(pollInterval);
      pollInterval = setInterval(async () => {
        if (!scanActive) return;
        try {
          const res = await fetch('/status');
          if (!res.ok) return;
          const data = await res.json();

          updateProgress(data.current_index, data.total_photos, data.state_name);

          // If ESP32 is READY_FOR_PHOTO for a new index
          if (data.state === 3 && data.current_index !== lastHandledIndex) {
            lastHandledIndex = data.current_index;
            document.getElementById('logText').innerText = `Capturing photo ${data.current_index} of ${totalPhotos}...`;
            
            try {
              const photoBlob = await captureCroppedPhoto();
              await uploadPhoto(photoBlob, data.current_index);
            } catch (captureErr) {
              console.error("Photo capture/upload error:", captureErr);
              document.getElementById('logText').innerText = "Upload failed: " + captureErr.message;
            }
          }

          if (data.state === 5) {
            // COMPLETE
            scanActive = false;
            clearInterval(pollInterval);
            document.getElementById('statusBadge').innerText = "COMPLETE";
            document.getElementById('statusBadge').className = "status-badge";
            document.getElementById('logText').innerText = "Scan Complete! RealityScan pipeline launched on laptop.";
            document.getElementById('btnStart').disabled = false;
            document.getElementById('btnAbort').disabled = true;
          }
        } catch (err) {
          console.warn("Status polling error:", err);
        }
      }, 500);
    }

    async function uploadPhoto(blob, index) {
      document.getElementById('logText').innerText = `Uploading photo ${index} to ESP32 PSRAM...`;
      const formData = new FormData();
      formData.append("photo", blob, `photo_${index}.jpg`);

      const resp = await fetch('/upload', {
        method: 'POST',
        body: formData
      });

      if (resp.ok) {
        document.getElementById('logText').innerText = `Photo ${index} relayed to Laptop successfully!`;
      } else {
        throw new Error(`HTTP ${resp.status}`);
      }
    }

    function updateProgress(current, total, stateName) {
      if (!total) total = 36;
      const pct = Math.min(100, Math.round((current / total) * 100));
      document.getElementById('progressCounter').innerText = `${current} / ${total} Photos`;
      document.getElementById('progressBar').style.width = pct + "%";
      if (stateName) {
        document.getElementById('statusBadge').innerText = stateName;
      }
    }
  </script>
</body>
</html>
)rawliteral";

#endif // INDEX_HTML_H
