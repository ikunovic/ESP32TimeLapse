# ESP32-WROVER-DEV Camera Time-Lapse Interface

A basic PlatformIO project for ESP32-WROVER-DEV with camera support and time-lapse functionality.

## Hardware Requirements

- ESP32-WROVER-DEV board
- Compatible camera module (OV2640, OV3660, etc.)
- USB cable for programming and power

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

## Software Setup

1. Open the project in PlatformIO
2. Edit the WiFi credentials in `src/main.cpp`:
   ```cpp
   const char* ssid = "YourWiFiSSID";
   const char* password = "YourWiFiPassword";
   ```
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

The default time-lapse interval is set to 60 seconds. To change this, modify the `TIMELAPSE_INTERVAL` constant in `src/main.cpp`:

```cpp
const unsigned long TIMELAPSE_INTERVAL = 60000; // 1 minute interval (in milliseconds)
```

**Note**: The current implementation demonstrates the time-lapse concept but doesn't save images to storage. For a complete time-lapse solution, you would need to add code for saving images to an SD card or other storage.

## Troubleshooting

- If the camera fails to initialize, check your camera connections
- If WiFi connection fails, verify your SSID and password
- If the web page loads but no image appears, check if your camera is properly supported

## Customization

- To change camera resolution or quality, modify the parameters in the `config` structure
- The web interface can be customized by editing the HTML code in the `loop()` function
- Additional camera settings can be added by using the functions in `camera_settings.h` 