#include <Arduino.h>
#include <esp_camera.h>
#include <WiFi.h>
#include <esp_err.h>
#include "camera_settings.h"

// Standard ESP32-CAM pin definition - this is more likely to work with most camera modules
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Replace with your WiFi credentials
const char* ssid = "Kunovic";
const char* password = "55886622Kunovic";

// Print WiFi credentials to serial during initialization
void printWiFiCredentials() {
  Serial.println("WiFi Credentials:");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
}

// Web server port
WiFiServer server(80);

// Time-lapse settings
const unsigned long TIMELAPSE_INTERVAL = 60000; // 1 minute interval by default
unsigned long lastCaptureTime = 0;
bool timelapseActive = false;
bool cameraInitialized = false;
String cameraErrorMsg = "";

// Helper function to translate ESP error codes to readable messages
const char* esp_err_to_name(esp_err_t code) {
  switch (code) {
    case ESP_OK: return "OK";
    case ESP_FAIL: return "Generic Failure";
    case ESP_ERR_NO_MEM: return "Out of memory";
    case ESP_ERR_INVALID_ARG: return "Invalid argument";
    case ESP_ERR_INVALID_STATE: return "Invalid state";
    case ESP_ERR_INVALID_SIZE: return "Invalid size";
    case ESP_ERR_NOT_FOUND: return "Not found";
    case ESP_ERR_NOT_SUPPORTED: return "Not supported";
    case ESP_ERR_TIMEOUT: return "Timeout";
    case ESP_ERR_INVALID_RESPONSE: return "Invalid response";
    case ESP_ERR_INVALID_CRC: return "Invalid CRC";
    case ESP_ERR_INVALID_VERSION: return "Invalid version";
    case ESP_ERR_INVALID_MAC: return "Invalid MAC address";
    default: return "Unknown error";
  }
}

void startCameraServer();

void checkPSRAM() {
  if (psramFound()) {
    size_t psramSize = ESP.getPsramSize();
    size_t freePsram = ESP.getFreePsram();
    Serial.println("PSRAM found!");
    Serial.printf("Total PSRAM: %d bytes\n", psramSize);
    Serial.printf("Free PSRAM: %d bytes\n", freePsram);
    
    if (psramSize < 1024*1024) {
      Serial.println("WARNING: PSRAM size is small, camera may have issues with higher resolutions");
    }
  } else {
    Serial.println("WARNING: PSRAM not found! Camera may not function correctly with high resolutions.");
  }
}

