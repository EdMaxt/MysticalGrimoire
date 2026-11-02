"""Parse subclasses (name, tagline, intro, granted-spell tables) from
the Outrunner's Handbook and the Overdrive Expansion.

Usage: python3 parse_neon_subclasses.py handbook.pdf overdrive.pdf neon_subclasses.json
"""
import re, json, sys, collections, subprocess
import pdfplumber

HB, OD, OUT = (sys.argv + [None] * 4)[1:4]
HB = HB or 'handbook.pdf'; OD = OD or 'overdrive.pdf'; OUT = OUT or 'neon_subclasses.json'

def dd(t):
    """Headings are printed twice with an offset ('CCRRYYPPTTIISSTT'): collapse."""
    t = t.strip()
    if len(t) % 2 == 0 and len(t) > 2 and all(t[i] == t[i + 1] for i in range(0, len(t), 2)):
        return t[::2]
    return t

def title(s):
    return '-'.join(w.capitalize() for w in s.split('-')).replace("’S", "’s")

def big_lines(page, minsize):
    lines = collections.defaultdict(list)
    for c in page.chars:
        if c['size'] >= minsize:
            lines[(round(c['top'] / 3), c['x0'] > 296)].append(c)
    out = []
    for (k, right), cs in sorted(lines.items(), key=lambda kv: (kv[0][1], kv[0][0])):
        cs = sorted(cs, key=lambda c: c['x0'])
        out.append(dict(text=dd(''.join(c['text'] for c in cs)), right=right,
                        top=min(c['top'] for c in cs), bottom=max(c['bottom'] for c in cs),
                        size=max(c['size'] for c in cs), x0=cs[0]['x0']))
    return out

# ---- 1. find subclass headings ------------------------------------------------
CLASSES = {'Cryptist', 'Disciple', 'Enforcer', 'Esper', 'Gridrunner', 'Icon', 'Juggernaut',
           'Machinist', 'Oracle', 'Primalist', 'Scoundrel', 'Soldier', 'Tracer'}
subs = []   # dicts: source, cls, name, doc, page, right, top, bottom

pdf_hb = pdfplumber.open(HB)
cls = None; in_sub = False
for pn in range(17, 127):
    page = pdf_hb.pages[pn - 1]
    # the huge class title is split into several fragments: join every glyph >= 50pt
    big = sorted([c for c in page.chars if c['size'] > 50], key=lambda c: c['x0'])
    if big:
        nm = title(dd(''.join(c['text'] for c in big)))
        if nm in CLASSES:
            cls = nm; in_sub = False
    for L in big_lines(page, 15):
        t = L['text']
        if L['size'] > 50:
            continue
        if t.endswith(' SUBCLASSES'):
            in_sub = True
        elif in_sub and L['size'] < 16 and 'SPELL LIST' not in t and 'FEATURES' not in t and 'BECOMING' not in t:
            subs.append(dict(source="Outrunner's Handbook", cls=cls, name=title(t), doc='hb',
                             page=pn, right=L['right'], top=L['top'], bottom=L['bottom']))

pdf_od = pdfplumber.open(OD)
for pn in range(4, 41):
    for L in big_lines(pdf_od.pages[pn - 1], 14.5):
        m = re.fullmatch(r'([A-Z]+): ([A-Z][A-Z\-]+)', L['text'])
        if m:
            subs.append(dict(source='Overdrive Expansion', cls=title(m.group(1)), name=title(m.group(2)),
                             doc='od', page=pn, right=L['right'], top=L['top'], bottom=L['bottom']))

# ---- 2. tagline + intro (pdftotext of the column region below the heading) ------
def region_lines(pdfpath, pn, right, ytop):
    x = 298 if right else 38
    t = subprocess.run(['pdftotext', '-f', str(pn), '-l', str(pn), '-x', str(x), '-y', str(int(ytop)),
                        '-W', '252', '-H', str(int(735 - ytop)), pdfpath, '-'],
                       capture_output=True, text=True).stdout
    return [l.strip() for l in t.replace('\f', '').split('\n') if l.strip()]

def join(lines):
    out = ''
    for ln in lines:
        if out.endswith('-') and ln[:1].islower():
            out += ln
        else:
            out += (' ' if out else '') + ln
    return out

for s in subs:
    path = HB if s['doc'] == 'hb' else OD
    L = region_lines(path, s['page'], s['right'], s['bottom'] + 1)
    # drop a repeated heading line if present
    intro = []
    for ln in L:
        if re.match(r'^Level \d+', ln) or ln.startswith('Prerequisite'):
            break
        intro.append(ln)
    # tagline = first line (or two) before the descriptive paragraph (paragraph lines end in sentences)
    tag = []
    for ln in intro[:3]:
        tag.append(ln)
        if len(join(tag)) > 30 and not intro[len(tag):len(tag) + 1]:
            break
        if not ln.endswith(('.', ',')) and len(tag) >= 1 and len(ln) < 60 and not re.search(r'[.]$', ln):
            # tagline lines do not end with a period; stop at the first line followed by a sentence
            nxt = intro[len(tag)] if len(tag) < len(intro) else ''
            if nxt and nxt[:1].isupper() and not nxt.startswith(('of ', 'the ')) and len(join(tag)) > 25:
                break
    s['tagline'] = join(tag)
    s['intro'] = join(intro[len(tag):])[:900]

