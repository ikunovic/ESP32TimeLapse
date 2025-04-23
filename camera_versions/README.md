# ESP32 Camera Pin Configurations

This directory contains different pin configurations for ESP32 camera modules:

## Available Configurations

1. **wrover_pins**: Original ESP32-WROVER-KIT camera pins
   - These are the default pins used in the ESP32-WROVER-KIT board

2. **esp32cam_pins**: Standard ESP32-CAM module pins
   - These are compatible with most ESP32-CAM modules like AI-Thinker
   - Uses GPIO0 for XCLK and GPIO32 for PWDN

3. **alt_pins**: Alternative pins configuration
   - Designed to avoid using input-only pins (GPIO34-39) for bidirectional data lines
   - May work better on some ESP32 boards

## How to Use

1. Copy the appropriate `camera_pins.h` file to the `src` directory of your project
2. In your `main.cpp` file, add the include: `#include "camera_pins.h"`

Or, use the provided `main_configurable.cpp` file:

1. Copy `main_configurable.cpp` to your `src` directory
2. Rename it to `main.cpp`
3. Uncomment the appropriate include line:
   ```cpp
   // IMPORTANT: Include one of the pin configuration files:
   #include "camera_versions/wrover_pins/camera_pins.h"
   // #include "camera_versions/esp32cam_pins/camera_pins.h"
   // #include "camera_versions/alt_pins/camera_pins.h"
   ```

## Troubleshooting Tips

1. **Input-Only Pins**: GPIO 34-39 on ESP32 are input-only pins. If your camera uses these for data lines that need to be bidirectional, it might cause issues. These pins work fine for VSYNC, HREF, and PCLK which are input-only signals.

2. **Common Errors**:
   - "Camera initialized but can't capture frames": Check your data line connections, especially if using input-only pins for bidirectional signals.
   - "Camera initialization failed": Check all pin connections and power supply.

3. **Testing Strategy**:
   - Try each pin configuration one by one
   - Monitor the serial output for diagnostic information
   - Use the web interface's diagnostic and reset features for troubleshooting 