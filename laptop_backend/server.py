# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "fastapi>=0.110.0",
#     "uvicorn>=0.28.0",
# ]
# ///

import os
import shutil
import subprocess
import logging
from pathlib import Path
from typing import Optional
from fastapi import FastAPI, Request, HTTPException, BackgroundTasks
from fastapi.responses import JSONResponse
from pydantic import BaseModel
import uvicorn

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    handlers=[logging.StreamHandler()]
)
logger = logging.getLogger("3DScannerBackend")

app = FastAPI(title="3D Turntable Scanner - Laptop Backend", version="1.0.0")

# ==========================================
# UNCHANGEABLE REALITYSCAN & STORAGE PATHS
# ==========================================
REALITYSCAN_EXE = Path(r"E:\Epic Games\RealityScan_2.2\RealityScan.exe")
PHOTOS_DIR = Path(r"E:\3D\photos")
MODELS_DIR = Path(r"E:\3D\models")
CACHE_DIR = Path(r"E:\3D\RealityScanCache")
PROJECT_FILE = Path(r"E:\3D\RealityScanProject\AutoScan.rsc")

from datetime import datetime

# Internal session state
class ScanSessionState:
    expected_photos: int = 0
    received_photos: int = 0
    is_scanning: bool = False
    realityscan_running: bool = False
    session_id: str = "default"
    photos_dir: Path = PHOTOS_DIR
    models_dir: Path = MODELS_DIR

session_state = ScanSessionState()


class StartSessionRequest(BaseModel):
    expected_photos: int


def clean_directory(dir_path: Path):
    """Safely removes all files and subdirectories inside the given directory."""
    if not dir_path.exists():
        dir_path.mkdir(parents=True, exist_ok=True)
        logger.info(f"Created directory: {dir_path}")
        return

    logger.info(f"Cleaning contents of directory: {dir_path}")
    for item in dir_path.iterdir():
        try:
            if item.is_dir():
                shutil.rmtree(item, ignore_errors=True)
            else:
                item.unlink(missing_ok=True)
        except Exception as e:
            logger.warning(f"Could not remove {item}: {e}")


def run_realityscan_pipeline():
    """Executes RealityScan CLI pipeline asynchronously."""
    if not REALITYSCAN_EXE.exists():
        logger.error(f"RealityScan executable not found at {REALITYSCAN_EXE}!")
        session_state.realityscan_running = False
        return

    session_state.models_dir.mkdir(parents=True, exist_ok=True)
    obj_path = str(session_state.models_dir / "texturedMesh.obj")

    PROJECTS_DIR = Path(r"E:\3D\RealityScanProject")
    session_proj_dir = PROJECTS_DIR / f"scan_{session_state.session_id}"
    session_proj_dir.mkdir(parents=True, exist_ok=True)
    project_path = str(session_proj_dir / f"scan_{session_state.session_id}.rsc")

    cmd = [
        str(REALITYSCAN_EXE),
        "-newScene",
        "-addFolder",
        str(session_state.photos_dir),
        "-align",
        "-downscale=1",
        "-calculateHighModel",
        "-calculateTexture",
        "-textureSize=4096",
        "-saveProject",
        project_path,
        "-exportModel",
        "Model 1",
        obj_path,
        "-save",
        "-quit"
    ]

    logger.info(f"Launching RealityScan 2.2 CLI:\n{' '.join(cmd)}")
    session_state.realityscan_running = True

    try:
        # Launch as subprocess without blocking FastAPI
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0
        )
        logger.info(f"RealityScan started with PID {process.pid}")
        # Note: Background monitoring can be added if needed
    except Exception as e:
        logger.error(f"Failed to launch RealityScan: {e}")
        session_state.realityscan_running = False


