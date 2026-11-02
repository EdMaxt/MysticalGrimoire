// Mystical Grimoire - D&D 2024 (SRD 5.2.1) + Neon Odyssey spell companion for the CYD ESP32.
// All data lives on the microSD card (see README.md and SD_CARD/).

#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "config.h"
#include "touch_soft.h"

TFT_eSPI tft = TFT_eSPI();
SPIClass sdSPI(VSPI);
TouchSoft touch(TOUCH_CLK, TOUCH_MOSI, TOUCH_MISO, TOUCH_CS);

// ---------------------------------------------------------------------------
// Types and constants
// ---------------------------------------------------------------------------
enum Screen { SCR_HOME, SCR_CLASS, SCR_READER, SCR_ERROR, SCR_OPTIONS, SCR_SUBS,
              SCR_SHEET, SCR_EDIT, SCR_PICK, SCR_KEYBOARD, SCR_CAST, SCR_WZBOOK };

enum LineKind : uint8_t { K_BODY = 0, K_HEAD = 1, K_TIME = 2, K_RANGE = 3,
                          K_COMP = 4, K_DUR = 5, K_GAP = 6, K_CLS = 7,
                          K_BASE = 8, K_TAG = 9 };

// K_HEAD "first" values: 1 = higher level, 2 = cantrip upgrade, 3 = subclass spells
static const char* const KLABEL[10] = { "", "", "Time: ", "Range: ", "Components: ",
                                        "Duration: ", "", "Classes: ", "Reskin of: ", "" };

static const char* const SCHOOLS[8] = { "Abjuration", "Conjuration", "Divination",
                                        "Enchantment", "Evocation", "Illusion",
                                        "Necromancy", "Transmutation" };
static const uint8_t SCHOOL_RGB[8][3] = { {80,140,255}, {240,170,60}, {110,220,230},
                                          {240,110,180}, {240,80,60}, {170,120,255},
                                          {110,200,90}, {210,200,80} };
static const uint8_t CLASS_RGB[16][3] = { {200,90,200}, {240,220,120}, {90,190,90},
                                          {240,200,70}, {70,170,110}, {230,90,70},
                                          {150,90,220}, {80,140,240}, {240,130,60},
                                          {90,210,220}, {230,110,160}, {170,200,80},
                                          {120,130,240}, {220,80,120}, {100,220,170},
                                          {200,160,110} };

struct ClassInfo { char name[12]; uint16_t count; uint8_t maxLevel; uint8_t subCount; uint16_t color; char basedOn[12]; };
// src: 0 = SRD spell (/grimoire/spells), 1 = Neon spell (/neon/spells)
struct Entry     { uint16_t id; uint8_t level; uint8_t school; uint8_t flags; uint8_t src; char name[40]; char base[28]; };
struct SubEntry  { uint16_t id; uint8_t hasSpells; uint8_t fromOverdrive; char name[28]; };   // fromOverdrive: 0 handbook, 1 overdrive, 2 SRD
struct Line      { uint16_t off; uint8_t len; uint8_t kind; uint8_t first; };

#define MAX_CLASSES   16
#define MAX_ENTRIES   300
#define MAX_SUBS      8
#define MAX_LINES     260
#define FILE_BUF_SZ   6144
#define MARGIN        8
#define TAB_Y         30
#define TAB_H         24
#define LIST_Y        58
#define ROW_H         30
#define LIST_ROWS     ((SCREEN_H - LIST_Y) / ROW_H)

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static uint16_t C_BG, C_PANEL, C_PANEL2, C_TEXT, C_DIM, C_GOLD, C_SEL, C_ERR;

static uint8_t curPack = 0;          // 0 = D&D 2024 SRD, 1 = Neon Odyssey (saved in NVS)
static bool neonAvailable = false;   // /neon/index/classes.txt exists on the card
static Preferences prefs;

// Brightness (5 levels, PWM on the backlight pin) and BOOT-button sleep: same scheme as PlanarNexus.
#ifndef TFT_BL
#define TFT_BL 21
#endif
#define BOOT_BTN_PIN  0        // not named BOOT_PIN: the ESP32 core defines that macro
#define BOOT_HOLD_MS  2000
static const uint8_t BRI_DUTY[5] = { 51, 102, 153, 204, 255 };
static uint8_t briLevel = 5;   // 1..5, saved in NVS key "bri"
// Soft sleep: after N idle minutes the backlight dims, 10 s later it goes off; a tap or BOOT wakes it.
static const uint8_t SOFT_MIN[5] = {0, 1, 2, 5, 10};   // 0 = never
static uint8_t softIdx = 3;                            // index into SOFT_MIN, NVS key "soff"
static uint8_t idleState = 0;                          // 0 awake, 1 dimmed, 2 screen off
static uint32_t lastActive = 0;
static const uint32_t SOFT_DIM_MS = 10000;
static const uint8_t SOFT_DIM_DUTY = 12;
static uint8_t currentRot = 0;  // 0=0°, 1=90°, 2=180°, 3=270°, NVS key "rot"

static bool smoothFont = false;      // true when /grimoire/fonts/body.vlw was loaded
static bool fontTried  = false;
static int  lineH = 18, hdrH = 44, readY = 46, viewW = 298;

static ClassInfo classes[MAX_CLASSES];
static int nClasses = 0;

static Entry entries[MAX_ENTRIES];
static int nEntries = 0;
static int levelFirst[10], levelCount[10];
static int tabLevels[10], nTabs = 0;

static char fileBuf[FILE_BUF_SZ];
static Line lines[MAX_LINES];
static int  nLines = 0;

static char EMPTY[1] = { 0 };
static struct {
  char *name, *classes, *time, *range, *comp, *dur, *body;
  uint8_t level, school, flags;
} sv;

static SubEntry subs[MAX_SUBS];
static int nSubs = 0;
static Screen subsBack = SCR_HOME;

static Screen screen = SCR_HOME, readerBack = SCR_HOME;
static int curClass = 0, curLevel = 0, listScroll = 0, scrollLine = 0;
static int readerBar = 0;            // height of the Cast bar under the reader (spellbooks), 0 = none
static char errMsg[96];
static char readerTitle[48], readerSub[64], readerBase[28];

// ---------------------------------------------------------------------------
// Prototypes (PlatformIO does not generate them for .cpp files)
// ---------------------------------------------------------------------------
static void initColors();
static void setTitleFont();
static void setBodyFont();
static void applyFont();
static void foldAscii(char* s);
static uint8_t schoolIndex(const char* s);
static uint16_t schoolColor(uint8_t s);
static bool mountSD();
static bool loadClasses();
static bool parseIdxLine(char* line, Entry& e);
static bool loadClassIndex(int ci);
static bool loadSpell(uint8_t src, uint16_t id);
static bool loadTextFile(const char* path);
static const char* packRoot();
static void applyTheme();
static void setPack(uint8_t p);
static bool loadSubIndexRoot(const char* root, const char* cls, SubEntry* out, int& n);
static bool loadSubIndex(int ci);
static bool loadSub(uint16_t id);
static bool openSub(int si);
static void openSubs(int ci, Screen back);
static void tileRect(int i, int& x, int& y, int& w, int& h);
static void drawOptions();
static void drawSubs();
static void drawPackBtn(int y, int pack, const char* title, const char* sub, bool avail);
static void blWrite(uint8_t duty);
static void applyBrightness();
static void setBrightness(int level);
static void enterSleep();
static void setRotation(uint8_t r);
static void cycleRotation();
static bool isPortrait();
static uint16_t getLogicalWidth();
static uint16_t getLogicalHeight();
static const char* getRotationLabel(uint8_t rot);
static void pushLine(const char* s, int len, uint8_t kind, uint8_t first);
static int  measure(const char* s, int n);
static void wrapPara(const char* s, uint8_t kind, int firstIndent);
static void addInfo(uint8_t kind, const char* val);
static void buildSpellLines(const Entry& e);
static bool openSpell(const Entry& e);
static void openAbout();
static void drawFit(const char* s, int x, int y, int maxW);
static void drawBackBtn(int h);
static void drawArrowBtn(int x, int y, int w, int h, bool up);
static void drawHome();
static void drawClassScreen();
static void drawClassHeader();
static void drawTabs();
static void drawList();
static void drawReaderScreen();
static void drawLine(int li, int y);
static void drawReaderContent();
static void drawError();
static void redrawAll();
static void bootData();
static void showError(const char* msg);
static void listScrollBy(int d);
static void readerScrollBy(int d);
static void scrollBy(int d);
static void handleTap(int x, int y);
static void startClass(int ci);
static void bgFree();
static bool inRect(int x, int y, int rx, int ry, int rw, int rh);
#ifdef TOUCH_TEST
static void touchTest();
#endif

#include "spellbook.h"
#include "battery.h"

// ---------------------------------------------------------------------------
// Fonts and colors
// ---------------------------------------------------------------------------
static void initColors() {
  C_BG     = tft.color565(14, 12, 24);
  C_PANEL  = tft.color565(36, 30, 58);
  C_PANEL2 = tft.color565(28, 22, 46);
  C_TEXT   = tft.color565(232, 228, 240);
  C_DIM    = tft.color565(140, 134, 160);
  C_ERR    = tft.color565(240, 90, 80);
  applyTheme();
}