// Test I2C connection to camera
bool testCameraI2C() {
  Serial.println("Testing camera I2C connection...");
  
  // Hardware reset sequence
  pinMode(RESET_GPIO_NUM, OUTPUT);
  pinMode(PWDN_GPIO_NUM, OUTPUT);
  
  if (RESET_GPIO_NUM >= 0) {
    Serial.println("Performing hardware reset for camera");
    digitalWrite(RESET_GPIO_NUM, LOW);
    delay(10);
    digitalWrite(RESET_GPIO_NUM, HIGH);
    delay(100);
  }
  
  if (PWDN_GPIO_NUM >= 0) {
    Serial.println("Toggling camera power down pin");
    digitalWrite(PWDN_GPIO_NUM, HIGH);  // Power down
    delay(10);
    digitalWrite(PWDN_GPIO_NUM, LOW);   // Power up
    delay(100);
  }
  
  // Test I2C lines
  Serial.println("Testing I2C SDA and SCL pins");
  pinMode(SIOD_GPIO_NUM, OUTPUT);
  pinMode(SIOC_GPIO_NUM, OUTPUT);
  
  // Check if we can control the I2C lines (simple check for shorts or disconnects)
  for (int i = 0; i < 3; i++) {
    digitalWrite(SIOD_GPIO_NUM, LOW);
    digitalWrite(SIOC_GPIO_NUM, HIGH);
    delay(5);
    digitalWrite(SIOD_GPIO_NUM, HIGH);
    digitalWrite(SIOC_GPIO_NUM, LOW);
    delay(5);
  }
  
  // Set pins to INPUT_PULLUP for detection
  pinMode(SIOD_GPIO_NUM, INPUT_PULLUP);
  pinMode(SIOC_GPIO_NUM, INPUT_PULLUP);
  delay(50);
  
  // Both lines should be HIGH due to pull-ups
  bool sda_level = digitalRead(SIOD_GPIO_NUM);
  bool scl_level = digitalRead(SIOC_GPIO_NUM);
  
  Serial.printf("SDA pin state: %s\n", sda_level ? "HIGH (OK)" : "LOW (possible short)");
  Serial.printf("SCL pin state: %s\n", scl_level ? "HIGH (OK)" : "LOW (possible short)");
  
  if (!sda_level || !scl_level) {
    Serial.println("ERROR: I2C lines not at expected level - check for shorts or improper connections");
    cameraErrorMsg = "I2C pins shorted or improperly connected";
    return false;
  }
  
  // Now try to send a START condition (both lines HIGH, then SDA goes LOW while SCL is HIGH)
  pinMode(SIOD_GPIO_NUM, OUTPUT);
  pinMode(SIOC_GPIO_NUM, OUTPUT);
  
  // Initial state: both HIGH
  digitalWrite(SIOC_GPIO_NUM, HIGH);
  digitalWrite(SIOD_GPIO_NUM, HIGH);
  delay(10);
  
  // START condition: SDA goes LOW while SCL is HIGH
  digitalWrite(SIOD_GPIO_NUM, LOW);
  delay(10);
  
  // Return lines to HIGH state
  digitalWrite(SIOD_GPIO_NUM, HIGH);
  delay(10);
  
  // Configure the I2C pins with pullups using the ESP32's peripheral
  // Set pins back to INPUT for camera initialization
  pinMode(SIOD_GPIO_NUM, INPUT);
  pinMode(SIOC_GPIO_NUM, INPUT);
  
  Serial.println("I2C lines tested");
  return true;
}

bool initCamera() {
  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;   // 10MHz clock
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Use lowest settings for better compatibility
  config.frame_size = FRAMESIZE_QVGA;   // 320x240
  config.jpeg_quality = 12;           // 0-63 (lower is better quality)
  config.fb_count = 2;
  
  // Debug information
  Serial.println("Camera configuration:");
  Serial.printf("- frame_size: %d\n", config.frame_size);
  Serial.printf("- jpeg_quality: %d\n", config.jpeg_quality);
  Serial.printf("- fb_count: %d\n", config.fb_count);
  Serial.printf("- xclk_freq_hz: %d\n", config.xclk_freq_hz);
  
  // Initialize camera with delay
  delay(100);
  Serial.println("Initializing camera...");
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x: %s\n", err, esp_err_to_name(err));
    cameraErrorMsg = String("Camera init error: ") + esp_err_to_name(err);
    return false;
  }
  
  Serial.println("Camera initialized successfully!");
  
  // Check if we can capture a frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed after initialization");
    cameraErrorMsg = "Camera initialized but can't capture images";
    return false;
  }
  
  // Success! We got a frame
  Serial.printf("Camera working! Captured %d bytes\n", fb->len);
  esp_camera_fb_return(fb);
  
  // Configure sensor with optimized settings
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    // Apply more compatible settings
    s->set_brightness(s, 0);
    s->set_contrast(s, 0);
    s->set_saturation(s, 0);
    s->set_special_effect(s, 0);
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, 0);
    s->set_exposure_ctrl(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_aec2(s, 0);
    s->set_aec_value(s, 300);
    s->set_agc_gain(s, 0);
    s->set_gainceiling(s, (gainceiling_t)0);
    s->set_bpc(s, 0);
    s->set_wpc(s, 1);
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
    s->set_hmirror(s, 0);
    s->set_vflip(s, 0);
    s->set_dcw(s, 1);
    s->set_colorbar(s, 0);
    
    Serial.println("Camera settings applied");
  }
  
  return true;
}

