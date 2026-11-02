"""Step 2. Match the Spanish PHB spells to the SRD spells (level, school, classes, range, time, duration, dice and numbers in the text). Run from the project root."""
import json,re,collections
import numpy as np
from scipy.optimize import linear_sum_assignment
srd=json.load(open('data/spells.json',encoding='utf-8'))
es=json.load(open('phb_es.json',encoding='utf-8'))
CL={'bardo':'Bard','brujo':'Warlock','clérigo':'Cleric','druida':'Druid','explorador':'Ranger','hechicero':'Sorcerer','mago':'Wizard','paladín':'Paladin'}
def esclasses(o):
    r=set()
    for c in o['classes']:
        c=c.strip().lower()
        m=[k for k in CL if k[:4]==c[:4]]
        r.add(CL[m[0]] if m else '?'+c)
    return r
def es_range(s):
    s=s.lower()
    if s.startswith('toque'): return ('touch',[])
    if s.startswith('vista'): return ('sight',[])
    if s.startswith('ilimit'): return ('unl',[])
    kind='self' if s.startswith('lanzador') else 'rng'
    vals=[]
    for m in re.finditer(r'(\d+(?:,\d+)?)\s*(km|m)\b',s):
        v=float(m.group(1).replace(',','.')); vals.append(v*1000/0.3048 if m.group(2)=='km' else v/0.3048)
    return (kind,vals)
def en_range(s):
    s=s.lower()
    if s.startswith('touch'): return ('touch',[])
    if s.startswith('sight'): return ('sight',[])
    if s.startswith('unl'): return ('unl',[])
    kind='self' if s.startswith('self') else 'rng'
    vals=[]
    for m in re.finditer(r'(\d+(?:,\d+)?)[- ](feet|foot|mile|miles)',s):
        v=float(m.group(1).replace(',','')); vals.append(v*5280 if m.group(2).startswith('mile') else v)
    return (kind,vals)
def rscore(a,b):
    s=2*(a[0]==b[0]); va,vb=a[1],b[1]
    if va and vb and abs(va[0]-vb[0])<=max(2,0.12*vb[0]): s+=2
    elif not va and not vb: s+=1
    return s
def tkind(s,en):
    s=s.lower(); r='ritual' in s
    if en:
        k='bonus' if s.startswith('bonus') else 'react' if s.startswith('reaction') else 'action' if s.startswith('action') else None
        if not k:
            m=re.match(r'(\d+) (minute|hour)',s); k=(m.group(1)+m.group(2)[0]) if m else s[:6]
    else:
        k='bonus' if s.startswith('acción adicional') else 'react' if s.startswith('reacci') else 'action' if s.startswith('acci') else None
        if not k:
            m=re.match(r'(\d+) (minuto|hora)',s); k=(m.group(1)+m.group(2)[0]) if m else s[:6]
    return k,r
def dscore(a,b):
    s=2*(a.lower().startswith('concentra')==b.lower().startswith('concentration'))
    s+=2*(re.findall(r'\d+',a)==re.findall(r'\d+',b)); s+=(a.lower().startswith('instant')==b.lower().startswith('instant'))
    return s
def comp(s):
    m=re.match(r'^([VSM](?:, [VSM])*)',s.strip()); return m.group(1) if m else s[:6]
import math
def dice(t): return collections.Counter(re.findall(r'\d+d\d+',t.replace('D','d')))
def ints(t):
    t=re.sub(r'\d+(?:,\d+)?\s*(?:m|km|cm|kg|pies|feet|foot|ft|miles?|lb|pounds)\b','',t)
    t=re.sub(r'\d+d\d+','',t)
    return collections.Counter(re.findall(r'\b\d+\b',t))
def jac(a,b):
    u=sum((a|b).values()); return sum((a&b).values())/u if u else 1.0
def stext(s): return ' '.join(x for x in [s['description'],s['higher_level'] or '',s['cantrip_upgrade'] or ''])
groups=collections.defaultdict(lambda:([],[]))
for i,s in enumerate(srd): groups[(s['level'],s['school'])][0].append(i)
for j,o in enumerate(es): groups[(o['level'],o['school'])][1].append(j)
match={}
for key,(si,ej) in groups.items():
    if not si or not ej: continue
    C=np.zeros((len(ej),len(si)))
    for a,j in enumerate(ej):
        o=es[j]; ec=esclasses(o); er=es_range(o['range']); et=tkind(o['time'],False)
        for b,i in enumerate(si):
            s=srd[i]; sc=set(s['classes'])
            C[a,b]=6*(ec==sc)+3*len(ec&sc)/max(1,len(ec|sc))+rscore(er,en_range(s['range']))+3*(et==tkind(s['casting_time'],True))+dscore(o['dur'],s['duration'])+(comp(o['comp'])==comp(s['components']))+10*jac(dice(o['text']),dice(stext(s)))+5*jac(ints(o['text']),ints(stext(s)))+4*max(0,1-abs(math.log(max(1,len(o['text']))/max(1,len(stext(s)))/1.15)))
    r,c=linear_sum_assignment(-C)
    for a,b in zip(r,c): match[ej[a]]=(si[b],float(C[a,b]))
used={v[0] for v in match.values()}
print('ES total',len(es),'matched',len(match),'SRD unmatched',[srd[i]['name'] for i in range(len(srd)) if i not in used])
low=sorted((v[1],es[j]['name'],srd[v[0]]['name']) for j,v in match.items() if v[1]<18)
for x in low: print(x)
json.dump({str(j):v for j,v in match.items()},open('match.json','w'))
