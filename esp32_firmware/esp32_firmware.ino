#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include "index_html.h"

// ==========================================
// WIFI & HOST LAPTOP CONFIGURATION
// ==========================================
const char* WIFI_SSID     = "3D_SCANNER_HOST";     // SSID of your Windows 2.4GHz Hotspot
const char* WIFI_PASSWORD = "your_hotspot_password";
const char* LAPTOP_URL    = "http://192.168.137.1:8000";

// ==========================================
// UNCHANGEABLE L298N PINOUT & HARDWARE
// ==========================================
#define MOTOR_PIN1  4   // GPIO 4 (Header P1, Pin 4) -> IN1
#define MOTOR_PIN2  5   // GPIO 5 (Header P1, Pin 5) -> IN2
#define ENABLE_PIN  6   // GPIO 6 (Header P1, Pin 6) -> ENA (PWM)

// ==========================================
// PSRAM IMAGE RELAY BUFFER
// ==========================================
uint8_t* psramImageBuffer = NULL;
size_t   psramImageLen = 0;
size_t   psramImageCapacity = 0;
const size_t DEFAULT_PSRAM_BUF_SIZE = 2 * 1024 * 1024; // 2MB dedicated buffer in 8MB PSRAM

// ==========================================
// SCAN STATE MACHINE
// ==========================================
enum ScanState {
  STATE_IDLE = 0,
  STATE_ROTATING = 1,
  STATE_STABILIZING = 2,
  STATE_READY_FOR_PHOTO = 3,
  STATE_RELAYING = 4,
  STATE_COMPLETE = 5,
  STATE_ABORTED = 6
};

ScanState currentState = STATE_IDLE;
int currentPhotoIndex = 0;
int totalPhotos = 36;
int scanDegreeStep = 10;
int scanSpeedPWM = 200;
unsigned long scanDelayMs = 1500;
unsigned long stabilizationStartTime = 0;

WebServer server(80);

// ==========================================
// OFFICIAL MOTOR ROTATION FUNCTION (PROVIDED)
// ==========================================
void rotateMotor(int degrees, int speed = 255) {
  speed = constrain(speed, 0, 255);
  Serial.printf("[MOTOR] Rotating turntable by %d degrees at PWM speed %d...\n", degrees, speed);
  int durationMs = abs(degrees) * 50; 
  
  if (degrees >= 0) {
    digitalWrite(MOTOR_PIN1, HIGH);
    digitalWrite(MOTOR_PIN2, LOW);
  } else {
    digitalWrite(MOTOR_PIN1, LOW);
    digitalWrite(MOTOR_PIN2, HIGH);
  }
  
  // Apply PWM speed to ENABLE_PIN (0 = stop, 255 = 100% full speed)
  analogWrite(ENABLE_PIN, speed);
  
  delay(durationMs);
  
  // Clean Brake
  analogWrite(ENABLE_PIN, 0);
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  Serial.println("[MOTOR] Rotation Complete. Ready for Photo!");
}

// ==========================================
// HELPER: LIGHTWEIGHT JSON VALUE EXTRACTOR
// ==========================================
int extractIntFromJson(const String& json, const char* key, int defaultVal) {
  String searchKey = String("\"") + key + "\":";
  int idx = json.indexOf(searchKey);
  if (idx == -1) return defaultVal;
  idx += searchKey.length();
  while (idx < json.length() && (json[idx] == ' ' || json[idx] == '\t')) idx++;
  String numStr = "";
  while (idx < json.length() && (isDigit(json[idx]) || json[idx] == '-')) {
    numStr += json[idx];
    idx++;
  }
  return numStr.length() > 0 ? numStr.toInt() : defaultVal;
}

const char* getStateName(ScanState state) {
  switch(state) {
    case STATE_IDLE: return "IDLE";
    case STATE_ROTATING: return "ROTATING";
    case STATE_STABILIZING: return "STABILIZING";
    case STATE_READY_FOR_PHOTO: return "READY_FOR_PHOTO";
    case STATE_RELAYING: return "RELAYING_TO_LAPTOP";
    case STATE_COMPLETE: return "COMPLETE";
    case STATE_ABORTED: return "ABORTED";
    default: return "UNKNOWN";
  }
}