// Function to test all data pins of the camera
void testCameraPins() {
  Serial.println("\n========= CAMERA PIN TESTS =========");
  Serial.println("Testing all camera pins for proper connection");
  
  // Setup all data pins as inputs with pullups 
  // to test if they're properly connected or floating
  const int dataPins[] = {
    Y2_GPIO_NUM, Y3_GPIO_NUM, Y4_GPIO_NUM, Y5_GPIO_NUM, 
    Y6_GPIO_NUM, Y7_GPIO_NUM, Y8_GPIO_NUM, Y9_GPIO_NUM,
    VSYNC_GPIO_NUM, HREF_GPIO_NUM, PCLK_GPIO_NUM
  };
  
  const char* pinNames[] = {
    "Y2 (D0)", "Y3 (D1)", "Y4 (D2)", "Y5 (D3)", 
    "Y6 (D4)", "Y7 (D5)", "Y8 (D6)", "Y9 (D7)",
    "VSYNC", "HREF", "PCLK"
  };
  
  bool allPinsOK = true;
  
  for (int i = 0; i < 11; i++) {
    if (dataPins[i] < 0) continue; // Skip unused pins
    
    pinMode(dataPins[i], INPUT_PULLUP);
    delay(1);
    bool value = digitalRead(dataPins[i]);
    
    Serial.printf("Pin %s (GPIO %d): %s\n", 
      pinNames[i], 
      dataPins[i], 
      value ? "HIGH (OK or disconnected)" : "LOW (possible short to GND)"
    );
    
    // Toggle as output to see if we can control it
    pinMode(dataPins[i], OUTPUT);
    digitalWrite(dataPins[i], LOW);
    delay(1);
    digitalWrite(dataPins[i], HIGH);
    delay(1);
    
    // Return to input pullup
    pinMode(dataPins[i], INPUT_PULLUP);
  }
  
  Serial.println("Testing I2C and clock pins");
  
  // Test XCLK
  pinMode(XCLK_GPIO_NUM, OUTPUT);
  for (int i = 0; i < 5; i++) {
    digitalWrite(XCLK_GPIO_NUM, HIGH);
    delayMicroseconds(100);
    digitalWrite(XCLK_GPIO_NUM, LOW);
    delayMicroseconds(100);
  }
  Serial.printf("XCLK (GPIO %d): Toggled\n", XCLK_GPIO_NUM);
  
  // Test I2C pins again
  pinMode(SIOD_GPIO_NUM, INPUT_PULLUP);
  pinMode(SIOC_GPIO_NUM, INPUT_PULLUP);
  delay(10);
  
  bool sda = digitalRead(SIOD_GPIO_NUM);
  bool scl = digitalRead(SIOC_GPIO_NUM);
  
  Serial.printf("SIOD/SDA (GPIO %d): %s\n", SIOD_GPIO_NUM, 
    sda ? "HIGH (OK or disconnected)" : "LOW (possible short to GND)");
  Serial.printf("SIOC/SCL (GPIO %d): %s\n", SIOC_GPIO_NUM, 
    scl ? "HIGH (OK or disconnected)" : "LOW (possible short to GND)");
  
  if (!sda || !scl) {
    Serial.println("WARNING: I2C pins not at expected level");
    allPinsOK = false;
  }
  
  // Reset all pins to input mode
  for (int i = 0; i < 11; i++) {
    if (dataPins[i] >= 0) {
      pinMode(dataPins[i], INPUT);
    }
  }
  pinMode(XCLK_GPIO_NUM, INPUT);
  pinMode(SIOD_GPIO_NUM, INPUT);
  pinMode(SIOC_GPIO_NUM, INPUT);
  
  Serial.println("Camera pin test complete");
  Serial.println("====================================\n");
}

