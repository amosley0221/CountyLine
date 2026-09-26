"""Original bevelled drugstore joinery and individually shaped display stock."""
from pathlib import Path
import bpy, math
# Reuse the mesh-authoring primitives without regenerating the rest of the street.
helpers=Path(__file__).with_name('author_street_benchmark.py')
exec(compile(helpers.read_text().split('# Individual branches')[0],str(helpers),'exec'))
OUT=ROOT/'SourceAssets/Authored/DrugstoreBenchmark';OUT.mkdir(parents=True,exist_ok=True)
colors={'Sage':(.095,.16,.12),'Stone':(.43,.40,.33),'Oak':(.19,.125,.064),'Iron':(.045,.052,.047),'Brass':(.38,.29,.13),'Amber':(.14,.063,.019),'Paper':(.63,.58,.46),'Glass':(.045,.085,.095)}
mats={}
for name,c in colors.items():
 m=bpy.data.materials.new(name);m.diffuse_color=(*c,1);mats[name]=m
b=Kit('SM_DrugstoreJoinery')
# Recessed lower door panels, raised rails, and a divided transom.
for s in (-1,1):
 b.box((s*59,-373,120),(11,22,240),'Sage')
 b.box((s*22,-372,64),(36,8,78),'Sage')
 for edge in (-1,1):b.box((s*22+edge*17,-378,64),(3,5,78),'Oak')
 b.box((s*22,-374,172),(36,6,62),'Glass')
 for z in (139,205):b.box((s*22,-379,z),(40,7,4),'Sage')
b.box((0,-374,242),(130,25,13),'Sage');b.box((0,-382,132),(108,9,9),'Sage')
b.box((0,-376,111),(106,9,6),'Sage');b.box((0,-378,172),(5,9,72),'Sage')
b.box((0,-382,8),(126,46,12),'Stone')
# Windows retain the existing transparent glazing and readable display depth.
for s in (-1,1):
 x=s*266
 for edge in (-1,1):
  b.box((x+edge*137,-379,157),(12,26,192),'Sage')
  b.box((x+edge*127,-387,157),(3,6,174),'Oak')
 for z in (63,252):b.box((x,-382,z),(286,29,12),'Sage')
 b.box((x,-391,56),(300,48,9),'Stone')
 b.box((x,-385,261),(298,31,9),'Stone')
 for j in range(9):b.box((x-125+j*31,-371,282),(27,20,33),'Stone')
# Cornice with a stepped drip edge and small brackets, plus a worn base course.
for y,z,depth,h in ((-376,481,42,11),(-382,491,48,9),(-386,498,57,6)):
 b.box((0,y,z),(991,depth,h),'Stone')
for x in (-445,-370,-295,-220,-145,-70,70,145,220,295,370,445):
 b.box((x,-387,458),(14,40,27),'Stone')
for s in (-1,1):b.box((s*452,-381,20),(49,40,32),'Stone')
# One downpipe, attached to the shop edge rather than scattered street clutter.
b.rod((449,-397,16),(449,-397,444),4,4,'Iron',12)
b.rod((449,-397,16),(449,-420,7),4,4,'Iron',12)
for z in (76,238,411):b.box((449,-389,z),(15,17,5),'Iron')
b.finish(.65)

b=Kit('SM_DrugstoreStock')
for side in (-1,1):
 for row in range(2):
  for i in range(4):
   x=side*266+(i-1.5)*52;y=-295+(i%2)*12;base=94+row*70
   radius=8 if i%2 else 10;height=27+(i%3)*5
   # Shouldered apothecary bottles, neck, rim and stopper.
   for a,z,r in ((0,2,radius*.85),(2,height-7,radius),(height-7,height-2,radius*.45),(height-2,height+7,radius*.40)):
    b.rod((x,y,base+a),(x,y,base+z),radius if a==2 else r,r,'Amber',16)
   b.rod((x,y,base+height+5),(x,y,base+height+9),radius*.48,radius*.48,'Oak',12)
   b.box((x,y-radius-.5,base+height*.45),(radius*1.5,1,13),'Paper')
   # Printed-looking ruled label, intentionally no fabricated medicine claims.
   for line in range(3):b.box((x,y-radius-1.1,base+height*.45-3+line*3),(radius,0.35,.65),'Iron')
b.finish(.25)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'DrugstoreBenchmark.blend'))
print('CL_DRUGSTORE_BENCHMARK_AUTHORED')
