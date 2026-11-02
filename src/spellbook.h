// Spellbooks: one character per spellbook (included once from main.cpp, shares its state).
//
// Files:  /characters/cNN.txt   key=value lines (see saveChar), NN = 00..11
// Data:   <book>/index/progress.txt   Class|kind|level|cantrips|prepared|slots
//         <book>/index/always.txt     Class|level|spell id   (always prepared, free of the prepared limit)
//         <book>/sub/NNN.txt          subclass doc, "@GRANT" lines: level|group|ids
#pragma once

#define MAX_CHARS    12
#define MAX_PREP     48
#define MAX_ALWAYS   24
#define CHAR_DIR     "/characters"
#define PICK_MAX     26
#define PICK_LEN     30
#define SH_TABS_Y    66
#define SH_CONTENT_Y 90
#define SH_ROW       30

struct Prog { uint8_t kind; uint8_t cantrips, prepared; uint8_t slots[9]; uint8_t pactCount, pactLevel; };  // kind: 0 none, 1 slots, 2 pact
struct Character {
  uint8_t slot, book, level, mod, autoCalc, dc;
  int8_t  atk;
  uint8_t used[9];
  uint16_t subId;                       // 0 = no subclass yet
  char name[20], species[20], cls[12], subName[28], choice[14];
  uint16_t prep[MAX_PREP];              // prepared spells and known cantrips: ref = (neon<<15)|id
  uint8_t nPrep;
};
struct CharSum { uint8_t slot, book, level; char name[20]; char cls[12]; };

enum PickKind : uint8_t { PK_SPECIES, PK_CLASS, PK_SUB, PK_CHOICE, PK_EDITSUB, PK_EDITCHOICE };
enum KbKind   : uint8_t { KB_WZ_NAME, KB_WZ_SPECIES, KB_RENAME };

static Character ch;
static Prog prog;
static CharSum chars[MAX_CHARS];
static int nChars = 0;
static int homeTab = 0, homeScroll = 0;

static Entry prepView[MAX_PREP + MAX_ALWAYS];   static int nPrepView = 0;
static Entry alwaysView[MAX_ALWAYS]; static int nAlways = 0;
static int shTab = 0, shScroll = 0, shLevel = 0;      // sheet tab (0 slots, 1 prepared, 2 all, 3 always)

// reader / cast
static Entry castEntry;
static char castMsg[40];

// generic picker
static char pickItems[PICK_MAX][PICK_LEN];
static int  nPick = 0, pickScroll = 0;
static PickKind pickKind = PK_SPECIES;
static char pickTitle[32];

// keyboard
static char kbBuf[24];
static int  kbMax = 16;
static KbKind kbKind = KB_WZ_NAME;
static bool kbShift = true;
static char kbTitle[24];

// wizard / edit
static int wzStep = 0;                      // 0 book, 1 name, 2 species, 3 class, 4 subclass, 5 choice, 6 stats
static bool editNew = false, delArmed = false;
static char wzGroups[4][14];
static int  nWzGroups = 0;
static SubEntry wzSubs[MAX_SUBS];
static int wzNSubs = 0;

// ---------------------------------------------------------------------------
// Prototypes
// ---------------------------------------------------------------------------
static void sbHomeScan();
static void sbDrawHomeList();
static bool sbHomeTap(int x, int y);
static void sbHomeScrollBy(int d);
static void sbOpenChar(int slot);
static void sbDrawSheet();
static void sbDrawEdit();
static void sbDrawPick();
static void sbDrawKeyboard();
static void sbDrawCast();
static void sbDrawWzBook();
static void wzGo(int step);
static void pickStart(PickKind k, const char* title);
static void kbStart(KbKind k, const char* title, const char* initial, int maxLen);
static void sbRecalc();
static bool saveChar();

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------
static const char* bookRoot(int b) { return b ? NEON_ROOT : SD_ROOT; }
static int  profBonus(int lv) { return 2 + (lv - 1) / 4; }
static void autoStats() { int pb = profBonus(ch.level); ch.dc = (uint8_t)(8 + pb + ch.mod); ch.atk = (int8_t)(pb + ch.mod); }
static uint16_t refOf(const Entry& e) { return (uint16_t)((e.src ? 0x8000 : 0) | e.id); }
static uint16_t parseRef(const char* t) {
  uint16_t src = 0;
  if (*t == 'n') { src = 0x8000; t++; } else if (*t == 'g') t++;
  return (uint16_t)(src | (uint16_t)atoi(t));
}
static void lowerCopy(char* dst, const char* src, size_t n) {
  size_t i = 0;
  for (; src[i] && i < n - 1; i++) dst[i] = (char)tolower((unsigned char)src[i]);
  dst[i] = 0;
}
static void copyStr(char* dst, const char* src, size_t n) { strncpy(dst, src, n - 1); dst[n - 1] = 0; }

static bool fileLine(File& f, char* buf, size_t n) {          // false at EOF; strips CR
  if (!f.available()) return false;
  size_t k = f.readBytesUntil('\n', buf, n - 1);
  buf[k] = 0;
  if (k && buf[k - 1] == '\r') buf[--k] = 0;
  return true;
}

// ---------------------------------------------------------------------------
// Progression: cantrips, prepared spells and spell slots for (book, class, level)
// ---------------------------------------------------------------------------
static bool loadProgress(int book, const char* cls, int level, Prog& p) {
  memset(&p, 0, sizeof(p));
  char path[48];
  snprintf(path, sizeof(path), "%s/index/progress.txt", bookRoot(book));
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  char line[96];
  bool found = false;
  while (fileLine(f, line, sizeof(line))) {
    if (line[0] == '#' || !line[0]) continue;
    char* fld[6];
    int nf = 0;
    fld[nf++] = line;
    for (char* q = line; *q && nf < 6; q++) if (*q == '|') { *q = 0; fld[nf++] = q + 1; }
    if (nf < 6 || strcmp(fld[0], cls) != 0 || atoi(fld[2]) != level) continue;
    p.cantrips = (uint8_t)atoi(fld[3]);
    p.prepared = (uint8_t)atoi(fld[4]);
    const char* s = fld[5];
    if (!strcmp(fld[1], "pact")) {
      p.kind = 2; p.pactCount = (uint8_t)atoi(s);
      const char* c = strchr(s, ',');
      p.pactLevel = c ? (uint8_t)atoi(c + 1) : 1;
    } else {
      p.kind = 1;
      for (int i = 0; i < 9 && s && *s; i++) {
        p.slots[i] = (uint8_t)atoi(s);
        s = strchr(s, ',');
        if (s) s++;
      }
    }
    found = true;
    break;
  }
  f.close();
  return found;
}

static int slotMax(int l) {                      // l = 1..9
  if (prog.kind == 2) return l == prog.pactLevel ? prog.pactCount : 0;
  return (l >= 1 && l <= 9) ? prog.slots[l - 1] : 0;
}
static int topSlotLevel() { for (int l = 9; l >= 1; l--) if (slotMax(l) > 0) return l; return 0; }
static int slotLeft(int l) { int m = slotMax(l), u = ch.used[l - 1]; return m > u ? m - u : 0; }

// recompute everything derived from class + level (call after loading or changing them)
static void sbRecalc() {
  loadProgress(ch.book, ch.cls, ch.level, prog);
  for (int l = 1; l <= 9; l++) if (ch.used[l - 1] > slotMax(l)) ch.used[l - 1] = (uint8_t)slotMax(l);
}

// ---------------------------------------------------------------------------
// Character files
// ---------------------------------------------------------------------------
static void charPath(int slot, char* out, size_t n) { snprintf(out, n, CHAR_DIR "/c%02d.txt", slot); }

