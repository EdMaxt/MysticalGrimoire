"""Build the Neon Odyssey pack for the microSD card (SD_CARD/neon).

Inputs (all next to this script's working dir unless given):
  neon_spells.json, neon_lists.json, neon_subclasses.json   (from the parse_* scripts)
  SD_CARD/grimoire/index/all.idx                            (SRD pack, for SRD spell ids)
Output: SD_CARD/neon/...
"""
import json, os, re, shutil, sys, difflib, collections

SD = sys.argv[1] if len(sys.argv) > 1 else 'SD_CARD'
OUT = os.path.join(SD, 'neon')

def J(n):
    return json.load(open(n if os.path.exists(n) else os.path.join('data', n), encoding='utf-8'))
neon, lists, subs = J('neon_spells.json'), J('neon_lists.json'), J('neon_subclasses.json')

norm = lambda x: re.sub(r'[^a-z]', '', x.lower())

# SRD spells (name -> id/level/school/flags) from the already-built SRD pack
srd = {}
for ln in open(os.path.join(SD, 'grimoire/index/all.idx'), encoding='utf-8'):
    ln = ln.strip()
    if not ln: continue
    i, lv, nm, sc, fl = ln.split('|')
    srd[nm] = dict(id='g' + i, level=int(lv), school=sc, flags=fl)
srd_n = {norm(k): k for k in srd}

neon.sort(key=lambda s: s['name'].lower())
nspell = {}
for n, s in enumerate(neon, 1):
    s['id'] = 'n%03d' % n
    s['flags'] = ('C' if s['concentration'] else '') + ('R' if s['ritual'] else '')
    nspell[s['name']] = s
neon_n = {norm(k): k for k in nspell}

CLASS_BASE = {'Cryptist': 'Warlock', 'Disciple': 'Monk', 'Enforcer': 'Paladin', 'Esper': 'Sorcerer',
              'Gridrunner': 'Wizard', 'Icon': 'Bard', 'Juggernaut': 'Barbarian', 'Machinist': 'New class',
              'Oracle': 'Cleric', 'Primalist': 'Druid', 'Scoundrel': 'Rogue', 'Soldier': 'Fighter',
              'Tracer': 'Ranger'}

notes = []
def one_line(t): return ' '.join(t.split())

# ---- reset output --------------------------------------------------------------
if os.path.exists(OUT): shutil.rmtree(OUT)
for d in ('spells', 'index', 'sub'):
    os.makedirs(os.path.join(OUT, d))

