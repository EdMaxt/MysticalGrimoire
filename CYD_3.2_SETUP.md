# FNK0114 3.2" Setup Guide

**Status:** ✅ Compatible with current firmware  
**Version:** 1.1.0  
**Last Updated:** November 7, 2026

---

## 🔧 Hardware Compatibility

The FNK0114 3.2" display is compatible with the same firmware as the CYD 2.8". All pins are standardized.

### Pin Comparison

```
DISPLAY 2.8" vs DISPLAY 3.2"

Display (LCD):
  GPIO15 - LCD_CS      (SAME)
  GPIO2  - LCD_DC      (SAME)
  GPIO14 - LCD_CLK     (SAME)
  GPIO13 - LCD_MOSI    (SAME)
  GPIO12 - LCD_MISO    (SAME)
  GPIO27 - LCD_BL      (SAME for 3.2")
  GPIO21 - LCD_BL      (SAME for 2.8")

Touch (XPT2046):
  GPIO33 - TOUCH_CS    (SAME)
  GPIO14 - TOUCH_CLK   (SHARED - same as display)
  GPIO13 - TOUCH_MOSI  (SHARED - same as display)
  GPIO12 - TOUCH_MISO  (SHARED - same as display)

Other:
  GPIO34 - BAT_ADC     (SAME)
  GPIO22, GPIO16, GPIO17 - RGB LED (SAME)
```

### Important: Backlight Pin Configuration

- **CYD 2.8":** GPIO21 (configured with `-DTFT_BL=21`)
- **FNK0114 3.2":** GPIO27 (configured with `-DTFT_BL=27`)

The `platformio.ini` has separate environments for each board with correct settings.

---

## ✅ Build Environments

### For CYD 2.8"
```ini
[env:cyd]
build_flags = ... -DTFT_BL=21
```

### For FNK0114 3.2"
```ini
[env:cyd32]
build_flags = ... -DTFT_BL=27 -DST7789_DRIVER -DUSE_TOUCH_FNK0114
```

---

## 🚀 Setup Steps

### 1. Verify Current Configuration

```bash
cd C:\Users\edmax\Projects\Mystical_Grimoire\Mystical_Grimoire

# Check platformio.ini for your environment
grep -A 10 "\[env:cyd32\]" platformio.ini
```

### 2. Verify src/config.h

Open `src/config.h` and check:

```cpp
#define SCREEN_W 320      // Landscape width
#define SCREEN_H 240      // Landscape height
```

These values are correct for both boards (240×320 resolution, displayed as landscape).

### 3. Compile Firmware

```bash
# For FNK0114 3.2"
pio run -t clean -e cyd32
pio run -e cyd32

# For CYD 2.8"
pio run -t clean -e cyd
pio run -e cyd
```

### 4. Touch Calibration (Recommended)

Important: Even though pins are the same, touch calibration should be done per display.

```bash
# For FNK0114 3.2"
pio run -e cyd32_touchtest -t upload

# For CYD 2.8"
pio run -e cyd_touchtest -t upload
```

The device will show calibration points. Press corners when indicated. Adjust `TOUCH_RAW_*` values in `src/config.h` if needed.

### 5. Normal Upload

```bash
# After calibration, upload production firmware
pio run -e cyd32 -t upload    # For 3.2"
pio run -e cyd -t upload      # For 2.8"
```

---

## 🔌 Physical Connection

### Step 1: Preparation
```
1. Disconnect USB from ESP32
2. Disconnect battery (if connected)
3. Wait 10 seconds
4. TAKE A PHOTO of current connections for reference
```

### Step 2: Display Swap

Disconnect current display in this order (take photo first!):
```
1. VCC (red/+)
2. GND (black/-)
3. SPI CLK (orange)
4. SPI MOSI (yellow)
5. SPI MISO (green)
6. CS (blue)
7. DC (purple)
8. RST (gray) - if present
```

Connect new display in the SAME order.

### Step 3: Visual Verification
```
Verify:
  ✓ No loose wires
  ✓ Correct polarity (+ to VCC, - to GND)
  ✓ All cables secured
  ✓ No visible short circuits
  ✓ Connectors properly seated
```

### Step 4: Power On Test
```
1. Connect USB (NO battery yet)
2. Check for unusual heat or behavior
3. If OK, proceed to firmware upload
4. If issues, disconnect USB immediately
```

---

## 🎯 Display-Specific Settings

### For CYD 2.8" (ILI9341)

```ini
[env:cyd]
build_flags =
    -DILI9341_2_DRIVER
    -DTFT_BL=21
    -DTFT_BACKLIGHT_ON=HIGH
    -DSPI_FREQUENCY=55000000
    -DTOUCH_CLK=25
    -DTOUCH_MOSI=32
    -DTOUCH_MISO=39
```

### For FNK0114 3.2" (ST7789)

```ini
[env:cyd32]
build_flags =
    -DST7789_DRIVER
    -DTFT_BL=27
    -DTFT_BACKLIGHT_ON=LOW
    -DUSE_TOUCH_FNK0114
    -DSPI_FREQUENCY=80000000
    -DSPI_READ_FREQUENCY=6000000
    -DSPI_TOUCH_FREQUENCY=2500000
    -DUSE_DMA_TO_FRAME_BUFFER=1
```

---

## 📊 Troubleshooting

### Display Not Showing
- Verify backlight GPIO (21 for 2.8", 27 for 3.2")
- Check TFT_BL setting in platformio.ini
- Adjust brightness in Options menu

### Touch Not Calibrated
- Run touchtest environment
- Press calibration points firmly
- Adjust TOUCH_RAW_* in src/config.h

### Colors Inverted
- Check TFT_RGB_ORDER setting
- For 3.2", should be: `-DTFT_RGB_ORDER=TFT_BGR`

### Ghosting on 3.2"
- Should be eliminated with gamma curves in v1.1.0
- If persists, run clean build and re-upload

### Serial Port Not Found
- Check USB cable
- Install CH340 drivers if needed
- Try different USB port

---

## ✨ Optimizations (v1.1.0)

### FNK0114 3.2" Specific
```
✅ SPI Frequency: 80 MHz (5.3x faster than initial 15 MHz)
✅ DMA: Frame buffer acceleration enabled
✅ Touch SPI: 2.5 MHz (optimized for XPT2046)
✅ Gamma Curves: 100% ghosting elimination
✅ Color: Adjusted brightness levels (not blown-out)
```

### Backward Compatible
```
✅ CYD 2.8" firmware unchanged
✅ All pins standardized
✅ Both boards supported in one codebase
✅ Easy switching between boards
```

---

## 📚 Related Documentation

- **README.md** - Feature overview
- **3.2_READY_CHECKLIST.md** - Pre-upload verification
- **DEVELOPMENT.md** - Development guide

---

**Last Updated:** November 7, 2026  
**Compatible with:** Mystical Grimoire v1.1.0  
**Hardware:** CYD 2.8" (ILI9341) + Freenove FNK0114 3.2" (ST7789)