static bool saveChar() {
  char path[32];
  charPath(ch.slot, path, sizeof(path));
  SD.remove(path);
  File f = SD.open(path, FILE_WRITE);
  if (!f) return false;
  char b[200];
  snprintf(b, sizeof(b), "name=%s\nspecies=%s\nbook=%d\nclass=%s\nsubid=%d\nsub=%s\nchoice=%s\n", ch.name, ch.species, ch.book,
           ch.cls, ch.subId, ch.subName, ch.choice);
  f.print(b);
  snprintf(b, sizeof(b), "level=%d\nmod=%d\nauto=%d\ndc=%d\natk=%d\nused=", ch.level, ch.mod, ch.autoCalc, ch.dc, ch.atk);
  f.print(b);
  for (int i = 0; i < 9; i++) { snprintf(b, sizeof(b), i ? ",%d" : "%d", ch.used[i]); f.print(b); }
  f.print("\nprep=");
  for (int i = 0; i < ch.nPrep; i++) {
    snprintf(b, sizeof(b), "%s%c%03d", i ? "," : "", (ch.prep[i] & 0x8000) ? 'n' : 'g', ch.prep[i] & 0x7FFF);
    f.print(b);
  }
  f.print("\n");
  f.close();
  return true;
}

static bool loadChar(int slot) {
  char path[32];
  charPath(slot, path, sizeof(path));
  File f = SD.open(path, FILE_READ);
  if (!f) return false;
  memset(&ch, 0, sizeof(ch));
  ch.slot = (uint8_t)slot; ch.level = 1; ch.autoCalc = 1;
  char line[400];
  while (fileLine(f, line, sizeof(line))) {
    char* eq = strchr(line, '=');
    if (!eq) continue;
    *eq = 0;
    const char *k = line, *v = eq + 1;
    if      (!strcmp(k, "name"))    copyStr(ch.name, v, sizeof(ch.name));
    else if (!strcmp(k, "species")) copyStr(ch.species, v, sizeof(ch.species));
    else if (!strcmp(k, "book"))    ch.book = (uint8_t)(atoi(v) ? 1 : 0);
    else if (!strcmp(k, "class"))   copyStr(ch.cls, v, sizeof(ch.cls));
    else if (!strcmp(k, "subid"))   ch.subId = (uint16_t)atoi(v);
    else if (!strcmp(k, "sub"))     copyStr(ch.subName, v, sizeof(ch.subName));
    else if (!strcmp(k, "choice"))  copyStr(ch.choice, v, sizeof(ch.choice));
    else if (!strcmp(k, "level"))   { int l = atoi(v); ch.level = (uint8_t)(l < 1 ? 1 : l > 20 ? 20 : l); }
    else if (!strcmp(k, "mod"))     ch.mod = (uint8_t)atoi(v);
    else if (!strcmp(k, "auto"))    ch.autoCalc = (uint8_t)atoi(v);
    else if (!strcmp(k, "dc"))      ch.dc = (uint8_t)atoi(v);
    else if (!strcmp(k, "atk"))     ch.atk = (int8_t)atoi(v);
    else if (!strcmp(k, "used")) {
      for (int i = 0; i < 9 && *v; i++) { ch.used[i] = (uint8_t)atoi(v); v = strchr(v, ','); if (!v) break; v++; }
    } else if (!strcmp(k, "prep")) {
      while (*v && ch.nPrep < MAX_PREP) {
        ch.prep[ch.nPrep++] = parseRef(v);
        v = strchr(v, ',');
        if (!v) break;
        v++;
      }
    }
  }
  f.close();
  return ch.name[0] != 0;
}

static void sbHomeScan() {
  nChars = 0;
  for (int s = 0; s < MAX_CHARS; s++) {
    char path[32];
    charPath(s, path, sizeof(path));
    File f = SD.open(path, FILE_READ);
    if (!f) continue;
    CharSum& c = chars[nChars];
    memset(&c, 0, sizeof(c));
    c.slot = (uint8_t)s; c.level = 1;
    char line[96];
    for (int n = 0; n < 12 && fileLine(f, line, sizeof(line)); n++) {
      char* eq = strchr(line, '=');
      if (!eq) continue;
      *eq = 0;
      if      (!strcmp(line, "name"))  copyStr(c.name, eq + 1, sizeof(c.name));
      else if (!strcmp(line, "class")) copyStr(c.cls, eq + 1, sizeof(c.cls));
      else if (!strcmp(line, "book"))  c.book = (uint8_t)(atoi(eq + 1) ? 1 : 0);
      else if (!strcmp(line, "level")) { c.level = (uint8_t)atoi(eq + 1); break; }
    }
    f.close();
    if (c.name[0]) nChars++;
  }
}

static int freeSlot() {
  for (int s = 0; s < MAX_CHARS; s++) {
    char path[32];
    charPath(s, path, sizeof(path));
    if (!SD.exists(path)) return s;
  }
  return -1;
}

// ---------------------------------------------------------------------------
// Spell info for lists: class index first, spell file header as a fallback
// ---------------------------------------------------------------------------
static void readSpellHeader(uint8_t src, uint16_t id, Entry& e) {
  memset(&e, 0, sizeof(e));
  e.src = src; e.id = id;
  copyStr(e.name, "(missing spell)", sizeof(e.name));
  char path[48];
  snprintf(path, sizeof(path), "%s/spells/%03d.txt", src ? NEON_ROOT : SD_ROOT, (int)id);
  File f = SD.open(path, FILE_READ);
  if (!f) return;
  char line[100];
  while (fileLine(f, line, sizeof(line))) {
    if (!strcmp(line, "---")) break;
    char* eq = strchr(line, '=');
    if (!eq) continue;
    *eq = 0;
    const char* v = eq + 1;
    if      (!strcmp(line, "name"))   { copyStr(e.name, v, sizeof(e.name)); if (!smoothFont) foldAscii(e.name); }
    else if (!strcmp(line, "level"))  e.level = (uint8_t)atoi(v);
    else if (!strcmp(line, "school")) e.school = schoolIndex(v);
    else if (!strcmp(line, "flags"))  e.flags = (uint8_t)((strchr(v, 'C') ? 1 : 0) | (strchr(v, 'R') ? 2 : 0) | (strchr(v, 'P') ? 4 : 0));
  }
  f.close();
}

static void entryFromRef(uint16_t ref, Entry& out) {
  uint8_t src = (ref & 0x8000) ? 1 : 0;
  uint16_t id = ref & 0x7FFF;
  for (int i = 0; i < nEntries; i++)
    if (entries[i].src == src && entries[i].id == id) { out = entries[i]; return; }
  readSpellHeader(src, id, out);
}

static void sortEntries(Entry* a, int n) {
  for (int i = 1; i < n; i++) {
    Entry t = a[i];
    int j = i - 1;
    while (j >= 0 && (a[j].level > t.level || (a[j].level == t.level && strcmp(a[j].name, t.name) > 0))) { a[j + 1] = a[j]; j--; }
    a[j + 1] = t;
  }
}

static bool inPrep(uint16_t ref) { for (int i = 0; i < ch.nPrep; i++) if (ch.prep[i] == ref) return true; return false; }
static bool inAlways(uint16_t ref) {
  for (int i = 0; i < nAlways; i++) if (refOf(alwaysView[i]) == ref) return true;
  return false;
}