// Accent colors follow the active book: gold/violet for the SRD, cyan/magenta for Neon Odyssey.
static void applyTheme() {
  if (curPack == 1) {
    C_GOLD = tft.color565(80, 230, 245);
    C_SEL  = tft.color565(150, 40, 140);
  } else {
    C_GOLD = tft.color565(232, 190, 90);
    C_SEL  = tft.color565(90, 60, 150);
  }
}

static const char* packRoot() { return curPack == 1 ? NEON_ROOT : SD_ROOT; }

static void setTitleFont() { if (!smoothFont) tft.setTextFont(4); }
static void setBodyFont()  { if (!smoothFont) tft.setTextFont(2); }

static void applyFont() {
  setBodyFont();
  lineH = tft.fontHeight() + 2;
  hdrH  = 2 * lineH + 8;
  if (hdrH < 44) hdrH = 44;
  readY = hdrH + 2;
  uint16_t logW = getLogicalWidth();
  viewW = logW - 2 * MARGIN - 6;
}

// Replace typographic characters with plain ASCII (used with the built-in
// bitmap fonts, which have no glyphs for them). Works in place.
static void foldAscii(char* s) {
  char* w = s;
  while (*s) {
    uint8_t c = (uint8_t)s[0];
    if (c == 0xE2 && (uint8_t)s[1] == 0x80) {
      uint8_t d = (uint8_t)s[2];
      if (d == 0x98 || d == 0x99) { *w++ = '\''; s += 3; continue; }
      if (d == 0x9C || d == 0x9D) { *w++ = '"';  s += 3; continue; }
      if (d == 0x93 || d == 0x94) { *w++ = '-';  s += 3; continue; }
      if (d == 0xA2)              { *w++ = '*';  s += 3; continue; }
      if (d == 0xA6)              { *w++ = '.'; *w++ = '.'; *w++ = '.'; s += 3; continue; }
    } else if (c == 0xE2 && (uint8_t)s[1] == 0x96 && (uint8_t)s[2] == 0xBA) {
      *w++ = '>'; s += 3; continue;                   // U+25BA pointer
    } else if (c == 0xE2 && (uint8_t)s[1] == 0x88 && (uint8_t)s[2] == 0x92) {
      *w++ = '-'; s += 3; continue;                   // U+2212 minus
    } else if (c == 0xC3 && (uint8_t)s[1] == 0x97) {
      *w++ = 'x'; s += 2; continue;                   // multiplication sign
    } else if (c == 0xC3 && ((uint8_t)s[1] & 0xC0) == 0x80) {          // accented Latin letters (Spanish text)
      static const char LCASE[] = "aaaaaaaceeeeiiiidnooooo/ouuuuypy";    // U+00E0..U+00FF
      static const char UCASE[] = "AAAAAAACEEEEIIIIDNOOOOOxOUUUUYPs";    // U+00C0..U+00DF
      uint8_t d = (uint8_t)s[1];
      *w++ = d >= 0xA0 ? LCASE[d - 0xA0] : UCASE[d - 0x80]; s += 2; continue;
    } else if (c == 0xC2 && (uint8_t)s[1] == 0xBF) {
      *w++ = '?'; s += 2; continue;                   // inverted question mark
    } else if (c == 0xC2 && (uint8_t)s[1] == 0xA1) {
      *w++ = '!'; s += 2; continue;                   // inverted exclamation mark
    } else if (c == 0xC2 && (uint8_t)s[1] == 0xA0) {
      *w++ = ' '; s += 2; continue;                   // no-break space
    } else if (c == 0xC2 && (uint8_t)s[1] == 0xBA) {
      *w++ = 'o'; s += 2; continue;                   // masculine ordinal
    }
    *w++ = *s++;
  }
  *w = 0;
}

static uint8_t schoolIndex(const char* s) {
  for (uint8_t i = 0; i < 8; i++)
    if (strncmp(s, SCHOOLS[i], 3) == 0) return i;
  return 0;
}

static uint16_t schoolColor(uint8_t s) {
  return tft.color565(SCHOOL_RGB[s][0], SCHOOL_RGB[s][1], SCHOOL_RGB[s][2]);
}

// ---------------------------------------------------------------------------
// SD card and data loading
// ---------------------------------------------------------------------------
static bool mountSD() {
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (SD.begin(SD_CS, sdSPI, 20000000)) return true;
  SD.end();
  return SD.begin(SD_CS, sdSPI, 4000000);
}

static bool loadClasses() {
  char path[48];
  snprintf(path, sizeof(path), "%s/index/classes.txt", packRoot());
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  nClasses = 0;
  char line[96];
  while (f.available() && nClasses < MAX_CLASSES) {
    size_t n = f.readBytesUntil('\n', line, sizeof(line) - 1);
    line[n] = 0;
    if (n && line[n - 1] == '\r') line[--n] = 0;
    if (n == 0 || line[0] == '#') continue;
    // name|count|maxLevel[|subclassCount|basedOn]
    char* fld[5];
    int nf = 0;
    fld[nf++] = line;
    for (char* q = line; *q && nf < 5; q++) {
      if (*q == '|') { *q = 0; fld[nf++] = q + 1; }
    }
    if (nf < 3) continue;
    ClassInfo& c = classes[nClasses];
    strncpy(c.name, fld[0], sizeof(c.name) - 1);
    c.name[sizeof(c.name) - 1] = 0;
    c.count = (uint16_t)atoi(fld[1]);
    c.maxLevel = (uint8_t)atoi(fld[2]);
    c.subCount = nf > 3 ? (uint8_t)atoi(fld[3]) : 0;
    c.basedOn[0] = 0;
    if (nf > 4) { strncpy(c.basedOn, fld[4], sizeof(c.basedOn) - 1); c.basedOn[sizeof(c.basedOn) - 1] = 0; }
    c.color = tft.color565(CLASS_RGB[nClasses][0], CLASS_RGB[nClasses][1], CLASS_RGB[nClasses][2]);
    nClasses++;
  }
  f.close();
  return nClasses > 0;
}

// Line format: id|level|name|school|flags[|baseName]
//   id: "009" (SRD pack) or "g009" / "n012" (Neon pack: g = SRD spell, n = Neon spell)
//   flags: C = concentration, R = ritual; baseName: SRD spell a Neon reskin is based on
static bool parseIdxLine(char* line, Entry& e) {
  char* f[6];
  int n = 0;
  f[n++] = line;
  for (char* p = line; *p && n < 6; p++) {
    if (*p == '|') { *p = 0; f[n++] = p + 1; }
  }
  if (n < 4) return false;                // flags/base fields may be absent
  e.src = 0;
  const char* idp = f[0];
  if (*idp == 'n') { e.src = 1; idp++; }
  else if (*idp == 'g') idp++;
  e.id = (uint16_t)atoi(idp);
  e.base[0] = 0;
  if (n >= 6) {
    strncpy(e.base, f[5], sizeof(e.base) - 1);
    e.base[sizeof(e.base) - 1] = 0;
    if (!smoothFont) foldAscii(e.base);
  }
  e.level = (uint8_t)atoi(f[1]);
  strncpy(e.name, f[2], sizeof(e.name) - 1);
  e.name[sizeof(e.name) - 1] = 0;
  if (!smoothFont) foldAscii(e.name);
  e.school = schoolIndex(f[3]);
  const char* fl = n > 4 ? f[4] : "";
  e.flags = (uint8_t)((strchr(fl, 'C') ? 1 : 0) | (strchr(fl, 'R') ? 2 : 0) | (strchr(fl, 'P') ? 4 : 0));
  return true;
}

static bool loadClassIndex(int ci) {
  char lower[12];
  strncpy(lower, classes[ci].name, sizeof(lower) - 1);
  lower[sizeof(lower) - 1] = 0;
  for (char* p = lower; *p; p++) *p = (char)tolower((unsigned char)*p);
  char path[64];
  snprintf(path, sizeof(path), "%s/index/%s.idx", packRoot(), lower);
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  nEntries = 0;
  char line[128];
  while (f.available() && nEntries < MAX_ENTRIES) {
    size_t n = f.readBytesUntil('\n', line, sizeof(line) - 1);
    line[n] = 0;
    if (n && line[n - 1] == '\r') line[--n] = 0;
    if (n == 0) continue;
    if (parseIdxLine(line, entries[nEntries])) nEntries++;
  }
  f.close();
  for (int l = 0; l < 10; l++) { levelFirst[l] = 0; levelCount[l] = 0; }
  for (int i = 0; i < nEntries; i++) {
    int l = entries[i].level;
    if (l < 10) {
      if (levelCount[l] == 0) levelFirst[l] = i;
      levelCount[l]++;
    }
  }
  nTabs = 0;
  for (int l = 0; l < 10; l++) if (levelCount[l] > 0) tabLevels[nTabs++] = l;
  return nEntries > 0;
}

