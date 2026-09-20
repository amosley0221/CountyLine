"""Draw the current town footprints from native source; standard-library only."""
from pathlib import Path
from html import escape
import re
import math

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
    return 50 + (x + 8000) * .062, 110 + (7800 - y) * .062

def box(x, y, w, h, color, stroke='#6e6048'):
    sx, sy = point(x-w/2, y+h/2)
    parts.append(f'<rect x="{sx:.1f}" y="{sy:.1f}" width="{w*.062:.1f}" height="{h*.062:.1f}" fill="{color}" stroke="{stroke}"/>')

def shape(name):
    match = re.search(r'Shape\(TEXT\("'+name+r'"\),FVector\(([^)]+)\),FVector\(([^)]+)\)', source)
    if not match:
        raise ValueError('Source geometry changed: '+name)
    return numbers(match[1]), numbers(match[2])

for name, color in [('TownGround','#e1d2ae'), ('ResidentialGround','#e1d2ae'), ('ResidentialLane','#c6b18c'), ('CourtNorthExtension','#c6b18c'), ('CourtStreet','#c6b18c'), ('SquareRoad','#c6b18c'), ('WestMarketStreet','#c6b18c'), ('WestServiceLane','#c6b18c'), ('EastShopWalk','#d9c7a3'), ('CourthouseWalk','#d9c7a3')]:
    p, size = shape(name)
    box(p[0], p[1], size[0], size[1], color)

court = numbers(re.search(r'const FVector Court\(([^)]+)\)', source)[1])
court_size = numbers(re.search(r'Shape\(TEXT\("CourthouseMass"\).*?FVector\(([^)]+)\),TEXT\("Brick"\)', source)[1])
lang, lang_size = shape('LangFloor')
buildings = [('Courthouse (exterior)',court[0],court[1],court_size[0],court_size[1]),
             ('Jail office',0,0,1120,900),
             ("Lang’s (lobby open)",lang[0],lang[1],lang_size[0],lang_size[1])]
fronts = {'Courthouse (exterior)': (0,-1), 'Jail office': (-1,0), "Lang’s (lobby open)": (0,1)}
for title, pos, width, yaw in re.findall(r'Store\(TEXT\("[^"]+"\),TEXT\("([^"]+)"\),FVector\(([^)]+)\),([\d.]+),[\d.]+,(-?[\d.]+)\)', source):
    p = numbers(pos)
    angle=math.radians(float(yaw))
    assert float(yaw)%90==0, 'Map needs a polygon for non-cardinal shops'
    facing=(round(math.sin(angle)),round(-math.cos(angle)))
    assert abs(facing[0])+abs(facing[1])==1, 'Map needs a polygon for non-cardinal shops'
    label=title.title()+' (closed)'
    w,h=(700,float(width)) if facing[0] else (float(width),700)
    buildings.append((label,p[0],p[1],w,h))
    fronts[label]=facing

for name, pos, width, depth in re.findall(r'Home\(TEXT\("([^" ]+)"\),FVector\(([^)]+)\),([\d.]+),([\d.]+),TEXT', source):
    p = numbers(pos)
    buildings.append((name.replace('Home', '')+' home (closed)',p[0],p[1],float(width),float(depth)))
    fronts[buildings[-1][0]]=(0,-1)

def arrow(x,y,fx,fy,length=15,color='#245d75'):
    sx,sy=point(x,y)
    dx,dy=fx,-fy
    ex,ey=sx+dx*length,sy+dy*length
    parts.append(f'<path d="M {sx} {sy} L {ex} {ey} l {-dx*5-dy*3} {-dy*5+dx*3} M {ex} {ey} l {-dx*5+dy*3} {-dy*5-dx*3}" fill="none" stroke="{color}" stroke-width="2"/>')

for i, (name,x,y,w,h) in enumerate(buildings,1):
    box(x,y,w,h,'#99735b')
    fx,fy=fronts[name]
    edge_x,edge_y=x+fx*w/2,y+fy*h/2
    arrow(edge_x,edge_y,fx,fy)
    sx,sy = point(x,y)
    parts.append(f'<circle cx="{sx}" cy="{sy}" r="14" fill="#fff5df"/><text x="{sx}" y="{sy+5}" text-anchor="middle" font-size="16">{i}</text>')
    parts.append(f'<text x="680" y="{235+i*34}" font-size="17">{i}. {escape(name)}</text>')

for side in (-1,1):
    for end in (-1,1):
        x,y=-3900+side*800,(-150 if end<0 else 2600)
        box(x,y,190,60,'#3f6645')
        arrow(x,y,0,end,10,'#3f6645')
for label,x,y in [('Market Street',-5650,1400),('Service lane',-7250,1600),('Court Street',-2200,2300)]:
    sx,sy=point(x,y)
    parts.append(f'<text x="{sx}" y="{sy}" text-anchor="middle" font-size="12" transform="rotate(-90 {sx} {sy})">{label}</text>')

parts += ['<text x="680" y="150" font-size="23">N ↑</text>',
          '<text x="680" y="207" font-size="19">EXISTING BUILDINGS</text>',
          '<text x="680" y="710" font-size="16">Interiors open only where noted</text>',
          '<text x="680" y="740" font-size="16">and at the jail office.</text>',
          '<text x="680" y="785" font-size="16" fill="#245d75">Blue arrows: building fronts</text>',
          '<text x="680" y="815" font-size="16" fill="#3f6645">Green arrows: benches face this way</text>']
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