# ---- 1. new spell files -------------------------------------------------------------
for s in neon:
    lines = ['name=' + s['name'], 'level=%d' % s['level'], 'school=' + s['school'],
             'classes=' + ', '.join(s['classes']), 'time=' + one_line(s['casting_time']),
             'range=' + one_line(s['range']), 'comp=' + one_line(s['components']),
             'dur=' + one_line(s['duration']), 'flags=' + s['flags'], '---', s['description']]
    if s['higher_level']: lines += ['@HIGHER', s['higher_level']]
    if s['cantrip_upgrade']: lines += ['@CANTRIP', s['cantrip_upgrade']]
    open('%s/spells/%s.txt' % (OUT, s['id'][1:]), 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')

# ---- 2. resolve class lists ----------------------------------------------------------
def resolve(e, cls, taken):
    """Return (id, level, school, flags, display, base) for a list row, or None."""
    name, alias = e['name'], e['alias_of']
    if e['new'] or name in nspell or norm(name) in neon_n:
        key = name if name in nspell else neon_n.get(norm(name))
        if key is None:
            pool = [n for n in nspell if cls in nspell[n]['classes'] and n not in taken]
            m = difflib.get_close_matches(name, pool, n=1, cutoff=0.75)
            if m:
                notes.append('%s list: "%s*" matched to spell "%s" (name differs in the book)' % (cls, name, m[0]))
                key = m[0]
        if key:
            s = nspell[key]
            if s['level'] != e['level']:
                notes.append('%s list places "%s" at level %d but the spell says level %d (using %d)' % (cls, key, e['level'], s['level'], s['level']))
            return s['id'], s['level'], s['school'], s['flags'], key, ''
    base = alias or name
    key = base if base in srd else srd_n.get(norm(base))
    if key:
        s = srd[key]
        return s['id'], s['level'], s['school'], s['flags'], name, (key if alias else '')
    return None

cls_rows = {}
for cls, rows in lists.items():
    taken, out = set(), []
    for e in rows:
        r = resolve(e, cls, taken)
        if r is None:
            notes.append('%s list: "%s" not found in SRD or new spells (skipped)' % (cls, e['name']))
            continue
        if r[0] in [x[0] for x in out] and r[4] == [x[4] for x in out if x[0] == r[0]][0]:
            continue
        taken.add(r[4]); out.append(r)
    # new spells that declare this class in their header but are missing from its list table
    have = {x[0] for x in out}
    for n, s in nspell.items():
        if cls in s['classes'] and s['id'] not in have:
            out.append((s['id'], s['level'], s['school'], s['flags'], n, ''))
            notes.append('%s: "%s" names %s in its header but is missing from the class table (added)' % (cls, n, cls))
    cls_rows[cls] = out

for cls, out in cls_rows.items():
    out.sort(key=lambda r: (r[1], r[4].lower()))
    open('%s/index/%s.idx' % (OUT, cls.lower()), 'w', encoding='utf-8', newline='\n').write(
        '\n'.join('|'.join([r[0], str(r[1]), r[4], r[2], r[3], r[5]]) for r in out) + '\n')

# ---- 3. subclass documents -------------------------------------------------------------
def clean_hdr(h):
    return re.sub(r'\b(Spells|Prepared)\b', '', h).strip()

# name -> id resolver for the granted-spell tables ("*" marks new Neon spells)
disp = {}                       # display name (as shown in a Neon class list) -> id, per class and global
for cls_, rows_ in cls_rows.items():
    for r in rows_:
        disp.setdefault((cls_, norm(r[4])), r[0]); disp.setdefault((None, norm(r[4])), r[0])
def resolve_name(n, cls_):
    k = norm(n.rstrip('*'))
    for key in ((cls_, k), (None, k)):
        if key in disp: return disp[key]
    if k in neon_n: return nspell[neon_n[k]]['id']
    if k in srd_n: return srd[srd_n[k]]['id']
    return None
COMMON_GROUPS = ('Cacophony', 'General')       # first column of the multi-column tables: always granted

by_class = collections.defaultdict(list)
for n, s in enumerate(sorted(subs, key=lambda s: (s['cls'], s['name'])), 1):
    sid = '%03d' % n
    lines = ['name=' + s['name'], 'class=' + s['cls'], 'source=' + s['source'],
             'tagline=' + s['tagline'], '---', s['intro'] or '(No summary extracted.)']
    if s['spell_rows']:
        lines.append('@SPELLS')
        grants = []
        for r in s['spell_rows']:
            multi = len(r['cells']) > 1
            for h, txt in r['cells']:
                h = clean_hdr(h)
                lines.append('Lv %s%s: %s' % (r['level'], (' ' + h) if (multi and h) else '', txt))
                ids = []
                for nm in re.split(r',\s*', txt):
                    nm = nm.strip()
                    if not nm: continue
                    i = resolve_name(nm, s['cls'])
                    if i is None: notes.append('%s/%s: granted spell "%s" not resolved' % (s['cls'], s['name'], nm))
                    else: ids.append(i)
                grp = (('*' if h in COMMON_GROUPS else '') + h) if multi else ''
                grants.append('%s|%s|%s' % (r['level'], grp, ','.join(ids)))
        lines.append('@GRANT')
        lines += grants
    open('%s/sub/%s.txt' % (OUT, sid), 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
    by_class[s['cls']].append((sid, s['name'], s['source'], bool(s['spell_rows'])))

for cls, lst in by_class.items():
    open('%s/sub/%s.idx' % (OUT, cls.lower()), 'w', encoding='utf-8', newline='\n').write(
        '\n'.join('|'.join([sid, nm, src, 'S' if sp else '']) for sid, nm, src, sp in lst) + '\n')

# ---- 4. classes.txt + attribution ---------------------------------------------------------
rows = []
for cls in sorted(CLASS_BASE):
    out = cls_rows.get(cls, [])
    rows.append('%s|%d|%d|%d|%s' % (cls, len(out), max([r[1] for r in out], default=0), len(by_class[cls]), CLASS_BASE[cls]))
open(OUT + '/index/classes.txt', 'w', encoding='utf-8', newline='\n').write(
    '# class|spell_count|max_spell_level|subclass_count|based_on\n' + '\n'.join(rows) + '\n')

# class-granted always-prepared spells and progression tables
ALWAYS = [('Enforcer', 2, 'Cosmic Smite'), ('Enforcer', 5, 'Relentless Ride'), ('Tracer', 1, 'Hunter\u2019s Mark')]
al = []
for c_, lv_, nm_ in ALWAYS:
    i_ = resolve_name(nm_, c_)
    if i_ is None: notes.append('always-prepared "%s" not resolved' % nm_)
    else: al.append('%s|%d|%s' % (c_, lv_, i_))
open(OUT + '/index/always.txt', 'w', encoding='utf-8', newline='\n').write('# class|level|spell id (always prepared, free of the prepared limit)\n' + '\n'.join(al) + '\n')
shutil.copy('data/progress_neon.txt' if os.path.exists('data/progress_neon.txt') else 'progress_neon.txt', OUT + '/index/progress.txt')
SPECIES = ['Aetheron', 'Amanite', 'Anitron', 'Clawderan', 'Glowblin', 'Gorgonite', 'Jozza', 'Laika', 'Murexian', 'Nematoan', 'Nilari',
           'Oozoid', 'Raizo', 'Rangor', 'Saurian', 'Solari', 'Stardust Human', 'Tekal', 'Yaungu', 'Zamani', 'Zorbrak']
open(OUT + '/index/species.txt', 'w', encoding='utf-8', newline='\n').write('\n'.join(SPECIES) + '\n')

open(OUT + '/ATTRIBUTION.txt', 'w', encoding='utf-8', newline='\n').write(
    "Neon Odyssey playtest material (c) 2026 Avantris LLC. All rights reserved. "
    "This card holds a personal, unofficial copy converted from the Playtest 1 PDFs for private use; "
    "do not share it. SRD spell text is under CC BY 4.0: this work includes material from the System "
    "Reference Document 5.2.1 by Wizards of the Coast LLC, available at https://www.dndbeyond.com/srd.\n")

total = sum(os.path.getsize(os.path.join(dp, f)) for dp, _, fs in os.walk(OUT) for f in fs)
print('classes:', len(rows), '| new spell files:', len(neon), '| subclass docs:', len(subs), '| bytes:', total)
for r in rows: print('  ', r)
print('NOTES (%d):' % len(notes))
for n in notes: print('  -', n)