// ==========================================
// LAPTOP COMMUNICATION HELPERS
// ==========================================
bool notifyLaptopSessionStart(int expectedCount) {
  HTTPClient http;
  String url = String(LAPTOP_URL) + "/session_start";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  String payload = "{\"expected_photos\":" + String(expectedCount) + "}";
  int httpCode = http.POST(payload);
  http.end();
  
  if (httpCode == 200 || httpCode == 201) {
    Serial.println("[LAPTOP] Session started successfully on Laptop backend.");
    return true;
  } else {
    Serial.printf("[LAPTOP] ERROR: /session_start returned HTTP %d\n", httpCode);
    return false;
  }
}

bool relayImageToLaptop() {
  if (psramImageBuffer == NULL || psramImageLen == 0) {
    Serial.println("[RELAY] ERROR: PSRAM buffer is empty!");
    return false;
  }
  
  Serial.printf("[RELAY] Sending photo %d/%d (%u bytes) from PSRAM to Laptop...\n", 
                currentPhotoIndex, totalPhotos, psramImageLen);
                
  HTTPClient http;
  String url = String(LAPTOP_URL) + "/upload_image";
  http.begin(url);
  http.addHeader("Content-Type", "image/jpeg");
  
  int httpCode = http.POST(psramImageBuffer, psramImageLen);
  http.end();
  
  if (httpCode == 200 || httpCode == 201) {
    Serial.printf("[RELAY] Photo %d relayed successfully!\n", currentPhotoIndex);
    return true;
  } else {
    Serial.printf("[RELAY] ERROR: Laptop /upload_image returned HTTP %d\n", httpCode);
    return false;
  }
}

// ==========================================
// WEBSERVER HANDLERS
// ==========================================
void handleRoot() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  String json = "{";
  json += "\"state\":" + String((int)currentState) + ",";
  json += "\"state_name\":\"" + String(getStateName(currentState)) + "\",";
  json += "\"current_index\":" + String(currentPhotoIndex) + ",";
  json += "\"total_photos\":" + String(totalPhotos);
  json += "}";
  server.send(200, "application/json", json);
}

void handleStartScan() {
  if (currentState != STATE_IDLE && currentState != STATE_COMPLETE && currentState != STATE_ABORTED) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Scan already in progress\"}");
    return;
  }
  
  String body = server.arg("plain");
  scanDegreeStep = extractIntFromJson(body, "degree", 10);
  scanSpeedPWM   = constrain(extractIntFromJson(body, "speed", 200), 0, 255);
  totalPhotos    = extractIntFromJson(body, "total_photos", 36);
  scanDelayMs    = extractIntFromJson(body, "delay", 1500);

  Serial.printf("[SCAN START] Config: degree=%d, speed=%d, total=%d, delay=%lums\n",
                scanDegreeStep, scanSpeedPWM, totalPhotos, scanDelayMs);

  // 1. Notify Laptop to clean directories and wait for images
  notifyLaptopSessionStart(totalPhotos);

  // 2. Reset scan progress
  currentPhotoIndex = 1;
  currentState = STATE_ROTATING;

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleAbort() {
  Serial.println("[ABORT] Scan aborted by user!");
  // Immediate clean motor brake
  analogWrite(ENABLE_PIN, 0);
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);

  currentState = STATE_ABORTED;
  server.send(200, "application/json", "{\"status\":\"aborted\"}");
}

// Upload endpoint finish handler
void handleUploadFinish() {
  server.send(200, "application/json", "{\"status\":\"ok\",\"bytes\":" + String(psramImageLen) + "}");
  // Trigger relaying state in main loop
  if (currentState == STATE_READY_FOR_PHOTO) {
    currentState = STATE_RELAYING;
  }
}

