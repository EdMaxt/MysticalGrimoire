// Battery gauge: voltage on IO34 -> percentage, charging indicator, calibration (NVS key "bcal", per mille).
#pragma once

static uint16_t batCal = 1000;               // multiplier x1000 (800..1200)
static float    batV = 0;                    // smoothed battery voltage, 0 = no reading yet
static int      batPct = -1;
static bool     batCharging = false;
static unsigned long batNext = 0, batChargeUntil = 0, batWinStart = 0;
static float    batWinMin = 9;

static void batteryDrawIcon();
static void batteryDrawOptions();
static void batteryDrawBar();

static const float BAT_CURVE[][2] = {        // typical single-cell LiPo, volts -> percent
  {4.20f,100},{4.15f,95},{4.11f,90},{4.08f,85},{4.02f,80},{3.98f,75},{3.95f,70},{3.91f,65},{3.87f,60},{3.85f,55},
  {3.84f,50},{3.82f,45},{3.80f,40},{3.79f,35},{3.77f,30},{3.75f,25},{3.73f,20},{3.71f,15},{3.69f,10},{3.61f,5},{3.27f,0} };

static int batPercent(float v) {
  const int n = sizeof(BAT_CURVE) / sizeof(BAT_CURVE[0]);
  if (v >= BAT_CURVE[0][0]) return 100;
  for (int i = 1; i < n; i++) {
    if (v >= BAT_CURVE[i][0]) {
      float t = (v - BAT_CURVE[i][0]) / (BAT_CURVE[i - 1][0] - BAT_CURVE[i][0]);
      return (int)(BAT_CURVE[i][1] + t * (BAT_CURVE[i - 1][1] - BAT_CURVE[i][1]) + 0.5f);
    }
  }
  return 0;
}

static float batReadVolts() {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(PIN_BAT_ADC);
  return (mv / 16.0f) / 1000.0f * BAT_DIV * (batCal / 1000.0f);
}

static void batteryInit() {
  batCal = prefs.getUShort("bcal", 1000);
  if (batCal < 800 || batCal > 1200) batCal = 1000;
  pinMode(PIN_BAT_ADC, INPUT);
#if PIN_CHG_STATUS >= 0
  pinMode(PIN_CHG_STATUS, INPUT);
#endif
}

// Every 2 s: new reading (smoothed), charging state. Redraws only the small battery widgets.
static void batteryTick() {
  unsigned long now = millis();
  if (batV > 0 && (long)(now - batNext) < 0) return;
  batNext = now + 2000;
  float v = batReadVolts();
  bool first = (batV <= 0);
  batV = first ? v : batV * 0.7f + v * 0.3f;
  int p = batPercent(batV);
  bool chg;
#if PIN_CHG_STATUS >= 0
  chg = (digitalRead(PIN_CHG_STATUS) == LOW);                   // TP4054 CHRG: open drain, low while charging
#else
  // estimate: the voltage climbs while charging; full/USB sits at ~4.2 V
  if (first || now - batWinStart > 60000UL) { batWinStart = now; batWinMin = batV; }
  if (batV < batWinMin) batWinMin = batV;
  if (batV - batWinMin >= 0.04f) batChargeUntil = now + 30000UL;
  chg = (long)(batChargeUntil - now) > 0 || batV >= 4.17f;
#endif
  bool changed = first || p != batPct || chg != batCharging;
  batPct = p; batCharging = chg;
  if (!changed) return;
  batteryDrawBar();
  if (screen == SCR_HOME) batteryDrawIcon();
  else if (screen == SCR_OPTIONS) batteryDrawOptions();
}

static void batteryDrawIcon() {
  const int x = 287, y = 4, w = 28, h = 13;
  tft.fillRect(x - 3, 0, SCREEN_W - x + 3, 30, C_BG);
  if (batPct < 0) return;
  uint16_t col = batCharging ? tft.color565(90, 220, 120) : (batPct <= 15 ? C_ERR : (batPct <= 30 ? tft.color565(240, 190, 70) : C_TEXT));
  tft.drawRect(x, y, w, h, col);
  tft.fillRect(x + w, y + 4, 2, h - 8, col);                    // terminal
  int fill = (w - 4) * batPct / 100;
  if (fill > 0) tft.fillRect(x + 2, y + 2, fill, h - 4, col);
  if (batCharging) {                                            // bolt over the cell
    tft.fillTriangle(x + 15, y + 1, x + 9, y + 7, x + 14, y + 7, C_BG);
    tft.fillTriangle(x + 13, y + 6, x + 18, y + 6, x + 12, y + 12, C_BG);
  }
  char b[8];
  snprintf(b, sizeof(b), "%d%%", batPct);
  tft.setTextFont(1);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(col, C_BG);
  tft.drawString(b, x + w / 2, y + h + 3);
  tft.setTextDatum(TL_DATUM);
  setTitleFont(); setBodyFont();                                // restore the app fonts
}

// Options: readout between two small buttons that nudge the calibration (0.5% per tap)
static void batteryDrawOptions() {
  tft.fillRect(96, 26, SCREEN_W - 96, 24, C_BG);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  tft.fillRoundRect(98, 28, 24, 20, 5, C_PANEL);
  tft.fillRoundRect(SCREEN_W - 30, 28, 24, 20, 5, C_PANEL);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.fillRect(104, 37, 12, 2, C_TEXT);
  tft.fillRect(SCREEN_W - 24, 37, 12, 2, C_TEXT);
  tft.fillRect(SCREEN_W - 19, 32, 2, 12, C_TEXT);
  char b[40];
  if (batPct < 0) snprintf(b, sizeof(b), "Battery --");
  else snprintf(b, sizeof(b), "%.2fV %d%%%s", batV, batPct, batCharging ? " chg" : "");
  tft.setTextColor(batCharging ? tft.color565(90, 220, 120) : C_TEXT, C_BG);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(b, (122 + SCREEN_W - 30) / 2, 38);
  tft.setTextDatum(TL_DATUM);
}

static bool batteryOptionsTap(int x, int y) {
  if (y < 26 || y >= 50) return false;
  int d = 0;
  if (x >= 98 && x < 122) d = -5;
  else if (x >= SCREEN_W - 30) d = 5;
  if (!d) return false;
  int c = (int)batCal + d;
  if (c < 800) c = 800;
  if (c > 1200) c = 1200;
  if (c != batCal) {
    batCal = (uint16_t)c;
    prefs.putUShort("bcal", batCal);
    batV = 0; batNext = 0; batteryTick();                       // re-read at once with the new factor
  }
  return true;
}

// Thin gauge along the very top edge (2 px), drawn on every screen: length = charge, green while charging,
// amber below 30%, red below 15%. The top 2 rows of the screen are reserved for it.
static void batteryDrawBar() {
  if (batPct < 0) return;
  uint16_t col = batCharging ? tft.color565(90, 220, 120) : (batPct <= 15 ? C_ERR : (batPct <= 30 ? tft.color565(240, 190, 70) : tft.color565(150, 140, 190)));
  int fill = SCREEN_W * batPct / 100;
  if (fill > 0) tft.fillRect(0, 0, fill, 2, col);
  if (fill < SCREEN_W) tft.fillRect(fill, 0, SCREEN_W - fill, 2, C_PANEL);
}

static void batteryBarTick() {              // self-healing: any full-screen redraw clears the strip, so repaint it often
  static unsigned long next = 0;
  unsigned long now = millis();
  if ((long)(now - next) < 0) return;
  next = now + 250;
  batteryDrawBar();
}