# ---- 3. granted-spell tables (cells are filled rectangles) ----------------------
def clusters(rects):
    rs = [r for r in rects if r['x1'] - r['x0'] > 20 and r['bottom'] - r['top'] > 8 and r['x0'] > 20 and r['top'] < 725]
    seen = [False] * len(rs); out = []
    def touch(a, b):
        return not (a['x1'] < b['x0'] - 1.5 or b['x1'] < a['x0'] - 1.5 or a['bottom'] < b['top'] - 1.5 or b['bottom'] < a['top'] - 1.5)
    for i in range(len(rs)):
        if seen[i]: continue
        comp, stack = [], [i]; seen[i] = True
        while stack:
            k = stack.pop(); comp.append(rs[k])
            for j in range(len(rs)):
                if not seen[j] and touch(rs[k], rs[j]):
                    seen[j] = True; stack.append(j)
        out.append(comp)
    return out

def line_sort(ws):
    """Reading order: group words whose tops are within 4pt into one visual line."""
    ws = sorted(ws, key=lambda w: w['top'])
    lines, cur, base = [], [], None
    for w in ws:
        if base is None or w['top'] - base > 4:
            if cur: lines.append(cur)
            cur, base = [w], w['top']
        else:
            cur.append(w)
    if cur: lines.append(cur)
    return [w for ln in lines for w in sorted(ln, key=lambda w: w['x0'])]

def words_in(page, box):
    x0, t, x1, b = box
    return [w for w in page.extract_words() if x0 <= (w['x0'] + w['x1']) / 2 <= x1 and t <= (w['top'] + w['bottom']) / 2 <= b]

def cell_text(page, r):
    ws = words_in(page, (r['x0'], r['top'], r['x1'], r['bottom']))
    ws = line_sort(ws)
    return re.sub(r'\s+', ' ', ' '.join(w['text'] for w in ws)).replace('- ', '-').strip()

tables = []   # dicts: doc, page, caption, header, rows
for doc, pdfobj, rng in (('hb', pdf_hb, range(17, 127)), ('od', pdf_od, range(4, 41))):
    for pn in rng:
        page = pdfobj.pages[pn - 1]
        for comp in clusters(page.rects):
            xs = sorted({round(r['x0']) for r in comp});
            x0 = min(r['x0'] for r in comp); x1 = max(r['x1'] for r in comp)
            top = min(r['top'] for r in comp); bot = max(r['bottom'] for r in comp)
            # header text above the table
            hdr_ws = words_in(page, (x0 - 90, top - 34, x1 + 2, top - 1))
            hdr_txt = ' '.join(w['text'] for w in line_sort(hdr_ws)).lower()
            if 'spells' not in hdr_txt and 'prepared' not in hdr_txt:
                continue
            # rows
            rows_by_top = collections.defaultdict(list)
            for r in comp:
                rows_by_top[round(r['top'] / 2)].append(r)
            rows = []
            for k in sorted(rows_by_top):
                rs = sorted(rows_by_top[k], key=lambda r: r['x0'])
                rt, rb = rs[0]['top'], rs[0]['bottom']
                lv_ws = words_in(page, (x0 - 90, rt - 1, x0 - 0.5, rb + 1))
                lv = None
                for w in lv_ws:
                    m = re.match(r'^(\d+)', w['text'])
                    if m: lv = int(m.group(1))
                cells = []
                for r in rs:
                    # header of this column
                    hw = words_in(page, (r['x0'], top - 34, r['x1'], top - 1))
                    h = ' '.join(w['text'] for w in line_sort(hw))
                    h = re.sub(r'(?i)\s*spells?\s*$', '', h).strip()
                    h = h.title() if h else ''
                    cells.append((h, cell_text(page, r)))
                rows.append(dict(level=lv, cells=cells))
            # caption: words in the ~60pt band above the table, looking for "<Name> Spells"
            cap_ws = words_in(page, (x0 - 90, top - 60, x1 + 2, top - 1))
            cap = ' '.join(w['text'] for w in line_sort(cap_ws)).lower()
            tables.append(dict(doc=doc, page=pn, caption=cap, rows=rows, x0=x0, top=top))

def spell_table_for(s):
    nm = s['name'].lower()
    cand = [t for t in tables if t['doc'] == s['doc'] and nm in t['caption'] and s['cls'].lower() in t['caption'] or
            (t['doc'] == s['doc'] and nm + ' spells' in t['caption'])]
    cand = [t for t in cand if t['page'] >= s['page']]
    cand.sort(key=lambda t: t['page'])
    return cand[0] if cand else None

for s in subs:
    t = spell_table_for(s)
    s['spell_rows'] = t['rows'] if t else []

json.dump(subs, open(OUT, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)

print('subclasses:', len(subs), collections.Counter(s['source'] for s in subs))
for s in subs:
    n = len(s['spell_rows'])
    print('%-11s %-14s %-3s p%-3d tag=%-52s spells-rows=%d' % (s['cls'], s['name'], s['doc'], s['page'], s['tagline'][:50], n))
