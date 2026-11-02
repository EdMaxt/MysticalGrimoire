#include "Arduino.h"
SerialStub Serial;
#include "SD.h"
SDClass SD;
#include "../src/main.cpp"

static int bad = 0;
#define CHECK(c, ...) do { if(!(c)) { bad++; printf("  FAIL: "); printf(__VA_ARGS__); printf("\n"); } } while(0)

struct Stats { int opened=0, fail=0, overWidth=0, nonAscii=0, maxLines=0; const char* maxName=""; };

static void scanLines(Stats& st, const char* name) {
  if (nLines > st.maxLines) { st.maxLines = nLines; st.maxName = name; }
  CHECK(nLines < MAX_LINES, "%s: %d lines hit MAX_LINES", name, nLines);
  for (int i = 0; i < nLines; i++) {
    const Line& L = lines[i];
    if (L.kind == K_GAP || L.kind == K_HEAD || L.kind == K_BASE) continue;
    int w = measure(fileBuf + L.off, L.len);
    if (L.first && KLABEL[L.kind][0]) w += tft.textWidth(KLABEL[L.kind]);
    if (w > viewW) st.overWidth++;
  }
  for (const char* p = fileBuf; *p; p++) if ((uint8_t)*p > 127) { st.nonAscii++; break; }
}

static void dump(int n) {
  printf("--- %s | %s\n", readerTitle, readerSub);
  for (int i = 0; i < nLines && i < n; i++) {
    char t[130] = ""; if (lines[i].kind != K_BASE && lines[i].kind != K_HEAD) { memcpy(t, fileBuf + lines[i].off, lines[i].len); t[lines[i].len] = 0; }
    if (lines[i].kind == K_BASE) strcpy(t, readerBase);
    printf("[k%d f%d] %s\n", lines[i].kind, lines[i].first, t);
  }
}

static void tap(int x, int y) { handleTap(x, y); }