// Upload streaming file handler (chunked binary receive into PSRAM)
void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    psramImageLen = 0;
    Serial.printf("[UPLOAD START] Receiving file: %s\n", upload.filename.c_str());
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (psramImageBuffer != NULL) {
      if (psramImageLen + upload.currentSize <= psramImageCapacity) {
        memcpy(psramImageBuffer + psramImageLen, upload.buf, upload.currentSize);
        psramImageLen += upload.currentSize;
      } else {
        Serial.println("[UPLOAD ERROR] PSRAM buffer overflow! Image exceeds allocated capacity.");
      }
    } else {
      Serial.println("[UPLOAD ERROR] PSRAM image buffer is NULL!");
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("[UPLOAD END] Received %u total bytes into PSRAM buffer.\n", psramImageLen);
  }
}

// ==========================================
// ARDUINO SETUP & LOOP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== 3D TURNTABLE SCANNER (YD-ESP32-S3-N16R8) ===");

  // 1. Configure Motor Pins
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN1, LOW);
  digitalWrite(MOTOR_PIN2, LOW);
  analogWrite(ENABLE_PIN, 0);

  // 2. Initialize PSRAM Safely
  if (psramInit()) {
    Serial.println("[PSRAM] PSRAM initialized successfully.");
    psramImageBuffer = (uint8_t*)ps_malloc(DEFAULT_PSRAM_BUF_SIZE);
    if (psramImageBuffer == NULL) {
      Serial.println("[PSRAM] ERROR: ps_malloc failed! Falling back to 512KB DRAM.");
      psramImageBuffer = (uint8_t*)malloc(512 * 1024);
      psramImageCapacity = 512 * 1024;
    } else {
      psramImageCapacity = DEFAULT_PSRAM_BUF_SIZE;
      Serial.printf("[PSRAM] Allocated %u bytes (2MB) in PSRAM for JPEG image relay buffer.\n", psramImageCapacity);
    }
  } else {
    Serial.println("[PSRAM] WARNING: PSRAM not available! Falling back to 512KB DRAM.");
    psramImageBuffer = (uint8_t*)malloc(512 * 1024);
    psramImageCapacity = 512 * 1024;
  }

  // 3. Connect to Host Laptop Wi-Fi Hotspot
  Serial.printf("[WIFI] Connecting to Windows Hotspot SSID: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WIFI] Connected! ESP32 IP Address: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("\n[WIFI] WARNING: Wi-Fi connection timed out. Check hotspot credentials.");
  }

  // 4. Configure WebServer Endpoints
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/start_scan", HTTP_POST, handleStartScan);
  server.on("/abort", HTTP_POST, handleAbort);
  server.on("/upload", HTTP_POST, handleUploadFinish, handleFileUpload);

  server.begin();
  Serial.println("[HTTP] ESP32 Web Server started on port 80.");
}

void loop() {
  server.handleClient();

  // State Machine Step Logic
  switch (currentState) {
    case STATE_ROTATING:
      Serial.printf("[STEP] Photo index %d/%d -> Rotating motor by %d deg...\n",
                    currentPhotoIndex, totalPhotos, scanDegreeStep);
      rotateMotor(scanDegreeStep, scanSpeedPWM);
      stabilizationStartTime = millis();
      currentState = STATE_STABILIZING;
      break;

    case STATE_STABILIZING:
      if (millis() - stabilizationStartTime >= scanDelayMs) {
        Serial.printf("[STEP] Turntable stabilized! Ready for photo %d.\n", currentPhotoIndex);
        currentState = STATE_READY_FOR_PHOTO;
      }
      break;

    case STATE_READY_FOR_PHOTO:
      // Waiting for Phone Web App to poll /status, capture frame, and POST /upload
      break;

    case STATE_RELAYING:
      Serial.printf("[STEP] Relaying photo %d to Windows Laptop...\n", currentPhotoIndex);
      if (relayImageToLaptop()) {
        if (currentPhotoIndex >= totalPhotos) {
          Serial.println("[SCAN] All photos captured and relayed! Scan COMPLETE.");
          currentState = STATE_COMPLETE;
        } else {
          currentPhotoIndex++;
          currentState = STATE_ROTATING;
        }
      } else {
        Serial.println("[STEP ERROR] Relay failed. Retrying after delay...");
        delay(1000);
      }
      break;

    default:
      break;
  }
}
