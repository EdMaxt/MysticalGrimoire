"""Step 1. Parse the Spanish PHB 2024 spell descriptions. First: pdftotext -f 241 -l 345 -raw <PHB.pdf> desc.txt (pages of the Spanish edition). Writes phb_es.json. Personal use only: the PDF and phb_es.json hold copyrighted text and are not part of this project."""
import re,json,difflib,collections
L=open('desc.txt',encoding='utf-8').read().replace('\f','\n').split('\n')
SCH={'abjuración':'Abjuration','conjuración':'Conjuration','adivinación':'Divination','encantamiento':'Enchantment','evocación':'Evocation','ilusionismo':'Illusion','nigromancia':'Necromancy','transmutación':'Transmutation'}
def school(w):
    m=difflib.get_close_matches(w.lower(),SCH,n=1,cutoff=0.6); return SCH[m[0]] if m else '?'
S0=re.compile(r'(?:\b(?:Abjuraci|Conjuraci|Adivinaci|Encantamiento|Evocaci|[HI]lusionismo|Nigromancia|Transmutaci)\w* de nivel|\bTruco de (?:abjuraci|conjuraci|adivinaci|encantamiento|evocaci|[hi]lusionismo|nigromancia|transmutaci)\w*)',re.I)
starts=[]
for i,l in enumerate(L):
    m=S0.search(l)
    if m and (m.start()==0 or l[:m.start()].strip()==''  or True):
        # header must be near a "lanzamiento"-like line within 6 lines
        if any(re.search(r'lanzam',L[k],re.I) for k in range(i,min(i+7,len(L)))): starts.append((i,m.start()))
out=[]
for n,(i,off) in enumerate(starts):
    h=L[i][off:]; j=i
    while ')' not in h and j<i+3: j+=1; h+=' '+L[j].strip()
    h=re.sub(r'\s+',' ',h); h=re.sub(r'nivel ?(\d) ?\(',r'nivel \1 (',h)
    m=re.match(r'^(\S+) de nivel (\d) \((.*?)\)',h)
    if m: sch,lvl,cl=school(m.group(1)),int(m.group(2)),m.group(3)
    else:
        m=re.match(r'^Truco de (\S+) \((.*?)\)',h)
        if not m: print('NOHDR',i,h[:80]); continue
        sch,lvl,cl=school(m.group(1)),0,m.group(2)
    k=i-1
    if L[i][:off].strip(): name=L[i][:off].strip()
    else:
        while k>=0 and not L[k].strip(): k-=1
        name=L[k].strip()
    if 'ROCIADA VENENOSA' in name: name='ROCIADA VENENOSA'
    end=starts[n+1][0]-1 if n+1<len(starts) else len(L)
    blk='\n'.join(L[j:end])
    def fl(a,b):
        mm=re.search(a+r'\w*:\s*(.*?)\s*(?=\n(?:'+b+r')\w*:|$)',blk,re.S|re.I)
        return re.sub(r'\s+',' ',mm.group(1)) if mm else ''
    d=re.search(r'Duraci\w*:\s*([^\n]*)',blk)
    out.append(dict(name=name,level=lvl,school=sch,classes=[c.strip() for c in cl.replace('\n',' ').split(',')],
        text=blk,time=fl('Tiempo de lanzam','Alcance'),range=fl('Alcance','Componentes'),comp=fl('Componentes','Duraci')[:90],dur=d.group(1) if d else ''))
print(len(out),collections.Counter(o['level'] for o in out))
json.dump(out,open('phb_es.json','w',encoding='utf-8'),ensure_ascii=False,indent=1)
