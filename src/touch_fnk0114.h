#pragma once
#include <Arduino.h>
#include "config.h"

// Freenove FNK0114 3.2" uses TFT_eSPI integrated touch
// This is a wrapper for compatibility with TouchSoft interface

class TouchSoft {
private:
  TFT_eSPI* tft;
  int16_t last_x = -1, last_y = -1;

public:
  TouchSoft(uint8_t clk, uint8_t mosi, uint8_t miso, uint8_t cs)
      : tft(nullptr) {}

  void setTFT(TFT_eSPI* t) { tft = t; }

  void begin() {
    // TFT_eSPI handles touch initialization automatically
    // Nothing to do here for FNK0114
  }

  // Returns true while the panel is pressed
  // Uses TFT_eSPI getTouch() method
  bool read(int16_t &sx, int16_t &sy, int16_t &rx, int16_t &ry, int16_t &z) {
    if (!tft) return false;

    uint16_t x = 0, y = 0;
    bool pressed = tft->getTouch(&x, &y);

    if (pressed && x > 0 && y > 0) {
      sx = (int16_t)x;
      sy = (int16_t)y;
      rx = (int16_t)x;
      ry = (int16_t)y;
      z = 4095;
      return true;
    }

    return false;
  }
};