// Reads <pack>/spells/NNN.txt into fileBuf and splits header and body.
static bool loadSpell(uint8_t src, uint16_t id) {
  char path[48];
  snprintf(path, sizeof(path), "%s/spells/%03d.txt", src ? NEON_ROOT : SD_ROOT, (int)id);
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  size_t sz = f.size();
  if (sz > FILE_BUF_SZ - 1) sz = FILE_BUF_SZ - 1;
  size_t n = f.readBytes(fileBuf, sz);
  fileBuf[n] = 0;
  f.close();
  if (!smoothFont) foldAscii(fileBuf);

  sv.name = sv.classes = sv.time = sv.range = sv.comp = sv.dur = EMPTY;
  sv.level = 0; sv.school = 0; sv.flags = 0;

  char* p = fileBuf;
  while (*p) {
    char* eol = strchr(p, '\n');
    if (eol) *eol = 0;
    char* next = eol ? eol + 1 : p + strlen(p);
    if (strcmp(p, "---") == 0) { p = next; break; }
    char* eq = strchr(p, '=');
    if (eq) {
      *eq = 0;
      const char* k = p;
      char* v = eq + 1;
      if      (!strcmp(k, "name"))    sv.name = v;
      else if (!strcmp(k, "level"))   sv.level = (uint8_t)atoi(v);
      else if (!strcmp(k, "school"))  sv.school = schoolIndex(v);
      else if (!strcmp(k, "classes")) sv.classes = v;
      else if (!strcmp(k, "time"))    sv.time = v;
      else if (!strcmp(k, "range"))   sv.range = v;
      else if (!strcmp(k, "comp"))    sv.comp = v;
      else if (!strcmp(k, "dur"))     sv.dur = v;
      else if (!strcmp(k, "flags"))   sv.flags = (uint8_t)((strchr(v, 'C') ? 1 : 0) | (strchr(v, 'R') ? 2 : 0) | (strchr(v, 'P') ? 4 : 0));
    }
    p = next;
  }
  sv.body = p;
  return true;
}

static bool loadTextFile(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  size_t sz = f.size();
  if (sz > FILE_BUF_SZ - 1) sz = FILE_BUF_SZ - 1;
  size_t n = f.readBytes(fileBuf, sz);
  fileBuf[n] = 0;
  f.close();
  if (!smoothFont) foldAscii(fileBuf);
  return true;
}

// ---------------------------------------------------------------------------
// Word wrapping (lines point into fileBuf, nothing is copied)
// ---------------------------------------------------------------------------
static void pushLine(const char* s, int len, uint8_t kind, uint8_t first) {
  if (nLines >= MAX_LINES) return;
  lines[nLines].off = (uint16_t)(s - fileBuf);
  lines[nLines].len = (uint8_t)(len > 255 ? 255 : len);
  lines[nLines].kind = kind;
  lines[nLines].first = first;
  nLines++;
}

static int measure(const char* s, int n) {
  char t[64];
  if (n > 63) n = 63;
  memcpy(t, s, n);
  t[n] = 0;
  return tft.textWidth(t);
}

static void wrapPara(const char* s, uint8_t kind, int firstIndent) {
  int spaceW = measure(" ", 1);
  const char* lineStart = s;
  const char* lineEnd = s;
  int lineW = 0;
  int indent = firstIndent;
  bool first = true, haveWord = false;
  const char* p = s;
  while (true) {
    while (*p == ' ') p++;
    if (!*p) break;
    const char* ws = p;
    while (*p && *p != ' ') p++;
    int wl = (int)(p - ws);
    int ww = measure(ws, wl);
    while (ww > viewW && wl > 1) {            // a single word wider than a full line
      if (haveWord) {
        pushLine(lineStart, (int)(lineEnd - lineStart), kind, first ? 1 : 0);
        first = false; indent = 0; haveWord = false;
      }
      int avail = viewW - indent;
      int n = wl > 63 ? 63 : wl;
      while (n > 1 && measure(ws, n) > avail) n--;
      pushLine(ws, n, kind, first ? 1 : 0);
      first = false; indent = 0;
      ws += n; wl -= n;
      ww = measure(ws, wl);
    }
    if (!haveWord) {
      lineStart = ws; lineEnd = p; lineW = ww; haveWord = true;
    } else if (lineW + spaceW + ww > viewW - indent) {
      pushLine(lineStart, (int)(lineEnd - lineStart), kind, first ? 1 : 0);
      first = false;
      indent = 0;
      lineStart = ws; lineEnd = p; lineW = ww;
    } else {
      lineEnd = p;
      lineW += spaceW + ww;
    }
  }
  if (haveWord) pushLine(lineStart, (int)(lineEnd - lineStart), kind, first ? 1 : 0);
}

static void addInfo(uint8_t kind, const char* val) {
  if (!val || !*val) return;
  wrapPara(val, kind, tft.textWidth(KLABEL[kind]));
}

static void buildSpellLines(const Entry& e) {
  setBodyFont();
  nLines = 0;
  readerBase[0] = 0;
  if (e.base[0]) {                       // Neon reskin: show which SRD spell it is
    strncpy(readerBase, e.base, sizeof(readerBase) - 1);
    readerBase[sizeof(readerBase) - 1] = 0;
    pushLine(fileBuf, 0, K_BASE, 1);
  }
  addInfo(K_TIME,  sv.time);
  addInfo(K_RANGE, sv.range);
  addInfo(K_COMP,  sv.comp);
  addInfo(K_DUR,   sv.dur);
  if (!(curPack == 1 && e.src == 0)) addInfo(K_CLS, sv.classes);   // SRD class names mean nothing in Neon mode
  pushLine(fileBuf, 0, K_GAP, 0);
  char* q = sv.body;
  while (*q) {
    char* eol = strchr(q, '\n');
    if (eol) *eol = 0;
    char* next = eol ? eol + 1 : q + strlen(q);
    if (!strcmp(q, "@HIGHER"))       pushLine(q, 0, K_HEAD, 1);
    else if (!strcmp(q, "@CANTRIP")) pushLine(q, 0, K_HEAD, 2);
    else if (*q)                     wrapPara(q, K_BODY, 0);
    q = next;
  }
}

static bool openSpell(const Entry& e) {
  if (!loadSpell(e.src, e.id)) return false;
  buildSpellLines(e);
  scrollLine = 0;
  strncpy(readerTitle, e.name[0] ? e.name : sv.name, sizeof(readerTitle) - 1);
  readerTitle[sizeof(readerTitle) - 1] = 0;
  char lvl[12];
  if (sv.level == 0) strcpy(lvl, "Cantrip");
  else snprintf(lvl, sizeof(lvl), "Level %d", sv.level);
  snprintf(readerSub, sizeof(readerSub), "%s %s%s%s%s", lvl, SCHOOLS[sv.school],
           (sv.flags & 1) ? " - Conc." : "", (sv.flags & 2) ? " - Ritual" : "",
           (sv.flags & 4) ? " - PHB only" : "");
  readerBack = SCR_CLASS;
  screen = SCR_READER;
  return true;
}

static void openAbout() {
  setBodyFont();
  nLines = 0;
  char apath[48];
  snprintf(apath, sizeof(apath), "%s/ATTRIBUTION.txt", packRoot());
  if (!loadTextFile(apath)) {
    strcpy(fileBuf, "ATTRIBUTION.txt not found on the SD card.");
  }
  char* q = fileBuf;
  while (*q) {
    char* eol = strchr(q, '\n');
    if (eol) *eol = 0;
    char* next = eol ? eol + 1 : q + strlen(q);
    if (*q) wrapPara(q, K_BODY, 0);
    q = next;
  }
  scrollLine = 0;
  strcpy(readerTitle, "About");
  strcpy(readerSub, curPack == 1 ? "Neon Odyssey - Avantris" : "SRD 5.2.1 - CC BY 4.0");
  readerBack = SCR_HOME;
  screen = SCR_READER;
}

// ---- Subclasses (Neon pack only): /neon/sub/<class>.idx and /neon/sub/NNN.txt ----
static const char* subLabel(uint8_t k) { return k == 2 ? "SRD" : k == 1 ? "Overdrive" : "Handbook"; }

static bool loadSubIndexRoot(const char* root, const char* cls, SubEntry* out, int& n) {
  char lower[12];
  strncpy(lower, cls, sizeof(lower) - 1);
  lower[sizeof(lower) - 1] = 0;
  for (char* p = lower; *p; p++) *p = (char)tolower((unsigned char)*p);
  char path[48];
  snprintf(path, sizeof(path), "%s/sub/%s.idx", root, lower);
  n = 0;
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  char line[128];
  while (f.available() && n < MAX_SUBS) {
    size_t k = f.readBytesUntil('\n', line, sizeof(line) - 1);
    line[k] = 0;
    if (k && line[k - 1] == '\r') line[--k] = 0;
    if (k == 0) continue;
    char* fld[4];
    int nf = 0;
    fld[nf++] = line;
    for (char* q = line; *q && nf < 4; q++) {
      if (*q == '|') { *q = 0; fld[nf++] = q + 1; }
    }
    if (nf < 3) continue;                       // sid|Name|Source|S?
    SubEntry& se = out[n];
    se.id = (uint16_t)atoi(fld[0]);
    strncpy(se.name, fld[1], sizeof(se.name) - 1);
    se.name[sizeof(se.name) - 1] = 0;
    if (!smoothFont) foldAscii(se.name);
    se.fromOverdrive = strncmp(fld[2], "Over", 4) == 0 ? 1 : (strncmp(fld[2], "SRD", 3) == 0 ? 2 : 0);
    se.hasSpells = nf > 3 && fld[3][0] == 'S';
    n++;
  }
  f.close();
  return n > 0;
}

