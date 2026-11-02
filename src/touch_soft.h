#pragma once
#include <Arduino.h>
#include "config.h"

// Minimal XPT2046 reader using bit-banged SPI (mode 0, MSB first).
// Frees the hardware VSPI host so the microSD card can use it.
class TouchSoft {
public:
  TouchSoft(uint8_t clk, uint8_t mosi, uint8_t miso, uint8_t cs)
      : _clk(clk), _mosi(mosi), _miso(miso), _cs(cs) {}

  void begin() {
    pinMode(_cs, OUTPUT);   digitalWrite(_cs, HIGH);
    pinMode(_clk, OUTPUT);  digitalWrite(_clk, LOW);
    pinMode(_mosi, OUTPUT); digitalWrite(_mosi, LOW);
    pinMode(_miso, INPUT);
  }

  // Returns true while the panel is pressed.
  // sx/sy: screen pixels, rx/ry: raw ADC values, z: pressure estimate.
  bool read(int16_t &sx, int16_t &sy, int16_t &rx, int16_t &ry, int16_t &z) {
    int16_t z1 = (int16_t)cmd(0xB1);
    int16_t z2 = (int16_t)cmd(0xC1);
    z = z1 + 4095 - z2;
    if (z < TOUCH_Z_MIN) {
      cmd(0x90);                       // power down
      return false;
    }
    cmd(0x91);                         // first X conversion is noisy, discard
    int32_t ax = 0, ay = 0;
    for (int i = 0; i < 3; i++) {
      ax += cmd(0x91);                 // X position
      ay += cmd(0xD1);                 // Y position
    }
    cmd(0x90);                         // power down
    rx = (int16_t)(ax / 3);
    ry = (int16_t)(ay / 3);

    long a = rx, b = ry;
#if TOUCH_SWAP_XY
    a = ry; b = rx;
#endif
    long px = map(a, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX, 0, SCREEN_W - 1);
    long py = map(b, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX, 0, SCREEN_H - 1);
    px = constrain(px, 0, SCREEN_W - 1);
    py = constrain(py, 0, SCREEN_H - 1);
#if TOUCH_INVERT_X
    px = SCREEN_W - 1 - px;
#endif
#if TOUCH_INVERT_Y
    py = SCREEN_H - 1 - py;
#endif
    sx = (int16_t)px;
    sy = (int16_t)py;
    return true;
  }

private:
  uint8_t _clk, _mosi, _miso, _cs;

  // Send one control byte, then clock in 16 bits; the 12-bit result is
  // left-aligned after one null bit (standard 24-clock XPT2046 transfer).
  uint16_t cmd(uint8_t c) {
    digitalWrite(_cs, LOW);
    for (int i = 7; i >= 0; i--) {
      digitalWrite(_mosi, (c >> i) & 1);
      digitalWrite(_clk, HIGH);
      delayMicroseconds(1);
      digitalWrite(_clk, LOW);
      delayMicroseconds(1);
    }
    uint16_t v = 0;
    for (int i = 0; i < 16; i++) {
      digitalWrite(_clk, HIGH);
      delayMicroseconds(1);
      v = (uint16_t)((v << 1) | (digitalRead(_miso) ? 1 : 0));
      digitalWrite(_clk, LOW);
      delayMicroseconds(1);
    }
    digitalWrite(_cs, HIGH);
    return (v >> 3) & 0x0FFF;
  }
};
