#!/usr/bin/env python3
"""Fills in the text of the 52 Player's Handbook 2024 spells that are not in the free SRD,
reading it from YOUR OWN copy of the Spanish PHB (Manual del Jugador) PDF.

    python import_phb_es.py <Manual_del_Jugador.pdf> <card folder> [first_page last_page]

Recommended (keeps the copyrighted text out of the repository's SD_CARD):
    xcopy /E /I SD_CARD SD_PERSONAL            (Linux/macOS: cp -r SD_CARD SD_PERSONAL)
    python tools\\import_phb_es.py "C:\\Users\\you\\Downloads\\Manual_del_Jugador.pdf" SD_PERSONAL
    ...and copy SD_PERSONAL\\grimoire (+ neon) to the microSD instead of SD_CARD.

* The text is copied as-is (Spanish). It is NOT translated and nothing is uploaded anywhere.
* It is written only to <SD_CARD>/grimoire/spells/340..391.txt, i.e. to your own card.
  Do not commit or share the resulting files: the PHB text is (c) Wizards of the Coast.
  (.gitignore already excludes SD_PERSONAL/.)
* Names/level/school/class lists stay as they are; the "P" (dimmed) flag of those spells is removed
  from the class indexes once their text is loaded.
* Needs either `pdftotext` (poppler) on the PATH, or `pip install pdfplumber`.
  Default page range is the spell descriptions of the Spanish edition: 241-345.
* Run it again at any time; run with --restore to put the "text not included" stubs back.
"""
import sys, os, re, glob, shutil, subprocess, unicodedata, difflib

EN = {
'AMISTAD':'Friends','ARMA ELEMENTAL':'Elemental Weapon','ARMADURA DE AGATHYS':'Armor of Agathys','AURA DE PUREZA':'Aura of Purity',
'AURA DE VITALIDAD':'Aura of Vitality','BRAZOS DE HADAR':'Arms of Hadar','CALDERO BURBUJEANTE DE TASHA':"Tasha's Bubbling Cauldron",
'CARCAJ VELOZ':'Swift Quiver','CASTIGO ABRUMADOR':'Staggering Smite','CASTIGO ATRONADOR':'Thunderous Smite','CASTIGO CEGADOR':'Blinding Smite',
'CASTIGO DESTERRADOR':'Banishing Smite','CASTIGO FURIOSO':'Wrathful Smite','CIRCULO DE PODER':'Circle of Power',
'CONJURAR DESCARGA DE PROYECTILES':'Conjure Barrage','CONJURAR LLUVIA DE FLECHAS':'Conjure Volley','CORDON DE FLECHAS':'Cordon of Arrows',
'CORONA DE LA LOCURA':'Crown of Madness','DUELO FORZADO':'Compelled Duel','ENREDADERA':'Grasping Vine','ESTATICA SINAPTICA':'Synaptic Static',
'FINGIR MUERTE':'Feign Death','FLECHA DE RELAMPAGO':'Lightning Arrow','FRAGMENTO MENTAL':'Mind Sliver','FUENTE DE LUZ LUNAR':'Fount of Moonlight',
'GOLPE DE VIENTO ACERADO':'Steel Wind Strike','GUARDIA DE CUCHILLAS':'Blade Ward','HAMBRE DE HADAR':'Hunger of Hadar',
'INVOCAR ABERRACION':'Summon Aberration','INVOCAR AUTOMATA':'Summon Construct','INVOCAR BESTIA':'Summon Beast','INVOCAR CELESTIAL':'Summon Celestial',
'INVOCAR ELEMENTAL':'Summon Elemental','INVOCAR FEERICO':'Summon Fey','INVOCAR INFERNAL':'Summon Fiend','INVOCAR MUERTO VIVIENTE':'Summon Undead',
'LATIGO DE ESPINAS':'Thorn Whip','MANTO DEL CRUZADO':"Crusader's Mantle",'NUBE DE DAGAS':'Cloud of Daggers','OLA DESTRUCTORA':'Destructive Wave',
'PALABRA DE PODER: FORTALECER':'Power Word Fortify','PALABRA DE RESPLANDOR':'Word of Radiance','PRESENCIA REGIA DE YOLANDE':"Yolande's Regal Presence",
'PUERTA ARCANA':'Arcane Gate','RAYO DE HECHICERIA':'Witch Bolt','SENTIDOS DE LA BESTIA':'Beast Sense','TANIDO POR LOS MUERTOS':'Toll the Dead',
'TELEPATIA':'Telepathy','TORMENTA DE ESPINAS':'Hail of Thorns','TORMENTA RESPLANDECIENTE DE JALLARZI':"Jallarzi's Storm of Radiance",
'TRONAR':'Thunderclap','VIGOR ARCANO':'Arcane Vigor'}

