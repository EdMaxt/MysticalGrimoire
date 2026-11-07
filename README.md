# Mystical Grimoire 🧙‍♂️

[![License: MIT/CC BY 4.0](https://img.shields.io/badge/License-MIT%2FCC%20BY%204.0-lightgrey.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-1.1.0-blue.svg)]()
[![Platform](https://img.shields.io/badge/Platform-ESP32_CYD_2.8%22_%26_Freenove_3.2%22-green.svg)]()
[![Status](https://img.shields.io/badge/Status-Stable_&_Optimized-brightgreen.svg)]()

---

## Overview

A D&D 2024 companion application for the CYD and Freenove ESP32 touchscreen displays. Browse spells from the D&D 5.2.1 SRD organized by class, read from microSD card, plus an optional second spellbook with classes and subclasses from **Neon Odyssey** (playtest). The app interface is in English.

> **Version 1.1.0:** ✅ **Dual-board hardware support** - CYD 2.8" (ILI9341) + Freenove FNK0114 3.2" (ST7789). Perfect display rendering, 100% ghosting elimination on 3.2", responsive touch, fluid performance. Firmware: 535 KB. Tested and stable on both boards.

---

## Quick Start

1. **MicroSD (FAT32):** Copy `SD_CARD/grimoire` to the card root (creates `/grimoire/index/...` and `/grimoire/spells/...`). For Neon Odyssey, also copy `SD_CARD/neon` (creates `/neon/...`); optional - the app works with SRD only.
2. **Project:** Clone this repository and open in VSCode with PlatformIO IDE.
3. If using a different ESP32 variant, update the `platform = ...` line in `platformio.ini`.
4. Select the **`cyd_touchtest`** or **`cyd32_touchtest`** environment (based on your board) and upload. Touch the corners - it should draw dots where you touch. Adjust `TOUCH_RAW_*`, `TOUCH_SWAP_XY`, and `TOUCH_INVERT_*` in `src/config.h` until it matches your display. Adjust `BAT_DIV` to calibrate battery voltage.
5. Switch to the production environment (**`cyd`** or **`cyd32`**), insert the microSD, and upload.

---

## Hardware Support

### Supported Boards

| Board | Display | Driver | Status | Features |
|-------|---------|--------|--------|----------|
| **CYD 2.8"** | 240×320 ILI9341 | ILI9341_2 | ✅ Tested | Software SPI touch, stable, proven |
| **Freenove FNK0114 3.2"** | 240×320 ST7789 | ST7789 | ✅ New! | Integrated touch, optimized rendering |

### Building for Your Board

**For CYD 2.8":**
```bash
pio run -e cyd -t upload
```

**For Freenove FNK0114 3.2":**
```bash
pio run -e cyd32 -t upload
```

### Board Configuration

Each board has its own `platformio.ini` environment with optimal settings:

- `[env:cyd]` - CYD 2.8" with ILI9341 and software SPI touch
- `[env:cyd32]` - Freenove FNK0114 3.2" with ST7789 and integrated touch
- `[env:cyd_touchtest]` - Touch calibration for 2.8"
- `[env:cyd32_touchtest]` - Touch calibration for 3.2"

---

## Version 1.1.0: FNK0114 Display Optimization

Mystical Grimoire now supports the **Freenove FNK0114 3.2" display** with full ST7789 optimizations:

### Key Features

- ✅ **100% Ghosting Elimination:** ST7789 gamma curves (0xE0/0xE1) completely eliminate screen artifacts
- ✅ **Display SPI:** 80 MHz (5.3x faster than initial 15 MHz)
- ✅ **Touch Performance:** 2.5 MHz XPT2046 polling, highly responsive
- ✅ **Frame Buffer:** DMA acceleration enabled for smooth transitions
- ✅ **Color Accuracy:** Optimized brightness levels prevent over-saturation
- ✅ **Backward Compatible:** All changes maintain full CYD 2.8" functionality

### Testing Results

✅ **Display Rendering** - Perfect rendering without ghosting artifacts  
✅ **Touch Input** - Responsive and accurate  
✅ **Performance** - Smooth, fluid transitions  
✅ **Multi-board** - Both 2.8" and 3.2" fully functional  

---

## Features

### Spellbooks (v0.4)

Create and manage character spell sheets with support for D&D 5e and Neon Odyssey classes:

- **Slots:** Track spell slots by level, mark used slots
- **Prepared:** Manage prepared/known spells
- **Cast:** Use spells with upcast tracking
- **Subclasses:** Full support for subclass abilities and bonus spells
- **Persistence:** All data saved to microSD card

### Battery Support (v0.7)

- Real-time battery voltage display with percentage
- Visual indicators: green (charging), amber (<30%), red (<15%)
- Calibration interface in Options
- Optional: Wire TP4054 CHRG pin for accurate charge detection

### Display Control

- **Brightness:** 5 levels adjustable from Options
- **Screen Off:** Auto-off timer (1-10 minutes) with instant wake
- **Deep Sleep:** Save power with wake-on-BOOT
- **Rotation:** 0°/180° landscape modes (v1.0.0+)

### Audio/Visual Polish

- Class icons (D&D and Neon artwork)
- Smooth scrolling with visual feedback
- Color-coded UI elements
- Battery and charging indicators

---

## How It Works

### Touch Input

**CYD 2.8":** Software SPI bit-banged XPT2046 (GPIO 25/32/39/33)  
**FNK0114 3.2":** TFT_eSPI integrated XPT2046 (GPIO 33 CS only)

### Data Storage

- Spell data: `/grimoire/index/*.idx` (metadata) + `/spells/*.txt` (full text)
- Character sheets: `/characters/*.txt` (editable plaintext)
- Neon Odyssey: `/neon/` (optional, same format)
- No data loaded into RAM at once - reads on demand

### Spell Books

**SRD 5.2.1:** 339 spells from official SRD  
**Neon Odyssey:** 95 new spells + 52 subclasses (optional)  
**PHB-only spells:** 52 additional spells reference the PHB (text not included)

### Power Management

- **Idle CPU scaling:** 240 MHz active → 80 MHz idle
- **Touch smart polling:** 8ms active, 50ms idle
- **SD card optimization:** 8 MHz read frequency
- **Deep sleep:** ~11 mA with all peripherals powered down

---

## Building the Data

The SD card content is generated from D&D SRD and Neon Odyssey PDFs using Python scripts:

```bash
# Extract spell text from D&D 5e SRD PDF
python3 tools/1_extract_pages.py

# Parse spells into JSON
python3 tools/2_parse_spells.py

# Build microSD card structure
python3 tools/3_build_sd.py
```

**Requirements:** `pdftotext` (poppler), `pdfplumber` (optional)

---

## Testing Checklist

- [ ] Compiles without errors with PlatformIO
- [ ] Display renders correctly (no inverted colors)
- [ ] microSD card mounts successfully
- [ ] Touch responds accurately to input
- [ ] Home → Class → Level tabs → Spell list → Details → Back navigation works
- [ ] Scroll by drag and arrow keys work smoothly
- [ ] Options: brightness adjustment persists across power cycles
- [ ] Options: sleep mode engages correctly
- [ ] Spellbooks: create, save, and load character sheets
- [ ] Battery: icon displays, voltage calibration works
- [ ] All screens render without graphical glitches
- [ ] Touch coordinates map correctly after display rotation (if supported)

---

## Technical Details

### ESP32 Pinout (CYD 2.8")

```
Display SPI (HSPI):    Touch SPI (Software):   Other:
  GPIO13 = MOSI          GPIO25 = CLK           GPIO21 = Backlight
  GPIO12 = MISO          GPIO32 = MOSI          GPIO34 = Battery ADC
  GPIO14 = SCLK          GPIO39 = MISO          GPIO0  = BOOT button
  GPIO15 = CS            GPIO33 = CS
  GPIO2  = DC
```

### ESP32 Pinout (FNK0114 3.2")

```
Display SPI (HSPI):    Touch:                 Other:
  GPIO13 = MOSI          GPIO33 = CS           GPIO27 = Backlight
  GPIO12 = MISO          (Integrated XPT2046)  GPIO34 = Battery ADC
  GPIO14 = SCLK                                GPIO0  = BOOT button
  GPIO15 = CS
  GPIO2  = DC
```

### Memory Usage

```
RAM:   20.3% (66 KB / 327 KB)  - Healthy
Flash: 40.8% (535 KB / 1.3 MB) - Comfortable
```

---

## License & Attribution

- **SRD 5.2.1 Content:** CC BY 4.0 - Attribution text in `SD_CARD/grimoire/ATTRIBUTION.txt`
- **Neon Odyssey:** Personal use only (copyright © Avantris, all rights reserved)
- **Application Code:** MIT License

**Important:** Do not upload `SD_CARD/neon` or `data/neon_*.json` to public repositories.

---

## Known Limitations

- **No multiclassing:** Single class per character sheet
- **Slot tables:** Pulled from official D&D tables (not user-editable)
- **Neon subclasses:** First column always-granted, others are "choose one"
- **PHB-only spells:** No text included (52 spells reference the Player's Handbook)
- **Two-column spell tables:** Some long tables render as continuous text
- **Portrait mode:** Not implemented (would require complete layout redesign)

---

## Future Plans

- Multiclassing and manual slot editing
- Per-character notes and favorites
- Spell search by name/effect
- Custom font support with accents/symbols
- Improved table rendering for complex spell descriptions
- Portrait display orientation (90°/270°) - requires layout redesign

---

## Documentation

- **[CHANGELOG.md](CHANGELOG.md)** - Complete version history and changes
- **[DEVELOPMENT.md](DEVELOPMENT.md)** - Development guide, debugging tips, and modifications
- **[GITHUB_SETUP.md](GITHUB_SETUP.md)** - Repository setup and clone instructions

---

## Support & Contributing

Found a bug? Have suggestions?

- **GitHub Issues:** Report bugs and request features via [Issues](../../issues)
- **Documentation:** Update docs for clarity and additional use cases
- **Testing:** Test on different ESP32 boards and report compatibility

---

## Credits

- **D&D 5e SRD Content:** © Wizards of the Coast (CC BY 4.0)
- **Neon Odyssey Playtest:** © Avantris (used with permission for personal projects)
- **Application:** Custom development for ESP32 touchscreen devices
- **Icon artwork:** Class representations for D&D and Neon Odyssey
- **Contributors:** Users who tested and provided feedback on display optimizations

---

**Latest Release:** [v1.1.0 - Freenove FNK0114 Support & Optimizations](../../releases/tag/v1.1.0)

**Last Updated:** November 7, 2026