static bool loadSubIndex(int ci) { return loadSubIndexRoot(packRoot(), classes[ci].name, subs, nSubs); }

static bool loadSub(uint16_t id) {
  char path[48];
  snprintf(path, sizeof(path), "%s/sub/%03d.txt", packRoot(), (int)id);
  return loadTextFile(path);
}

static bool openSub(int si) {
  if (!loadSub(subs[si].id)) return false;
  setBodyFont();
  nLines = 0;
  const char *tag = EMPTY, *cls = EMPTY;
  char* p = fileBuf;
  while (*p) {
    char* eol = strchr(p, '\n');
    if (eol) *eol = 0;
    char* next = eol ? eol + 1 : p + strlen(p);
    if (strcmp(p, "---") == 0) { p = next; break; }
    char* eq = strchr(p, '=');
    if (eq) {
      *eq = 0;
      if      (!strcmp(p, "tagline")) tag = eq + 1;
      else if (!strcmp(p, "class"))   cls = eq + 1;
    }
    p = next;
  }
  if (*tag) { wrapPara(tag, K_TAG, 0); pushLine(fileBuf, 0, K_GAP, 0); }
  while (*p) {
    char* eol = strchr(p, '\n');
    if (eol) *eol = 0;
    char* next = eol ? eol + 1 : p + strlen(p);
    if (!strcmp(p, "@SPELLS")) { pushLine(fileBuf, 0, K_GAP, 0); pushLine(p, 0, K_HEAD, 3); }
    else if (*p)               wrapPara(p, K_BODY, 0);
    p = next;
  }
  scrollLine = 0;
  strncpy(readerTitle, subs[si].name, sizeof(readerTitle) - 1);
  readerTitle[sizeof(readerTitle) - 1] = 0;
  snprintf(readerSub, sizeof(readerSub), "%s subclass - %s", cls, subLabel(subs[si].fromOverdrive));
  readerBack = SCR_SUBS;
  screen = SCR_READER;
  return true;
}

static void openSubs(int ci, Screen back) {
  if (!loadSubIndex(ci)) { showError("Could not read the subclass list."); return; }
  curClass = ci;
  subsBack = back;
  screen = SCR_SUBS;
  drawSubs();
}

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------
static bool inRect(int x, int y, int rx, int ry, int rw, int rh) {
  return x >= rx && x < rx + rw && y >= ry && y < ry + rh;
}

static void drawFit(const char* s, int x, int y, int maxW) {
  char t[64], u[70];
  strncpy(t, s, sizeof(t) - 1);
  t[sizeof(t) - 1] = 0;
  if (tft.textWidth(t) <= maxW) { tft.drawString(t, x, y); return; }
  int n = (int)strlen(t);
  while (n > 1) {
    n--;
    t[n] = 0;
    snprintf(u, sizeof(u), "%s...", t);
    if (tft.textWidth(u) <= maxW) break;
  }
  tft.drawString(u, x, y);
}

static void drawBackBtn(int h) {
  tft.fillRoundRect(2, 2, 40, h, 5, C_PANEL);
  tft.drawLine(26, 2 + h / 2 - 7, 17, 2 + h / 2, C_TEXT);
  tft.drawLine(17, 2 + h / 2, 26, 2 + h / 2 + 7, C_TEXT);
  tft.drawLine(27, 2 + h / 2 - 7, 18, 2 + h / 2, C_TEXT);
  tft.drawLine(18, 2 + h / 2, 27, 2 + h / 2 + 7, C_TEXT);
}

static void drawArrowBtn(int x, int y, int w, int h, bool up) {
  tft.fillRoundRect(x, y, w, h, 5, C_PANEL);
  int cx = x + w / 2, cy = y + h / 2;
  if (up) tft.fillTriangle(cx, cy - 5, cx - 7, cy + 4, cx + 7, cy + 4, C_TEXT);
  else    tft.fillTriangle(cx, cy + 5, cx - 7, cy - 4, cx + 7, cy - 4, C_TEXT);
}

// ---- Class icons: raw RGB565 from <book>/icons/<class>_{t,w,b}.bin (optional; made by tools/build_icons.py) ----
//   t = 36x36 tile icon, w = 34x34 dim watermark for compact tiles, b = 160x160 dim background of the class screen
#define ICON_T 36
#define ICON_W 34
#define BG_SZ  160
#define BG_X   ((SCREEN_W - BG_SZ) / 2)
#define BG_Y   (LIST_Y + (SCREEN_H - LIST_Y - BG_SZ) / 2)
static uint16_t* bgCache = nullptr;
static uint16_t iconBuf[ICON_T * ICON_T];

static bool iconFile(File& f, const char* cls, char kind) {
  char lower[12], path[56];
  strncpy(lower, cls, sizeof(lower) - 1);
  lower[sizeof(lower) - 1] = 0;
  for (char* p = lower; *p; p++) *p = (char)tolower((unsigned char)*p);
  snprintf(path, sizeof(path), "%s/icons/%s_%c.bin", packRoot(), lower, kind);
  f = SD.open(path, FILE_READ);
  return (bool)f;
}

// draws rows [row0, row0+rows) of a square icon of size sz with its top-left at (x, y)
static bool drawIcon(const char* cls, char kind, int sz, int x, int y, int row0 = 0, int rows = -1) {
  File f;
  if (!iconFile(f, cls, kind)) return false;
  if (rows < 0 || rows > sz - row0) rows = sz - row0;
  if (f.size() < (size_t)sz * sz * 2 || !f.seek((size_t)row0 * sz * 2)) { f.close(); return false; }
  size_t want = (size_t)sz * rows * 2;
  size_t got = f.readBytes((char*)iconBuf, want);
  f.close();
  if (got != want) return false;
  tft.setSwapBytes(true);
  tft.pushImage(x, y, sz, rows, iconBuf);
  return true;
}

static void bgFree() { if (bgCache) { free(bgCache); bgCache = nullptr; } }

static void bgLoad(const char* cls) {
  bgFree();
  if (smoothFont) return;                     // transparent text needs the bitmap fonts
  File f;
  if (!iconFile(f, cls, 'b')) return;
  size_t bytes = (size_t)BG_SZ * BG_SZ * 2;
  if (f.size() < bytes) { f.close(); return; }
  bgCache = (uint16_t*)malloc(bytes);
  if (!bgCache) { f.close(); return; }
  if (f.readBytes((char*)bgCache, bytes) != bytes) bgFree();
  f.close();
}

// repaints a rectangle of the class screen: background color plus the part of the faded art inside it
static void bgPaint(int x, int y, int w, int h) {
  tft.fillRect(x, y, w, h, C_BG);
  if (!bgCache) return;
  int x0 = x > BG_X ? x : BG_X, x1 = (x + w < BG_X + BG_SZ) ? x + w : BG_X + BG_SZ;
  int y0 = y > BG_Y ? y : BG_Y, y1 = (y + h < BG_Y + BG_SZ) ? y + h : BG_Y + BG_SZ;
  if (x1 <= x0 || y1 <= y0) return;
  tft.setSwapBytes(true);
  for (int yy = y0; yy < y1; yy++) tft.pushImage(x0, yy, x1 - x0, 1, bgCache + (yy - BG_Y) * BG_SZ + (x0 - BG_X));
}

static void setFg(uint16_t fg, uint16_t bg) { if (bgCache) tft.setTextColor(fg); else tft.setTextColor(fg, bg); }

// Class tiles: 4 columns; 2 rows of big tiles (SRD) or compact ones (Neon). Classes without a
// spell list go to a separate strip at the bottom (only when the book has both kinds).
static bool splitHome() {
  if (nClasses <= 8) return false;
  for (int i = 0; i < nClasses; i++) if (classes[i].count == 0) return true;
  return false;
}

// Calculate tile dimensions adapting to current orientation
static void tileRect(int i, int& x, int& y, int& w, int& h) {
  bool portrait = isPortrait();
  uint16_t logW = getLogicalWidth();

  // Landscape: 4 columns, 76px width, normal size (62px height)
  // Portrait: 2 columns, adapt to available space
  int numCols = portrait ? 2 : 4;
  int tileW = portrait ? ((logW - 16) / 2) : 76;
  int colSpacing = portrait ? (logW / 2) : 79;
  int tileH = portrait ? 55 : 62;  // slightly smaller in portrait
  int rowSpacing = portrait ? 60 : 68;

  w = tileW;

  if (splitHome()) {
    int rank = 0;
    for (int k = 0; k < i; k++) if ((classes[k].count == 0) == (classes[i].count == 0)) rank++;
    x = 4 + (rank % numCols) * colSpacing;
    if (classes[i].count > 0) { h = 35; y = 50 + (rank / numCols) * 38; }
    else                      { h = 28; y = (portrait ? 150 : 182) + (rank / numCols) * 30; }
    return;
  }

  // Always use normal sizing (never compact), adapt spacing to orientation
  h = tileH;
  x = 4 + (i % numCols) * colSpacing;
  y = 50 + (i / numCols) * rowSpacing;
}