def norm(s):
    s = unicodedata.normalize('NFD', s)
    s = ''.join(c for c in s if unicodedata.category(c) != 'Mn')
    return re.sub(r'\s+', ' ', re.sub(r'^[^A-Za-z]+', '', s)).strip().upper()

def extract(pdf, a, b, mode='-raw'):
    if shutil.which('pdftotext'):
        r = subprocess.run(['pdftotext', '-f', str(a), '-l', str(b), mode, pdf, '-'], capture_output=True)
        if r.returncode == 0:
            return r.stdout.decode('utf-8', 'replace')
    try:
        import pdfplumber
    except ImportError:
        sys.exit('Need pdftotext (poppler) on the PATH, or: pip install pdfplumber')
    out = []
    with pdfplumber.open(pdf) as doc:
        for p in range(a - 1, min(b, len(doc.pages))):
            out.append(doc.pages[p].extract_text() or '')
    return '\n'.join(out)

SCHOOL_START = r'(?:\b(?:Abjuraci|Conjuraci|Adivinaci|Encantamiento|Evocaci|[HI]lusionismo|Nigromancia|Transmutaci)\w* de nivel|\bTruco de (?:abjuraci|conjuraci|adivinaci|encantamiento|evocaci|[hi]lusionismo|nigromancia|transmutaci)\w*)'

def parse(text):
    """-> list of (spanish name, block text starting at the school/level header line)"""
    L = text.replace('\f', '\n').split('\n')
    S0 = re.compile(SCHOOL_START, re.I)
    starts = []
    for i, l in enumerate(L):
        m = S0.search(l)
        if m and any(re.search(r'lanzam', L[k], re.I) for k in range(i, min(i + 7, len(L)))):
            starts.append((i, m.start()))
    out = []
    for n, (i, off) in enumerate(starts):
        if L[i][:off].strip():
            name = L[i][:off].strip()
        else:
            k = i - 1
            while k >= 0 and not L[k].strip():
                k -= 1
            name = L[k].strip()
        if 'ROCIADA VENENOSA' in name:
            name = 'ROCIADA VENENOSA'
        end = starts[n + 1][0] - 1 if n + 1 < len(starts) else len(L)
        # the name of the next spell sits on the line just before its header: cut it
        blk = [L[i][off:]] + L[i + 1:end]
        out.append((name, blk))
    return out

def field(blk, label, nxt):
    t = '\n'.join(blk)
    m = re.search(label + r'\w*:\s*(.*?)\s*(?=\n(?:' + nxt + r')\w*:|$)', t, re.S | re.I)
    return re.sub(r'\s+', ' ', m.group(1)).strip() if m else ''

NOISE = re.compile(r'^(?:\d{1,3}|DESCRIPCIONES DE CONJUROS|CAP[IÍ]TULO \d+.*|[A-ZÁÉÍÓÚÑ ]{3,}\s*\d{0,3})$')

