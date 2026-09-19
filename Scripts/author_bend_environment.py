"""Original deterministic Bend Lateral meshes. Run with Blender --background --python.

Coordinates are centimetres in the existing gameplay layout. The exported scene
is decorative; native collision keeps the office and investigation route stable.
"""
import bpy, bmesh, math, random
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'SourceAssets'/'Authored'/'BendLateral'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system='METRIC'
bpy.context.scene.unit_settings.scale_length=.01
rng=random.Random(1926)
COLORS={
 'Soil':(.36,.27,.16), 'RoadDust':(.52,.42,.28), 'Rut':(.29,.22,.13),
 'Clay':(.27,.21,.12), 'Water':(.075,.105,.074), 'Timber':(.21,.15,.086),
 'Endgrain':(.31,.24,.14), 'Grass':(.30,.30,.13), 'DryGrass':(.49,.40,.20),
 'Leaf':(.18,.24,.09), 'LeafLight':(.31,.35,.14), 'Rock':(.35,.32,.24),
 'BottleGlass':(.052,.095,.041), 'PaperLabel':(.60,.54,.38), 'Iron':(.08,.07,.05),
}
mats={}
for name,col in COLORS.items():
 m=bpy.data.materials.new(name);m.diffuse_color=(*col,1);m.use_nodes=True;m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(*col,1);mats[name]=m

class Mesh:
 def __init__(self,name): self.name=name;self.v=[];self.f=[];self.mi=[]
 def face(self,verts,mat):
  i=len(self.v);self.v.extend(verts);self.f.append(tuple(range(i,i+len(verts))));self.mi.append(list(mats).index(mat))
 def quad(self,a,b,c,d,mat):self.face([a,b,c,d],mat)
 def rod(self,a,b,r1,r2,mat,n=7):
  a,b=Vector(a),Vector(b);z=(b-a).normalized();x=z.cross(Vector((0,0,1)))
  if x.length<.01:x=z.cross(Vector((1,0,0)))
  x.normalize();y=z.cross(x)
  ra=[a+r1*(math.cos(i*2*math.pi/n)*x+math.sin(i*2*math.pi/n)*y) for i in range(n)]
  rb=[b+r2*(math.cos(i*2*math.pi/n)*x+math.sin(i*2*math.pi/n)*y) for i in range(n)]
  for i in range(n):self.quad(ra[i],ra[(i+1)%n],rb[(i+1)%n],rb[i],mat)
  self.face(list(reversed(ra)),mat);self.face(rb,mat)
 def ellipsoid(self,c,s,mat,n=9,rings=5):
  c=Vector(c);s=Vector(s)
  def p(j,i):
   t=math.pi*j/rings;u=2*math.pi*i/n
   return c+Vector((s.x*math.sin(t)*math.cos(u),s.y*math.sin(t)*math.sin(u),s.z*math.cos(t)))
  for j in range(rings):
   for i in range(n):self.quad(p(j,i),p(j+1,i),p(j+1,i+1),p(j,i+1),mat)
 def finish(self):
  mesh=bpy.data.meshes.new(self.name);mesh.from_pydata([(x,-y,z) for x,y,z in self.v],[],[tuple(reversed(f)) for f in self.f]);mesh.update()
  ob=bpy.data.objects.new(self.name,mesh);bpy.context.collection.objects.link(ob)
  for m in mats.values():mesh.materials.append(m)
  for p,idx in zip(mesh.polygons,self.mi):p.material_index=idx;p.use_smooth=True
  bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.001);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free();mesh.update()
  uv=mesh.uv_layers.new(name='SurfaceUV')
  for poly in mesh.polygons:
   normal=poly.normal;axis=max(range(3),key=lambda i:abs(normal[i]));axes=[i for i in range(3) if i!=axis]
   for li in poly.loop_indices:
    v=mesh.vertices[mesh.loops[li].vertex_index].co;uv.data[li].uv=(v[axes[0]]*.01,v[axes[1]]*.01)
  bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);bpy.context.view_layer.objects.active=ob
  bpy.ops.export_scene.fbx(filepath=str(OUT/(self.name+'.fbx')),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_anim=False,mesh_smooth_type='FACE')
  return ob

def center(x):return 545+45*math.sin(x/680)
def ground(x,y):
 d=abs(y-center(x))
 if d<80:return -65
 if d<185:return -65+(d-80)*65/105
 # The walkable office-to-witness route stays almost flat.
 near=abs(x)<1700 and -1200<y<330
 return (.9 if near else 11)*math.sin(x*.013)*math.cos(y*.009)

terrain=Mesh('SM_BendGround')
# A continuous bank profile: extra rows follow both shoulders and the waterline.
for ix in range(100):
 x=-5000+ix*100;xn=x+100
 rows=[-4500,-2600,-1500,-950,-720,-650,-600,-550,-500,-450,-400,-350,-280,-100,150]
 offsets=[-220,-185,-135,-80,0,80,135,185,220]
 ys=rows+[center(x)+o for o in offsets]+[1100,1500,2200,3400,5000]
 yn=rows+[center(xn)+o for o in offsets]+[1100,1500,2200,3400,5000]
 for j in range(len(ys)-1):
  mid=(ys[j]+ys[j+1])/2
  mat='Clay' if abs(mid-center(x))<185 else 'RoadDust' if -720<mid<-280 else 'Soil'
  terrain.quad((x,ys[j],ground(x,ys[j])),(xn,yn[j],ground(xn,yn[j])),(xn,yn[j+1],ground(xn,yn[j+1])),(x,ys[j+1],ground(x,ys[j+1])),mat)
