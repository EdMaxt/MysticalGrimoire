#pragma once

// ---- Screen (rotation 1 = landscape) --------------------------------------
#define SCREEN_W 320
#define SCREEN_H 240

// ---- microSD slot on the CYD (uses the VSPI host) ---------------------------
// Verify against PINS.md of witnessmenow/ESP32-Cheap-Yellow-Display.
#define SD_SCK   18
#define SD_MISO  19
#define SD_MOSI  23
#define SD_CS     5

// ---- Touch XPT2046 on its own pins, read with software SPI -----------------
// The hardware VSPI host is used by the SD card, so touch is bit-banged.
// Values can be overridden from platformio.ini with -DTOUCH_XXX flags
#ifndef TOUCH_CLK
  #define TOUCH_CLK   25
#endif
#ifndef TOUCH_MOSI
  #define TOUCH_MOSI  32
#endif
#ifndef TOUCH_MISO
  #define TOUCH_MISO  39
#endif
#ifndef TOUCH_CS
  #define TOUCH_CS    33
#endif

// Minimum pressure to count as a touch (same idea as the Stoffregen library).
#define TOUCH_Z_MIN 400

// Touch screen power optimization (v1.0.0): smart polling rates
#define TOUCH_POLL_ACTIVE_MS   8      // poll every 8ms when actively touching
#define TOUCH_POLL_IDLE_MS    50      // poll every 50ms when no touch (reduces SPI traffic)

// Raw ADC range -> screen. Start with the common CYD values; if PlanarNexus
// uses different numbers, copy them here. Use the cyd_touchtest env to tune.
#define TOUCH_RAW_X_MIN 200
#define TOUCH_RAW_X_MAX 3700
#define TOUCH_RAW_Y_MIN 240
#define TOUCH_RAW_Y_MAX 3800
#define TOUCH_SWAP_XY   0
#define TOUCH_INVERT_X  0
#define TOUCH_INVERT_Y  0

// ---- Misc pins (RGB LED is active low) -------------------------------------
#define PIN_LED_R 22
#define PIN_LED_G 16
#define PIN_LED_B 17

// ---- SD layout ---------------------------------------------------------------
#define SD_ROOT   "/grimoire"   // D&D 2024 SRD 5.2.1 pack
#define NEON_ROOT "/neon"        // Neon Odyssey pack (optional)

// ---- Battery (2.8" ESP32-32E display / CYD with the 1.25 mm LiPo connector) -----------------
// IO34 reads the battery through the board's divider. BAT_DIV is the divider ratio (Vbat = Vpin * BAT_DIV);
// 2.0 is the usual 100k/100k. Fine-tune it on the device: Options > battery row, [-] [+], comparing
// with a multimeter on the battery.
#define PIN_BAT_ADC      34
#define BAT_DIV          2.0f
// The TP4054 charger has no status pin routed by default. Optional wire from its CHRG pin to a free
// input (IO35 is input-only and has NO internal pull-up: add 10k to 3.3V). -1 = estimate from the voltage.
#define PIN_CHG_STATUS   -1