@app.post("/session_start")
async def session_start(req: StartSessionRequest):
    """
    Called by ESP32 when 'Start Scan' is pressed on the Web App.
    Clears cache, models, and photos directories and resets progress counters.
    """
    logger.info(f"[SESSION START] Starting scan session for {req.expected_photos} photos.")
    
    # 1. Create timestamped unique scan session folders so all previous images & models are kept forever!
    session_id = datetime.now().strftime("%Y%m%d_%H%M%S")
    session_state.session_id = session_id
    session_state.photos_dir = PHOTOS_DIR / f"scan_{session_id}"
    session_state.models_dir = MODELS_DIR / f"scan_{session_id}"
    session_state.photos_dir.mkdir(parents=True, exist_ok=True)
    session_state.models_dir.mkdir(parents=True, exist_ok=True)

    # 2. Only clean RealityScan cache directory so old scans are never deleted
    clean_directory(CACHE_DIR)
    PROJECT_FILE.parent.mkdir(parents=True, exist_ok=True)

    # 3. Reset internal state
    session_state.expected_photos = req.expected_photos
    session_state.received_photos = 0
    session_state.is_scanning = True
    session_state.realityscan_running = False

    return {
        "status": "ok",
        "message": f"Session scan_{session_id} initialized. All photos and models preserved.",
        "expected_photos": session_state.expected_photos,
        "session_id": session_id
    }


@app.post("/upload_image")
async def upload_image(request: Request, background_tasks: BackgroundTasks):
    """
    Called by ESP32 to relay a JPEG captured by the phone.
    Accepts raw JPEG bytes in HTTP body (or standard binary upload).
    """
    body = await request.body()
    if not body:
        raise HTTPException(status_code=400, detail="Empty image payload received")

    session_state.photos_dir.mkdir(parents=True, exist_ok=True)
    session_state.received_photos += 1
    index = session_state.received_photos

    # Format filename cleanly inside session_photos_dir: photo_0001.jpg, etc.
    filename = session_state.photos_dir / f"photo_{index:04d}.jpg"
    
    try:
        with open(filename, "wb") as f:
            f.write(body)
        logger.info(f"[UPLOAD] Saved {filename.name} ({len(body)} bytes) -> [{index}/{session_state.expected_photos}]")
    except Exception as e:
        logger.error(f"Error saving image {filename}: {e}")
        raise HTTPException(status_code=500, detail="Failed to save image on laptop")

    triggered_scan = False
    if session_state.expected_photos > 0 and session_state.received_photos >= session_state.expected_photos:
        logger.info("[COMPLETE] All expected photos received! Triggering RealityScan pipeline...")
        session_state.is_scanning = False
        triggered_scan = True
        background_tasks.add_task(run_realityscan_pipeline)

    return {
        "status": "ok",
        "received_photos": session_state.received_photos,
        "expected_photos": session_state.expected_photos,
        "triggered_scan": triggered_scan
    }


@app.post("/session_complete")
async def session_complete(background_tasks: BackgroundTasks):
    """
    Explicit endpoint to trigger RealityScan on demand.
    """
    logger.info("[MANUAL COMPLETE] Triggering RealityScan pipeline via /session_complete endpoint...")
    session_state.is_scanning = False
    background_tasks.add_task(run_realityscan_pipeline)
    return {"status": "ok", "message": "RealityScan pipeline triggered in background."}


@app.get("/status")
async def get_status():
    """Returns current server state for diagnostics."""
    return {
        "is_scanning": session_state.is_scanning,
        "received_photos": session_state.received_photos,
        "expected_photos": session_state.expected_photos,
        "realityscan_running": session_state.realityscan_running,
        "photos_dir": str(PHOTOS_DIR),
        "models_dir": str(MODELS_DIR)
    }


if __name__ == "__main__":
    logger.info("Starting 3D Scanner Laptop Backend Server on 0.0.0.0:8000...")
    logger.info("Ensure the Windows Hotspot is active (IP 192.168.137.1)")
    uvicorn.run("server:app", host="0.0.0.0", port=8000, reload=False)
