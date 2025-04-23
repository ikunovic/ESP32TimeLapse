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

// Include WiFi credentials from a separate file
// NOTE: wifi_credentials.h is not tracked by Git
// Copy wifi_credentials_template.h to wifi_credentials.h and update with your credentials
#include "wifi_credentials.h"

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
  
  Serial.println("configurable------------------------------------------------------");
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

void loop() {
  // Handle time-lapse functionality and web server clients
  if (timelapseActive && millis() - lastCaptureTime >= TIMELAPSE_INTERVAL) {
    lastCaptureTime = millis();
    // Here you would implement the time-lapse capture
    Serial.println("Time-lapse capture triggered");
  }
  
  // Handle web clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");
    // Handle the client connection (you'd implement this based on your web interface needs)
    client.stop();
  }
  
  // Small delay to prevent the loop from running too fast
  delay(10);
} 