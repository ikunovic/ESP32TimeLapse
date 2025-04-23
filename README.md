# ESP32-WROVER-DEV Camera Time-Lapse Interface

A basic PlatformIO project for ESP32-WROVER-DEV with camera support and time-lapse functionality.

![ESP32-WROVER-DEV Board](readme-hardwareINFO.jpg)

## Project Code Structure

This project contains two main entry points:

### 1. main.cpp

This is the original, standard implementation of the ESP32 camera time-lapse project with:
- Fixed ESP32-CAM pin configuration
- Basic web interface for camera control
- Time-lapse functionality
- Primarily intended for debugging and quick setup

### 2. main_configurable.cpp

This is the enhanced, configurable version with:
- Support for multiple camera pin configurations through header files
- Improved error handling and debugging output
- Enhanced camera initialization process
- More detailed pin configuration output
- Better PSRAM utilization

## Selecting Which Main File to Use

The project is configured to use `main_configurable.cpp` by default. To switch between the two versions:

1. Open `platformio.ini`
2. Modify the `build_src_filter` parameter:

```ini
; To use main_configurable.cpp (default)
build_src_filter = 
    -<main.cpp>
    +<main_configurable.cpp>

; OR to use main.cpp instead
; build_src_filter = 
;     +<main.cpp>
;     -<main_configurable.cpp>
```

## Hardware Requirements

- ESP32-WROVER-DEV board
- Compatible camera module (OV2640, OV3660, etc.)
- USB cable for programming and power
- External 5V power supply (recommended for stable operation)

## Camera Connection

Connect the camera to your ESP32-WROVER-DEV using the following pins:

| Camera Pin | ESP32 Pin |
|------------|-----------|
| D0         | GPIO 4    |
| D1         | GPIO 5    |
| D2         | GPIO 18   |
| D3         | GPIO 19   |
| D4         | GPIO 36   |
| D5         | GPIO 39   |
| D6         | GPIO 34   |
| D7         | GPIO 35   |
| XCLK       | GPIO 21   |
| PCLK       | GPIO 22   |
| VSYNC      | GPIO 25   |
| HREF       | GPIO 23   |
| SDA        | GPIO 26   |
| SCL        | GPIO 27   |
| PWDN       | Not used  |
| RESET      | Not used  |

## Pin Configuration Options

When using `main_configurable.cpp`, you can select different pin configurations by uncommenting the appropriate include:

```cpp
// ===============================================================
// IMPORTANT: Include one of the pin configuration files:
#include "camera_pins/wrover.h"
// #include "camera_pins/esp32cam.h"
// #include "camera_pins/alt_pins.h"
// ===============================================================
```

## Software Setup

1. Open the project in PlatformIO
2. Set up WiFi credentials:
   - Copy `src/wifi_credentials_template.h` to `src/wifi_credentials.h`
   - Edit `src/wifi_credentials.h` with your WiFi credentials:
     ```cpp
     const char* ssid = "YourWiFiSSID";
     const char* password = "YourWiFiPassword";
     ```
   - Note: `wifi_credentials.h` is not tracked by Git for security
3. Build and upload the firmware to your ESP32-WROVER-DEV board

## Usage

1. After successful upload, open the Serial Monitor (115200 baud)
2. The ESP32 will connect to WiFi and display its IP address
3. Open a web browser and navigate to the displayed IP address
4. You should see the camera control interface with the following features:
   - Live camera preview
   - "Capture Image" button to take a single photo
   - "Start Time-lapse" button to begin automatic captures at regular intervals
   - "Stop Time-lapse" button to end automatic captures
   - Camera settings form to adjust quality, brightness, and contrast

## Time-Lapse Settings

The default time-lapse interval is set to 60 seconds. To change this, modify the `TIMELAPSE_INTERVAL` constant:

```cpp
const unsigned long TIMELAPSE_INTERVAL = 60000; // 1 minute interval (in milliseconds)
```

**Note**: The current implementation demonstrates the time-lapse concept but doesn't save images to storage. For a complete time-lapse solution, you would need to add code for saving images to an SD card or other storage.

## Debugging

When using `main_configurable.cpp`, the code provides enhanced debugging information:

- PSRAM detection and reporting
- Camera pin configuration display
- Detailed error messages during camera initialization
- Troubleshooting tips displayed on failure
- ESP error code translation to readable messages

## Troubleshooting

- If the camera fails to initialize, check your camera connections
- If WiFi connection fails, verify your SSID and password
- If the web page loads but no image appears, check if your camera is properly supported
- PSRAM issues can cause higher resolution captures to fail; try a lower resolution
- Verify you're using the correct pin configuration for your hardware model

## Customization

- To change camera resolution or quality, modify the parameters in the `config` structure
- The web interface can be customized by editing the HTML code in the `loop()` function
- Additional camera settings can be added by using the functions in `camera_settings.h` 