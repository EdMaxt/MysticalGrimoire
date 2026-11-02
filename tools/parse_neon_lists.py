"""Parse the class spell-list tables of the Outrunner's Handbook.

Input : handbook.pdf (needs pdftotext)
Output: neon_lists.json   {class: [{name, level, school, special, new, alias_of}]}
"""
import re, json, subprocess, sys, collections

PDF = sys.argv[1] if len(sys.argv) > 1 else 'handbook.pdf'
OUT = sys.argv[2] if len(sys.argv) > 2 else 'neon_lists.json'

# class -> (first page of its spell list, last page that may still hold list rows)
RANGES = {
    'Cryptist': (23, 24), 'Enforcer': (38, 39), 'Esper': (47, 49),
    'Gridrunner': (55, 59), 'Icon': (65, 67), 'Machinist': (82, 83),
    'Oracle': (90, 92), 'Primalist': (101, 103), 'Tracer': (122, 123),
}
SCHOOLS = {'Abjuration', 'Conjuration', 'Divination', 'Enchantment',
           'Evocation', 'Illusion', 'Necromancy', 'Transmutation'}
SPECIAL = re.compile(r'^(—|[CRM](, [CRM])*)$')

def page_lines(pg):
    out = []
    for x, w in ((38, 252), (298, 252)):
        t = subprocess.run(['pdftotext', '-f', str(pg), '-l', str(pg), '-x', str(x), '-y', '0',
                            '-W', str(w), '-H', '735', PDF, '-'],
                           capture_output=True, text=True).stdout
        out += [l.strip() for l in t.replace('\f', '').split('\n') if l.strip()]
    return out

result = {}
for cls, (a, b) in RANGES.items():
    heading = re.compile(r'^(?:Cantrips \(Level 0 %s Spells\)|Level (\d) %s Spells)$' % (cls, cls))
    cur = None
    entries = []
    seen = set()
    for pg in range(a, b + 1):
        L = page_lines(pg)
        for i, ln in enumerate(L):
            m = heading.match(ln)
            if m:
                cur = int(m.group(1)) if m.group(1) else 0
                continue
            # any other "Level N: ..." / "... Spells" heading closes the list
            if cur is not None and re.match(r'^Level \d+: ', ln):
                cur = None
                continue
            if cur is None:
                continue
            if ln in SCHOOLS and i + 1 < len(L) and SPECIAL.match(L[i + 1]) and i >= 1:
                name = L[i - 1]
                # reskin names can wrap: "Cosmic Awareness (Detect Evil" / "and Good)"
                if name.endswith(')') and '(' not in name and i >= 2:
                    name = L[i - 2] + ' ' + name
                key = (cur, name)
                if key in seen:
                    continue
                seen.add(key)
                new = name.endswith('*')
                name = name.rstrip('*').strip()
                alias = None
                am = re.match(r'^(.*?) \((.+)\)$', name)
                if am:
                    name, alias = am.group(1).strip(), am.group(2).strip()
                entries.append(dict(name=name, level=cur, school=ln, special=L[i + 1],
                                    new=new, alias_of=alias))
    result[cls] = entries

json.dump(result, open(OUT, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)

# ---- report ----------------------------------------------------------------
print('class        total  by level')
for cls, es in result.items():
    lv = collections.Counter(e['level'] for e in es)
    print('%-11s %5d  %s' % (cls, len(es), dict(sorted(lv.items()))))
print('aliases:', [(c, e['name'], e['alias_of']) for c, es in result.items() for e in es if e['alias_of']])