static void drawHome() {
  tft.fillScreen(C_BG);
  tft.setTextDatum(TC_DATUM);
  setTitleFont();
  tft.setTextColor(C_GOLD, C_BG);
  tft.drawString("Mystical Grimoire", SCREEN_W / 2, 2);
  setBodyFont();

  bool portrait = isPortrait();
  uint16_t logW = getLogicalWidth();

  // Adapt tabs to orientation
  int tabAvailW = logW - 8;  // margin 4 on each side
  int tabW = tabAvailW / 2;  // split available width for 2 tabs
  int tabX0 = 4;

  for (int t = 0; t < 2; t++) {                       // tabs: Spells | Spellbooks
    bool sel = (t == homeTab);
    uint16_t bg = sel ? C_SEL : C_PANEL;
    int tabX = tabX0 + t * tabW;
    tft.fillRoundRect(tabX, 30, tabW - 2, 18, 5, bg);
    tft.setTextColor(sel ? C_TEXT : C_DIM, bg);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(t == 0 ? (curPack == 1 ? "Neon spells" : "D&D spells") : "Spellbooks", tabX + (tabW - 2) / 2, 39);
  }
  tft.setTextDatum(TL_DATUM);
  if (homeTab == 1) sbDrawHomeList();
  batteryDrawIcon();
  for (int i = 0; homeTab == 0 && i < nClasses; i++) {
    int x, y, w, h;
    tileRect(i, x, y, w, h);
    tft.fillRoundRect(x, y, w, h, 6, C_PANEL);
    tft.drawRoundRect(x, y, w, h, 6, classes[i].color);
    tft.setTextDatum(MC_DATUM);
    bool icon = false;
    if (h > 50)       icon = drawIcon(classes[i].name, 't', ICON_T, x + (w - ICON_T) / 2, y + 3);
    else if (h > 30)  icon = drawIcon(classes[i].name, 'w', ICON_W, x + (w - ICON_W) / 2, y + (h - ICON_W) / 2);
    else              icon = drawIcon(classes[i].name, 'w', ICON_W, x + (w - ICON_W) / 2, y, 3, h);
    if (icon) tft.setTextColor(C_TEXT); else tft.setTextColor(C_TEXT, C_PANEL);
    int ny = (h > 50) ? (icon ? y + 47 : y + 24) : (icon ? y + h / 2 : (h > 30 ? y + 11 : y + h / 2));
    int cy = (h > 50) ? y + 44 : y + 26;
    drawFit(classes[i].name, x + w / 2, ny, w - 4);
    if (h <= 30 || icon) continue;
    char b[20];
    if (classes[i].count > 0) snprintf(b, sizeof(b), "%d spells", classes[i].count);
    else                      snprintf(b, sizeof(b), "%d subclasses", classes[i].subCount);
    if (icon) tft.setTextColor(C_DIM); else tft.setTextColor(C_DIM, C_PANEL);
    drawFit(b, x + w / 2, cy, w - 4);
  }
  if (homeTab == 0 && splitHome()) {
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString("No spell list (subclasses only)", 6, portrait ? 135 : 164);
  }

  // Adapt bottom buttons to orientation
  tft.setTextDatum(MC_DATUM);
  int btnY = portrait ? 195 : 212;
  int btnH = 24;
  int optionsBtnW = portrait ? (logW / 2 - 10) : 82;
  int optionsBtnX = portrait ? 5 : 6;
  int aboutBtnX = portrait ? (logW / 2 + 5) : (SCREEN_W - 88);
  int aboutBtnW = portrait ? (logW / 2 - 10) : 82;
  int centerX_options = portrait ? (logW / 4) : 47;
  int centerX_about = portrait ? (logW * 3 / 4) : (SCREEN_W - 47);

  tft.fillRoundRect(optionsBtnX, btnY, optionsBtnW, btnH, 5, C_PANEL);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.drawString("Options", centerX_options, btnY + 12);

  tft.fillRoundRect(aboutBtnX, btnY, aboutBtnW, btnH, 5, C_PANEL);
  tft.drawString("About", centerX_about, btnY + 12);

  tft.setTextDatum(TL_DATUM);
}

static void drawPackBtn(int y, int pack, const char* title, const char* sub, bool avail) {
  bool sel = (curPack == pack);
  uint16_t bg = sel ? C_SEL : C_PANEL;
  tft.fillRoundRect(10, y, 300, 38, 8, bg);
  if (sel) tft.drawRoundRect(10, y, 300, 38, 8, C_GOLD);
  tft.drawCircle(28, y + 19, 8, avail ? C_TEXT : C_DIM);      // radio dot
  if (sel) tft.fillCircle(28, y + 19, 4, C_GOLD);
  tft.setTextDatum(TL_DATUM);
  setBodyFont();
  tft.setTextColor(avail ? C_TEXT : C_DIM, bg);
  drawFit(title, 44, y + 2, 258);
  tft.setTextColor(C_DIM, bg);
  drawFit(sub, 44, y + 2 + lineH - 1, 258);
}

// Options layout - adapts to landscape and portrait orientations
static void drawOptions() {
  tft.fillScreen(C_BG);
  drawBackBtn(24);
  tft.setTextDatum(TL_DATUM);
  setBodyFont();
  tft.setTextColor(C_GOLD, C_BG);
  tft.drawString("Options", 52, 6);

  bool portrait = isPortrait();
  uint16_t logW = getLogicalWidth();

  // Spell book section
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("Spell book", 10, 31);
  batteryDrawOptions();

  drawPackBtn(50, 0, "D&D 2024 - SRD 5.2.1", "Official classes, CC BY 4.0", true);
  drawPackBtn(92, 1, "Neon Odyssey (playtest)", neonAvailable ? "Playtest classes + subclasses" : "Not on the SD card (/neon)", neonAvailable);

  // Brightness row - adapt to portrait
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("Brightness", 10, 138 + 7);

  int briMinusX = portrait ? 70 : 170;
  int briPlusX = portrait ? (logW - 10 - 44) : 266;
  int briTextX = portrait ? (logW / 2) : 240;

  tft.fillRoundRect(briMinusX, 138, 44, 30, 6, C_PANEL);
  tft.fillRoundRect(briPlusX, 138, 44, 30, 6, C_PANEL);
  tft.fillRect(briMinusX + 10, 152, 24, 3, briLevel > 1 ? C_TEXT : C_DIM);
  tft.fillRect(briPlusX + 10, 152, 24, 3, briLevel < 5 ? C_TEXT : C_DIM);
  tft.fillRect(briPlusX + 20, 142, 3, 22, briLevel < 5 ? C_TEXT : C_DIM);
  tft.setTextDatum(MC_DATUM);
  char b[8];
  snprintf(b, sizeof(b), "%d/5", briLevel);
  tft.setTextColor(C_GOLD, C_BG);
  tft.drawString(b, briTextX, 153);

  // Deep sleep + rotate buttons - adapt to portrait
  int btnY = 176;
  int btnH = 30;
  int leftBtnX = 10;
  int leftBtnW = portrait ? (logW / 2 - 15) : 145;
  int rightBtnX = portrait ? (logW / 2 + 5) : 165;
  int rightBtnW = portrait ? (logW / 2 - 15) : 145;
  int btnTextCenterX_left = portrait ? (logW / 4) : 82;
  int btnTextCenterX_right = portrait ? (logW * 3 / 4) : 237;

  tft.fillRoundRect(leftBtnX, btnY, leftBtnW, btnH, 8, C_PANEL);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Deep sleep", btnTextCenterX_left, btnY + 15);

  tft.fillRoundRect(rightBtnX, btnY, rightBtnW, btnH, 8, C_PANEL);
  tft.drawString(getRotationLabel(currentRot), btnTextCenterX_right, btnY + 15);

  // Soft sleep (screen off) - adapt to portrait
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_TEXT, C_BG);
  int screenOffY = 210 + 6;
  tft.drawString("Screen off", 10, screenOffY);

  int screenOffBtnX = portrait ? 10 : 170;
  int screenOffBtnW = portrait ? (logW - 20) : 140;
  int screenOffBtnY = portrait ? (screenOffY + 25) : 210;
  int screenOffTextX = portrait ? (logW / 2) : 240;
  int screenOffTextY = portrait ? (screenOffBtnY + 14) : 224;

  tft.fillRoundRect(screenOffBtnX, screenOffBtnY, screenOffBtnW, 28, 8, C_PANEL);
  tft.setTextDatum(MC_DATUM);
  char s[16];
  if (SOFT_MIN[softIdx]) snprintf(s, sizeof(s), "%d min", SOFT_MIN[softIdx]);
  else snprintf(s, sizeof(s), "Never");
  tft.setTextColor(C_GOLD, C_PANEL);
  tft.drawString(s, screenOffTextX, screenOffTextY);

  // Version string
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("v0.8", portrait ? 10 : (SCREEN_W - 40), 6);
}

