# Step 1: extract SRD spell pages (107-175) column by column. Needs poppler's pdftotext.
import subprocess
PDF = 'SRD_CC_v5_2_1.pdf'
out = []
for pg in range(107, 176):
    for x, w in ((40, 257), (297, 297)):
        t = subprocess.run(['pdftotext', '-f', str(pg), '-l', str(pg), '-x', str(x), '-y', '0',
                            '-W', str(w), '-H', '740', PDF, '-'], capture_output=True, text=True).stdout
        out.append(t.replace('\f', ''))
open('spells_raw.txt', 'w', encoding='utf-8').write('\n'.join(out))
print('spells_raw.txt written')
