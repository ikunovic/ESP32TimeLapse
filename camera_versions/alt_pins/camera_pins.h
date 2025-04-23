// Alternative ESP32 camera pin configuration
// Avoids using GPIO 34-39 (input-only pins) for data lines that need to be bidirectional
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    33
#define XCLK_GPIO_NUM     4
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35  // Input-only pin (OK for Y9)
#define Y8_GPIO_NUM       25
#define Y7_GPIO_NUM       23
#define Y6_GPIO_NUM       22
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM    34  // Input-only pin (OK for VSYNC)
#define HREF_GPIO_NUM     39  // Input-only pin (OK for HREF)
#define PCLK_GPIO_NUM     36  // Input-only pin (OK for PCLK) 