// refs granted by the class (always.txt) and by the subclass (@GRANT), for the character's level
static int collectAlways(uint16_t* refs, int maxRefs) {
  int n = 0;
  char path[48], line[160];
  snprintf(path, sizeof(path), "%s/index/always.txt", bookRoot(ch.book));
  File f = SD.open(path, FILE_READ);
  if (f) {
    while (fileLine(f, line, sizeof(line)) && n < maxRefs) {
      if (line[0] == '#') continue;
      char* a = strchr(line, '|');
      if (!a) continue;
      *a = 0;
      char* b = strchr(a + 1, '|');
      if (!b) continue;
      *b = 0;
      if (!strcmp(line, ch.cls) && atoi(a + 1) <= ch.level) refs[n++] = parseRef(b + 1);
    }
    f.close();
  }
  if (ch.subId) {
    snprintf(path, sizeof(path), "%s/sub/%03d.txt", bookRoot(ch.book), (int)ch.subId);
    f = SD.open(path, FILE_READ);
    if (f) {
      bool grant = false;
      while (fileLine(f, line, sizeof(line))) {
        if (!grant) { if (!strcmp(line, "@GRANT")) grant = true; continue; }
        char* a = strchr(line, '|');                         // level|group|ids
        if (!a) continue;
        *a = 0;
        char* b = strchr(a + 1, '|');
        if (!b) continue;
        *b = 0;
        const char* grp = a + 1;
        if (atoi(line) > ch.level) continue;
        if (grp[0] && grp[0] != '*' && strcmp(grp, ch.choice) != 0) continue;
        char* p = b + 1;
        while (*p && n < maxRefs) {
          uint16_t r = parseRef(p);
          bool dup = false;
          for (int i = 0; i < n; i++) if (refs[i] == r) dup = true;
          if (!dup) refs[n++] = r;
          p = strchr(p, ',');
          if (!p) break;
          p++;
        }
      }
      f.close();
    }
  }
  return n;
}

static void sbBuildViews() {
  uint16_t refs[MAX_ALWAYS];
  int n = collectAlways(refs, MAX_ALWAYS);
  nAlways = 0;
  for (int i = 0; i < n; i++) entryFromRef(refs[i], alwaysView[nAlways++]);
  sortEntries(alwaysView, nAlways);
  nPrepView = 0;                                           // Prepared tab = chosen spells + always-prepared ones
  for (int i = 0; i < ch.nPrep && nPrepView < MAX_PREP; i++) entryFromRef(ch.prep[i], prepView[nPrepView++]);
  for (int i = 0; i < nAlways; i++) if (!inPrep(refOf(alwaysView[i]))) prepView[nPrepView++] = alwaysView[i];
  sortEntries(prepView, nPrepView);
}

static int countPrepared(int* cantrips) {            // spells of level 1+ that count against the limit
  int c = 0, p = 0;
  for (int i = 0; i < nPrepView; i++) {
    if (inAlways(refOf(prepView[i]))) continue;
    if (prepView[i].level == 0) c++; else p++;
  }
  *cantrips = c;
  return p;
}

// ---------------------------------------------------------------------------
// Opening a character
// ---------------------------------------------------------------------------
static void sbLoadAllTab() {
  char lower[12];
  lowerCopy(lower, ch.cls, sizeof(lower));
  nEntries = 0;
  char path[64];
  snprintf(path, sizeof(path), "%s/index/%s.idx", bookRoot(ch.book), lower);
  File f = SD.open(path, FILE_READ);
  if (f) {
    char line[128];
    while (fileLine(f, line, sizeof(line)) && nEntries < MAX_ENTRIES) {
      if (!line[0]) continue;
      if (parseIdxLine(line, entries[nEntries])) nEntries++;
    }
    f.close();
  }
  int top = topSlotLevel();
  for (int l = 0; l < 10; l++) { levelFirst[l] = 0; levelCount[l] = 0; }
  for (int i = 0; i < nEntries; i++) {
    int l = entries[i].level;
    if (l < 10 && l <= top) { if (levelCount[l] == 0) levelFirst[l] = i; levelCount[l]++; }
  }
  nTabs = 0;
  for (int l = 0; l < 10; l++) if (levelCount[l] > 0) tabLevels[nTabs++] = l;
  shLevel = nTabs ? tabLevels[0] : 0;
}

static void sbOpenChar(int slot) {
  if (!loadChar(slot)) { showError("Could not read the spellbook file."); return; }
  sbRecalc();
  sbLoadAllTab();
  sbBuildViews();
  shTab = 0; shScroll = 0;
  screen = SCR_SHEET;
  sbDrawSheet();
}

// ---------------------------------------------------------------------------
// Drawing: shared pieces
// ---------------------------------------------------------------------------
static void drawBtn(int x, int y, int w, int h, const char* label, uint16_t bg, uint16_t fg) {
  tft.fillRoundRect(x, y, w, h, 5, bg);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(fg, bg);
  setBodyFont();
  drawFit(label, x + w / 2, y + h / 2, w - 6);
  tft.setTextDatum(TL_DATUM);
}

static void drawSeg(const char* s, int& x, int y, uint16_t col, uint16_t bg) {
  tft.setTextColor(col, bg);
  tft.drawString(s, x, y);
  x += tft.textWidth(s);
}

static void drawStepper(int y, const char* label, const char* value) {
  tft.setTextDatum(TL_DATUM);
  setBodyFont();
  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString(label, 10, y + 6);
  tft.fillRoundRect(170, y, 44, 26, 6, C_PANEL);
  tft.fillRoundRect(266, y, 44, 26, 6, C_PANEL);
  tft.fillRect(180, y + 12, 24, 3, C_TEXT);
  tft.fillRect(276, y + 12, 24, 3, C_TEXT);
  tft.fillRect(286, y + 3, 3, 20, C_TEXT);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(C_GOLD, C_BG);
  tft.drawString(value, 240, y + 13);
  tft.setTextDatum(TL_DATUM);
}

// ---------------------------------------------------------------------------
// Home: Spellbooks tab
// ---------------------------------------------------------------------------
static void sbDrawHomeList() {
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  int total = nChars + 1;                                   // + "New spellbook"
  for (int r = 0; r < 5; r++) {
    int k = homeScroll + r;
    if (k >= total) break;
    int y = 52 + r * 31;
    tft.fillRoundRect(6, y, 300, 28, 5, C_PANEL);
    if (k < nChars) {
      const CharSum& c = chars[k];
      tft.fillRect(6, y + 4, 4, 20, c.book ? tft.color565(80, 230, 245) : tft.color565(232, 190, 90));
      tft.setTextColor(C_TEXT, C_PANEL);
      drawFit(c.name, 16, y + 2, 140);
      char b[48];
      snprintf(b, sizeof(b), "%s %d", c.cls, c.level);
      tft.setTextColor(C_DIM, C_PANEL);
      drawFit(b, 162, y + 6, 70);
      tft.drawString(c.book ? "Neon" : "D&D", 262, y + 6);
    } else {
      tft.setTextColor(C_GOLD, C_PANEL);
      tft.drawString("+ New spellbook", 16, y + 6);
    }
  }
  if (total > 5) {
    int th = 156 * 5 / total;
    int ty = 52 + (156 - th) * homeScroll / (total - 5);
    tft.fillRect(312, 52, 4, 156, C_PANEL);
    tft.fillRect(312, ty, 4, th, C_DIM);
  }
}

static void sbHomeScrollBy(int d) {
  int total = nChars + 1;
  int maxS = total > 5 ? total - 5 : 0;
  int ns = homeScroll + d;
  if (ns < 0) ns = 0;
  if (ns > maxS) ns = maxS;
  if (ns != homeScroll) {
    homeScroll = ns;
    tft.fillRect(0, 52, SCREEN_W, 158, C_BG);
    sbDrawHomeList();
  }
}

