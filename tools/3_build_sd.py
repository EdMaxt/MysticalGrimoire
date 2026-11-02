import json, os, re, shutil, collections, sys

# Usage (from the project root): python3 tools/3_build_sd.py [spells.json] [SD_CARD]
SRC = sys.argv[1] if len(sys.argv) > 1 else ('./spells.json' if os.path.exists('./spells.json') else 'data/spells.json')
OUT = sys.argv[2] if len(sys.argv) > 2 else './SD_CARD'
PHB = os.path.join(os.path.dirname(os.path.abspath(SRC)), 'phb_only.json')   # optional, see 5_phb_crosscheck notes in README
spells = json.load(open(SRC, encoding='utf-8'))
spells.sort(key=lambda s: s['name'].lower())

if os.path.exists(OUT + '/grimoire'):          # leave /neon alone
    shutil.rmtree(OUT + '/grimoire')
os.makedirs(OUT + '/grimoire/spells')
os.makedirs(OUT + '/grimoire/index')

def flags(s):
    return ('C' if s['concentration'] else '') + ('R' if s['ritual'] else '') + ('P' if s.get('phb_only') else '')

def one_line(t):
    return ' '.join(t.split())

for n, s in enumerate(spells, 1):
    s['id'] = '%03d' % n
    lines = [
        'name=' + s['name'],
        'level=%d' % s['level'],
        'school=' + s['school'],
        'classes=' + ', '.join(s['classes']),
        'time=' + one_line(s['casting_time']),
        'range=' + one_line(s['range']),
        'comp=' + one_line(s['components']),
        'dur=' + one_line(s['duration']),
        'flags=' + flags(s),
        '---',
        s['description'],
    ]
    if s['higher_level']:
        lines += ['@HIGHER', s['higher_level']]
    if s['cantrip_upgrade']:
        lines += ['@CANTRIP', s['cantrip_upgrade']]
    with open('%s/grimoire/spells/%s.txt' % (OUT, s['id']), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines) + '\n')

# ---- Player's Handbook 2024 spells that are NOT in the SRD: metadata only, text left out (copyright) ----
STUB = ("This spell is in the Player's Handbook (2024) but not in the free SRD 5.2.1, so its text is not "
        "included on this card. Look it up in your book. Name, level, school and class lists are shown "
        "so your class spell lists are complete.")
phb_only = json.load(open(PHB, encoding='utf-8')) if os.path.exists(PHB) else []
for k, o in enumerate(phb_only, len(spells) + 1):
    o.update(id='%03d' % k, phb_only=True)
    with open('%s/grimoire/spells/%s.txt' % (OUT, o['id']), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(['name=' + o['name'], 'level=%d' % o['level'], 'school=' + o['school'],
                           'classes=' + ', '.join(o['classes']), 'time=', 'range=', 'comp=', 'dur=',
                           'flags=' + flags(o), '---', STUB]) + '\n')
spells = spells + phb_only

def idx_line(s):
    return '|'.join([s['id'], str(s['level']), s['name'], s['school'], flags(s)])

bycls = collections.defaultdict(list)
for s in sorted(spells, key=lambda s: s['name'].lower()):
    for c in s['classes']:
        bycls[c].append(s)

summary = []
for c, lst in sorted(bycls.items()):
    lst.sort(key=lambda s: (s['level'], s['name'].lower()))
    with open('%s/grimoire/index/%s.idx' % (OUT, c.lower()), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(idx_line(s) for s in lst) + '\n')
    summary.append('%s|%d|%d' % (c, len(lst), max(s['level'] for s in lst)))

with open(OUT + '/grimoire/index/all.idx', 'w', encoding='utf-8', newline='\n') as f:
    f.write('\n'.join(idx_line(s) for s in spells) + '\n')
# ---- SRD subclasses (granted spells only), class-granted always-prepared spells, progression tables ----
SUBSRC = os.path.join(os.path.dirname(os.path.abspath(SRC)), 'srd_subclasses.json')
norm = lambda x: re.sub(r'[^a-z]', '', x.lower())
byname = {norm(s['name']): s['id'] for s in spells}
subdata = json.load(open(SUBSRC, encoding='utf-8')) if os.path.exists(SUBSRC) else {'always': [], 'subclasses': []}
os.makedirs(OUT + '/grimoire/sub', exist_ok=True)
subcount = collections.Counter()
per_class = collections.defaultdict(list)
for n, sc in enumerate(subdata['subclasses'], 1):
    sid = '%03d' % n
    lines = ['name=' + sc['name'], 'class=' + sc['cls'], 'source=SRD 5.2.1', 'tagline=', '---',
             'SRD 5.2.1 subclass. This card lists only the spells it always prepares; read its features in the SRD or your Player\'s Handbook.']
    if sc['grants']:
        lines.append('@SPELLS')
        grant = []
        for lv, grp, names in sc['grants']:
            lines.append('Lv %d%s: %s' % (lv, (' ' + grp) if grp else '', names))
            ids = [byname[norm(x)] for x in re.split(r',\s*', names)]     # KeyError = unknown spell name
            grant.append('%d|%s|%s' % (lv, grp, ','.join(ids)))
        lines += ['@GRANT'] + grant
    open('%s/grimoire/sub/%s.txt' % (OUT, sid), 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
    per_class[sc['cls']].append('|'.join([sid, sc['name'], 'SRD 5.2.1', 'S' if sc['grants'] else '']))
for c_, rows_ in per_class.items():
    open('%s/grimoire/sub/%s.idx' % (OUT, c_.lower()), 'w', encoding='utf-8', newline='\n').write('\n'.join(rows_) + '\n')
    subcount[c_] = len(rows_)
al = ['%s|%d|%s' % (c_, lv_, byname[norm(nm_)]) for c_, lv_, nm_ in subdata['always']]
open(OUT + '/grimoire/index/always.txt', 'w', encoding='utf-8', newline='\n').write('# class|level|spell id (always prepared, free of the prepared limit)\n' + '\n'.join(al) + '\n')
PROG = os.path.join(os.path.dirname(os.path.abspath(SRC)), 'progress_srd.txt')
if os.path.exists(PROG): shutil.copy(PROG, OUT + '/grimoire/index/progress.txt')
summary = [x + '|%d|' % subcount[x.split('|')[0]] for x in summary]      # name|count|maxLevel|subclassCount|basedOn(empty)
with open(OUT + '/grimoire/index/classes.txt', 'w', encoding='utf-8', newline='\n') as f:
    f.write('# class|spell_count|max_spell_level|subclass_count|based_on\n' + '\n'.join(summary) + '\n')

attrib = (
    "This work includes material from the System Reference Document 5.2.1 (\"SRD 5.2.1\") by Wizards of the Coast LLC, "
    "available at https://www.dndbeyond.com/srd. The SRD 5.2.1 is licensed under the Creative Commons Attribution 4.0 "
    "International License, available at https://creativecommons.org/licenses/by/4.0/legalcode.\n")
open(OUT + '/grimoire/ATTRIBUTION.txt', 'w', encoding='utf-8', newline='\n').write(attrib)

total = sum(os.path.getsize(os.path.join(dp, f)) for dp, _, fs in os.walk(OUT) for f in fs)
nfiles = sum(len(fs) for _, _, fs in os.walk(OUT))
print('spells:', len(spells), 'files:', nfiles, 'bytes:', total)
print(open(OUT + '/grimoire/index/classes.txt').read())
