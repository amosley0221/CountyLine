"""Original, deterministic street kit. Blender background; centimetres, Unreal Y reflection."""
import bpy, bmesh, math, random
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'SourceAssets/Authored/StreetBenchmark';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.scene.unit_settings.scale_length=.01
random.seed(1927)
colors={'Oak':(.20,.135,.078),'Iron':(.055,.062,.059),'Bark':(.16,.135,.10),'Leaf':(.13,.205,.075),'LeafLight':(.25,.32,.12),'LeafGold':(.32,.34,.14),'Canvas':(.64,.58,.43),'CanvasGreen':(.14,.21,.16),'Terracotta':(.34,.16,.09),'Glass':(.19,.24,.20)}
mats={}
for name,c in colors.items():
 m=bpy.data.materials.new(name);m.diffuse_color=(*c,1);mats[name]=m
class Kit:
 def __init__(self,name):self.name=name;self.vertices=[];self.faces=[];self.materials=[]
 def face(self,vs,mat):
  start=len(self.vertices);self.vertices.extend(vs);self.faces.append(tuple(range(start,start+len(vs))));self.materials.append(list(mats).index(mat))
 def box(self,p,s,mat):
  p=Vector(p);s=Vector(s)/2
  v=[p+Vector((x*s.x,y*s.y,z*s.z)) for x,y,z in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]]
  for f in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:self.face([v[i] for i in f],mat)
 def rod(self,a,b,r1,r2,mat,n=10):
  a,b=Vector(a),Vector(b);z=(b-a).normalized();x=z.cross(Vector((0,0,1)))
  if x.length<.01:x=z.cross(Vector((1,0,0)))
  x.normalize();y=z.cross(x)
  rings=[[p+r*(x*math.cos(i*math.tau/n)+y*math.sin(i*math.tau/n)) for i in range(n)] for p,r in [(a,r1),(b,r2)]]
  for i in range(n):self.face([rings[0][i],rings[0][(i+1)%n],rings[1][(i+1)%n],rings[1][i]],mat)
  self.face(list(reversed(rings[0])),mat);self.face(rings[1],mat)
 def leaf(self,p,length,width,angle,tilt,mat):
  p=Vector(p);u=Vector((math.cos(angle),math.sin(angle),tilt)).normalized();v=u.cross(Vector((0,0,1))).normalized()
  self.face([p-u*length*.45,p+v*width*.5,p+u*length*.55,p+Vector((0,0,1.4))],mat)
  self.face([p-u*length*.45,p+Vector((0,0,1.4)),p+u*length*.55,p-v*width*.5],mat)
 def finish(self,bevel=0):
  mesh=bpy.data.meshes.new(self.name);mesh.from_pydata([(x,-y,z) for x,y,z in self.vertices],[],[tuple(reversed(f)) for f in self.faces]);mesh.update()
  ob=bpy.data.objects.new(self.name,mesh);bpy.context.collection.objects.link(ob)
  for m in mats.values():mesh.materials.append(m)
  for poly,mi in zip(mesh.polygons,self.materials):poly.material_index=mi
  bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.0001);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free()
  uv=mesh.uv_layers.new(name='SurfaceUV')
  for poly in mesh.polygons:
   axes=[i for i in range(3) if i!=max(range(3),key=lambda i:abs(poly.normal[i]))]
   for li in poly.loop_indices:
    p=mesh.vertices[mesh.loops[li].vertex_index].co;uv.data[li].uv=(p[axes[0]]*.01,p[axes[1]]*.01)
  bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
  if bevel:
   mod=ob.modifiers.new('Soft worn edges','BEVEL');mod.width=bevel;mod.segments=2;mod.limit_method='ANGLE';bpy.ops.object.modifier_apply(modifier=mod.name)
   normals=ob.modifiers.new('Weighted corner normals','WEIGHTED_NORMAL');bpy.ops.object.modifier_apply(modifier=normals.name)
  bpy.ops.export_scene.fbx(filepath=str(OUT/(self.name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_anim=False,mesh_smooth_type='FACE')
  print(self.name,len(mesh.polygons))

# Individual branches and folded leaves, not solid canopy blobs.
t=Kit('SM_StreetCottonwood')
t.rod((0,0,0),(8,-3,240),24,14,'Bark',14);t.rod((8,-3,240),(0,12,510),14,4,'Bark',12)
for i in range(15):
 a=i*2.399;z=190+i*19;r=170+50*math.sin(i*1.6)
 base=Vector((5,0,z));mid=Vector((math.cos(a)*r*.58,math.sin(a)*r*.58,z+95));tip=Vector((math.cos(a)*r,math.sin(a)*r,z+165))
 t.rod(base,mid,9-i*.35,4,'Bark');t.rod(mid,tip,4,1,'Bark',7)
 for j in range(7):
  phi=a+(j-3)*.32;end=tip+Vector((math.cos(phi)*random.uniform(25,85),math.sin(phi)*random.uniform(25,85),random.uniform(-20,65)))
  t.rod(mid.lerp(tip,.55),end,1.8,.25,'Bark',5)
  for k in range(27):
   v=Vector((random.gauss(0,33),random.gauss(0,33),random.gauss(0,22)))
   t.leaf(end+v,random.uniform(15,24),random.uniform(9,16),random.random()*math.tau,random.uniform(-.8,.8),random.choice(['Leaf','Leaf','LeafLight','LeafGold']))
t.finish()

b=Kit('SM_StreetBench')
for j in range(5):b.box((0,-23+j*11,45),(184,9,4),'Oak')
for j in range(4):b.box((0,28+j*2,65+j*9),(184,4,7),'Oak')
for side in [-1,1]:
 x=side*68
 for y in [-19,23]:b.rod((x,y,0),(x,y,45),3.5,3,'Iron');b.box((x,y,2),(12,14,4),'Iron')
 b.rod((x,-25,40),(x,31,40),3,3,'Iron');b.rod((x,23,30),(x,36,101),3,2.5,'Iron')
 b.rod((x,-20,44),(x,-20,68),2.5,2.5,'Iron');b.rod((x,-20,68),(x,31,75),2.5,2.5,'Iron')
b.finish(.6)

b=Kit('SM_StreetBarrel')
for i in range(20):
 a0=math.tau*i/20+.008;a1=math.tau*(i+1)/20-.008
 for z0,z1 in [(0,18),(18,52),(52,76),(76,90)]:
  r0=30+7*math.sin(math.pi*z0/90);r1=30+7*math.sin(math.pi*z1/90)
  b.face([(r0*math.cos(a0),r0*math.sin(a0),z0),(r0*math.cos(a1),r0*math.sin(a1),z0),(r1*math.cos(a1),r1*math.sin(a1),z1),(r1*math.cos(a0),r1*math.sin(a0),z1)],'Oak')
for z in [8,28,65,83]:b.rod((0,0,z-2),(0,0,z+2),31+7*math.sin(math.pi*z/90),31+7*math.sin(math.pi*z/90),'Iron',32)
b.rod((0,0,88),(0,0,90),29,29,'Oak',32);b.finish(.2)

b=Kit('SM_StreetDisplay')
for x in [-58,58]:
 for z in [8,24,40]:
  for y in [-42,42]:b.box((x,y,z),(100,5,13),'Oak')
  for dx in [-48,48]:b.box((x+dx,0,z),(5,84,13),'Oak')
 for j in range(5):b.box((x,-34+j*17,2),(100,15,4),'Oak')
 for j in range(7):
  px=x+random.uniform(-35,35);py=random.uniform(-25,25)
  b.rod((px,py,8),(px,py,45),8,7,'Terracotta',10)
b.finish(.45)

# Striped fabric canopy, local front -Y, authored for a 1000 cm shop.
b=Kit('SM_StreetAwning')
for i in range(20):
 x0=-510+i*51;x1=x0+51;mat='Canvas' if i%2==0 else 'CanvasGreen'
 for j in range(8):
  y0=-355-j*37;y1=y0-37
  z0=310-j*6-3*math.sin(j*.7);z1=310-(j+1)*6-3*math.sin((j+1)*.7)
  b.face([(x0,y0,z0),(x1,y0,z0),(x1,y1,z1),(x0,y1,z1)],mat)
 b.face([(x0,-651,262),(x1,-651,262),(x1,-651,240),(x0,-651,240)],mat)
for x in [-488,488]:b.rod((x,-360,306),(x,-645,260),3,3,'Iron');b.rod((x,-645,0),(x,-645,260),4,4,'Iron')
b.finish()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'StreetBenchmark.blend'))
print('CL_STREET_AUTHOR_SUCCESS')