// taps below the title: tabs and (in the Spellbooks tab) the list. Returns true when handled.
static bool sbHomeTap(int x, int y) {
  if (y >= 30 && y < 50) {
    int t = x < 160 ? 0 : 1;
    if (t != homeTab) {
      homeTab = t;
      homeScroll = 0;
      if (t == 1) sbHomeScan();
      drawHome();
    }
    return true;
  }
  if (homeTab == 1 && y >= 52 && y < 210) {
    int k = homeScroll + (y - 52) / 31;
    if (k < nChars) sbOpenChar(chars[k].slot);
    else if (k == nChars) {
      if (nChars >= MAX_CHARS) return true;
      memset(&ch, 0, sizeof(ch));
      ch.level = 1; ch.autoCalc = 1;
      wzGo(0);
    }
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Spellbook sheet
// ---------------------------------------------------------------------------
static void sheetGeom(int& rows, int& rowH, int& box) {
  rows = 0;
  for (int l = 1; l <= 9; l++) if (slotMax(l) > 0) rows++;
  if (rows == 0) rows = 1;
  rowH = 150 / rows;
  if (rowH > 30) rowH = 30;
  box = rowH - 4;
  if (box > 24) box = 24;
}

static int allListY() { return SH_CONTENT_Y + 24; }
static int allRows()  { return (SCREEN_H - allListY()) / SH_ROW; }

static void sbDrawSheetList() {
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  Entry* arr = nullptr;
  int cnt = 0, y0 = SH_CONTENT_Y, rowsVis = (SCREEN_H - SH_CONTENT_Y) / SH_ROW;
  if (shTab == 1) { arr = prepView; cnt = nPrepView; }
  else if (shTab == 3) { arr = alwaysView; cnt = nAlways; }
  else { arr = entries + (levelCount[shLevel] ? levelFirst[shLevel] : 0); cnt = levelCount[shLevel]; y0 = allListY(); rowsVis = allRows(); }
  tft.fillRect(0, y0, SCREEN_W, SCREEN_H - y0, C_BG);
  if (cnt == 0) {
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString(shTab == 1 ? "Nothing prepared yet. Use the All tab." : shTab == 3 ? "No always-prepared spells." : "No spells.", 10, y0 + 8);
    return;
  }
  for (int r = 0; r < rowsVis; r++) {
    int k = shScroll + r;
    if (k >= cnt) break;
    const Entry& e = arr[k];
    int y = y0 + r * SH_ROW;
    int ty = y + (SH_ROW - (lineH - 2)) / 2 - 1;
    uint16_t ref = refOf(e);
    bool alw = inAlways(ref), prepd = inPrep(ref);
    tft.fillRoundRect(6, y + 1, 298, SH_ROW - 3, 4, C_PANEL);
    tft.fillRect(6, y + 4, 4, SH_ROW - 9, schoolColor(e.school));
    int nx = 16;
    if (shTab == 2) {                                        // checkbox: prepared / known
      tft.drawRect(16, y + 6, 18, 18, C_DIM);
      if (alw) { tft.setTextColor(C_GOLD, C_PANEL); tft.drawString("A", 21, ty); }
      else if (prepd) tft.fillRect(20, y + 10, 10, 10, C_GOLD);
      nx = 42;
    }
    tft.setTextColor((e.flags & 4) ? C_DIM : (alw && shTab != 2 ? C_GOLD : C_TEXT), C_PANEL);
    drawFit(e.name, nx, ty, 232 - nx + 16);
    tft.setTextColor(C_GOLD, C_PANEL);
    if (shTab != 2) { char lv[6]; if (e.level == 0) strcpy(lv, "C"); else snprintf(lv, sizeof(lv), "L%d", e.level); tft.drawString(lv, 252, ty); }
    else {
      if (e.flags & 1) tft.drawString("C", 252, ty);
      if (e.flags & 2) tft.drawString("R", 266, ty);
    }
    if (shTab == 3 && !prepd) { /* nothing */ }
    char ab[4]; memcpy(ab, SCHOOLS[e.school], 3); ab[3] = 0;
    tft.setTextColor(C_DIM, C_PANEL);
    tft.drawString(ab, 278, ty);
  }
  if (cnt > rowsVis) {
    int barH = SCREEN_H - y0;
    int th = barH * rowsVis / cnt;
    if (th < 12) th = 12;
    int ty = y0 + (barH - th) * shScroll / (cnt - rowsVis);
    tft.fillRect(310, y0, 4, barH, C_PANEL);
    tft.fillRect(310, ty, 4, th, C_DIM);
  }
}

static void sbDrawAllTabs() {
  tft.fillRect(0, SH_CONTENT_Y, SCREEN_W, 22, C_BG);
  if (nTabs == 0) return;
  int tw = 300 / nTabs;
  if (tw > 32) tw = 32;
  int x0 = (SCREEN_W - nTabs * tw) / 2;
  tft.setTextDatum(MC_DATUM);
  setBodyFont();
  for (int i = 0; i < nTabs; i++) {
    int l = tabLevels[i];
    bool sel = (l == shLevel);
    uint16_t bg = sel ? C_SEL : C_PANEL;
    tft.fillRoundRect(x0 + i * tw, SH_CONTENT_Y, tw - 2, 20, 4, bg);
    char b[3];
    if (l == 0) strcpy(b, "C"); else { b[0] = (char)('0' + l); b[1] = 0; }
    tft.setTextColor(sel ? C_TEXT : C_DIM, bg);
    tft.drawString(b, x0 + i * tw + (tw - 2) / 2, SH_CONTENT_Y + 10);
  }
  tft.setTextDatum(TL_DATUM);
}

static void sbDrawSlots() {
  tft.fillRect(0, SH_CONTENT_Y, SCREEN_W, SCREEN_H - SH_CONTENT_Y, C_BG);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  if (prog.kind == 0) {
    tft.setTextColor(C_DIM, C_BG);
    tft.drawString("No spell slot table for this class.", 10, SH_CONTENT_Y + 8);
    return;
  }
  int rows, rowH, box;
  sheetGeom(rows, rowH, box);
  int row = 0;
  for (int l = 1; l <= 9; l++) {
    int m = slotMax(l);
    if (m <= 0) continue;
    int y = SH_CONTENT_Y + row * rowH;
    char lab[16];
    if (prog.kind == 2) snprintf(lab, sizeof(lab), "Pact L%d", l); else snprintf(lab, sizeof(lab), "Level %d", l);
    tft.setTextColor(C_TEXT, C_BG);
    tft.drawString(lab, 8, y + (rowH - (lineH - 2)) / 2 - 1);
    for (int k = 0; k < m; k++) {
      int bx = 80 + k * (box + 6);
      tft.drawRect(bx, y + 2, box, box, C_TEXT);
      if (k < ch.used[l - 1]) tft.fillRect(bx + 3, y + 5, box - 6, box - 6, C_GOLD);
    }
    row++;
  }
  drawBtn(226, SH_CONTENT_Y + 4, 90, 30, "Long rest", C_PANEL, C_TEXT);
  if (prog.kind == 2) drawBtn(226, SH_CONTENT_Y + 40, 90, 30, "Short rest", C_PANEL, C_TEXT);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("Tap a box = slot used", 226 - 40, SCREEN_H - 18);
}

static void sbDrawSheetHeader() {
  tft.fillRect(0, 0, SCREEN_W, SH_CONTENT_Y, C_BG);
  drawBackBtn(24);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_GOLD, C_BG);
  drawFit(ch.name, 50, 5, SCREEN_W - 50 - 70);
  drawBtn(SCREEN_W - 66, 2, 64, 24, "Edit", C_PANEL, C_TEXT);
  char b[96];
  snprintf(b, sizeof(b), "%s %s%s%s%s  Lv %d", ch.species[0] ? ch.species : "-", ch.cls, ch.subName[0] ? " (" : "", ch.subName, ch.subName[0] ? ")" : "", ch.level);
  tft.setTextColor(C_DIM, C_BG);
  drawFit(b, 8, 28, SCREEN_W - 16);
  int cant = 0, prp = countPrepared(&cant);
  int x = 8, y = 46;
  snprintf(b, sizeof(b), "DC %d", ch.dc);                          drawSeg(b, x, y, C_TEXT, C_BG);
  snprintf(b, sizeof(b), "  Atk %+d", ch.atk);                     drawSeg(b, x, y, C_TEXT, C_BG);
  snprintf(b, sizeof(b), "  Cantrips %d/%d", cant, prog.cantrips); drawSeg(b, x, y, cant > prog.cantrips ? C_ERR : C_GOLD, C_BG);
  snprintf(b, sizeof(b), "  Prepared %d/%d", prp, prog.kind ? prog.prepared : 0); drawSeg(b, x, y, prp > prog.prepared ? C_ERR : C_GOLD, C_BG);
  static const char* const T[4] = { "Slots", "Prepared", "All", "Always" };
  tft.setTextDatum(MC_DATUM);
  for (int i = 0; i < 4; i++) {
    bool sel = (i == shTab);
    uint16_t bg = sel ? C_SEL : C_PANEL;
    tft.fillRoundRect(2 + i * 79, SH_TABS_Y, 77, 20, 4, bg);
    tft.setTextColor(sel ? C_TEXT : C_DIM, bg);
    tft.drawString(T[i], 2 + i * 79 + 38, SH_TABS_Y + 10);
  }
  tft.setTextDatum(TL_DATUM);
}

static void sbDrawSheet() {
  tft.fillScreen(C_BG);
  sbDrawSheetHeader();
  if (shTab == 0) sbDrawSlots();
  else {
    if (shTab == 2) sbDrawAllTabs();
    sbDrawSheetList();
  }
}

static int sheetListCount() { return shTab == 1 ? nPrepView : shTab == 3 ? nAlways : levelCount[shLevel]; }
static int sheetListRows()  { return shTab == 2 ? allRows() : (SCREEN_H - SH_CONTENT_Y) / SH_ROW; }

static void sbSheetScrollBy(int d) {
  if (shTab == 0) return;
  int cnt = sheetListCount(), vis = sheetListRows();
  int maxS = cnt > vis ? cnt - vis : 0;
  int ns = shScroll + d;
  if (ns < 0) ns = 0;
  if (ns > maxS) ns = maxS;
  if (ns != shScroll) { shScroll = ns; sbDrawSheetList(); }
}

static void togglePrep(const Entry& e) {
  uint16_t ref = refOf(e);
  for (int i = 0; i < ch.nPrep; i++) {
    if (ch.prep[i] == ref) {
      for (int j = i; j + 1 < ch.nPrep; j++) ch.prep[j] = ch.prep[j + 1];
      ch.nPrep--;
      return;
    }
  }
  if (ch.nPrep < MAX_PREP) ch.prep[ch.nPrep++] = ref;
}

// open a spell from the sheet: reader with a Cast bar
static void sbOpenSpell(const Entry& e) {
  if (!openSpell(e)) { showError("Could not open the spell file."); return; }
  readerBack = SCR_SHEET;
  castEntry = e;
  castMsg[0] = 0;
  readerBar = (e.level >= 1 && prog.kind) ? 32 : 0;
  drawReaderScreen();
}

static void sbSheetTap(int x, int y) {
  if (y < 28) {
    if (x < 44) { screen = SCR_HOME; homeTab = 1; sbHomeScan(); drawHome(); }
    else if (x >= SCREEN_W - 66) { editNew = false; delArmed = false; screen = SCR_EDIT; sbDrawEdit(); }
    return;
  }
  if (y >= SH_TABS_Y && y < SH_TABS_Y + 22) {
    int t = (x - 2) / 79;
    if (t >= 0 && t < 4 && t != shTab) { shTab = t; shScroll = 0; sbDrawSheet(); }
    return;
  }
  if (y < SH_CONTENT_Y) return;
  if (shTab == 0) {
    if (prog.kind == 0) return;
    if (x >= 226 && y >= SH_CONTENT_Y + 4 && y < SH_CONTENT_Y + 34) {                 // long rest
      memset(ch.used, 0, sizeof(ch.used)); saveChar(); sbDrawSlots(); return;
    }
    if (prog.kind == 2 && x >= 226 && y >= SH_CONTENT_Y + 40 && y < SH_CONTENT_Y + 70) {   // short rest: pact slots
      ch.used[prog.pactLevel - 1] = 0; saveChar(); sbDrawSlots(); return;
    }
    int rows, rowH, box;
    sheetGeom(rows, rowH, box);
    int row = (y - SH_CONTENT_Y) / rowH, r = 0;
    for (int l = 1; l <= 9; l++) {
      if (slotMax(l) <= 0) continue;
      if (r++ != row) continue;
      int k = (x - 80) / (box + 6);
      if (x >= 80 && k >= 0 && k < slotMax(l)) {
        ch.used[l - 1] = (uint8_t)(k < ch.used[l - 1] ? k : k + 1);
        saveChar();
        sbDrawSlots();
      }
      return;
    }
    return;
  }
  if (shTab == 2 && y < allListY()) {                                                  // level sub-tabs
    if (nTabs == 0) return;
    int tw = 300 / nTabs;
    if (tw > 32) tw = 32;
    int x0 = (SCREEN_W - nTabs * tw) / 2;
    int i = (x - x0) / tw;
    if (x >= x0 && i >= 0 && i < nTabs && tabLevels[i] != shLevel) {
      shLevel = tabLevels[i]; shScroll = 0; sbDrawAllTabs(); sbDrawSheetList();
    }
    return;
  }
  int y0 = shTab == 2 ? allListY() : SH_CONTENT_Y;
  int k = shScroll + (y - y0) / SH_ROW;
  if (k >= sheetListCount()) return;
  if (shTab == 2) {
    const Entry& e = entries[levelFirst[shLevel] + k];
    if (x < 40) {                                                                      // checkbox
      if (!inAlways(refOf(e))) { togglePrep(e); saveChar(); sbBuildViews(); sbDrawSheetHeader(); sbDrawSheetList(); }
    } else sbOpenSpell(e);
  } else {
    sbOpenSpell(shTab == 1 ? prepView[k] : alwaysView[k]);
  }
}

// ---------------------------------------------------------------------------
// Cast dialog (upcasting): pick the slot level, base level = the spell's own level
// ---------------------------------------------------------------------------
static int castRows(int* levels) {
  int n = 0;
  int base = castEntry.level < 1 ? 1 : castEntry.level;
  for (int l = base; l <= 9; l++) if (slotMax(l) > 0) levels[n++] = l;
  return n;
}

static void sbDrawCast() {
  tft.fillScreen(C_BG);
  drawBackBtn(24);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_GOLD, C_BG);
  char b[64];
  snprintf(b, sizeof(b), "Cast %s", castEntry.name);
  drawFit(b, 50, 5, SCREEN_W - 56);
  tft.setTextColor(C_DIM, C_BG);
  snprintf(b, sizeof(b), "Base level %d. Pick the slot to spend%s.", castEntry.level, prog.kind == 2 ? " (pact slots)" : " (higher = upcast)");
  drawFit(b, 8, 30, SCREEN_W - 16);
  int lv[9];
  int n = castRows(lv);
  int rowH = n ? 150 / n : 30;
  if (rowH > 30) rowH = 30;
  for (int i = 0; i < n; i++) {
    int y = 52 + i * rowH;
    int left = slotLeft(lv[i]);
    bool ok = left > 0;
    tft.fillRoundRect(10, y, 300, rowH - 3, 5, ok ? C_PANEL : C_PANEL2);
    tft.setTextColor(ok ? C_TEXT : C_DIM, ok ? C_PANEL : C_PANEL2);
    snprintf(b, sizeof(b), "Level %d slot%s", lv[i], lv[i] > castEntry.level ? "  (upcast)" : "");
    tft.drawString(b, 20, y + (rowH - 3 - (lineH - 2)) / 2 - 1);
    snprintf(b, sizeof(b), "%d/%d left", left, slotMax(lv[i]));
    tft.setTextColor(ok ? C_GOLD : C_ERR, ok ? C_PANEL : C_PANEL2);
    tft.drawString(b, 220, y + (rowH - 3 - (lineH - 2)) / 2 - 1);
  }
  if (n == 0) { tft.setTextColor(C_ERR, C_BG); tft.drawString("No slot of this level or higher.", 10, 60); }
  drawBtn(10, 208, 150, 28, "No slot (free/ritual)", C_PANEL, C_TEXT);
  drawBtn(170, 208, 140, 28, "Cancel", C_PANEL, C_TEXT);
}