static void drawSubs() {
  tft.fillScreen(C_BG);
  drawBackBtn(24);
  tft.setTextDatum(TL_DATUM);
  setBodyFont();
  tft.setTextColor(classes[curClass].color, C_BG);
  char t[40];
  snprintf(t, sizeof(t), "%s subclasses", classes[curClass].name);
  tft.drawString(t, 52, 6);

  bool portrait = isPortrait();
  uint16_t logW = getLogicalWidth();

  int rectX = 6, rectW = portrait ? (logW - 12) : 308;
  int nameX = 18, maxNameW = portrait ? (logW - 58) : 200;
  int labelX = portrait ? (logW - 26) : 232;

  for (int i = 0; i < nSubs; i++) {
    int y = 36 + i * 38;
    tft.fillRoundRect(rectX, y, rectW, 34, 5, C_PANEL);
    tft.fillRect(rectX, y + 4, 4, 26, classes[curClass].color);
    tft.setTextColor(C_TEXT, C_PANEL);
    drawFit(subs[i].name, nameX, y + 9, maxNameW);
    tft.setTextColor(C_DIM, C_PANEL);
    tft.drawString(subLabel(subs[i].fromOverdrive), labelX, y + 9);
  }
}

static void drawClassHeader() {
  tft.fillRect(0, 0, SCREEN_W, TAB_Y, C_BG);
  drawBackBtn(24);
  tft.setTextDatum(TL_DATUM);
  setBodyFont();
  tft.setTextColor(classes[curClass].color, C_BG);
  tft.drawString(classes[curClass].name, 52, 6);

  bool portrait = isPortrait();
  uint16_t logW = getLogicalWidth();

  int upBtnX = portrait ? (logW - 82) : (SCREEN_W - 82);
  int downBtnX = portrait ? (logW - 41) : (SCREEN_W - 41);
  int subBtnX = portrait ? (logW - 134) : (SCREEN_W - 134);

  if (classes[curClass].subCount > 0) {
    tft.fillRoundRect(subBtnX, 2, 48, 24, 5, C_SEL);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(C_TEXT, C_SEL);
    tft.drawString("Sub", subBtnX + 24, 14);
    tft.setTextDatum(TL_DATUM);
  }
  drawArrowBtn(upBtnX, 2, 40, 24, true);
  drawArrowBtn(downBtnX, 2, 40, 24, false);
}

static void drawTabs() {
  tft.fillRect(0, TAB_Y, SCREEN_W, TAB_H + 2, C_BG);
  if (nTabs == 0) return;

  bool portrait = isPortrait();
  uint16_t logW = getLogicalWidth();

  // Landscape: 300px available for tabs, max 32px per tab
  // Portrait: adapt to available space
  int availWidth = portrait ? (logW - 20) : 300;
  int tw = availWidth / nTabs;
  if (tw > 32) tw = 32;
  int x0 = portrait ? (10 + (logW - 20 - nTabs * tw) / 2) : ((SCREEN_W - nTabs * tw) / 2);

  tft.setTextDatum(MC_DATUM);
  setBodyFont();
  for (int i = 0; i < nTabs; i++) {
    int l = tabLevels[i];
    bool sel = (l == curLevel);
    uint16_t bg = sel ? C_SEL : C_PANEL;
    tft.fillRoundRect(x0 + i * tw, TAB_Y, tw - 2, TAB_H, 4, bg);
    char b[3];
    if (l == 0) strcpy(b, "C"); else { b[0] = (char)('0' + l); b[1] = 0; }
    tft.setTextColor(sel ? C_TEXT : C_DIM, bg);
    tft.drawString(b, x0 + i * tw + (tw - 2) / 2, TAB_Y + TAB_H / 2);
  }
  tft.setTextDatum(TL_DATUM);
}

static void drawList() {
  bgPaint(0, LIST_Y, SCREEN_W, SCREEN_H - LIST_Y);
  int cnt = levelCount[curLevel], first = levelFirst[curLevel];

  bool portrait = isPortrait();
  uint16_t logW = getLogicalWidth();

  // Adapt column positions to orientation
  int rectX = 6, rectW = portrait ? (logW - 12) : 298;
  int nameX = 16, maxNameW = portrait ? (logW - 54) : 226;
  int flagCX = portrait ? (logW - 32) : 250;
  int flagRX = portrait ? (logW - 18) : 264;
  int schoolX = portrait ? (logW - 4) : 278;
  int barX = portrait ? (logW - 4) : 310;

  tft.setTextDatum(TL_DATUM);
  setBodyFont();
  for (int r = 0; r < LIST_ROWS; r++) {
    int k = listScroll + r;
    if (k >= cnt) break;
    const Entry& e = entries[first + k];
    int y = LIST_Y + r * ROW_H;
    int ty = y + (ROW_H - (lineH - 2)) / 2 - 1;
    if (bgCache) tft.fillRect(rectX + 2, y + ROW_H - 2, rectW - 4, 1, C_PANEL);
    else         tft.fillRoundRect(rectX, y + 1, rectW, ROW_H - 3, 4, C_PANEL);
    tft.fillRect(rectX, y + 4, 4, ROW_H - 9, schoolColor(e.school));
    setFg((e.flags & 4) ? C_DIM : C_TEXT, C_PANEL);
    drawFit(e.name, nameX, ty, maxNameW);
    setFg(C_GOLD, C_PANEL);
    if (e.flags & 1) tft.drawString("C", flagCX, ty);
    if (e.flags & 2) tft.drawString("R", flagRX, ty);
    char ab[4];
    memcpy(ab, SCHOOLS[e.school], 3);
    ab[3] = 0;
    setFg(C_DIM, C_PANEL);
    tft.drawString(ab, schoolX, ty);
  }
  if (cnt > LIST_ROWS) {
    int barH = SCREEN_H - LIST_Y;
    int th = barH * LIST_ROWS / cnt;
    if (th < 12) th = 12;
    int ty = LIST_Y + (barH - th) * listScroll / (cnt - LIST_ROWS);
    tft.fillRect(barX, LIST_Y, 4, barH, C_PANEL);
    tft.fillRect(barX, ty, 4, th, C_DIM);
  }
}

static void drawClassScreen() {
  tft.fillScreen(C_BG);
  drawClassHeader();
  drawTabs();
  drawList();
}

static void drawLine(int li, int y) {
  const Line& L = lines[li];
  if (L.kind == K_GAP) return;
  tft.setTextDatum(TL_DATUM);
  if (L.kind == K_HEAD) {
    tft.setTextColor(C_GOLD, C_BG);
    tft.drawString(L.first == 1 ? "Using a Higher-Level Spell Slot"
                   : L.first == 2 ? "Cantrip Upgrade" : "Subclass Spells (always prepared)", MARGIN, y);
    return;
  }
  if (L.kind == K_BASE) {
    tft.setTextColor(C_GOLD, C_BG);
    tft.drawString(KLABEL[K_BASE], MARGIN, y);
    tft.setTextColor(C_TEXT, C_BG);
    drawFit(readerBase, MARGIN + tft.textWidth(KLABEL[K_BASE]), y, viewW - tft.textWidth(KLABEL[K_BASE]));
    return;
  }
  char t[130];
  int n = L.len;
  if (n > 127) n = 127;
  memcpy(t, fileBuf + L.off, n);
  t[n] = 0;
  int x = MARGIN;
  if (L.first && KLABEL[L.kind][0]) {
    tft.setTextColor(C_GOLD, C_BG);
    tft.drawString(KLABEL[L.kind], x, y);
    x += tft.textWidth(KLABEL[L.kind]);
  }
  tft.setTextColor(L.kind == K_TAG ? C_GOLD : C_TEXT, C_BG);
  tft.drawString(t, x, y);
}

static int visLines() { return (SCREEN_H - readY - readerBar) / lineH; }

static void drawReaderContent() {
  int vis = visLines();
  int areaH = SCREEN_H - readY - readerBar;

  bool portrait = isPortrait();
  uint16_t logW = getLogicalWidth();
  int scrollBarX = portrait ? (logW - 5) : (SCREEN_W - 5);

  tft.fillRect(0, readY, SCREEN_W, areaH, C_BG);
  setBodyFont();
  for (int i = 0; i < vis; i++) {
    int li = scrollLine + i;
    if (li >= nLines) break;
    drawLine(li, readY + i * lineH);
  }
  if (nLines > vis) {
    int barH = areaH;
    int th = barH * vis / nLines;
    if (th < 12) th = 12;
    int ty = readY + (barH - th) * scrollLine / (nLines - vis);
    tft.fillRect(scrollBarX, readY, 4, barH, C_PANEL);
    tft.fillRect(scrollBarX, ty, 4, th, C_DIM);
  }
  sbDrawReaderBar();
}