// Add hardware reset function
void hardwareResetCamera() {
  Serial.println("Performing hardware reset of camera module...");
  
  // If reset pin is available, use it
  if (RESET_GPIO_NUM >= 0) {
    pinMode(RESET_GPIO_NUM, OUTPUT);
    digitalWrite(RESET_GPIO_NUM, LOW);  // Active low reset
    delay(100);
    digitalWrite(RESET_GPIO_NUM, HIGH);
    delay(100);
    Serial.println("Camera RESET pin toggled");
  }
  
  // If PWDN pin is available, use it to power cycle the camera
  if (PWDN_GPIO_NUM >= 0) {
    pinMode(PWDN_GPIO_NUM, OUTPUT);
    // Power down the camera
    digitalWrite(PWDN_GPIO_NUM, HIGH); // Active high power down
    delay(100);
    // Power up the camera
    digitalWrite(PWDN_GPIO_NUM, LOW);
    delay(100);
    Serial.println("Camera PWDN pin toggled for power cycle");
  }
  
  // Reset the XCLK line which provides the clock to the camera
  pinMode(XCLK_GPIO_NUM, OUTPUT);
  digitalWrite(XCLK_GPIO_NUM, LOW);
  delay(100);
  // Don't drive it high, just return to input to let the ESP32 peripheral handle it
  pinMode(XCLK_GPIO_NUM, INPUT);
  delay(100);
  Serial.println("Camera XCLK line reset");
  
  // Reset the I2C communication lines
  pinMode(SIOD_GPIO_NUM, OUTPUT);
  pinMode(SIOC_GPIO_NUM, OUTPUT);
  
  digitalWrite(SIOD_GPIO_NUM, LOW);
  digitalWrite(SIOC_GPIO_NUM, LOW);
  delay(100);
  
  digitalWrite(SIOD_GPIO_NUM, HIGH);
  digitalWrite(SIOC_GPIO_NUM, HIGH);
  delay(100);
  
  // Return to input mode
  pinMode(SIOD_GPIO_NUM, INPUT);
  pinMode(SIOC_GPIO_NUM, INPUT);
  
  Serial.println("Camera I2C lines reset");
  Serial.println("Camera hardware reset complete");
  
  // Add extra delay after reset
  delay(500);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("ESP32-WROVER-DEV Camera Time-Lapse Demo");

  // Check if PSRAM is available
  checkPSRAM();
  
  // Reset camera hardware before starting
  Serial.println("Performing initial camera hardware reset");
  hardwareResetCamera();
  
  // Test all camera pins
  testCameraPins();
  
  // Add helpful info about input-only pins
  Serial.println("\nNOTE: GPIOs 34-39 are INPUT-ONLY pins on ESP32.");
  Serial.println("If you're using these pins for camera data (Y6-Y9), they may not work correctly.");
  Serial.println("Please check your camera module documentation for compatible pin assignments.\n");

  // Initialize camera
  cameraInitialized = initCamera();
  if (!cameraInitialized) {
    Serial.println("Failed to initialize camera. Web interface will function without camera features.");
    Serial.println("Camera error: " + cameraErrorMsg);
    Serial.println("\nTROUBLESHOOTING TIPS:");
    Serial.println("1. Check if you are using the correct pin definitions for your specific camera module");
    Serial.println("2. Make sure all camera pins are properly connected (not loose/disconnected)");
    Serial.println("3. Verify that your camera module is compatible (OV2640, OV3660, etc.)");
    Serial.println("4. Test with a known-working camera module if possible");
    Serial.println("5. Try decreasing the XCLK frequency further in the code");
    Serial.println("6. Use the Reset and Diagnostics buttons in the web interface for more troubleshooting");
  } else {
    Serial.println("Camera initialization successful!");
  }
  
  printWiFiCredentials();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    
    // Start the server
    server.begin();
    Serial.println("Server started");
    
    // Print the IP address
    Serial.print("Camera Ready! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
    Serial.println("");
    Serial.println("For detailed diagnostics, visit http://" + WiFi.localIP().toString() + "/diagnose");
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed");
  }
}

void captureTimeLapseImage() {
  if (!cameraInitialized) {
    Serial.println("Cannot capture image: Camera not initialized");
    return;
  }
  
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  
  // Here you would normally save the image to storage
  // For this example, we'll just print that we captured an image
  Serial.println("Time-lapse image captured!");
  Serial.printf("Image size: %d bytes\n", fb->len);
  
  // Return the frame buffer back to the camera
  esp_camera_fb_return(fb);
}