static void castBack() { screen = SCR_READER; drawReaderScreen(); }

static void sbCastTap(int x, int y) {
  if (y < 28 && x < 44) { castBack(); return; }
  if (y >= 208) {
    if (x < 165) snprintf(castMsg, sizeof(castMsg), "Cast without a slot");
    castBack();
    return;
  }
  int lv[9];
  int n = castRows(lv);
  if (!n) return;
  int rowH = 150 / n;
  if (rowH > 30) rowH = 30;
  int i = (y - 52) / rowH;
  if (y < 52 || i < 0 || i >= n) return;
  if (slotLeft(lv[i]) <= 0) return;
  ch.used[lv[i] - 1]++;
  saveChar();
  snprintf(castMsg, sizeof(castMsg), "Used L%d slot, %d/%d left", lv[i], slotLeft(lv[i]), slotMax(lv[i]));
  castBack();
}

// bar at the bottom of the reader (only when readerBar > 0)
static void sbDrawReaderBar() {
  if (!readerBar) return;
  int y = SCREEN_H - readerBar;
  tft.fillRect(0, y, SCREEN_W, readerBar, C_BG);
  drawBtn(6, y + 2, 100, readerBar - 4, "Cast", C_SEL, C_TEXT);
  tft.setTextDatum(TL_DATUM);
  setBodyFont();
  tft.setTextColor(C_DIM, C_BG);
  drawFit(castMsg, 114, y + (readerBar - (lineH - 2)) / 2, SCREEN_W - 120);
}