static void drawReaderScreen() {
  tft.fillScreen(C_BG);
  tft.fillRect(0, 0, SCREEN_W, hdrH, C_PANEL2);
  drawBackBtn(hdrH - 4);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_GOLD, C_PANEL2);
  drawFit(readerTitle, 48, 4, SCREEN_W - 48 - 86);
  tft.setTextColor(C_DIM, C_PANEL2);
  drawFit(readerSub, 48, 4 + lineH, SCREEN_W - 48 - 86);
  drawArrowBtn(SCREEN_W - 82, 2, 40, hdrH - 4, true);
  drawArrowBtn(SCREEN_W - 41, 2, 40, hdrH - 4, false);
  drawReaderContent();
}

static void drawError() {
  tft.fillScreen(C_BG);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_ERR, C_BG);
  tft.drawString("Problem reading the SD card", MARGIN, 20);
  tft.setTextColor(C_TEXT, C_BG);
  drawFit(errMsg, MARGIN, 20 + lineH * 2, SCREEN_W - 2 * MARGIN);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("FAT32 card, folders /grimoire (and /neon) at the root.", MARGIN, 20 + lineH * 4);
  tft.drawString("Tap anywhere to retry.", MARGIN, 20 + lineH * 5);
}

static void redrawAll() {
  switch (screen) {
    case SCR_HOME:   drawHome(); break;
    case SCR_CLASS:  drawClassScreen(); break;
    case SCR_READER: drawReaderScreen(); break;
    case SCR_ERROR:  drawError(); break;
    case SCR_OPTIONS: drawOptions(); break;
    case SCR_SUBS:   drawSubs(); break;
    default:         sbDrawScreen(); break;
  }
}

static void showError(const char* msg) {
  strncpy(errMsg, msg, sizeof(errMsg) - 1);
  errMsg[sizeof(errMsg) - 1] = 0;
  screen = SCR_ERROR;
  drawError();
}

// ---------------------------------------------------------------------------
// Boot, navigation and touch
// ---------------------------------------------------------------------------
static void bootData() {
  tft.fillScreen(C_BG);
  tft.setTextDatum(MC_DATUM);
  setBodyFont();
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("Reading SD card...", SCREEN_W / 2, SCREEN_H / 2);
  tft.setTextDatum(TL_DATUM);
  if (!mountSD()) { showError("Could not mount the microSD card."); return; }
  if (!fontTried) {
    fontTried = true;
    if (SD.exists(SD_ROOT "/fonts/body.vlw")) {
      tft.loadFont(SD_ROOT "/fonts/body.vlw", SD);
      smoothFont = true;
    }
    applyFont();
  }
  if (!SD.exists(CHAR_DIR)) SD.mkdir(CHAR_DIR);
  neonAvailable = SD.exists(NEON_ROOT "/index/classes.txt");
  if (curPack == 1 && !neonAvailable) { curPack = 0; applyTheme(); }
  if (!loadClasses()) { showError("Book index not found (/grimoire/index)."); return; }
  screen = SCR_HOME;
  drawHome();
}

static void blWrite(uint8_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(TFT_BL, duty);
#else
  ledcWrite(0, duty);
#endif
}

static void applyBrightness() { blWrite(BRI_DUTY[briLevel - 1]); }

static void setBrightness(int level) {
  if (level < 1) level = 1;
  if (level > 5) level = 5;
  if (level == briLevel) return;
  briLevel = (uint8_t)level;
  applyBrightness();
  prefs.putUChar("bri", briLevel);
  drawOptions();
}

// Rotation helpers: returns true if currently in portrait orientation (90° or 270°)
static bool isPortrait() {
  return (currentRot == 1 || currentRot == 3);
}

// Returns logical screen width based on current rotation
static uint16_t getLogicalWidth() {
  return isPortrait() ? 240 : 320;
}

// Returns logical screen height based on current rotation
static uint16_t getLogicalHeight() {
  return isPortrait() ? 320 : 240;
}

// Returns rotation label string for display (landscape only)
static const char* getRotationLabel(uint8_t rot) {
  return (rot == 2) ? "Rotate: 180" : "Rotate: 0";
}

// Deep sleep. BOOT (GPIO 0, active low) wakes the board and restarts the program.
static void setRotation(uint8_t r) {
  currentRot = (r == 2) ? 2 : 0;  // Landscape only: 0° or 180°
  // Map to TFT_eSPI rotations: 0->1 (landscape), 2->3 (landscape 180)
  tft.setRotation((currentRot == 2) ? 3 : 1);
  prefs.putUChar("rot", currentRot);
  redrawAll();
}

// Cycles rotation to next state: 0 -> 2 -> 0 (landscape only: 0° and 180°)
static void cycleRotation() {
  setRotation((currentRot == 0) ? 2 : 0);
}

// ---- soft sleep -----------------------------------------------------------
static void idleWake() {
  if (idleState) { idleState = 0; applyBrightness(); }
  lastActive = millis();
}

static void setSoftIdx(int i) {
  softIdx = (uint8_t)(i % 5);
  prefs.putUChar("soff", softIdx);
  idleWake();
  drawOptions();
}

// Call every loop pass. Dims after the chosen idle time, turns the backlight off 10 s later.
static void idleTick() {
  if (!SOFT_MIN[softIdx]) { if (idleState) idleWake(); return; }
  uint32_t idle = millis() - lastActive;
  uint32_t limit = (uint32_t)SOFT_MIN[softIdx] * 60000UL;
  if (idleState == 0 && idle >= limit) { idleState = 1; blWrite(SOFT_DIM_DUTY < BRI_DUTY[briLevel - 1] ? SOFT_DIM_DUTY : BRI_DUTY[briLevel - 1]); }
  else if (idleState == 1 && idle >= limit + SOFT_DIM_MS) { idleState = 2; blWrite(0); }
}

static void enterSleep() {
  tft.fillScreen(TFT_BLACK);
  blWrite(0);
  digitalWrite(PIN_LED_R, HIGH); digitalWrite(PIN_LED_G, HIGH); digitalWrite(PIN_LED_B, HIGH);
  SD.end();
  while (digitalRead(BOOT_BTN_PIN) == LOW) delay(10);      // wait for BOOT to be released
  delay(50);
  rtc_gpio_pullup_en(GPIO_NUM_0);
  rtc_gpio_pulldown_dis(GPIO_NUM_0);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
  esp_deep_sleep_start();
}

// Switches the active book (SRD <-> Neon Odyssey), remembers it, reloads the class list.
static void setPack(uint8_t p) {
  if (p == curPack) return;
  if (p == 1 && !neonAvailable) return;
  uint8_t old = curPack;
  curPack = p;
  if (!loadClasses()) {                      // keep the old book if the new one is broken
    curPack = old;
    loadClasses();
  } else {
    prefs.putUChar("pack", curPack);
  }
  applyTheme();
  drawOptions();
}

static void startClass(int ci) {
  if (classes[ci].count == 0) {              // no spell list (martial class): straight to its subclasses
    if (classes[ci].subCount > 0) openSubs(ci, SCR_HOME);
    return;
  }
  if (!loadClassIndex(ci)) { showError("Could not read the class index."); return; }
  curClass = ci;
  bgLoad(classes[ci].name);
  curLevel = nTabs ? tabLevels[0] : 0;
  listScroll = 0;
  screen = SCR_CLASS;
  drawClassScreen();
}

static void listScrollBy(int d) {
  int cnt = levelCount[curLevel];
  int maxS = cnt > LIST_ROWS ? cnt - LIST_ROWS : 0;
  int ns = listScroll + d;
  if (ns < 0) ns = 0;
  if (ns > maxS) ns = maxS;
  if (ns != listScroll) { listScroll = ns; drawList(); }
}

static void readerScrollBy(int d) {
  int vis = visLines();
  int maxS = nLines > vis ? nLines - vis : 0;
  int ns = scrollLine + d;
  if (ns < 0) ns = 0;
  if (ns > maxS) ns = maxS;
  if (ns != scrollLine) { scrollLine = ns; drawReaderContent(); }
}

static void scrollBy(int d) {
  if (screen == SCR_CLASS) listScrollBy(d);
  else if (screen == SCR_READER) readerScrollBy(d);
  else sbScrollBy(d);
}