int main() {
  setenv("SD_HOST_ROOT", "/tmp/sd_host", 1);
  g_autoRelease = true;
  remove("/tmp/prefs_host.txt");
  setup();
  printf("== SRD pack ==\nscreen=%d pack=%d classes=%d neonAvailable=%d lineH=%d\n", screen, curPack, nClasses, neonAvailable, lineH);
  CHECK(curPack == 0 && nClasses == 8, "SRD should start with 8 classes");
  {
    Stats st; int total = 0;
    for (int c = 0; c < nClasses; c++) {
      startClass(c);
      CHECK(nEntries == classes[c].count, "%s entries %d vs count %d", classes[c].name, nEntries, classes[c].count);
      for (int i = 0; i < nEntries; i++) {
        total++;
        if (!openSpell(entries[i])) { st.fail++; continue; }
        st.opened++; scanLines(st, entries[i].name);
      }
    }
    printf("SRD list entries=%d opened=%d fail=%d maxLines=%d (%s) overWidth=%d nonAscii=%d\n", total, st.opened, st.fail, st.maxLines, st.maxName, st.overWidth, st.nonAscii);
    CHECK(st.fail == 0 && st.overWidth == 0 && st.nonAscii == 0, "SRD problems");
  }

  { int ph = 0, phOpen = 0;
    for (int c = 0; c < nClasses; c++) { startClass(c); for (int i = 0; i < nEntries; i++) if (entries[i].flags & 4) { ph++; if (openSpell(entries[i]) && (sv.flags & 4) && strstr(sv.body, "not in the free SRD")) phOpen++; } }
    printf("PHB-only entries across class lists=%d, readable stubs=%d\n", ph, phOpen);
    CHECK(ph > 0 && ph == phOpen, "PHB-only stubs");
    startClass(7); int n = 0; for (int i = 0; i < nEntries; i++) if (entries[i].flags & 4) n++;
    printf("Wizard: %d entries, %d PHB-only; sample subtitle: ", nEntries, n);
    for (int i = 0; i < nEntries; i++) if (entries[i].flags & 4) { openSpell(entries[i]); printf("%s | %s\n", readerTitle, readerSub); break; }
    CHECK(nEntries <= MAX_ENTRIES, "MAX_ENTRIES"); }

  printf("== Toggle to Neon via the Options screen ==\n");
  screen = SCR_HOME; drawHome();
  tap(40, 224);                        CHECK(screen == SCR_OPTIONS, "Options button");
  tap(150, 110);                       CHECK(curPack == 1 && nClasses == 13, "tap Neon -> pack=%d classes=%d", curPack, nClasses);
  { FILE* f = fopen("/tmp/prefs_host.txt", "r"); unsigned v = 9; char k[16]; if (f) { fscanf(f, "%15s %u", k, &v); fclose(f);} CHECK(v == 1, "pref saved"); }
  tap(150, 66);                        CHECK(curPack == 0 && nClasses == 8, "tap SRD back");
  tap(150, 110);                       CHECK(curPack == 1, "Neon again");
  printf("== rotation / home split ==\n");
  CHECK(rot180 == 0, "rotation default");
  tap(237, 190);                       CHECK(rot180 == 1, "rotate -> 180");
  { FILE* f = fopen("/tmp/prefs_host.txt", "r"); char k[16]; unsigned v; bool ok = false; while (f && fscanf(f, "%15s %u", k, &v) == 2) if (!strcmp(k, "rot") && v == 1) ok = true; if (f) fclose(f); CHECK(ok, "rotation saved"); }
  tap(237, 190);                       CHECK(rot180 == 0, "rotate back");
  { screen = SCR_HOME; homeTab = 0; drawHome(); int nc = 0; for (int i = 0; i < nClasses; i++) if (classes[i].count == 0) nc++;
    CHECK(splitHome() && nc == 4, "neon home split, no-spell classes=%d", nc);
    int maxCasterY = 0, minStripY = 999;
    for (int i = 0; i < nClasses; i++) { int x, y, w, h; tileRect(i, x, y, w, h); CHECK(y + h <= 212 && x + w <= 320, "tile %d fits (%d,%d)", i, y, h);
      if (classes[i].count > 0) { if (y + h > maxCasterY) maxCasterY = y + h; } else if (y < minStripY) minStripY = y; }
    CHECK(maxCasterY < 164 && minStripY > 180, "caster rows end %d, strip starts %d", maxCasterY, minStripY);
    for (int i = 0; i < nClasses; i++) if (classes[i].count == 0) { int x, y, w, h; tileRect(i, x, y, w, h); tap(x + 5, y + 5); CHECK(screen == SCR_SUBS, "strip class opens subclasses"); screen = SCR_HOME; drawHome(); break; }
    for (int i = 0; i < nClasses; i++) if (classes[i].count > 0) { int x, y, w, h; tileRect(i, x, y, w, h); tap(x + 5, y + 5); CHECK(screen == SCR_CLASS, "caster tile opens list"); screen = SCR_HOME; drawHome(); break; }
    screen = SCR_HOME; drawHome();
    printf("icons: pushes after Neon home = %d (bad %d)\n", tft.pushes, tft.badPush);
    CHECK(tft.pushes >= 13 && tft.badPush == 0, "neon tile icons drawn (%d, bad %d)", tft.pushes, tft.badPush);
    int before = tft.pushes; startClass(0);
    CHECK(bgCache != nullptr && tft.pushes > before + 100 && tft.badPush == 0, "class background loaded and painted (%d)", tft.pushes - before);
    listScrollBy(3); listScrollBy(-3);
    for (int c = 0; c < nClasses; c++) if (classes[c].count > 0) { startClass(c); CHECK(bgCache != nullptr, "bg for %s", classes[c].name); }
    tap(10, 10);                       CHECK(bgCache == nullptr && screen == SCR_HOME, "back frees the background");
    setPack(0); screen = SCR_HOME; drawHome();
    CHECK(tft.badPush == 0, "srd tile icons ok");
    startClass(7); CHECK(bgCache != nullptr, "wizard bg"); tap(10, 10);
    setPack(1);
    screen = SCR_OPTIONS; drawOptions(); }
  printf("== brightness / sleep ==\n");
  CHECK(briLevel == 5 && g_bri == 255, "default brightness %d duty %d", briLevel, g_bri);
  tap(190, 150);                       CHECK(briLevel == 4 && g_bri == 204, "minus -> %d duty %d", briLevel, g_bri);
  tap(190, 150); tap(190, 150); tap(190, 150); tap(190, 150);   CHECK(briLevel == 1 && g_bri == 51, "floor -> %d duty %d", briLevel, g_bri);
  tap(290, 150);                       CHECK(briLevel == 2 && g_bri == 102, "plus -> %d duty %d", briLevel, g_bri);
  { FILE* f = fopen("/tmp/prefs_host.txt", "r"); char k[16]; unsigned v; bool ok = false; while (f && fscanf(f, "%15s %u", k, &v) == 2) if (!strcmp(k, "bri") && v == 2) ok = true; if (f) fclose(f); CHECK(ok, "brightness saved"); }
  tap(290, 150); tap(290, 150); tap(290, 150); tap(290, 150);   CHECK(briLevel == 5 && g_bri == 255, "ceiling");
  tap(190, 150); tap(190, 150);        // leave it at 3 for the reboot test below
  prefs.begin("grimoire", false);
  tap(150, 190);                       CHECK(g_slept && g_bri == 0, "Sleep now: slept=%d duty=%d", g_slept, g_bri);
  g_slept = false; applyBrightness();
  printf("== soft sleep ==\n");
  { screen = SCR_OPTIONS; drawOptions();
    CHECK(softIdx == 3, "default 5 min (%d)", softIdx);
    tap(240, 224); CHECK(softIdx == 4, "cycle -> 10 min");
    tap(240, 224); CHECK(softIdx == 0, "cycle -> never");
    g_ms = 1000000; lastActive = 0; idleTick(); CHECK(idleState == 0 && g_bri == BRI_DUTY[briLevel - 1], "never: stays on");
    tap(240, 224); CHECK(softIdx == 1, "cycle -> 1 min");
    g_ms = 5000000; lastActive = g_ms; idleTick(); CHECK(idleState == 0, "fresh activity: awake");
    g_ms += 59000; idleTick(); CHECK(idleState == 0, "59 s: awake");
    g_ms += 2000;  idleTick(); CHECK(idleState == 1 && g_bri == 12, "61 s: dimmed duty %d", g_bri);
    g_ms += 10000; idleTick(); CHECK(idleState == 2 && g_bri == 0, "+10 s: off duty %d", g_bri);
    idleWake();    CHECK(idleState == 0 && g_bri == BRI_DUTY[briLevel - 1], "wake restores brightness %d", g_bri);
    g_ms += 61000; idleTick(); idleWake(); CHECK(idleState == 0, "wake from dim");
    { FILE* f = fopen("/tmp/prefs_host.txt", "r"); char k[16]; unsigned v; bool ok = false; while (f && fscanf(f, "%15s %u", k, &v) == 2) if (!strcmp(k, "soff") && v == 1) ok = true; if (f) fclose(f); CHECK(ok, "soft sleep saved"); }
    tap(240, 224); tap(240, 224); tap(240, 224); CHECK(softIdx == 4, "cycle to 10"); tap(240, 224); CHECK(softIdx == 0, "wraps to never");
    tap(240, 224); tap(240, 224); tap(240, 224); CHECK(softIdx == 3, "back to 5 min"); }
  // BOOT held 2 s from loop(): must be released once first (armed), then held
  g_autoRelease = false;
  g_bootLevel = 0; g_ms = 100; loop(); g_ms = 3000; loop();   CHECK(!g_slept, "BOOT held from boot must not sleep until released once");
  g_bootLevel = 1; loop(); g_bootLevel = 0; g_ms = 5000; loop(); g_ms = 6500; loop(); CHECK(!g_slept, "BOOT held 1.5 s must not sleep");
  g_ms = 7100; g_autoRelease = true; loop();   CHECK(g_slept, "BOOT held 2 s sleeps");
  g_bootLevel = 1; g_slept = false; applyBrightness();
  screen = SCR_OPTIONS;
  tap(20, 14);                         CHECK(screen == SCR_HOME, "back to home");

  printf("== simulated reboot keeps Neon ==\n");
  prefs.begin("grimoire", false); curPack = prefs.getUChar("pack", 0); briLevel = prefs.getUChar("bri", 5); nClasses = 0; bootData();
  CHECK(curPack == 1 && nClasses == 13 && briLevel == 3, "reboot pack=%d classes=%d bri=%d", curPack, nClasses, briLevel);

  printf("== Neon home tiles ==\n");
  for (int i = 0; i < nClasses; i++) {
    int x, y, w, h; tileRect(i, x, y, w, h);
    CHECK(x >= 0 && x + w <= SCREEN_W && y >= 40 && y + h <= 212, "tile %d out of bounds (%d,%d,%d,%d)", i, x, y, w, h);
    CHECK(strlen(classes[i].name) < sizeof(classes[i].name), "name");
  }

  printf("== Neon class lists ==\n");
  Stats st; int total = 0, aliases = 0, newSp = 0, srdRef = 0;
  for (int c = 0; c < nClasses; c++) {
    if (classes[c].count == 0) { printf("%-11s (no spell list) subclasses=%d based on %s\n", classes[c].name, classes[c].subCount, classes[c].basedOn); continue; }
    startClass(c);
    CHECK(screen == SCR_CLASS, "%s did not open", classes[c].name);
    CHECK(nEntries == classes[c].count, "%s entries %d vs count %d", classes[c].name, nEntries, classes[c].count);
    printf("%-11s entries=%3d tabs=%d  based on %s\n", classes[c].name, nEntries, nTabs, classes[c].basedOn);
    for (int i = 0; i < nEntries; i++) {
      total++;
      aliases += entries[i].base[0] != 0; newSp += entries[i].src == 1; srdRef += entries[i].src == 0;
      if (!openSpell(entries[i])) { st.fail++; printf("  cannot open %s\n", entries[i].name); continue; }
      st.opened++; scanLines(st, entries[i].name);
      CHECK(entries[i].level == sv.level, "%s list level %d vs spell level %d", entries[i].name, entries[i].level, sv.level);
    }
  }
  printf("Neon entries=%d (new=%d srd=%d reskins=%d) opened=%d fail=%d maxLines=%d (%s) overWidth=%d nonAscii=%d\n",
         total, newSp, srdRef, aliases, st.opened, st.fail, st.maxLines, st.maxName, st.overWidth, st.nonAscii);
  CHECK(st.fail == 0 && st.overWidth == 0 && st.nonAscii == 0, "Neon spell problems");

  printf("== Every Neon-only spell file opens ==\n");
  { Stats s2; Entry e = {}; for (int id = 1; id <= 95; id++) { e.src = 1; e.id = id; e.name[0] = 0; e.base[0] = 0;
      if (!openSpell(e)) s2.fail++; else { s2.opened++; scanLines(s2, sv.name); } }
    printf("neon spell files opened=%d fail=%d maxLines=%d (%s) overWidth=%d nonAscii=%d\n", s2.opened, s2.fail, s2.maxLines, s2.maxName, s2.overWidth, s2.nonAscii);
    CHECK(s2.opened == 95 && s2.overWidth == 0 && s2.nonAscii == 0, "neon files"); }

  printf("== Subclasses ==\n");
  { Stats s3; int docs = 0, withSpells = 0;
    for (int c = 0; c < nClasses; c++) {
      if (!loadSubIndex(c)) { printf("  no sub index for %s\n", classes[c].name); bad++; continue; }
      CHECK(nSubs == classes[c].subCount, "%s subs %d vs %d", classes[c].name, nSubs, classes[c].subCount);
      for (int k = 0; k < nSubs; k++) {
        if (!openSub(k)) { s3.fail++; continue; }
        docs++; withSpells += subs[k].hasSpells; scanLines(s3, subs[k].name);
      }
    }
    printf("subclass docs=%d withSpellTables=%d fail=%d maxLines=%d (%s) overWidth=%d nonAscii=%d\n", docs, withSpells, s3.fail, s3.maxLines, s3.maxName, s3.overWidth, s3.nonAscii);
    CHECK(docs == 52 && s3.fail == 0 && s3.overWidth == 0 && s3.nonAscii == 0, "subclass problems"); }

  printf("== Navigation flows ==\n");
  screen = SCR_HOME; drawHome();
  { int di = -1; for (int i = 0; i < nClasses; i++) if (!strcmp(classes[i].name, "Disciple")) di = i;
    int x, y, w, h; tileRect(di, x, y, w, h);
    tap(x + 5, y + 5);                 CHECK(screen == SCR_SUBS && nSubs == 4, "Disciple -> subclass list (screen=%d)", screen);
    tap(100, 40);                      CHECK(screen == SCR_READER, "open subclass");
    dump(14);
    tap(20, 14);                       CHECK(screen == SCR_SUBS, "reader back -> subs");
    tap(20, 14);                       CHECK(screen == SCR_HOME, "subs back -> home");
    int gi = -1; for (int i = 0; i < nClasses; i++) if (!strcmp(classes[i].name, "Cryptist")) gi = i;
    tileRect(gi, x, y, w, h);
    tap(x + 5, y + 5);                 CHECK(screen == SCR_CLASS, "Cryptist -> spells");
    tap(SCREEN_W - 110, 14);           CHECK(screen == SCR_SUBS, "Sub button");
    tap(100, 40);                      CHECK(screen == SCR_READER, "open Cacophonous");
    dump(40);
    tap(20, 14); tap(20, 14);          CHECK(screen == SCR_CLASS && nEntries == classes[gi].count, "back to class list intact");
    tap(100, LIST_Y + 5);              CHECK(screen == SCR_READER, "open a spell");
  }
  // a reskin spell, e.g. Enforcer "Bolster"
  { int ei = -1; for (int i = 0; i < nClasses; i++) if (!strcmp(classes[i].name, "Enforcer")) ei = i;
    startClass(ei); for (int i = 0; i < nEntries; i++) if (entries[i].base[0]) { openSpell(entries[i]); dump(8); break; } }
  // a new Neon spell
  { for (int i = 0; i < nEntries; i++) if (entries[i].src == 1) { openSpell(entries[i]); dump(10); break; } }
  openAbout(); printf("--- ABOUT (neon) lines=%d\n", nLines);

  printf("== fold ==\n");
  char s[] = "Tasha\xE2\x80\x99s \xE2\x80\x9Cx\xE2\x80\x9D \xE2\x80\x94 5\xC3\x97 \xE2\x88\x92" "1 \xE2\x80\xA6 \xE2\x96\xBA go";
  foldAscii(s); printf("fold: %s\n", s);
  CHECK(strstr(s, "> go") != nullptr, "pointer fold");
  { char e[] = "Acci\xC3\xB3n \xC3\xA1\xC3\xA9\xC3\xAD\xC3\xB3\xC3\xBA \xC3\xB1 \xC3\x91 \xC2\xBFque\xC2\xA1"; foldAscii(e); printf("fold es: %s\n", e);
    CHECK(!strcmp(e, "Accion aeiou n N ?que!"), "spanish fold: %s", e); }


  // ================= spellbooks =================
  printf("== spellbooks ==\n");
  auto key = [&](int r, int i) { int x, y, w, h; kbKeyRect(r, i, x, y, w, h); tap(x + w / 2, y + h / 2); };
  auto letter = [&](char c) {
    static const char* rows[3] = { "1234567890", "qwertyuiop", "asdfghjkl'" };
    for (int r = 0; r < 3; r++) { const char* q = strchr(rows[r], c); if (q) { key(r, (int)(q - rows[r])); return; } }
    const char* q = strchr("zxcvbnm", c); if (q) key(3, 1 + (int)(q - "zxcvbnm"));
  };
  auto typeName = [&](const char* n) { for (const char* p = n; *p; p++) letter(*p); key(4, 3); };
  auto pickRow = [&](int k) { tap(100, 34 + (k - pickScroll) * 30 + 8); };
  curPack = 0; nClasses = 0; loadClasses(); applyTheme(); screen = SCR_HOME; homeTab = 0; drawHome();
  tap(250, 39);                         CHECK(homeTab == 1 && nChars == 0, "Spellbooks tab, chars=%d", nChars);
  tap(100, 60);                         CHECK(screen == SCR_WZBOOK, "wizard book screen (%d)", screen);
  tap(100, 70);                         CHECK(screen == SCR_KEYBOARD && ch.book == 0, "keyboard (%d)", screen);
  typeName("ana");                      CHECK(screen == SCR_PICK && !strcmp(ch.name, "Ana"), "name=%s screen=%d", ch.name, screen);
  pickRow(0);                           CHECK(screen == SCR_PICK && !strcmp(ch.species, "Aasimar"), "species=%s", ch.species);
  CHECK(!strcmp(pickItems[2], "Druid"), "class list item2=%s", pickItems[2]);
  pickRow(2);                           CHECK(!strcmp(ch.cls, "Druid") && wzNSubs > 0, "class=%s subs=%d", ch.cls, wzNSubs);
  pickRow(1);                           CHECK(ch.subId != 0 && nWzGroups > 0, "sub=%s id=%d groups=%d", ch.subName, ch.subId, nWzGroups);
  printf("  groups:"); for (int i = 0; i < nWzGroups; i++) printf(" %s", wzGroups[i]); printf("\n");
  pickRow(0);                           CHECK(screen == SCR_EDIT && editNew && ch.choice[0], "edit screen, choice=%s", ch.choice);
  for (int i = 0; i < 4; i++) tap(290, 70);       // level 5
  for (int i = 0; i < 3; i++) tap(290, 98);       // mod +3
  CHECK(ch.level == 5 && ch.mod == 3 && ch.dc == 14 && ch.atk == 6, "lvl=%d mod=%d dc=%d atk=%d", ch.level, ch.mod, ch.dc, ch.atk);
  tap(290, 126); CHECK(ch.dc == 15 && !ch.autoCalc, "manual dc %d", ch.dc);
  tap(50, 220); CHECK(ch.autoCalc && ch.dc == 14, "auto restored dc %d", ch.dc);
  tap(200, 220);                        CHECK(screen == SCR_SHEET && ch.slot == 0, "sheet open (%d)", screen);
  CHECK(prog.kind == 1 && slotMax(1) == 4 && slotMax(2) == 3 && slotMax(3) == 2 && slotMax(4) == 0, "druid 5 slots %d/%d/%d", slotMax(1), slotMax(2), slotMax(3));
  CHECK(prog.cantrips == 3, "druid cantrips %d", prog.cantrips);
  tap(90, 100);                         CHECK(ch.used[0] == 1, "slot box 1 -> %d", ch.used[0]);
  tap(150, 100);                        CHECK(ch.used[0] == 3, "slot box 3 -> %d", ch.used[0]);
  tap(90, 100);                         CHECK(ch.used[0] == 0, "tap first used box clears -> %d", ch.used[0]);
  tap(90, 130);                         CHECK(ch.used[1] == 1, "L2 box");
  tap(270, 100);                        CHECK(ch.used[1] == 0 && ch.used[0] == 0, "long rest");
  tap(90, 130);
  // always tab
  tap(2 + 3 * 79 + 30, 76);             CHECK(shTab == 3 && nAlways >= 4, "always tab n=%d", nAlways);
  for (int i = 0; i < nAlways; i++) printf("  always: %s L%d\n", alwaysView[i].name, alwaysView[i].level);
  // all tab: prepare something
  tap(2 + 2 * 79 + 30, 76);             CHECK(shTab == 2 && nTabs == 4, "all tab nTabs=%d", nTabs);
  tap(138, 100);                        CHECK(shLevel == 1, "level 1 subtab");
  { Entry first = entries[levelFirst[1]];
    tap(20, 120);                       CHECK(ch.nPrep == 1 && inPrep(refOf(first)), "prepared %s n=%d", first.name, ch.nPrep);
    tap(20, 120);                       CHECK(ch.nPrep == 0, "unprepared");
    tap(20, 120);
    tap(20, 150);                       CHECK(ch.nPrep == 2, "second prep");
    tap(100, 120);                      CHECK(screen == SCR_READER && readerBar == 32, "reader with cast bar (%d,%d)", screen, readerBar);
    tap(50, 225);                       CHECK(screen == SCR_CAST, "cast dialog");
    tap(100, 52 + 30 + 8);              CHECK(ch.used[1] == 2 && screen == SCR_READER, "upcast at L2 -> used=%d screen=%d", ch.used[1], screen);
    tap(50, 225); tap(100, 52 + 8);     CHECK(ch.used[0] == 1, "cast at base L1 used=%d", ch.used[0]);
    tap(50, 225); tap(250, 225);        CHECK(screen == SCR_READER && ch.used[0] == 1, "free cast leaves slots");
    for (int i = 0; i < 4; i++) { tap(50, 225); tap(100, 60); }
    CHECK(ch.used[0] == 4, "cannot overspend L1: %d", ch.used[0]);
    if (screen == SCR_CAST) tap(10, 10);
    CHECK(screen == SCR_READER, "reader after casts (%d)", screen);
    tap(10, 10);                        CHECK(screen == SCR_SHEET && readerBar == 0, "back to sheet");
  }
  tap(2 + 1 * 79 + 30, 76);             CHECK(shTab == 1 && nPrepView == 2 + nAlways, "prepared tab %d (2 chosen + %d always)", nPrepView, nAlways);
  { int a = 0; for (int i = 0; i < nPrepView; i++) if (inAlways(refOf(prepView[i]))) a++; CHECK(a == nAlways, "always shown in Prepared tab"); }
  // persistence round trip
  { Character keep = ch;
    screen = SCR_HOME; homeTab = 1; sbHomeScan();
    CHECK(nChars == 1 && !strcmp(chars[0].name, "Ana") && chars[0].level == 5, "home list n=%d name=%s lvl=%d", nChars, chars[0].name, chars[0].level);
    tap(100, 60);                       CHECK(screen == SCR_SHEET, "reopen");
    CHECK(!memcmp(ch.used, keep.used, 9) && ch.nPrep == keep.nPrep && ch.subId == keep.subId && !strcmp(ch.choice, keep.choice) && ch.dc == keep.dc, "roundtrip");
  }
  // scroll on the All tab does not crash
  tap(2 + 2 * 79 + 30, 76); sbScrollBy(5); sbScrollBy(-9);
  // edit: change level, delete
  tap(300, 14);                         CHECK(screen == SCR_EDIT && !editNew, "edit");
  tap(290, 70);                         CHECK(ch.level == 6, "lvl6");
  tap(200, 220);                        CHECK(delArmed, "delete armed");
  tap(10, 10);                          CHECK(screen == SCR_SHEET, "back saves");
  CHECK(ch.level == 6, "level saved %d", ch.level);
  tap(300, 14); tap(200, 220); tap(200, 220);
  CHECK(screen == SCR_HOME && nChars == 0, "deleted n=%d", nChars);

  // Neon: Cryptist (pact slots) + Cacophonous with a choice
  printf("== spellbooks (Neon) ==\n");
  setPack(1); screen = SCR_HOME; homeTab = 1; drawHome();
  tap(100, 60);                         CHECK(screen == SCR_WZBOOK, "wz");
  tap(100, 118);                        CHECK(ch.book == 1 && screen == SCR_KEYBOARD, "neon book");
  typeName("zed");
  CHECK(nPick > 3, "neon species %d", nPick);
  pickRow(0);
  int ci = -1; for (int i = 0; i < nPick; i++) if (!strcmp(pickItems[i], "Cryptist")) ci = i;
  CHECK(ci >= 0, "Cryptist caster in list");
  { while (ci - pickScroll >= 6) sbPickScrollBy(1); pickRow(ci); }
  CHECK(!strcmp(ch.cls, "Cryptist"), "cls=%s", ch.cls);
  pickRow(1);                           printf("  sub=%s groups=%d screen=%d\n", ch.subName, nWzGroups, screen);
  if (screen == SCR_PICK) pickRow(0);
  CHECK(screen == SCR_EDIT, "edit (%d)", screen);
  for (int i = 0; i < 2; i++) tap(290, 70);       // level 3
  tap(200, 220);                        CHECK(screen == SCR_SHEET && prog.kind == 2, "cryptist sheet kind=%d", prog.kind);
  CHECK(prog.pactCount == 2 && prog.pactLevel == 2, "pact %d x L%d", prog.pactCount, prog.pactLevel);
  tap(90, 100); CHECK(ch.used[1] == 1, "pact box");
  tap(270, 130);                        CHECK(ch.used[1] == 0, "short rest");
  { castEntry.level = 1; int lv[9]; int n = castRows(lv); CHECK(n == 1 && lv[0] == 2, "pact cast rows n=%d", n); }
  printf("  always (cryptist): %d\n", nAlways);
  tap(10, 10);                          CHECK(screen == SCR_HOME, "home");
  tap(100, 60); tap(300, 14); tap(200, 220); tap(200, 220);
  CHECK(nChars == 0, "cleanup");
  setPack(0);

  // ================= battery =================
  printf("== battery ==\n");
  { auto settle = [&](unsigned mv) { g_batmv = mv; for (int i = 0; i < 30; i++) { g_ms += 2100; batteryTick(); } };
    settle(1950);                              CHECK(batV > 3.85f && batV < 3.95f && batPct >= 55 && batPct <= 70, "3.9 V -> %.2f V %d%%", batV, batPct);
    CHECK(!batCharging, "not charging at a steady 3.9 V");
    settle(1500);                              CHECK(batV < 3.1f && batPct == 0, "3.0 V -> %d%%", batPct);
    settle(2100);                              CHECK(batPct == 100 && batCharging, "4.2 V -> %d%% charging=%d", batPct, batCharging);
    settle(1900);                              g_ms += 61000; settle(1900);
    CHECK(!batCharging && batPct >= 35 && batPct <= 45, "3.8 V discharging -> %d%% chg=%d", batPct, batCharging);
    for (int k = 0; k < 6; k++) { g_batmv += 20; g_ms += 2100; batteryTick(); }          // rising voltage = charging
    CHECK(batCharging, "rising voltage detected as charging");
    screen = SCR_HOME; homeTab = 0; drawHome();
    screen = SCR_OPTIONS; drawOptions();
    unsigned short c0 = batCal;
    tap(310, 38);                              CHECK(batCal == c0 + 5, "calibration + -> %d", batCal);
    tap(110, 38);  tap(110, 38);               CHECK(batCal == c0 - 5, "calibration - -> %d", batCal);
    { FILE* f = fopen("/tmp/prefs_host.txt", "r"); char k[16]; unsigned v; bool ok = false; while (f && fscanf(f, "%15s %u", k, &v) == 2) if (!strcmp(k, "bcal") && v == batCal) ok = true; if (f) fclose(f); CHECK(ok, "bcal saved"); }
    for (int i = 0; i < 120; i++) { tap(110, 38); }  CHECK(batCal == 800, "calibration floor %d", batCal);
    for (int i = 0; i < 120; i++) { tap(310, 38); }  CHECK(batCal == 1200, "calibration ceiling %d", batCal);
    batCal = 1000; prefs.putUShort("bcal", 1000); screen = SCR_HOME; drawHome();
    batteryDrawBar(); g_ms += 300; batteryBarTick();
    CHECK(batPercent(4.3f) == 100 && batPercent(3.0f) == 0 && batPercent(3.84f) == 50, "curve ends");
  }

  printf("\nRESULT: %s (%d failures)\n", bad ? "FAIL" : "PASS", bad);
  return bad ? 1 : 0;
}