static bool sbReaderTap(int x, int y) {
  if (readerBar && y >= SCREEN_H - readerBar) {
    if (x < 110) { screen = SCR_CAST; sbDrawCast(); }
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Edit screen (also the last step of the creation wizard)
// ---------------------------------------------------------------------------
static void sbDrawEdit() {
  tft.fillScreen(C_BG);
  drawBackBtn(24);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_GOLD, C_BG);
  tft.drawString(editNew ? "New spellbook: level and spellcasting" : "Edit spellbook", 50, 6);
  char b[40];
  tft.fillRoundRect(10, 30, 300, 26, 5, C_PANEL);                                    // name row
  tft.setTextColor(C_DIM, C_PANEL);  tft.drawString("Name", 18, 36);
  tft.setTextColor(C_TEXT, C_PANEL); drawFit(ch.name, 80, 36, 222);
  snprintf(b, sizeof(b), "%d", ch.level);               drawStepper(60, "Level", b);
  snprintf(b, sizeof(b), "%+d", ch.mod);                drawStepper(88, "Casting mod", b);
  snprintf(b, sizeof(b), "%d", ch.dc);                  drawStepper(116, "Spell save DC", b);
  snprintf(b, sizeof(b), "%+d", ch.atk);                drawStepper(144, "Spell attack", b);
  tft.fillRoundRect(10, 174, 300, 26, 5, C_PANEL);                                   // subclass row
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_DIM, C_PANEL);  tft.drawString("Subclass", 18, 180);
  char sb2[64];
  snprintf(sb2, sizeof(sb2), "%s%s%s", ch.subName[0] ? ch.subName : "None", ch.choice[0] ? " - " : "", ch.choice);
  tft.setTextColor(C_TEXT, C_PANEL); drawFit(sb2, 90, 180, 212);
  drawBtn(10, 206, 96, 28, ch.autoCalc ? "Auto: on" : "Auto DC/Atk", C_PANEL, C_TEXT);
  if (editNew) drawBtn(114, 206, 196, 28, "Create spellbook", C_SEL, C_TEXT);
  else drawBtn(114, 206, 196, 28, delArmed ? "Tap again to DELETE" : "Delete spellbook", delArmed ? C_ERR : C_PANEL, C_TEXT);
}

static void editChanged(bool statsTouched) {
  if (statsTouched) ch.autoCalc = 0;
  if (ch.autoCalc) autoStats();
  sbRecalc();
}

static void sbEditTap(int x, int y) {
  if (y < 28 && x < 44) {
    if (editNew) { wzGo(wzNSubs > 0 ? 4 : 3); return; }
    saveChar(); sbOpenChar(ch.slot);
    return;
  }
  if (y >= 30 && y < 58) { kbStart(KB_RENAME, "Spellbook name", ch.name, 16); return; }
  bool minus = x >= 170 && x < 214, plus = x >= 266 && x < 310;
  if ((minus || plus) && y >= 60 && y < 172) {
    int d = plus ? 1 : -1;
    int row = (y - 60) / 28;
    if (row == 0) { int l = ch.level + d; ch.level = (uint8_t)(l < 1 ? 1 : l > 20 ? 20 : l); editChanged(false); }
    else if (row == 1) { int m = (int8_t)ch.mod + d; ch.mod = (uint8_t)(m < 0 ? 0 : m > 10 ? 10 : m); editChanged(false); }
    else if (row == 2) { int v = ch.dc + d; ch.dc = (uint8_t)(v < 0 ? 0 : v > 30 ? 30 : v); editChanged(true); }
    else if (row == 3) { int v = ch.atk + d; ch.atk = (int8_t)(v < -5 ? -5 : v > 20 ? 20 : v); editChanged(true); }
    delArmed = false;
    sbDrawEdit();
    return;
  }
  if (y >= 174 && y < 202) {
    delArmed = false;
    if (ch.cls[0]) {
      // subclass list for the class
      loadSubIndexRoot(bookRoot(ch.book), ch.cls, wzSubs, wzNSubs);
      pickStart(PK_EDITSUB, "Subclass");
    }
    return;
  }
  if (y >= 206) {
    if (x < 108) { ch.autoCalc = 1; editChanged(false); delArmed = false; sbDrawEdit(); return; }
    if (editNew) {
      int s = freeSlot();
      if (s < 0) { showError("No free spellbook slot (max 12)."); return; }
      ch.slot = (uint8_t)s;
      editChanged(false);
      if (!saveChar()) { showError("Could not write /characters on the SD card."); return; }
      sbOpenChar(s);
      return;
    }
    if (!delArmed) { delArmed = true; sbDrawEdit(); return; }
    char path[32];
    charPath(ch.slot, path, sizeof(path));
    SD.remove(path);
    screen = SCR_HOME; homeTab = 1; sbHomeScan(); drawHome();
  }
}

