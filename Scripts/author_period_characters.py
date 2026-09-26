"""Author stylized, animation-compatible period characters on the template rig.

Requires export_character_base.py first. Clothing, faces and hats are original
procedural models; the existing licensed Epic skeleton supplies animation.
"""
import bpy, bmesh, math, sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'SourceAssets'/'Authored'/'Characters';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'Saved'/'ArtAuthoring'/'MannequinRig.fbx'))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
for ob in list(bpy.context.scene.objects):
 if ob.type=='MESH':bpy.data.objects.remove(ob,do_unlink=True)
bones={b.name:rig.matrix_world@b.head_local for b in rig.data.bones}
COLORS={'OliveWool':(.115,.13,.073),'BrownWool':(.14,.086,.044),'Waistcoat':(.105,.067,.037),
 'Shirt':(.51,.44,.29),'Linen':(.66,.62,.48),'Trouser':(.09,.075,.048),
 'Leather':(.048,.03,.017),'Skin':(.43,.255,.145),'Hair':(.045,.033,.024),
 'HatFelt':(.22,.17,.10),'HatBand':(.074,.049,.025),'Brass':(.41,.31,.13),
 'EyeWhite':(.54,.49,.40),'Iris':(.04,.03,.02),'Mouth':(.19,.079,.044)}
mats={}
for name,col in COLORS.items():
 m=bpy.data.materials.new(name);m.diffuse_color=(*col,1);m.use_nodes=True;m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(*col,1);mats[name]=m

