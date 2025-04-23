#include <Arduino.h>
#include <esp_camera.h>
#include <WiFi.h>
#include <esp_err.h>
#include "camera_settings.h"

// ===============================================================
// IMPORTANT: Include one of the pin configuration files:
#include "camera_pins/wrover.h"
// #include "camera_pins/esp32cam.h"
// #include "camera_pins/alt_pins.h"
// ===============================================================

// Replace with your WiFi credentials
const char* ssid = "Kunovic";
const char* password = "55886622Kunovic";

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

// Print pin configuration
void printPinConfiguration() {
  Serial.println("\nCamera Pin Configuration:");
  Serial.printf("PWDN: GPIO %d\n", PWDN_GPIO_NUM);
  Serial.printf("RESET: GPIO %d\n", RESET_GPIO_NUM);
  Serial.printf("XCLK: GPIO %d\n", XCLK_GPIO_NUM);
  Serial.printf("SIOD: GPIO %d\n", SIOD_GPIO_NUM);
  Serial.printf("SIOC: GPIO %d\n", SIOC_GPIO_NUM);
  Serial.printf("Y9: GPIO %d\n", Y9_GPIO_NUM);
  Serial.printf("Y8: GPIO %d\n", Y8_GPIO_NUM);
  Serial.printf("Y7: GPIO %d\n", Y7_GPIO_NUM);
  Serial.printf("Y6: GPIO %d\n", Y6_GPIO_NUM);
  Serial.printf("Y5: GPIO %d\n", Y5_GPIO_NUM);
  Serial.printf("Y4: GPIO %d\n", Y4_GPIO_NUM);
  Serial.printf("Y3: GPIO %d\n", Y3_GPIO_NUM);
  Serial.printf("Y2: GPIO %d\n", Y2_GPIO_NUM);
  Serial.printf("VSYNC: GPIO %d\n", VSYNC_GPIO_NUM);
  Serial.printf("HREF: GPIO %d\n", HREF_GPIO_NUM);
  Serial.printf("PCLK: GPIO %d\n", PCLK_GPIO_NUM);
  Serial.println();
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
  
  // Use moderate settings for better compatibility
  config.frame_size = FRAMESIZE_QVGA;   // 320x240
  config.jpeg_quality = 12;             // 0-63 (lower is better quality)
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

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("ESP32 Camera Configurable Test");

  // Print the pin configuration being used
  printPinConfiguration();
  
  // Check if PSRAM is available
  checkPSRAM();

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
  } else {
    Serial.println("Camera initialization successful!");
  }

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
  client.println("<h1>ESP32-DEV Camera</h1>");
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
            client.println("<title>ESP32-DEV Camera</title>");
            client.println("</head><body>");
            client.println("<h1>ESP32-DEV Camera Control</h1>");
            
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
          client.println("<h1>ESP32-DEV Camera</h1>");
          client.println("<div class=\"message\"><h2>Camera Reset</h2>");
          client.println("<p>Performing camera hardware reset...</p></div>");
          client.println("<p>Please wait, the page will refresh in 5 seconds.</p>");
          client.println("<script>setTimeout(function(){window.location.href='/';}, 5000);</script>");
          client.println("</body></html>");
          
          // Simulate a hardware reset process
          Serial.println("Resetting camera hardware...");
          delay(500);
          
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