// ---------------------------------------------------------------------------
// Picker (species, class, subclass, subclass choice)
// ---------------------------------------------------------------------------
static void pickStart(PickKind k, const char* title) {
  pickKind = k;
  copyStr(pickTitle, title, sizeof(pickTitle));
  pickScroll = 0;
  nPick = 0;
  if (k == PK_SPECIES) {
    if (ch.book == 1) {
      File f = SD.open(NEON_ROOT "/index/species.txt", FILE_READ);
      char line[PICK_LEN + 4];
      if (f) { while (fileLine(f, line, sizeof(line)) && nPick < PICK_MAX - 1) if (line[0]) copyStr(pickItems[nPick++], line, PICK_LEN); f.close(); }
    } else {
      static const char* const S[] = { "Aasimar", "Dragonborn", "Dwarf", "Elf", "Gnome", "Goliath", "Halfling", "Human", "Orc", "Tiefling" };
      for (unsigned i = 0; i < sizeof(S) / sizeof(S[0]); i++) copyStr(pickItems[nPick++], S[i], PICK_LEN);
    }
    copyStr(pickItems[nPick++], "Other (type it)...", PICK_LEN);
  } else if (k == PK_CLASS) {
    char path[48];
    snprintf(path, sizeof(path), "%s/index/classes.txt", bookRoot(ch.book));
    File f = SD.open(path, FILE_READ);
    char line[96];
    if (f) {
      while (fileLine(f, line, sizeof(line)) && nPick < PICK_MAX) {
        if (line[0] == '#' || !line[0]) continue;
        char* a = strchr(line, '|');
        if (!a) continue;
        *a = 0;
        if (atoi(a + 1) > 0) copyStr(pickItems[nPick++], line, PICK_LEN);     // casters only
      }
      f.close();
    }
  } else if (k == PK_SUB || k == PK_EDITSUB) {
    copyStr(pickItems[nPick++], "None (not chosen yet)", PICK_LEN);
    for (int i = 0; i < wzNSubs && nPick < PICK_MAX; i++) copyStr(pickItems[nPick++], wzSubs[i].name, PICK_LEN);
  } else if (k == PK_CHOICE || k == PK_EDITCHOICE) {
    for (int i = 0; i < nWzGroups && nPick < PICK_MAX; i++) copyStr(pickItems[nPick++], wzGroups[i], PICK_LEN);
  }
  screen = SCR_PICK;
  sbDrawPick();
}

static void sbDrawPick() {
  tft.fillScreen(C_BG);
  drawBackBtn(24);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_GOLD, C_BG);
  drawFit(pickTitle, 50, 6, SCREEN_W - 56);
  int rows = (SCREEN_H - 34) / 30;
  for (int r = 0; r < rows; r++) {
    int k = pickScroll + r;
    if (k >= nPick) break;
    int y = 34 + r * 30;
    tft.fillRoundRect(6, y, 298, 27, 5, C_PANEL);
    tft.setTextColor(C_TEXT, C_PANEL);
    drawFit(pickItems[k], 16, y + 5, 276);
  }
  if (nPick > rows) {
    int th = (SCREEN_H - 34) * rows / nPick;
    if (th < 12) th = 12;
    int ty = 34 + ((SCREEN_H - 34) - th) * pickScroll / (nPick - rows);
    tft.fillRect(310, 34, 4, SCREEN_H - 34, C_PANEL);
    tft.fillRect(310, ty, 4, th, C_DIM);
  }
}

static void sbPickScrollBy(int d) {
  int rows = (SCREEN_H - 34) / 30;
  int maxS = nPick > rows ? nPick - rows : 0;
  int ns = pickScroll + d;
  if (ns < 0) ns = 0;
  if (ns > maxS) ns = maxS;
  if (ns != pickScroll) { pickScroll = ns; sbDrawPick(); }
}

// distinct selectable groups (not the "*common" ones) of a subclass doc
static void loadGroups(int book, uint16_t sid) {
  nWzGroups = 0;
  if (!sid) return;
  char path[48], line[160];
  snprintf(path, sizeof(path), "%s/sub/%03d.txt", bookRoot(book), (int)sid);
  File f = SD.open(path, FILE_READ);
  if (!f) return;
  bool grant = false;
  while (fileLine(f, line, sizeof(line))) {
    if (!grant) { if (!strcmp(line, "@GRANT")) grant = true; continue; }
    char* a = strchr(line, '|');
    if (!a) continue;
    char* b = strchr(a + 1, '|');
    if (!b) continue;
    *b = 0;
    const char* g = a + 1;
    if (!g[0] || g[0] == '*') continue;
    bool have = false;
    for (int i = 0; i < nWzGroups; i++) if (!strcmp(wzGroups[i], g)) have = true;
    if (!have && nWzGroups < 4) copyStr(wzGroups[nWzGroups++], g, sizeof(wzGroups[0]));
  }
  f.close();
}

static void afterSubChosen(bool editing) {
  loadGroups(ch.book, ch.subId);
  ch.choice[0] = 0;
  if (nWzGroups > 0) { pickStart(editing ? PK_EDITCHOICE : PK_CHOICE, "Choose your option"); return; }
  if (editing) { sbRecalc(); screen = SCR_EDIT; sbDrawEdit(); }
  else wzGo(6);
}