static void handleTap(int x, int y) {
  switch (screen) {
    case SCR_ERROR:
      bootData();
      break;

    case SCR_HOME: {
      if (sbHomeTap(x, y)) return;
      for (int i = 0; homeTab == 0 && i < nClasses; i++) {
        int tx, ty, tw, th;
        tileRect(i, tx, ty, tw, th);
        if (inRect(x, y, tx, ty, tw, th)) { startClass(i); return; }
      }

      // Handle tabs click
      bool portrait = isPortrait();
      uint16_t logW = getLogicalWidth();
      int tabAvailW = logW - 8;
      int tabW = tabAvailW / 2;
      if (inRect(x, y, 4, 30, tabW - 2, 18)) { homeTab = 0; redrawAll(); return; }
      else if (inRect(x, y, 4 + tabW, 30, tabW - 2, 18)) { homeTab = 1; redrawAll(); return; }

      // Handle bottom buttons
      int btnY = portrait ? 195 : 212;
      int btnH = 24;
      int optionsBtnW = portrait ? (logW / 2 - 10) : 82;
      int optionsBtnX = portrait ? 5 : 6;
      int aboutBtnX = portrait ? (logW / 2 + 5) : (SCREEN_W - 88);
      int aboutBtnW = portrait ? (logW / 2 - 10) : 82;

      if (inRect(x, y, optionsBtnX, btnY, optionsBtnW, btnH)) { screen = SCR_OPTIONS; drawOptions(); }
      else if (inRect(x, y, aboutBtnX, btnY, aboutBtnW, btnH)) { openAbout(); drawReaderScreen(); }
      break;
    }

    case SCR_OPTIONS: {
      if (y < 34 && x < 44) { screen = SCR_HOME; drawHome(); }
      else if (batteryOptionsTap(x, y)) {}
      else if (inRect(x, y, 10, 50, 300, 38)) setPack(0);
      else if (inRect(x, y, 10, 92, 300, 38)) setPack(1);

      // Brightness and sleep buttons - dynamically positioned
      bool portrait = isPortrait();
      uint16_t logW = getLogicalWidth();
      int briMinusX = portrait ? 70 : 170;
      int briPlusX = portrait ? (logW - 10 - 44) : 266;
      int btnY = 176;
      int leftBtnX = 10;
      int leftBtnW = portrait ? (logW / 2 - 15) : 145;
      int rightBtnX = portrait ? (logW / 2 + 5) : 165;
      int rightBtnW = portrait ? (logW / 2 - 15) : 145;
      int screenOffBtnX = portrait ? 10 : 170;
      int screenOffBtnW = portrait ? (logW - 20) : 140;
      int screenOffBtnY = portrait ? (btnY + 25) : 208;

      if (inRect(x, y, briMinusX, 134, 44, 38)) setBrightness(briLevel - 1);
      else if (inRect(x, y, briPlusX, 134, 44, 38)) setBrightness(briLevel + 1);
      else if (inRect(x, y, leftBtnX, btnY, leftBtnW, 30)) enterSleep();
      else if (inRect(x, y, rightBtnX, btnY, rightBtnW, 30)) cycleRotation();
      else if (inRect(x, y, screenOffBtnX, screenOffBtnY, screenOffBtnW, 28)) setSoftIdx(softIdx + 1);
      break;
    }

    case SCR_SUBS:
      if (y < 34) {
        if (x < 44) {
          screen = subsBack;
          redrawAll();
        }
      } else {
        int k = (y - 36) / 38;
        if (k >= 0 && k < nSubs && y >= 36) {
          if (openSub(k)) drawReaderScreen();
          else showError("Could not open the subclass file.");
        }
      }
      break;

    case SCR_CLASS:
      if (y < TAB_Y) {
        if (x < 44) { bgFree(); screen = SCR_HOME; drawHome(); }
        else if (classes[curClass].subCount > 0 && inRect(x, y, SCREEN_W - 134, 2, 48, 24)) openSubs(curClass, SCR_CLASS);
        else if (x >= SCREEN_W - 82 && x < SCREEN_W - 42) listScrollBy(-(LIST_ROWS - 1));
        else if (x >= SCREEN_W - 41) listScrollBy(LIST_ROWS - 1);
      } else if (y < LIST_Y) {
        if (nTabs == 0) break;
        int tw = 300 / nTabs;
        if (tw > 32) tw = 32;
        int x0 = (SCREEN_W - nTabs * tw) / 2;
        int i = (x - x0) / tw;
        if (x >= x0 && i >= 0 && i < nTabs && tabLevels[i] != curLevel) {
          curLevel = tabLevels[i];
          listScroll = 0;
          drawTabs();
          drawList();
        }
      } else {
        int k = listScroll + (y - LIST_Y) / ROW_H;
        if (k < levelCount[curLevel]) {
          if (openSpell(entries[levelFirst[curLevel] + k])) drawReaderScreen();
          else showError("Could not open the spell file.");
        }
      }
      break;

    case SCR_READER:
      if (sbReaderTap(x, y)) break;
      if (y < hdrH) {
        if (x < 44) { readerBar = 0; screen = readerBack; redrawAll(); }
        else if (x >= SCREEN_W - 82 && x < SCREEN_W - 42) readerScrollBy(-(visLines() - 1));
        else if (x >= SCREEN_W - 41) readerScrollBy(visLines() - 1);
      }
      break;

    default:
      sbTap(x, y);
      break;
  }
}

#ifdef TOUCH_TEST
static void touchTest() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Touch test: tap the corners", 6, 6);
  tft.drawString("Compare raw values with config.h", 6, 24);
  int16_t sx, sy, rx, ry, z;
  while (true) {
    if (touch.read(sx, sy, rx, ry, z)) {
      tft.fillCircle(sx, sy, 3, TFT_GREEN);
      char b[64];
      snprintf(b, sizeof(b), "raw %4d %4d z%4d  ->  %3d %3d   ", rx, ry, z, sx, sy);
      tft.drawString(b, 6, 50);
    }
    delay(10);
  }
}
#endif

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_R, OUTPUT); digitalWrite(PIN_LED_R, HIGH);   // active low: off
  pinMode(PIN_LED_G, OUTPUT); digitalWrite(PIN_LED_G, HIGH);
  pinMode(PIN_LED_B, OUTPUT); digitalWrite(PIN_LED_B, HIGH);
  prefs.begin("grimoire", false);
  curPack = prefs.getUChar("pack", 0);
  if (curPack > 1) curPack = 0;
  briLevel = prefs.getUChar("bri", 5);
  if (briLevel < 1 || briLevel > 5) briLevel = 5;
  softIdx = prefs.getUChar("soff", 3);
  if (softIdx > 4) softIdx = 3;
  tft.init();
  // Load rotation from NVS - landscape only (0° or 180°)
  uint8_t savedRot = prefs.getUChar("rot", 0);
  currentRot = (savedRot == 2) ? 2 : 0;
  // Map to TFT_eSPI rotations: 0->1 (landscape), 2->3 (landscape 180)
  tft.setRotation((currentRot == 2) ? 3 : 1);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(TFT_BL, 5000, 8);
#else
  ledcSetup(0, 5000, 8);
  ledcAttachPin(TFT_BL, 0);
#endif
  applyBrightness();
  pinMode(BOOT_BTN_PIN, INPUT_PULLUP);
  batteryInit();
  initColors();
  tft.fillScreen(C_BG);
  touch.begin();
  applyFont();
#ifdef TOUCH_TEST
  touchTest();
#endif
  lastActive = millis();
  bootData();
}

void loop() {
  static bool wasDown = false, dragging = false;
  static int16_t downX = 0, downY = 0, lastY = 0;
  static int scrollAcc = 0;
  static uint8_t upCount = 0;

  // BOOT held for 2 s -> deep sleep (armed only after it has been released once since boot/wake)
  static bool bootArmed = false;
  static uint32_t bootDownAt = 0;
  if (digitalRead(BOOT_BTN_PIN) == LOW) {
    idleWake();
    if (bootArmed) {
      if (!bootDownAt) bootDownAt = millis();
      else if (millis() - bootDownAt >= BOOT_HOLD_MS) enterSleep();
    }
  } else {
    bootArmed = true;
    bootDownAt = 0;
  }

  batteryTick();
  batteryBarTick();
  idleTick();
  int16_t x, y, rx, ry, z;
  static bool swallow = false;                 // the tap that woke the screen is not a button press
  bool touched = touch.read(x, y, rx, ry, z);
  if (touched) {
    if (idleState) { idleWake(); swallow = true; }
    lastActive = millis();
  }
  if (swallow) {
    if (touched) upCount = 0;
    else if (++upCount >= 3) { swallow = false; upCount = 0; }
  } else if (touched) {
    // Adjust coordinates based on rotation (TFT_eSPI doesn't auto-adjust touch coordinates)
    switch(currentRot) {
      case 0:  // 0°, landscape normal
        break;
      case 1:  // 90°, portrait
        { uint16_t tmp = x; x = SCREEN_H - 1 - y; y = tmp; }
        break;
      case 2:  // 180°, landscape inverted
        x = SCREEN_W - 1 - x;
        y = SCREEN_H - 1 - y;
        break;
      case 3:  // 270°, portrait inverted
        { uint16_t tmp = x; x = y; y = SCREEN_W - 1 - tmp; }
        break;
    }
    upCount = 0;
    if (!wasDown) {
      wasDown = true; dragging = false;
      downX = x; downY = y; lastY = y; scrollAcc = 0;
    } else {
      if (!dragging && abs(y - downY) > 10 && (screen == SCR_CLASS || screen == SCR_READER || sbScrollable())) {
        dragging = true;
        lastY = y;
        scrollAcc = 0;
      }
      if (dragging) {
        scrollAcc += lastY - y;
        lastY = y;
        int unit = (screen == SCR_READER) ? lineH : ROW_H;
        while (scrollAcc >= unit)  { scrollAcc -= unit; scrollBy(1); }
        while (scrollAcc <= -unit) { scrollAcc += unit; scrollBy(-1); }
      }
    }
  } else if (wasDown) {
    if (++upCount >= 3) {
      wasDown = false;
      upCount = 0;
      if (!dragging) handleTap(downX, downY);
    }
  }
  delay(8);
}
