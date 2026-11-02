"""Parse the 95 new spells of the Outrunner's Handbook (pages 163-182).

Input : nspells_raw.txt  (pdftotext, one column at a time)
Output: neon_spells.json
"""
import re, json, sys, collections

RAW = open(sys.argv[1] if len(sys.argv) > 1 else 'nspells_raw.txt', encoding='utf-8').read()
OUT = sys.argv[2] if len(sys.argv) > 2 else 'neon_spells.json'

SCH = 'Abjuration|Conjuration|Divination|Enchantment|Evocation|Illusion|Necromancy|Transmutation'
HDR_START = re.compile(r'^(Level \d (%s)|(%s) Cantrip) \(' % (SCH, SCH))

lines = [l.strip() for l in RAW.replace('\f', '\n').split('\n')]
# drop the preamble (Hypertech / Overtone notes) and the vertical page-tab letters
try:
    start = next(i for i, l in enumerate(lines) if l == 'SPELL DESCRIPTIONS') + 2
except StopIteration:
    start = 0
lines = [l for l in lines[start:] if l not in ('SPE', 'LLS')]
out = []
for s in lines:
    if s == '' and out and out[-1] == '':
        continue
    out.append(s)
lines = out

# ---- locate headers ---------------------------------------------------------
starts = []
i = 0
while i < len(lines):
    if HDR_START.match(lines[i]):
        j, hdr = i, lines[i]
        while ')' not in hdr and j + 1 < len(lines):
            j += 1
            hdr += ' ' + lines[j]
        k = i - 1
        while k >= 0 and lines[k] == '':
            k -= 1
        starts.append((k, i, j, lines[k], hdr))
        i = j + 1
    else:
        i += 1

vocab = collections.Counter(w.lower() for ln in lines for w in re.findall(r"[A-Za-z]+", ln))

def dehyphenate(text_lines):
    res = ''
    for ln in text_lines:
        if res.endswith('-') and ln and ln[0].islower():
            mp = re.search(r"([A-Za-z]+)-$", res)
            mn = re.match(r"([A-Za-z]+)", ln)
            if mp and mn and vocab.get((mp.group(1) + mn.group(1)).lower(), 0) >= 2:
                res = res[:-1] + ln
                continue
            res += ln
        else:
            res += (' ' if res else '') + ln
    return res

FIELD_RE = re.compile(r'(Casting Time|Range|Components?|Duration):')
RUNIN = re.compile(r"^[A-Z][A-Za-z’'\-/ ]{2,45}\. [A-Z(]")

spells = []
for idx, (k, i, j, name, hdr) in enumerate(starts):
    end = starts[idx + 1][0] if idx + 1 < len(starts) else len(lines)
    block = lines[j + 1:end]
    m = re.match(r'^Level (\d) (\w+) \((.*)\)$', hdr)
    if m:
        level, school, classes = int(m.group(1)), m.group(2), m.group(3)
    else:
        m = re.match(r'^(\w+) Cantrip \((.*)\)$', hdr)
        level, school, classes = 0, m.group(1), m.group(2)
    classes = [c.strip() for c in classes.split(',')]

    # field block: from the first line to the line holding "Duration:" (+ wrapped tail)
    p = 0
    while p < len(block) and block[p] == '':
        p += 1
    fld = []
    while p < len(block):
        ln = block[p]
        if ln == '':
            if any(x.startswith('Duration:') for x in fld):
                break
            p += 1
            continue
        fld.append(ln)
        p += 1
        if ln.startswith('Duration:'):
            # wrapped duration tail (rare)
            while p < len(block) and block[p] and block[p][0].islower() and len(fld[-1]) > 40:
                fld.append(block[p]); p += 1
            break
    ftxt = dehyphenate(fld).replace('\xad', '')
    fields = {}
    marks = list(FIELD_RE.finditer(ftxt))
    for n, mk in enumerate(marks):
        e = marks[n + 1].start() if n + 1 < len(marks) else len(ftxt)
        key = 'Components' if mk.group(1).startswith('Component') else mk.group(1)
        fields[key] = ftxt[mk.end():e].strip()

    flat = [b for b in block[p:] if b != '']
    paras, cur = [], []
    for b in flat:
        if cur and (b.startswith('•') or (RUNIN.match(b) and re.search(r'[.:!?)”"]$', cur[-1]))):
            paras.append(cur); cur = []
        cur.append(b)
    if cur:
        paras.append(cur)
    body = '\n'.join(dehyphenate(pp) for pp in paras).replace('\xad', '')

    higher = cantrip = None
    mh = re.search(r'Using a Higher-Level Spell Slot\. ', body)
    if mh:
        higher = body[mh.end():].strip()
        body = body[:mh.start()].strip()
    mc = re.search(r'Cantrip Upgrade\. ', body)
    if mc:
        cantrip = body[mc.end():].strip()
        body = body[:mc.start()].strip()

    ct, dur = fields.get('Casting Time', ''), fields.get('Duration', '')
    spells.append(dict(
        name=name, level=level, school=school, classes=classes,
        casting_time=ct, range=fields.get('Range', ''),
        components=fields.get('Components', ''), duration=dur,
        concentration=dur.lower().startswith('concentration'),
        ritual=bool(re.search(r'\bRitual\b', ct)),
        description=body, higher_level=higher, cantrip_upgrade=cantrip,
        source='Outrunner\'s Handbook'))

json.dump(spells, open(OUT, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)

print('spells parsed:', len(spells))
print('by level:', dict(sorted(collections.Counter(s['level'] for s in spells).items())))
print('by class:', dict(sorted(collections.Counter(c for s in spells for c in s['classes']).items())))
bad = [s['name'] for s in spells if not (s['casting_time'] and s['range'] and s['components'] and s['duration'])]
print('missing fields:', bad)
print('suspicious names:', [s['name'] for s in spells if not re.fullmatch(r"[A-Z][A-Za-z'’\- /]+", s['name'])])
print('duplicates:', [n for n, c in collections.Counter(s['name'] for s in spells).items() if c > 1])
lens = [len(s['description']) + len(s['higher_level'] or '') + len(s['cantrip_upgrade'] or '') for s in spells]
print('chars total:', sum(lens), 'max:', max(lens), [s['name'] for s in spells if len(s['description']) > 3000])
print('non-ascii:', dict(collections.Counter(ch for s in spells for ch in json.dumps(s, ensure_ascii=False) if ord(ch) > 127).most_common()))