static void sbPickTap(int x, int y) {
  if (y < 30 && x < 44) {
    if (pickKind == PK_EDITSUB || pickKind == PK_EDITCHOICE) { screen = SCR_EDIT; sbDrawEdit(); }
    else wzGo(wzStep - 1 < 0 ? 0 : (pickKind == PK_CHOICE ? 4 : wzStep - 1));
    return;
  }
  int k = pickScroll + (y - 34) / 30;
  if (y < 34 || k >= nPick) return;
  switch (pickKind) {
    case PK_SPECIES:
      if (k == nPick - 1) kbStart(KB_WZ_SPECIES, "Species", "", 18);
      else { copyStr(ch.species, pickItems[k], sizeof(ch.species)); wzGo(3); }
      break;
    case PK_CLASS:
      copyStr(ch.cls, pickItems[k], sizeof(ch.cls));
      ch.subId = 0; ch.subName[0] = 0; ch.choice[0] = 0;
      loadSubIndexRoot(bookRoot(ch.book), ch.cls, wzSubs, wzNSubs);
      wzGo(wzNSubs > 0 ? 4 : 6);
      break;
    case PK_SUB:
    case PK_EDITSUB: {
      bool ed = (pickKind == PK_EDITSUB);
      if (k == 0) { ch.subId = 0; ch.subName[0] = 0; ch.choice[0] = 0; afterSubChosen(ed); }
      else { ch.subId = wzSubs[k - 1].id; copyStr(ch.subName, wzSubs[k - 1].name, sizeof(ch.subName)); afterSubChosen(ed); }
      break;
    }
    case PK_CHOICE:
    case PK_EDITCHOICE: {
      bool ed = (pickKind == PK_EDITCHOICE);
      copyStr(ch.choice, pickItems[k], sizeof(ch.choice));
      if (ed) { sbRecalc(); screen = SCR_EDIT; sbDrawEdit(); } else wzGo(6);
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// On-screen keyboard
// ---------------------------------------------------------------------------
static const char* const KB_ROWS[3] = { "1234567890", "qwertyuiop", "asdfghjkl'" };
static const char KB_ROW4[] = "zxcvbnm";

static void kbStart(KbKind k, const char* title, const char* initial, int maxLen) {
  kbKind = k;
  copyStr(kbTitle, title, sizeof(kbTitle));
  copyStr(kbBuf, initial, sizeof(kbBuf));
  kbMax = maxLen > (int)sizeof(kbBuf) - 1 ? (int)sizeof(kbBuf) - 1 : maxLen;
  kbShift = (kbBuf[0] == 0);
  screen = SCR_KEYBOARD;
  sbDrawKeyboard();
}

static void kbKeyRect(int row, int i, int& x, int& y, int& w, int& h) {
  h = 32; y = 38 + row * 38;
  if (row < 3) { w = 30; x = 3 + i * 31; }
  else if (row == 3) {
    if (i == 0) { x = 2; w = 42; }                       // shift
    else if (i <= 7) { x = 46 + (i - 1) * 31; w = 29; }  // z..m
    else { x = 264; w = 54; }                            // backspace
  } else {
    if (i == 0) { x = 2; w = 66; }                       // cancel
    else if (i == 1) { x = 70; w = 30; }                 // -
    else if (i == 2) { x = 102; w = 104; }               // space
    else { x = 208; w = 110; }                           // ok
  }
}

static void sbDrawKeyboard() {
  tft.fillScreen(C_BG);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_GOLD, C_BG);
  tft.drawString(kbTitle, 6, 2);
  tft.fillRoundRect(4, 18, 312, 18, 4, C_PANEL2);
  tft.setTextColor(C_TEXT, C_PANEL2);
  char b[30];
  snprintf(b, sizeof(b), "%s_", kbBuf);
  drawFit(b, 10, 19, 300);
  for (int r = 0; r < 5; r++) {
    int n = r < 3 ? 10 : r == 3 ? 9 : 4;
    for (int i = 0; i < n; i++) {
      int x, y, w, h;
      kbKeyRect(r, i, x, y, w, h);
      char lab[8] = "";
      uint16_t bg = C_PANEL;
      if (r < 3) { lab[0] = KB_ROWS[r][i]; if (kbShift && lab[0] >= 'a' && lab[0] <= 'z') lab[0] = (char)(lab[0] - 32); }
      else if (r == 3) {
        if (i == 0) { strcpy(lab, "Shift"); if (kbShift) bg = C_SEL; }
        else if (i <= 7) { lab[0] = KB_ROW4[i - 1]; if (kbShift) lab[0] = (char)(lab[0] - 32); }
        else strcpy(lab, "Del");
      } else {
        if (i == 0) strcpy(lab, "Cancel"); else if (i == 1) strcpy(lab, "-"); else if (i == 2) strcpy(lab, "Space");
        else { strcpy(lab, "OK"); bg = C_SEL; }
      }
      drawBtn(x, y, w, h, lab, bg, C_TEXT);
    }
  }
}

static void kbFinish(bool ok) {
  if (!ok) {
    if (kbKind == KB_RENAME) { screen = SCR_EDIT; sbDrawEdit(); }
    else if (kbKind == KB_WZ_SPECIES) pickStart(PK_SPECIES, "Species");
    else wzGo(0);
    return;
  }
  // trim
  int n = (int)strlen(kbBuf);
  while (n > 0 && kbBuf[n - 1] == ' ') kbBuf[--n] = 0;
  if (n == 0) return;                                   // empty: keep typing
  if (kbKind == KB_WZ_NAME)         { copyStr(ch.name, kbBuf, sizeof(ch.name)); wzGo(2); }
  else if (kbKind == KB_WZ_SPECIES) { copyStr(ch.species, kbBuf, sizeof(ch.species)); wzGo(3); }
  else                              { copyStr(ch.name, kbBuf, sizeof(ch.name)); screen = SCR_EDIT; sbDrawEdit(); }
}

static void sbKeyTap(int x, int y) {
  for (int r = 0; r < 5; r++) {
    int n = r < 3 ? 10 : r == 3 ? 9 : 4;
    for (int i = 0; i < n; i++) {
      int kx, ky, kw, kh;
      kbKeyRect(r, i, kx, ky, kw, kh);
      if (!inRect(x, y, kx, ky, kw, kh)) continue;
      int len = (int)strlen(kbBuf);
      char c = 0;
      if (r < 3) c = KB_ROWS[r][i];
      else if (r == 3) {
        if (i == 0) { kbShift = !kbShift; sbDrawKeyboard(); return; }
        if (i <= 7) c = KB_ROW4[i - 1];
        else { if (len) kbBuf[len - 1] = 0; kbShift = (strlen(kbBuf) == 0 || kbBuf[strlen(kbBuf) - 1] == ' '); sbDrawKeyboard(); return; }
      } else {
        if (i == 0) { kbFinish(false); return; }
        if (i == 1) c = '-';
        else if (i == 2) c = ' ';
        else { kbFinish(true); return; }
      }
      if (c >= 'a' && c <= 'z' && kbShift) c = (char)(c - 32);
      if (len < kbMax && !(c == ' ' && len == 0)) {
        kbBuf[len] = c; kbBuf[len + 1] = 0;
        kbShift = (c == ' ');                           // capital after a space, small otherwise
      }
      sbDrawKeyboard();
      return;
    }
  }
}

// ---------------------------------------------------------------------------
// Creation wizard
// ---------------------------------------------------------------------------
static void sbDrawWzBook() {
  tft.fillScreen(C_BG);
  drawBackBtn(24);
  setBodyFont();
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_GOLD, C_BG);
  tft.drawString("New spellbook: which book?", 50, 6);
  tft.setTextColor(C_DIM, C_BG);
  tft.drawString("This decides the class and spell lists.", 10, 34);
  int saved = curPack;
  curPack = 0; drawPackBtn(60, 0, "D&D 2024 - SRD 5.2.1", "Official classes and spells", true);
  curPack = 1; drawPackBtn(106, 1, "Neon Odyssey (playtest)", neonAvailable ? "Neon classes, subclasses and spells" : "Not on the SD card (/neon)", neonAvailable);
  curPack = (uint8_t)saved;
  // drawPackBtn highlights the active book; redraw both un-highlighted for clarity
}

static void wzGo(int step) {
  if (step < 0) step = 0;
  wzStep = step;
  switch (step) {
    case 0: screen = SCR_WZBOOK; sbDrawWzBook(); break;
    case 1: kbStart(KB_WZ_NAME, "Character name", ch.name, 16); break;
    case 2: pickStart(PK_SPECIES, "Species"); break;
    case 3: pickStart(PK_CLASS, "Class"); break;
    case 4:
      if (!ch.cls[0]) { wzGo(3); return; }
      loadSubIndexRoot(bookRoot(ch.book), ch.cls, wzSubs, wzNSubs);
      pickStart(PK_SUB, "Subclass");
      break;
    case 5: pickStart(PK_CHOICE, "Choose your option"); break;
    case 6:
      editNew = true; delArmed = false;
      if (!ch.level) ch.level = 1;
      if (ch.autoCalc) autoStats();
      sbRecalc();
      screen = SCR_EDIT; sbDrawEdit();
      break;
  }
}

static void sbWzBookTap(int x, int y) {
  if (y < 30 && x < 44) { screen = SCR_HOME; homeTab = 1; sbHomeScan(); drawHome(); return; }
  int b = -1;
  if (y >= 60 && y < 98) b = 0;
  else if (y >= 106 && y < 144) b = 1;
  if (b < 0) return;
  if (b == 1 && !neonAvailable) return;
  ch.book = (uint8_t)b;
  ch.cls[0] = 0; ch.subName[0] = 0; ch.choice[0] = 0; ch.subId = 0; ch.species[0] = 0; ch.name[0] = 0;
  wzGo(1);
}

// ---------------------------------------------------------------------------
// Dispatchers used by main.cpp
// ---------------------------------------------------------------------------
static void sbDrawScreen() {
  switch (screen) {
    case SCR_SHEET:    sbDrawSheet(); break;
    case SCR_EDIT:     sbDrawEdit(); break;
    case SCR_PICK:     sbDrawPick(); break;
    case SCR_KEYBOARD: sbDrawKeyboard(); break;
    case SCR_CAST:     sbDrawCast(); break;
    case SCR_WZBOOK:   sbDrawWzBook(); break;
    default: break;
  }
}

static void sbTap(int x, int y) {
  switch (screen) {
    case SCR_SHEET:    sbSheetTap(x, y); break;
    case SCR_EDIT:     sbEditTap(x, y); break;
    case SCR_PICK:     sbPickTap(x, y); break;
    case SCR_KEYBOARD: sbKeyTap(x, y); break;
    case SCR_CAST:     sbCastTap(x, y); break;
    case SCR_WZBOOK:   sbWzBookTap(x, y); break;
    default: break;
  }
}

static void sbScrollBy(int d) {
  if (screen == SCR_SHEET) sbSheetScrollBy(d);
  else if (screen == SCR_PICK) sbPickScrollBy(d);
  else if (screen == SCR_HOME && homeTab == 1) sbHomeScrollBy(d);
}

static bool sbScrollable() {
  return screen == SCR_PICK || (screen == SCR_SHEET && shTab != 0) || (screen == SCR_HOME && homeTab == 1);
}