void serveErrorPage(WiFiClient &client, String message) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<style>body{font-family:Arial;text-align:center;margin-top:50px}");
  client.println(".error{color:red;}</style>");
  client.println("</head><body>");
  client.println("<h1>ESP32-WROVER-DEV Camera</h1>");
  client.println("<div class=\"error\"><h2>Error</h2>");
  client.println("<p>" + message + "</p></div>");
  client.println("<p><a href=\"/\">Back to Home</a></p>");
  client.println("</body></html>");
}

void loop() {
  // Handle time-lapse capture if active
  if (timelapseActive && cameraInitialized) {
    unsigned long currentTime = millis();
    if (currentTime - lastCaptureTime >= TIMELAPSE_INTERVAL) {
      captureTimeLapseImage();
      lastCaptureTime = currentTime;
    }
  }
  
  // Handle web server client
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 5000) { // 5 second timeout
      if (client.available()) {
        timeout = millis();
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<style>body { font-family: Arial; text-align: center; }</style>");
            client.println("<title>ESP32-WROVER-DEV Camera</title>");
            client.println("</head><body>");
            client.println("<h1>ESP32-WROVER-DEV Camera Control</h1>");
            
            if (cameraInitialized) {
              client.println("<img src=\"/capture\" style=\"width:auto; max-width:100%; height:auto;\">");
              client.println("<p><a href=\"/capture\"><button>Capture Image</button></a></p>");
              if (timelapseActive) {
                client.println("<p><a href=\"/timelapse/stop\"><button style=\"background-color:red;\">Stop Time-lapse</button></a></p>");
              } else {
                client.println("<p><a href=\"/timelapse/start\"><button style=\"background-color:green;\">Start Time-lapse</button></a></p>");
              }
              client.println("<p><form action=\"/settings\" method=\"get\">");
              client.println("Quality (0-63): <input type=\"number\" name=\"quality\" min=\"0\" max=\"63\" value=\"15\"><br>");
              client.println("Brightness (-2 to 2): <input type=\"number\" name=\"brightness\" min=\"-2\" max=\"2\" value=\"0\"><br>");
              client.println("Contrast (-2 to 2): <input type=\"number\" name=\"contrast\" min=\"-2\" max=\"2\" value=\"0\"><br>");
              client.println("<input type=\"submit\" value=\"Apply Settings\">");
              client.println("</form></p>");
            } else {
              client.println("<div style=\"color:red; padding:20px; border:1px solid red;\">");
              client.println("<h2>Camera Error</h2>");
              client.println("<p>" + cameraErrorMsg + "</p>");
              client.println("<p>Check connections and restart the device.</p>");
              client.println("</div>");
            }
            
            // Add diagnostic info
            client.println("<h3>System Info</h3>");
            client.println("<p>IP Address: " + WiFi.localIP().toString() + "</p>");
            client.println("<p>PSRAM: " + String(psramFound() ? "Available" : "Not Available") + "</p>");
            if (psramFound()) {
              client.println("<p>PSRAM Size: " + String(ESP.getPsramSize() / 1024) + " KB, Free: " + String(ESP.getFreePsram() / 1024) + " KB</p>");
            }
            client.println("<p>Camera Status: " + String(cameraInitialized ? "Initialized" : "Failed") + "</p>");
            client.println("<p>WiFi RSSI: " + String(WiFi.RSSI()) + " dBm</p>");
            client.println("<p>Heap: Free=" + String(ESP.getFreeHeap() / 1024) + "KB, Min=" + String(ESP.getMinFreeHeap() / 1024) + "KB</p>");
            
            // Add reset and diagnostic buttons
            client.println("<div style=\"margin-top:20px;\">");
            client.println("<a href=\"/reset\"><button style=\"background-color:orange;\">Reset Camera Hardware</button></a>");
            client.println("<a href=\"/diagnose\" style=\"margin-left:10px;\"><button style=\"background-color:blue;color:white;\">Detailed Diagnostics</button></a>");
            client.println("</div>");
            
            client.println("</body></html>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        // Handle different URL requests
        if (currentLine.endsWith("GET /capture")) {
          if (!cameraInitialized) {
            serveErrorPage(client, "Camera not initialized: " + cameraErrorMsg);
            break;
          }
          
          camera_fb_t * fb = esp_camera_fb_get();
          if(!fb) {
            Serial.println("Camera capture failed");
            serveErrorPage(client, "Camera capture failed - camera may be disconnected or in reset state");
          } else {
            Serial.printf("Captured image: %d bytes\n", fb->len);
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: image/jpeg");
            client.println("Content-Length: " + String(fb->len));
            client.println();
            client.write(fb->buf, fb->len);
            esp_camera_fb_return(fb);
          }
          break;
        }
        
        // Time-lapse control
        if (currentLine.endsWith("GET /timelapse/start")) {
          if (cameraInitialized) {
            timelapseActive = true;
            lastCaptureTime = millis();
            Serial.println("Time-lapse started");
          } else {
            serveErrorPage(client, "Cannot start time-lapse: Camera not initialized - " + cameraErrorMsg);
          }
          break;
        }
        
        if (currentLine.endsWith("GET /timelapse/stop")) {
          timelapseActive = false;
          Serial.println("Time-lapse stopped");
          break;
        }
        
        // Camera settings
        if (currentLine.indexOf("GET /settings?") >= 0) {
          if (!cameraInitialized) {
            serveErrorPage(client, "Cannot change settings: Camera not initialized - " + cameraErrorMsg);
            break;
          }
          
          int qualityPos = currentLine.indexOf("quality=");
          int brightnessPos = currentLine.indexOf("brightness=");
          int contrastPos = currentLine.indexOf("contrast=");
          
          if (qualityPos >= 0) {
            String qualityStr = currentLine.substring(qualityPos + 8);
            qualityStr = qualityStr.substring(0, qualityStr.indexOf("&") > 0 ? qualityStr.indexOf("&") : qualityStr.indexOf(" "));
            int quality = qualityStr.toInt();
            setCameraQuality(quality);
            Serial.println("Quality set to: " + String(quality));
          }
          
          if (brightnessPos >= 0) {
            String brightnessStr = currentLine.substring(brightnessPos + 11);
            brightnessStr = brightnessStr.substring(0, brightnessStr.indexOf("&") > 0 ? brightnessStr.indexOf("&") : brightnessStr.indexOf(" "));
            int brightness = brightnessStr.toInt();
            setCameraBrightness(brightness);
            Serial.println("Brightness set to: " + String(brightness));
          }
          
          if (contrastPos >= 0) {
            String contrastStr = currentLine.substring(contrastPos + 9);
            contrastStr = contrastStr.substring(0, contrastStr.indexOf("&") > 0 ? contrastStr.indexOf("&") : contrastStr.indexOf(" "));
            int contrast = contrastStr.toInt();
            setCameraContrast(contrast);
            Serial.println("Contrast set to: " + String(contrast));
          }
          
          break;
        }
        
        // Add a diagnostic endpoint
        if (currentLine.endsWith("GET /diagnose")) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println("<!DOCTYPE html><html>");
          client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<style>body{font-family:Arial;text-align:left;margin:20px}");
          client.println("table{border-collapse:collapse;width:100%;}");
          client.println("th,td{border:1px solid #ddd;padding:8px;text-align:left;}");
          client.println("th{background-color:#f2f2f2;}</style>");
          client.println("<title>ESP32 Camera Diagnostics</title>");
          client.println("</head><body>");
          client.println("<h1>ESP32 Camera Diagnostics</h1>");
          
          client.println("<h3>System Info</h3>");
          client.println("<table>");
          client.println("<tr><th>Parameter</th><th>Value</th></tr>");
          client.println("<tr><td>ESP32 Chip Model</td><td>ESP32-WROVER</td></tr>");
          client.println("<tr><td>ESP32 Heap Size</td><td>" + String(ESP.getHeapSize() / 1024) + " KB</td></tr>");
          client.println("<tr><td>ESP32 Free Heap</td><td>" + String(ESP.getFreeHeap() / 1024) + " KB</td></tr>");
          client.println("<tr><td>ESP32 Min Free Heap</td><td>" + String(ESP.getMinFreeHeap() / 1024) + " KB</td></tr>");
          client.println("<tr><td>PSRAM Available</td><td>" + String(psramFound() ? "Yes" : "No") + "</td></tr>");
          if (psramFound()) {
            client.println("<tr><td>PSRAM Size</td><td>" + String(ESP.getPsramSize() / 1024) + " KB</td></tr>");
            client.println("<tr><td>Free PSRAM</td><td>" + String(ESP.getFreePsram() / 1024) + " KB</td></tr>");
          }
          client.println("<tr><td>Camera Initialized</td><td>" + String(cameraInitialized ? "Yes" : "No") + "</td></tr>");
          if (!cameraInitialized) {
            client.println("<tr><td>Camera Error</td><td>" + cameraErrorMsg + "</td></tr>");
          }
          client.println("<tr><td>WiFi RSSI</td><td>" + String(WiFi.RSSI()) + " dBm</td></tr>");
          client.println("</table>");
          
          client.println("<h3>Camera Pin Configuration</h3>");
          client.println("<table>");
          client.println("<tr><th>Function</th><th>GPIO Pin</th></tr>");
          client.println("<tr><td>PWDN</td><td>" + String(PWDN_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>RESET</td><td>" + String(RESET_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>XCLK</td><td>" + String(XCLK_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>SIOD (I2C SDA)</td><td>" + String(SIOD_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>SIOC (I2C SCL)</td><td>" + String(SIOC_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>Y9</td><td>" + String(Y9_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>Y8</td><td>" + String(Y8_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>Y7</td><td>" + String(Y7_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>Y6</td><td>" + String(Y6_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>Y5</td><td>" + String(Y5_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>Y4</td><td>" + String(Y4_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>Y3</td><td>" + String(Y3_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>Y2</td><td>" + String(Y2_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>VSYNC</td><td>" + String(VSYNC_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>HREF</td><td>" + String(HREF_GPIO_NUM) + "</td></tr>");
          client.println("<tr><td>PCLK</td><td>" + String(PCLK_GPIO_NUM) + "</td></tr>");
          client.println("</table>");
          
          client.println("<p><a href=\"/\">Back to Home</a></p>");
          client.println("</body></html>");
          break;
        }

        // In the loop function, add this new endpoint handler
        if (currentLine.endsWith("GET /reset")) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println("<!DOCTYPE html><html>");
          client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<style>body{font-family:Arial;text-align:center;margin-top:50px}");
          client.println(".message{color:blue;}</style>");
          client.println("</head><body>");
          client.println("<h1>ESP32-WROVER-DEV Camera</h1>");
          client.println("<div class=\"message\"><h2>Camera Reset</h2>");
          client.println("<p>Performing camera hardware reset...</p></div>");
          client.println("<p>Please wait, the page will refresh in 5 seconds.</p>");
          client.println("<script>setTimeout(function(){window.location.href='/';}, 5000);</script>");
          client.println("</body></html>");
          
          // Start the reset process after sending the response
          hardwareResetCamera();
          
          // Re-initialize the camera
          if (esp_camera_deinit() == ESP_OK) {
            Serial.println("Camera deinitialized successfully");
          } else {
            Serial.println("Failed to deinitialize camera");
          }
          
          delay(500);
          cameraInitialized = initCamera();
          if (cameraInitialized) {
            Serial.println("Camera reinitialized successfully!");
          } else {
            Serial.println("Camera reinitialization failed: " + cameraErrorMsg);
          }
          
          break;
        }
      }
    }
    client.stop();
    Serial.println("Client disconnected.");
  }
} 