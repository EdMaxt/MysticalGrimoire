"""Step 3. Cross-check the Spanish PHB 2024 spell list against the SRD 5.2.1 data and list what the SRD lacks.
Run from the project root. Needs phb_es.json and match.json from steps 1-2. Writes ../neon/proj/data/phb_only.json and tools/phb_crosscheck.csv"""
import json,csv,re,sys
exec(open('tools/phb_2_match.py').read().split("groups=collections")[0])
m=json.load(open('match.json'))
EN={
'AMISTAD':'Friends','ARMA ELEMENTAL':'Elemental Weapon','ARMADURA DE AGATHYS':'Armor of Agathys','AURA DE PUREZA':'Aura of Purity',
'AURA DE VITALIDAD':'Aura of Vitality','BRAZOS DE HADAR':'Arms of Hadar','CALDERO BURBUJEANTE DE TASHA':"Tasha's Bubbling Cauldron",
'CARCAJ VELOZ':'Swift Quiver','CASTIGO ABRUMADOR':'Staggering Smite','CASTIGO ATRONADOR':'Thunderous Smite','CASTIGO CEGADOR':'Blinding Smite',
'CASTIGO DESTERRADOR':'Banishing Smite','CASTIGO FURIOSO':'Wrathful Smite','CÍRCULO DE PODER':'Circle of Power',
'CONJURAR DESCARGA DE PROYECTILES':'Conjure Barrage','CONJURAR LLUVIA DE FLECHAS':'Conjure Volley','CORDÓN DE FLECHAS':'Cordon of Arrows',
'CORONA DE LA LOCURA':'Crown of Madness','DUELO FORZADO':'Compelled Duel','ENREDADERA':'Grasping Vine','ESTÁTICA SINÁPTICA':'Synaptic Static',
'FINGIR MUERTE':'Feign Death','FLECHA DE RELÁMPAGO':'Lightning Arrow','FRAGMENTO MENTAL':'Mind Sliver','FUENTE DE LUZ LUNAR':'Fount of Moonlight',
'GOLPE DE VIENTO ACERADO':'Steel Wind Strike','GUARDIA DE CUCHILLAS':'Blade Ward','HAMBRE DE HADAR':'Hunger of Hadar',
'INVOCAR ABERRACIÓN':'Summon Aberration','INVOCAR AUTÓMATA':'Summon Construct','INVOCAR BESTIA':'Summon Beast','INVOCAR CELESTIAL':'Summon Celestial',
'INVOCAR ELEMENTAL':'Summon Elemental','INVOCAR FEÉRICO':'Summon Fey','INVOCAR INFERNAL':'Summon Fiend','INVOCAR MUERTO VIVIENTE':'Summon Undead',
'LÁTIGO DE ESPINAS':'Thorn Whip','MANTO DEL CRUZADO':"Crusader's Mantle",'NUBE DE DAGAS':'Cloud of Daggers','OLA DESTRUCTORA':'Destructive Wave',
'PALABRA DE PODER: FORTALECER':'Power Word Fortify','PALABRA DE RESPLANDOR':'Word of Radiance','PRESENCIA REGIA DE YOLANDE':"Yolande's Regal Presence",
'PUERTA ARCANA':'Arcane Gate','RAYO DE HECHICERÍA':'Witch Bolt','SENTIDOS DE LA BESTIA':'Beast Sense','TAÑIDO POR LOS MUERTOS':'Toll the Dead',
'TELEPATÍA':'Telepathy','TORMENTA DE ESPINAS':'Hail of Thorns','TORMENTA RESPLANDECIENTE DE JALLARZI':"Jallarzi's Storm of Radiance",
'TRONAR':'Thunderclap','VIGOR ARCANO':'Arcane Vigor'}
def clean(n): return re.sub(r'^[“"”\s]+','',n).strip()
only=[];rows=[]
for j,o in enumerate(es):
    esn=clean(o['name'])
    if str(j) in m:
        s=srd[m[str(j)][0]]; rows.append((esn,s['name'],o['level'],o['school'],'in SRD'))
    else:
        en=EN[esn]
        classes=sorted(esclasses(o))
        assert not any(c.startswith('?') for c in classes),(esn,classes)
        only.append(dict(name=en,level=o['level'],school=o['school'],classes=classes,
             concentration=o['dur'].lower().startswith('concentra'),ritual='ritual' in o['time'].lower()))
        rows.append((esn,en,o['level'],o['school'],'PHB only (not in SRD)'))
assert len(only)==52 and len({o['name'] for o in only})==52
only.sort(key=lambda o:o['name'].lower())
json.dump(only,open('data/phb_only.json','w',encoding='utf-8'),ensure_ascii=False,indent=1)
rows.sort(key=lambda r:(r[1].lower()))
with open('tools/phb_crosscheck.csv','w',encoding='utf-8-sig',newline='') as f:
    w=csv.writer(f); w.writerow(['PHB 2024 (Spanish) name','English name','Level','School','Status']); w.writerows(rows)
print(len(rows),'rows;',len(only),'PHB-only')