class Figure:
 def __init__(self,name):self.name=name;self.v=[];self.f=[];self.mi=[];self.weights=[]
 def face(self,points,mat,weights):
  i=len(self.v);self.v.extend(points);self.f.append(tuple(range(i,i+len(points))));self.mi.append(list(mats).index(mat))
  for p in points:self.weights.append(weights(Vector(p)) if callable(weights) else weights)
 def ellipsoid(self,c,s,mat,bone,n=14,rings=9):
  c=Vector(c);s=Vector(s)
  def p(j,i):
   t=math.pi*j/rings;u=math.tau*i/n
   return c+Vector((s.x*math.sin(t)*math.cos(u),s.y*math.sin(t)*math.sin(u),s.z*math.cos(t)))
  for j in range(rings):
   for i in range(n):self.face([p(j,i),p(j+1,i),p(j+1,i+1),p(j,i+1)],mat,{bone:1})
 def tube(self,points,radii,mat,bone,n=14):
  points=[Vector(p) for p in points];axis=(points[-1]-points[0]).normalized();x=axis.cross(Vector((0,1,0)))
  if x.length<.01:x=axis.cross(Vector((1,0,0)))
  x.normalize();y=axis.cross(x)
  rings=[[p+r*(math.cos(i*math.tau/n)*x+math.sin(i*math.tau/n)*y) for i in range(n)] for p,r in zip(points,radii)]
  for a,b in zip(rings,rings[1:]):
   for i in range(n):self.face([a[i],a[(i+1)%n],b[(i+1)%n],b[i]],mat,bone)
  self.face(list(reversed(rings[0])),mat,bone);self.face(rings[-1],mat,bone)
 def palm(self,hand,middle,index,pinky,bone):
  # A tapered palm spans wrist to knuckles, following the actual rig axes.
  along=(middle-hand).normalized();across=(index-pinky).normalized()
  normal=along.cross(across).normalized();across=normal.cross(along).normalized()
  end=(index+pinky)*.5
  width=(index-pinky).length*.62
  rows=[(hand-along*.01,.022,.013),(hand.lerp(end,.45),width,.017),(end+along*.009,width,.014)]
  rings=[[center+across*(w*math.cos(i*math.tau/16))+normal*(d*math.sin(i*math.tau/16)) for i in range(16)] for center,w,d in rows]
  for a,b in zip(rings,rings[1:]):
   for i in range(16):self.face([a[i],a[(i+1)%16],b[(i+1)%16],b[i]],'Skin',{bone:1})
  self.face(list(reversed(rings[0])),'Skin',{bone:1});self.face(rings[-1],'Skin',{bone:1})
 def torso(self,rows,mat,opening=0,weights=None,n=28):
  # Front is -Y in the exported template bind pose.
  weights=weights or body_weight
  rings=[]
  for z,rx,ry,cy in rows:
   rings.append([(rx*math.cos(t),cy+ry*math.sin(t),z) for t in [(-math.pi/2+opening)+(math.tau-2*opening)*i/n for i in range(n+1)]])
  for a,b in zip(rings,rings[1:]):
   for i in range(n):self.face([a[i],a[i+1],b[i+1],b[i]],mat,weights)
 def finish(self):
  mesh=bpy.data.meshes.new(self.name);mesh.from_pydata(self.v,[],self.f);mesh.update()
  ob=bpy.data.objects.new(self.name,mesh);bpy.context.collection.objects.link(ob)
  for m in mats.values():mesh.materials.append(m)
  for p,idx in zip(mesh.polygons,self.mi):p.material_index=idx;p.use_smooth=True
  for name in rig.data.bones.keys():ob.vertex_groups.new(name=name)
  for i,ws in enumerate(self.weights):
   for name,w in ws.items():ob.vertex_groups[name].add([i],w,'REPLACE')
  bm=bmesh.new();bm.from_mesh(mesh);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000001);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(mesh);bm.free();mesh.update()
  uv=mesh.uv_layers.new(name='GarmentUV')
  for poly in mesh.polygons:
   normal=poly.normal;axis=max(range(3),key=lambda i:abs(normal[i]));axes=[i for i in range(3) if i!=axis]
   for li in poly.loop_indices:
    v=mesh.vertices[mesh.loops[li].vertex_index].co;uv.data[li].uv=(v[axes[0]],v[axes[1]])
  mod=ob.modifiers.new('Period character rig','ARMATURE');mod.object=rig
  ob.parent=rig;ob.matrix_parent_inverse=rig.matrix_world.inverted()
  bpy.ops.object.select_all(action='DESELECT');ob.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=ob
  bpy.ops.export_scene.fbx(filepath=str(OUT/(self.name+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE')
  return ob

def body_weight(p):
 levels=[(.96,'pelvis'),(1.075,'spine_01'),(1.268,'spine_02'),(1.4,'spine_03')]
 if p.z<=levels[0][0]:return {'pelvis':1}
 for (a,an),(b,bn) in zip(levels,levels[1:]):
  if p.z<=b:
   t=(p.z-a)/(b-a);return {an:1-t,bn:t}
 return {'spine_03':1}

reed_only='--reed-only' in sys.argv
for kind in ([] if '--definitions-only' in sys.argv else ['Reed'] if reed_only else ['Reed','Salazar']):
 f=Figure('SK_'+kind+'_Period');coat='OliveWool' if kind=='Reed' else 'BrownWool'
 # Torso and high-waisted trousers.
 f.torso([(0.9,.145,.105,.025),(1.06,.155,.10,.025),(1.23,.157,.103,.028),(1.43,.188,.095,.035),(1.51,.18,.073,.038),(1.57,.075,.052,.038),(1.595,.05,.048,.038)],'Shirt' if kind=='Reed' else 'Linen')
 f.torso([(.88,.148,.103,.025),(1.01,.155,.105,.025),(1.06,.15,.10,.025)],'Trouser',weights={'pelvis':1})
 f.torso([(1.035,.157,.106,.025),(1.063,.153,.104,.025)],'Leather',weights={'pelvis':1})
 if kind=='Salazar':f.torso([(1.00,.155,.107,.025),(1.15,.163,.11,.025),(1.40,.187,.10,.035)],'Waistcoat')
 hem=.72 if kind=='Reed' else .98
 f.torso([(hem,.215,.148,.035),(1.03,.185,.128,.03),(1.21,.18,.126,.03),(1.43,.212,.12,.037),(1.51,.192,.085,.042),(1.565,.085,.067,.04),(1.59,.056,.055,.04)],coat,.33)
 # Lapels, pointed collar and visible buttons.
 for side in (-1,1):
  f.face([(side*.065,-.07,1.525),(side*.17,-.068,1.47),(side*.09,-.10,1.26),(side*.043,-.11,1.40)],coat,body_weight)
  f.face([(side*.01,-.066,1.54),(side*.07,-.065,1.52),(side*.049,-.09,1.445)],'Shirt' if kind=='Reed' else 'Linen',{'spine_03':1})
  for z in ([1.08,1.2,1.32] if kind=='Reed' else [1.08,1.2]):
   f.ellipsoid((side*.065,-.10,z),(.011,.006,.011),'Leather','spine_01' if z<1.2 else 'spine_02',10,5)
 # Reed garment construction: pocket flaps, hem seam, buckle and boot lacing.
 if kind=='Reed':
  for side in (-1,1):
   f.face([(side*.09,-.104,1.055),(side*.17,-.073,1.055),(side*.17,-.078,1.016),(side*.09,-.109,1.016)],coat,body_weight)
   f.tube([(side*.09,-.109,1.051),(side*.17,-.079,1.051)],[.002,.002],'HatBand',body_weight,6)
  f.face([(-.026,-.087,1.06),(.026,-.087,1.06),(.026,-.089,1.031),(-.026,-.089,1.031)],'Brass',{'pelvis':1})
 # Tie / neckerchief.
 f.ellipsoid((0,-.063,1.496),(.021,.018,.022),'Hair','spine_03',10,5)
 if kind=='Reed':
  # The portrait shows two short neckerchief ends, not a long office tie.
  for s in (-1,1):f.face([(0,-.085,1.49),(s*.014,-.085,1.475),(s*.031,-.092,1.38),(s*.009,-.095,1.40)],'Hair',body_weight)
 else:f.face([(-.018,-.081,1.48),(.018,-.081,1.48),(.025,-.091,1.31),(0,-.093,1.275),(-.024,-.091,1.31)],'Hair',body_weight)
 # Badge on Reed's left breast, a small original six-point star.
 if kind=='Reed':
  points=[]
  for i in range(12):
   a=math.tau*i/12;r=.027 if i%2==0 else .013
   points.append((.125+math.sin(a)*r,-.079,1.397+math.cos(a)*r))
  f.face(list(reversed(points)),'Brass',{'spine_03':1})
 for side in ['l','r']:
  upper,elbow,hand=[bones[n+'_'+side] for n in ['upperarm','lowerarm','hand']]
  def sleeve_weight(p,a=upper,b=elbow,c=hand,s=side):
   # Smooth the elbow over a narrow band while preserving a soft sleeve silhouette.
   axis=(c-a).normalized();t=(p-b).dot(axis);u=max(0,min(1,.5+t/.12))
   return {'upperarm_'+s:1-u,'lowerarm_'+s:u}
  if kind=='Reed':
   shoulder=upper.lerp(bones['clavicle_'+side],.38)
   f.tube([shoulder,upper,upper.lerp(elbow,.35),elbow,elbow.lerp(hand,.7),hand],[.062,.073,.069,.058,.052,.039],coat,sleeve_weight,20)
  else:f.tube([upper,upper.lerp(elbow,.35),elbow,elbow.lerp(hand,.7),hand],[.071,.071,.058,.052,.043],coat,sleeve_weight)
  if kind!='Reed':f.ellipsoid(upper,(.079,.072,.08),coat,'upperarm_'+side)
  thigh,knee,foot=[bones[n+'_'+side] for n in ['thigh','calf','foot']]
  def leg_weight(p,k=knee,s=side):
   u=max(0,min(1,.5+(k.z-p.z)/.16));return {'thigh_'+s:1-u,'calf_'+s:u}
  f.tube([thigh,thigh.lerp(knee,.3),knee,knee.lerp(foot,.8),foot],[.096,.092,.068,.06,.058],'Trouser' if kind=='Reed' else 'BrownWool',leg_weight)
  f.ellipsoid((foot.x,foot.y-.057,.065),(.071,.154,.059),'Leather','foot_'+side,16,8)
  f.tube([foot,foot+Vector((0,0,.13))],[.061,.061],'Leather',{'calf_'+side:1})
  if kind=='Reed':
   for j in range(5):
    z=.11+j*.017
    f.tube([(foot.x-.022,foot.y-.064,z),(foot.x+.022,foot.y-.064,z+.010)],[.002,.002],'HatBand',{'foot_'+side:1},6)
    f.tube([(foot.x+.022,foot.y-.066,z),(foot.x-.022,foot.y-.066,z+.010)],[.002,.002],'HatBand',{'foot_'+side:1},6)
   f.ellipsoid((foot.x,foot.y-.052,.021),(.073,.153,.018),'HatBand','foot_'+side,20,6)
  # Hand and articulated fingers follow their original bones.
  palm=hand.lerp(bones['middle_01_'+side],.42)
  if kind=='Reed':f.palm(hand,bones['middle_01_'+side],bones['index_01_'+side],bones['pinky_01_'+side],'hand_'+side)
  else:f.ellipsoid(palm,(.044,.039,.072),'Skin','hand_'+side)
  for finger in ['thumb','index','middle','ring','pinky']:
   names=[finger+'_'+str(i).zfill(2)+'_'+side for i in (1,2,3)]
   if not all(n in bones for n in names):continue
   pts=[bones[n] for n in names]
   for j in range(2):f.tube([pts[j],pts[j+1]],[.010,.009],'Skin',{names[j]:1},8)
   end=pts[2]+(pts[2]-pts[1])*.7;f.tube([pts[2],end],[.009,.006],'Skin',{names[2]:1},8)
 # Neck, sculpted head profile, ears and simple facial landmarks.
 f.tube([(0,.04,1.52),(0,.04,1.655)],[.046,.048],'Skin',{'neck_01':1})
 head_rows=[(1.625,.046,.05,.027),(1.65,.063,.065,.022),(1.69,.081,.078,.026),(1.745,.085,.081,.028),(1.79,.079,.079,.033),(1.825,.054,.055,.034),(1.84,.001,.001,.034)]
 if kind=='Reed':head_rows=[(1.625,.042,.047,.018),(1.642,.06,.055,.014),(1.665,.074,.068,.021),(1.70,.079,.075,.024),(1.735,.08,.076,.027),(1.77,.078,.075,.03),(1.80,.073,.07,.033),(1.825,.052,.05,.034),(1.84,.001,.001,.034)]
 f.torso(head_rows,'Skin',weights={'head':1},n=40 if kind=='Reed' else 28)
 for s in (-1,1):
  f.ellipsoid((s*.085,.027,1.735),(.014,.016,.03),'Skin','head')
  f.ellipsoid((s*.032,-.043,1.746),(.016,.005,.0045) if kind=='Reed' else (.018,.01,.008),'EyeWhite','head')
  f.ellipsoid((s*.032,-.048,1.746),(.004,.002,.004) if kind=='Reed' else (.006,.003,.006),'Iris','head')
  f.ellipsoid((s*.032,-.044,1.761),(.022,.004,.0035) if kind=='Reed' else (.023,.012,.006),'Hair','head')
 f.ellipsoid((0,-.059,1.724),(.013,.025,.023) if kind=='Reed' else (.018,.032,.026),'Skin','head')
 f.ellipsoid((0,-.055,1.684),(.027,.01,.004),'Mouth','head')
 if kind=='Reed':
  # A broad moustache follows the upper lip instead of two round lobes.
  f.ellipsoid((0,-.055,1.702),(.030,.005,.007),'Hair','head',20,8)
 else:f.ellipsoid((0,-.059,1.702),(.034,.012,.011),'Hair','head')
 f.torso([(1.785,.081,.081,.035),(1.82,.063,.064,.035),(1.846,.001,.001,.035)],'Hair',weights={'head':1})
 if kind=='Reed':
  f.torso([(1.812,.153,.178,.035),(1.824,.153,.178,.035),(1.825,.085,.098,.035),(1.914,.072,.08,.035),(1.95,.04,.065,.035),(1.951,.001,.001,.035)],'HatFelt',weights={'head':1})
  f.torso([(1.827,.086,.099,.035),(1.851,.083,.096,.035)],'HatBand',weights={'head':1})
 ob=f.finish();ob.hide_set(True)
if '--definitions-only' not in sys.argv:
 bpy.ops.wm.save_as_mainfile(filepath=str(OUT/('ReedAppearance.blend' if reed_only else 'PeriodCharacters.blend')))
 print('CL_CHARACTERS_AUTHORED')
