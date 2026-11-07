// User_Setup for Freenove FNK0114 3.2" ESP32 Display
// ST7789P3 Driver with integrated XPT2046 touch

#ifndef USER_SETUP_H_
#define USER_SETUP_H_

// ========== Display Setup ==========
#define ST7789_DRIVER          // ST7789 display driver
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#define TFT_RGB_ORDER TFT_BGR  // FNK0114 uses BGR color order

// ========== SPI Pins ==========
#define TFT_MOSI 13   // SPI write
#define TFT_MISO 12   // SPI read
#define TFT_SCLK 14   // SPI clock
#define TFT_CS   15   // Chip select
#define TFT_DC    2   // Data/Command
#define TFT_RST  -1   // Reset (shared with ESP32)
#define TFT_BL   27   // Backlight

// ========== SPI Speed ==========
#define SPI_FREQUENCY       55000000  // Display SPI speed
#define SPI_READ_FREQUENCY  20000000

// ========== Touch Setup ==========
#define TOUCH_CS 33   // XPT2046 chip select

// ========== Fonts ==========
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define SMOOTH_FONT

// ========== Display Rotation ==========
// 1 = landscape (default for this app)
#define TFT_ROTATION 1

// ========== ST7789 Specific Settings ==========
// These help with ghosting and color issues on ST7789
#define ST7789_SLPIN_TYP 10   // Sleep in delay (ms)
#define TFT_INVERT_DISPLAY 0  // Don't invert colors

#endif // USER_SETUP_H_
