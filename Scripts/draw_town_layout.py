"""Draw the current town footprints from native source; standard-library only."""
from pathlib import Path
from html import escape
import re

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'Source/CountyLine/World/CLPecosBend.cpp').read_text()
parts = ['<svg xmlns="http://www.w3.org/2000/svg" width="1200" height="980" viewBox="0 0 1200 980">',
         '<rect width="1200" height="980" fill="#f2e6cc"/>',
         '<g font-family="Georgia, serif" fill="#352f25">',
         '<text x="45" y="46" font-size="28">PECOS BEND — PLAYABLE LAYOUT</text>',
         '<text x="45" y="76" font-size="16">Development schematic • 20 September 2026 • north is +Y</text>']

def numbers(value):
    return tuple(float(x.strip()) for x in value.split(','))

def point(x, y):
    return 50 + (x + 7000) * .062, 110 + (7800 - y) * .062

def box(x, y, w, h, color, stroke='#6e6048'):
    sx, sy = point(x-w/2, y+h/2)
    parts.append(f'<rect x="{sx:.1f}" y="{sy:.1f}" width="{w*.062:.1f}" height="{h*.062:.1f}" fill="{color}" stroke="{stroke}"/>')

def shape(name):
    match = re.search(r'Shape\(TEXT\("'+name+r'"\),FVector\(([^)]+)\),FVector\(([^)]+)\)', source)
    if not match:
        raise ValueError('Source geometry changed: '+name)
    return numbers(match[1]), numbers(match[2])

for name, color in [('TownGround','#e1d2ae'), ('ResidentialGround','#e1d2ae'), ('ResidentialLane','#c6b18c'), ('CourtNorthExtension','#c6b18c'), ('CourtStreet','#c6b18c'), ('SquareRoad','#c6b18c'), ('CourthouseWalk','#d9c7a3')]:
    p, size = shape(name)
    box(p[0], p[1], size[0], size[1], color)

court = numbers(re.search(r'const FVector Court\(([^)]+)\)', source)[1])
court_size = numbers(re.search(r'Shape\(TEXT\("CourthouseMass"\).*?FVector\(([^)]+)\),TEXT\("Brick"\)', source)[1])
lang, lang_size = shape('LangFloor')
buildings = [('Courthouse (exterior)',court[0],court[1],court_size[0],court_size[1]),
             ('Jail office',0,0,1120,900),
             ("Lang’s (lobby open)",lang[0],lang[1],lang_size[0],lang_size[1])]
for title, pos, width in re.findall(r'Store\(TEXT\("[^"]+"\),TEXT\("([^"]+)"\),FVector\(([^)]+)\),([\d.]+),[\d.]+\)', source):
    p = numbers(pos)
    buildings.append((title.title()+' (closed)',p[0],p[1],float(width),700))

for name, pos, width, depth in re.findall(r'Home\(TEXT\("([^" ]+)"\),FVector\(([^)]+)\),([\d.]+),([\d.]+),TEXT', source):
    p = numbers(pos)
    buildings.append((name.replace('Home', '')+' home (closed)',p[0],p[1],float(width),float(depth)))

for i, (name,x,y,w,h) in enumerate(buildings,1):
    box(x,y,w,h,'#99735b')
    sx,sy = point(x,y)
    parts.append(f'<circle cx="{sx}" cy="{sy}" r="14" fill="#fff5df"/><text x="{sx}" y="{sy+5}" text-anchor="middle" font-size="16">{i}</text>')
    parts.append(f'<text x="620" y="{235+i*34}" font-size="17">{i}. {escape(name)}</text>')

parts += ['<text x="620" y="150" font-size="23">N ↑</text>',
          '<text x="620" y="207" font-size="19">EXISTING BUILDINGS</text>',
          '<text x="620" y="710" font-size="16">Interiors open only where noted</text>',
          '<text x="620" y="740" font-size="16">and at the jail office.</text>']
sx,sy = point(1000,-800)
parts += [f'<path d="M {sx-70} {sy} H {sx+35} l -10 -6 m 10 6 l -10 6" fill="none" stroke="#352f25" stroke-width="3"/>',
          f'<text x="{sx-35}" y="{sy+32}" font-size="14">East road → Bend Lateral</text>',
          '<text x="50" y="895" font-size="17">Footprints follow the current prototype; doorways, trees and props omitted for clarity.</text>',
          '<text x="50" y="925" font-size="17">Railway, river corridor and county expansion are planned separately in WORLD_GEOGRAPHY.md.</text>',
          '</g></svg>']
out = ROOT / 'Docs/Maps/PecosBend-layout.svg'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text('\n'.join(parts), encoding='utf-8')
print(out)
