"""Spellcasting progression tables (cantrips, prepared spells, spell slots by level) -> progress.txt per book.
SRD: srd_all.txt (pdftotext -layout of the SRD). Neon: hb.txt (pdftotext -layout of the Outrunner's Handbook).
Output lines: Class|kind|level|cantrips|prepared|slots   (kind: full, half, pact; slots = comma list L1..L9, or 'count,slotLevel' for pact)"""
import re,sys,json
def table_rows(lines,start,n=20):
    rows=[]
    for l in lines[start:start+60]:
        if re.search(r'\s\+\d\s',l+' ') and re.match(r'^\s*(\d{1,2}\s+)?\+\d|^\s+\d{1,2}\s+\+\d|^\s+\+\d',l):
            rows.append(l)
        if len(rows)==n: break
    return rows
def tail(l):
    toks=l.split()
    out=[]
    for t in reversed(toks):
        if re.fullmatch(r'\d+|[—–-]',t): out.append(t)
        else: break
    out.reverse()
    return [0 if t in '—–-' else int(t) for t in out]
def parse(lines,cls,kind,ntail,heads):
    i=heads[cls]; rows=table_rows(lines,i)
    assert len(rows)==20,(cls,len(rows))
    res=[]
    for lv,l in enumerate(rows,1):
        t=tail(l)
        if len(t)<ntail: raise SystemExit('short tail %s L%d: %s'%(cls,lv,l))
        t=t[-ntail:]
        if kind=='full': res.append((lv,t[0],t[1],t[2:]))
        elif kind=='half':
            res.append((lv,0,t[0],t[1:]))
        elif kind=='pact': res.append((lv,t[0],t[1],t[2:]))
        elif kind=='half_c': res.append((lv,t[0],t[1],t[2:]))
    return res
def heads_for(lines,names,minline=0):
    h={}
    for i,l in enumerate(lines):
        if i<minline: continue
        for n in names:
            if re.match(r'^\s+%s Features\s*$'%n,l,re.I) or re.match(r'^\s+%s FEATURES\s*$'%n,l): h.setdefault(n,i)
    return h
out=[]
srd=open('srd_all.txt',encoding='utf-8').read().split('\n')
H=heads_for(srd,['Bard','Cleric','Druid','Paladin','Ranger','Sorcerer','Warlock','Wizard'],1500)
CFG_SRD={'Bard':('full',11),'Cleric':('full',11),'Druid':('full',11),'Sorcerer':('full',11),'Wizard':('full',11),'Paladin':('half',6),'Ranger':('half',6),'Warlock':('pact',4)}
book={}
for c,(k,n) in CFG_SRD.items(): book[c]=(k,parse(srd,c,k,n,H))
json.dump({'srd':{c:[k,r] for c,(k,r) in book.items()}},open('progress_srd.json','w'))
hb=open('hb.txt',encoding='utf-8').read().split('\n')
HN=heads_for(hb,['Cryptist','Enforcer','Esper','Gridrunner','Icon','Machinist','Oracle','Primalist','Tracer'])
CFG_N={'Cryptist':('pact',4),'Esper':('full',11),'Gridrunner':('full',11),'Icon':('full',11),'Oracle':('full',11),'Primalist':('full',11),
       'Enforcer':('half',6),'Tracer':('half',6),'Machinist':('half_c',7)}
BASE={'Cryptist':'Warlock','Esper':'Sorcerer','Gridrunner':'Wizard','Icon':'Bard','Oracle':'Cleric','Primalist':'Druid','Enforcer':'Paladin','Tracer':'Ranger'}
nbook={}
for c,(k,n) in CFG_N.items(): nbook[c]=(k,parse(hb,c,k,n,HN))
# validation: Neon tables vs the SRD class they are based on
for c,b in BASE.items():
    same=[ (r1[1:],) == (r2[1:],) for r1,r2 in zip(nbook[c][1],book[b][1])]
    diffs=[i+1 for i,ok in enumerate(same) if not ok]
    print('%-10s vs %-9s kind=%s identical rows: %d/20 %s'%(c,b,nbook[c][0],20-len(diffs),('diff at levels %s'%diffs) if diffs else ''))
def lines_for(bk):
    out=[]
    for c in sorted(bk):
        k,rows=bk[c]
        for lv,can,prep,sl in rows:
            kind='half' if k in('half','half_c') else k
            if kind!='pact': sl=(sl+[0]*9)[:9]
            out.append('%s|%s|%d|%d|%d|%s'%(c,kind,lv,can,prep,','.join(map(str,sl))))
    return '\n'.join(out)+'\n'
open('progress_srd.txt','w').write('# class|kind|level|cantrips|prepared|slots(L1..L9) or pact: count,slotLevel\n'+lines_for(book))
open('progress_neon.txt','w').write('# class|kind|level|cantrips|prepared|slots(L1..L9) or pact: count,slotLevel\n'+lines_for(nbook))
print(lines_for(book).split('\n')[19]); print([l for l in lines_for(nbook).split('\n') if l.startswith('Machinist|half|20') or l.startswith('Cryptist|pact|20') or l.startswith('Machinist|half|1|')])