def body_of(blk):
    """lines after the Duración field, joined into paragraphs"""
    i = 0
    for k, l in enumerate(blk):
        if re.match(r'\s*Duraci', l, re.I):
            i = k + 1
            break
    lines = [l.strip() for l in blk[i:] if l.strip()]
    # drop page numbers / running heads / the next spell's title when it leaked in
    while lines and NOISE.match(lines[-1]) and lines[-1] == lines[-1].upper():
        lines.pop()
    lines = [l for l in lines if not re.fullmatch(r'\d{1,3}', l) and l != 'DESCRIPCIONES DE CONJUROS']
    if not lines:
        return ''
    width = max(len(l) for l in lines)
    paras, cur = [], ''
    for l in lines:
        newp = bool(cur) and (cur.endswith(('.', ':', '.)', '!', '?')) and prev_len < 0.75 * width)
        if cur and re.match(r'^[A-ZÁÉÍÓÚÑ][\w ]{2,40}\. ', l) and cur.endswith(('.', ':')):
            newp = True                                    # "Alarma mental. ..." style run-in headings
        if cur.endswith('-') and not cur.endswith(' -'):
            cur = cur[:-1] + l                              # hyphenated line break
        elif newp:
            paras.append(cur); cur = l
        else:
            cur = (cur + ' ' + l).strip()
        prev_len = len(l)
    paras.append(cur)
    return '\n'.join(re.sub(r'\s+', ' ', p) for p in paras)

TITLES = set()

def garbled(blk, text):
    """two-column pages sometimes interleave two spells; never write that to the card"""
    if re.search(r'Tiempo de lanzam|Componentes:|Alcance:|Duraci\w*:', text):
        return True
    nt = ''.join(c for c in unicodedata.normalize('NFD', text) if unicodedata.category(c) != 'Mn')   # case kept
    if any(t in nt for t in TITLES):                  # another spell's title leaked into this one
        return True
    return len(field(blk, 'Componentes', 'Duraci')) > 120 or len(field(blk, 'Tiempo de lanzam', 'Alcance')) > 160

def read_stub(path):
    hdr, body, seen = {}, [], False
    for line in open(path, encoding='utf-8').read().split('\n'):
        if not seen:
            if line == '---':
                seen = True
            elif '=' in line:
                k, v = line.split('=', 1)
                hdr[k] = v
        else:
            body.append(line)
    return hdr, '\n'.join(body).strip()

STUB_MARK = 'not in the free SRD'