terrain.quad((-45000,-45000,-160),(45000,-45000,-160),(45000,45000,-160),(-45000,45000,-160),'Soil')
terrain.finish()

details=Mesh('SM_BendDetails')
# Wheel ruts wander gently along a dirt track.
for x in range(-3800,3900,70):
 for lane in (-575,-425):
  y=lane+12*math.sin(x/450);yn=lane+12*math.sin((x+70)/450)
  details.quad((x,y-9,ground(x,y)+1),(x+70,yn-9,ground(x+70,yn)+1),(x+70,yn+9,ground(x+70,yn)+1),(x,y+9,ground(x,y)+1),'Rut')
# Actual bottle at the original interaction target.
details.rod((150,255,0),(150,255,23),4.5,4.5,'BottleGlass',12)
details.rod((150,255,23),(150,255,28),4.5,2,'BottleGlass',12)
details.rod((150,255,28),(150,255,38),2,2,'BottleGlass',12)
details.rod((150,255,9),(150,255,17),4.58,4.58,'PaperLabel',12)
details.rod((150,255,38),(150,255,40),2.4,2.4,'Iron',12)
for i in range(240):
 x=rng.uniform(-3400,3400);y=rng.uniform(-1600,1500)
 if abs(y-center(x))<160:continue
 r=rng.uniform(1,6)
 details.ellipsoid((x,y,ground(x,y)+1),(r,r*.7,r*.45),'Rock',6,3)
details.finish()

water=Mesh('SM_BendWater')
for x in range(-5000,5000,90):
 water.quad((x,center(x)-105,-48),(x+90,center(x+90)-105,-48),(x+90,center(x+90)+105,-48),(x,center(x)+105,-48),'Water')
water.finish()

fence=Mesh('SM_BendFence')
# Weathered split rails explain the walkable limits without enclosing black walls.
def rail_run(points):
 for x,y in points:
  z=ground(x,y);fence.rod((x,y,z),(x+3,y-2,z+102),6,4.5,'Timber')
 for (x,y),(xx,yy) in zip(points,points[1:]):
  for z in (42,79):fence.rod((x,y,ground(x,y)+z),(xx,yy,ground(xx,yy)+z-3),3.2,2.7,'Timber',6)
rail_run([(x,335) for x in range(-1700,1801,230)])
rail_run([(x,-1200) for x in range(-1700,1801,230)])
rail_run([(-1750,y) for y in range(-1100,1301,230)])
rail_run([(1750,y) for y in range(-1100,1301,230)])
# Return-route signboard, away from the road centre.
fence.rod((-1250,-450,0),(-1250,-450,140),5,4,'Timber')
for z in (112,132):fence.rod((-1320,-450,z),(-1180,-450,z),12,12,'Endgrain',4)
fence.finish()

plants=Mesh('SM_BendVegetation')
for i in range(2400):
 x=rng.uniform(-3900,3900);y=rng.uniform(-2400,3100)
 if -760<y<-260 or abs(y-center(x))<165:continue
 if any(math.hypot(x-a,y-b)<100 for a,b in [(-550,-50),(150,255),(850,255)]):continue
 z=ground(x,y);h=rng.uniform(14,48);mat=rng.choice(['Grass','DryGrass','DryGrass'])
 for j in range(5):
  t=rng.uniform(0,math.tau);dx=math.cos(t);dy=math.sin(t);w=rng.uniform(1.2,3)
  plants.face([(x-dy*w,y+dx*w,z),(x+dy*w,y-dx*w,z),(x+dx*h*.45,y+dy*h*.45,z+h)],mat)
# Cottonwoods: several irregular crowns, visible branches and lighter leaf tips.
for tx,ty,scale in [(-1050,1100,1.1),(600,1250,1.2),(1450,1100,.85),(-1700,-1050,.95),(1650,-1000,1.15),(-2300,1800,1.4),(2400,2100,1.3),(-3100,-1500,1.2),(3300,-1900,1.3)]:
 z=ground(tx,ty);h=520*scale
 plants.rod((tx,ty,z),(tx+22,ty,z+h*.8),19*scale,7*scale,'Timber',9)
 for j in range(11):
  a=j*2.4;rr=rng.uniform(70,190)*scale
  end=Vector((tx+math.cos(a)*rr,ty+math.sin(a)*rr,z+h*rng.uniform(.6,1.12)))
  plants.rod((tx+10,ty,z+h*.5),end,6*scale,2*scale,'Timber')
  for k in range(5):
   c=end+Vector((rng.uniform(-60,60),rng.uniform(-60,60),rng.uniform(-50,50)))*scale
   plants.ellipsoid(c,(rng.uniform(60,95)*scale,rng.uniform(65,95)*scale,rng.uniform(75,110)*scale),'Leaf' if k%3 else 'LeafLight',7,4)
plants.finish()

hills=Mesh('SM_BendHorizon')
for i in range(24):
 a=math.tau*i/24;r=rng.uniform(22000,29000)
 hills.ellipsoid((math.cos(a)*r,math.sin(a)*r,-240),(rng.uniform(4000,6500),rng.uniform(3500,5500),rng.uniform(400,900)),'Soil',12,6)
hills.finish()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'BendLateral.blend'))
print('CL_ENVIRONMENT_AUTHORED',OUT)
