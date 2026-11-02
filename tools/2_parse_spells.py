import re, json, collections

RAW = open('./spells_raw.txt', encoding='utf-8').read()

# ---- 1. strip page furniture -------------------------------------------------
lines = RAW.replace('\f', '\n').split('\n')
clean = []
for ln in lines:
    s = ln.strip()
    if re.fullmatch(r'\d{3}', s):            # printed page number
        continue
    if s == 'System Reference Document 5.2.1':
        continue
    if s == 'Spell Descriptions':
        continue
    clean.append(s)

# collapse multiple blank lines
out = []
for s in clean:
    if s == '' and out and out[-1] == '':
        continue
    out.append(s)
lines = out

# ---- 2. locate spell headers ------------------------------------------------
HDR_START = re.compile(r'^(Level \d (Abjuration|Conjuration|Divination|Enchantment|Evocation|Illusion|Necromancy|Transmutation)|'
                       r'(Abjuration|Conjuration|Divination|Enchantment|Evocation|Illusion|Necromancy|Transmutation) Cantrip) \(')

spells = []
i = 0
starts = []
while i < len(lines):
    if HDR_START.match(lines[i]):
        # header may wrap until ')'
        j = i
        hdr = lines[j]
        while ')' not in hdr and j + 1 < len(lines):
            j += 1
            hdr += ' ' + lines[j]
        # name = nearest previous non-blank line
        k = i - 1
        while k >= 0 and lines[k] == '':
            k -= 1
        starts.append((k, i, j, lines[k], hdr))
        i = j + 1
    else:
        i += 1

# ---- 3. build vocabulary for de-hyphenation --------------------------------
vocab = collections.Counter()
for ln in lines:
    for w in re.findall(r"[A-Za-z]+", ln):
        vocab[w.lower()] += 1
# words that only ever appear split can't be validated; fine.

def dehyphenate(text_lines):
    """Join wrapped lines; remove hyphen if joined word is known, keep otherwise."""
    res = ''
    for ln in text_lines:
        if res.endswith('-') and ln and ln[0].islower():
            m_prev = re.search(r"([A-Za-z]+)-$", res)
            m_next = re.match(r"([A-Za-z]+)", ln)
            if m_prev and m_next:
                joined = (m_prev.group(1) + m_next.group(1)).lower()
                if vocab.get(joined, 0) >= 2:
                    res = res[:-1] + ln
                    continue
            res += ln          # keep real hyphen
        else:
            res += (' ' if res else '') + ln
    return res

FIELDS = ['Casting Time', 'Range', 'Components', 'Duration']

for idx, (k, i, j, name, hdr) in enumerate(starts):
    end = starts[idx + 1][0] if idx + 1 < len(starts) else len(lines)
    block = lines[j + 1:end]
    # header meta
    m = re.match(r'^Level (\d) (\w+) \((.*)\)$', hdr)
    if m:
        level, school, classes = int(m.group(1)), m.group(2), m.group(3)
    else:
        m = re.match(r'^(\w+) Cantrip \((.*)\)$', hdr)
        level, school, classes = 0, m.group(1), m.group(2)
    classes = [c.strip() for c in classes.split(',')]

    # skip leading blanks, then read field block up to first blank after Duration
    p = 0
    while p < len(block) and block[p] == '':
        p += 1
    fld_lines = []
    seen_duration = False
    while p < len(block):
        if block[p] == '':
            if seen_duration:
                break
            p += 1
            continue
        if block[p].startswith('Duration:'):
            seen_duration = True
        fld_lines.append(block[p])
        p += 1
    ftxt = dehyphenate(fld_lines).replace('\xad', '')
    fields = {}
    marks = list(re.finditer(r'(Casting Time|Range|Components?|Duration):', ftxt))
    for n, mk in enumerate(marks):
        e = marks[n + 1].start() if n + 1 < len(marks) else len(ftxt)
        key = 'Components' if mk.group(1).startswith('Component') else mk.group(1)
        fields[key] = ftxt[mk.end():e].strip()

    body_lines = [b for b in block[p:]]
    # paragraphs: blank lines split paragraphs in the raw text
    paras, cur = [], []
    for b in body_lines:
        if b == '':
            if cur:
                paras.append(cur)
                cur = []
        else:
            cur.append(b)
    if cur:
        paras.append(cur)
    # each "paragraph" here is a visual chunk; keep raw lines per chunk for later
    RUNIN = re.compile(r"^[A-Z][A-Za-z’'\-/ ]{2,45}\. [A-Z(]")
    flat = [b for b in body_lines if b != '']
    paras2, cur2 = [], []
    for b in flat:
        starts_new = bool(cur2) and (b.startswith('•') or
                     (RUNIN.match(b) and re.search(r'[.:!?)”"]$', cur2[-1])))
        if starts_new:
            paras2.append(cur2)
            cur2 = []
        cur2.append(b)
    if cur2:
        paras2.append(cur2)
    body = '\n'.join(dehyphenate(p) for p in paras2).replace('\xad', '')

    higher = cantrip = None
    mh = re.search(r'Using a Higher-Level Spell Slot\. ', body)
    if mh:
        higher = body[mh.end():].strip()
        body = body[:mh.start()].strip()
    mc = re.search(r'Cantrip Upgrade\. ', body)
    if mc:
        cantrip = body[mc.end():].strip()
        body = body[:mc.start()].strip()

    ct = fields.get('Casting Time', '')
    dur = fields.get('Duration', '')
    spells.append(dict(
        name=name, level=level, school=school, classes=classes,
        casting_time=ct, range=fields.get('Range', ''),
        components=fields.get('Components', ''), duration=dur,
        concentration=dur.lower().startswith('concentration'),
        ritual=bool(re.search(r'\bRitual\b', ct)),
        description=body, higher_level=higher, cantrip_upgrade=cantrip))

json.dump(spells, open('./spells.json', 'w', encoding='utf-8'), ensure_ascii=False, indent=1)

print('spells parsed:', len(spells))
bylevel = collections.Counter(s['level'] for s in spells)
print('by level:', dict(sorted(bylevel.items())))
byclass = collections.Counter(c for s in spells for c in s['classes'])
print('by class:', dict(sorted(byclass.items())))
missing = [s['name'] for s in spells if not (s['casting_time'] and s['range'] and s['components'] and s['duration'])]
print('missing fields:', missing)
dup = [n for n, c in collections.Counter(s['name'] for s in spells).items() if c > 1]
print('duplicate names:', dup)
lens = [len(s['description']) + len(s['higher_level'] or '') + len(s['cantrip_upgrade'] or '') for s in spells]
print('text chars total:', sum(lens), 'max:', max(lens), 'avg:', sum(lens) // len(lens))
na = collections.Counter(ch for s in spells for ch in json.dumps(s, ensure_ascii=False) if ord(ch) > 127)
print('non-ascii chars:', {c: n for c, n in na.most_common()})