def idx_set_flag(sd, ids, drop_p):
    files = glob.glob(os.path.join(sd, '*', 'index', '*.idx'))
    n = 0
    for f in files:
        rows = open(f, encoding='utf-8').read().split('\n')
        ch = False
        for j, r in enumerate(rows):
            p = r.split('|')
            if len(p) < 5:
                continue
            sid = p[0].lstrip('gn')
            if not sid.isdigit() or int(sid) not in ids:
                continue
            if p[0].startswith('n'):
                continue
            old = p[4]
            p[4] = old.replace('P', '') if drop_p else (old if 'P' in old else old + 'P')
            if p[4] != old:
                rows[j] = '|'.join(p); ch = True; n += 1
        if ch:
            open(f, 'w', encoding='utf-8', newline='\n').write('\n'.join(rows))
    return n

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    restore = '--restore' in sys.argv
    if len(args) < 2:
        sys.exit(__doc__)
    pdf, sd = args[0], args[1]
    a, b = (int(args[2]), int(args[3])) if len(args) >= 4 else (241, 345)
    if os.path.basename(os.path.normpath(sd)) == 'SD_CARD' and not restore:
        print('WARNING: SD_CARD is the folder tracked in git. Prefer a copy (SD_PERSONAL) so the PHB text is never committed.')
    spells = os.path.join(sd, 'grimoire', 'spells')
    if not os.path.isdir(spells):
        sys.exit('Not a Grimoire SD folder: ' + spells)

    # the 52 PHB-only stubs, by English name (they are the files whose body is the stub notice, or already filled)
    stubs = {}
    for f in glob.glob(os.path.join(spells, '*.txt')):
        sid = int(os.path.basename(f)[:3])
        if sid < 340:
            continue
        hdr, body = read_stub(f)
        stubs[hdr['name']] = (sid, f, hdr, body)
    if len(stubs) != 52:
        sys.exit('Expected 52 PHB-only stubs (340-391), found %d' % len(stubs))
    ids = {v[0] for v in stubs.values()}

    if restore:
        STUB = ("This spell is in the Player's Handbook (2024) but not in the free SRD 5.2.1, so its text is not "
                "included on this card. Look it up in your book. Name, level, school and class lists are shown "
                "so your class spell lists are complete.")
        for name, (sid, f, hdr, body) in stubs.items():
            keep = ['name', 'level', 'school', 'classes']
            lines = [k + '=' + hdr[k] for k in keep] + ['time=', 'range=', 'comp=', 'dur=', 'flags=' + hdr.get('flags', '').replace('P', '') + 'P', '---', STUB]
            open(f, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines) + '\n')
        print('Restored', len(stubs), 'stubs;', idx_set_flag(sd, ids, False), 'index lines re-dimmed')
        return

    text_raw = extract(pdf, a, b)
    parsed = parse(text_raw)
    TITLES.update(norm(n) for n, _ in parsed if len(norm(n)) >= 9)
    # a few spells sit in awkward two-column spots: the second reading order (pdftotext -layout-free default) is the fallback
    alt = {norm(n): blk for n, blk in parse(extract(pdf, a, b, '-nopgbrk'))}
    print('Spells found in the PDF pages %d-%d: %d' % (a, b, len(parsed)))
    done, missing = 0, []
    byname = {norm(k): v for k, v in EN.items()}
    wanted = {en: es for es, en in EN.items()}
    got = set()
    for es_name, blk in parsed:
        key = norm(es_name)
        en = EN.get(key)
        if not en:
            close = difflib.get_close_matches(key, list(EN), n=1, cutoff=0.88)
            en = EN[close[0]] if close else None
        if not en or en not in stubs:
            continue
        sid, f, hdr, _ = stubs[en]
        text = body_of(blk)
        if (len(text) < 200 or garbled(blk, text)) and key in alt:
            blk2 = alt[key]
            t2 = body_of(blk2)
            if not garbled(blk2, t2) and len(t2) > len(text) * 0.8:
                blk, text = blk2, t2
        if garbled(blk, text):
            missing.append(en + ' (two-column page came out garbled: fill spells/%03d.txt by hand)' % sid)
            continue
        if len(text) < 40:
            missing.append(en + ' (empty text)')
            continue
        flags = hdr.get('flags', '').replace('P', '')
        note = '(Texto en espa\u00f1ol, de tu copia personal del Manual del Jugador 2024.)'
        out = ['name=' + hdr['name'], 'level=' + hdr['level'], 'school=' + hdr['school'], 'classes=' + hdr['classes'],
               'time=' + field(blk, 'Tiempo de lanzam', 'Alcance'), 'range=' + field(blk, 'Alcance', 'Componentes'),
               'comp=' + field(blk, 'Componentes', 'Duraci'), 'dur=' + field(blk, 'Duraci', r'\Z_'),
               'flags=' + flags, '---', note, text]
        # "dur" field must stay on one line: it is the text up to the first line break
        out[7] = 'dur=' + (re.search(r'Duraci\w*:\s*([^\n]*)', '\n'.join(blk)).group(1).strip()
                           if re.search(r'Duraci\w*:\s*([^\n]*)', '\n'.join(blk)) else '')
        open(f, 'w', encoding='utf-8', newline='\n').write('\n'.join(out) + '\n')
        got.add(en)
        done += 1
    for en in stubs:
        if en not in got and not any(m.startswith(en) for m in missing):
            missing.append(en)
    print('Loaded text for %d of 52 spells.' % done)
    if missing:
        print('Not loaded:', ', '.join(sorted(set(missing))))
    n = idx_set_flag(sd, {stubs[e][0] for e in got}, True)
    print('Updated %d index lines (spells no longer dimmed).' % n)
    print('Reminder: keep these files private (PHB text is copyrighted).')

if __name__ == '__main__':
    main()
