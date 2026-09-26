"""Rebuild Reed's garment/skin surfaces and bake an original UV atlas.

Editable source is preserved separately from the earlier primitive prototype.
No downloaded meshes or image textures are used. Run with Blender background.
"""
import bpy, bmesh, math, json
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
from mathutils.geometry import barycentric_transform

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'SourceAssets/Authored/ReedBenchmark';OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'SourceAssets/Authored/Characters/ReedAppearance.blend'))
rig=next(o for o in bpy.context.scene.objects if o.type=='ARMATURE')
source=next(o for o in bpy.context.scene.objects if o.type=='MESH' and o.name.startswith('SK_Reed'))
source.hide_set(False)
# The imported rig has a scaled parent Empty. Preserve its world transform
# before removing helper objects, or its bind pose becomes 100 times too large.
rig_world=rig.matrix_world.copy();rig.parent=None;rig.matrix_world=rig_world
bpy.context.view_layer.update()
for o in list(bpy.context.scene.objects):
 if o not in (source,rig):bpy.data.objects.remove(o,do_unlink=True)
names=[m.name.split('.')[0] for m in source.data.materials]
source.data.calc_loop_triangles()

def select(ob):
 bpy.ops.object.select_all(action='DESELECT');ob.hide_set(False);ob.select_set(True);bpy.context.view_layer.objects.active=ob

def subset(name, predicate):
 faces=[p for p in source.data.polygons if predicate(p)]
 used=sorted({v for p in faces for v in p.vertices});mapping={v:i for i,v in enumerate(used)}
 mesh=bpy.data.meshes.new(name);mesh.from_pydata([source.data.vertices[v].co[:] for v in used],[],[[mapping[v] for v in p.vertices] for p in faces]);mesh.update()
 ob=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(ob)
 for m in source.data.materials:mesh.materials.append(m)
 for p,old in zip(mesh.polygons,faces):p.material_index=old.material_index;p.use_smooth=True
 for group in source.vertex_groups:ob.vertex_groups.new(name=group.name)
 for new,old in enumerate(used):
  for w in source.data.vertices[old].groups:ob.vertex_groups[w.group].add([new],w.weight,'REPLACE')
 return ob

def transfer(ob,reference):
 reference.data.calc_loop_triangles();triangles=[tuple(t.vertices) for t in reference.data.loop_triangles]
 points=[v.co.copy() for v in reference.data.vertices]
 tree=BVHTree.FromPolygons(points,triangles,all_triangles=True)
 weights=[{reference.vertex_groups[w.group].name:w.weight for w in v.groups} for v in reference.data.vertices]
 for group in list(ob.vertex_groups):ob.vertex_groups.remove(group)
 groups={g.name:ob.vertex_groups.new(name=g.name) for g in reference.vertex_groups}
 for vertex in ob.data.vertices:
  near,normal,idx,dist=tree.find_nearest(vertex.co)
  a,b,c=triangles[idx]
  bc=barycentric_transform(near,points[a],points[b],points[c],Vector((1,0,0)),Vector((0,1,0)),Vector((0,0,1)))
  result={}
  for vi,amount in zip((a,b,c),bc):
   for key,value in weights[vi].items():result[key]=result.get(key,0)+max(0,amount)*value
  result=dict(sorted(result.items(),key=lambda kv:kv[1],reverse=True)[:4]);total=sum(result.values())
  for key,value in result.items():
   if total>0 and value>.0001:groups[key].add([vertex.index],value/total,'REPLACE')

def fuse(ob,size,solid=False):
 select(ob)
 if solid:
  mod=ob.modifiers.new('Garment thickness','SOLIDIFY');mod.thickness=.009;mod.offset=0;bpy.ops.object.modifier_apply(modifier=mod.name)
 mod=ob.modifiers.new('Joined surface','REMESH');mod.mode='VOXEL';mod.voxel_size=size;mod.use_smooth_shade=True;bpy.ops.object.modifier_apply(modifier=mod.name)
 mod=ob.modifiers.new('Surface relaxation','SMOOTH');mod.factor=.7;mod.iterations=5;bpy.ops.object.modifier_apply(modifier=mod.name)
 mod=ob.modifiers.new('Surface budget','DECIMATE');mod.ratio=.46;bpy.ops.object.modifier_apply(modifier=mod.name)

# Join coat torso, sleeves and lapels. Transfer deformation weights after topology changes.
coat=subset('ReedCoatReference',lambda p:names[p.material_index]=='OliveWool')
joined=coat.copy();joined.data=coat.data.copy();bpy.context.collection.objects.link(joined);joined.name='ReedJoinedCoat'
fuse(joined,.0035,True);transfer(joined,coat)
# Blend the sleeve roots into the clavicle/torso instead of rigid upper-arm caps.
for v in joined.data.vertices:
 if v.co.z>1.43 and abs(v.co.x)<.215:
  side='l' if v.co.x>0 else 'r';spine=joined.vertex_groups.get('spine_03')
  blend=max(0,min(1,(.215-abs(v.co.x))/.075))
  if blend>0:
   old=[(g.group,g.weight) for g in v.groups]
   for idx,w in old:joined.vertex_groups[idx].add([v.index],w*(1-blend),'REPLACE')
   spine.add([v.index],blend,'ADD')
coat.hide_set(True)

# Retain accessories and lower garments. Replace facial primitives with one surface.
rest=subset('ReedAccessories',lambda p:names[p.material_index] not in ('OliveWool','Skin','EyeWhite','Iris','Mouth') and not (names[p.material_index]=='Hair' and max(source.data.vertices[v].co.z for v in p.vertices)>1.66 and min(source.data.vertices[v].co.z for v in p.vertices)<1.78))
skin=subset('ReedSkinReference',lambda p:names[p.material_index]=='Skin' and max(source.data.vertices[v].co.z for v in p.vertices)<1.625)

# Ring-based face with integrated nose, brow, cheeks and lips; no sphere nose.
verts=[];faces=[];segments=64;rows=48
profile=[(1.625,.041,.046,.018),(1.646,.061,.054,.014),(1.672,.073,.067,.021),(1.706,.079,.074,.025),(1.746,.081,.076,.029),(1.79,.075,.073,.033),(1.818,.06,.058,.034),(1.841,.003,.003,.034)]
def section(z):
 for a,b in zip(profile,profile[1:]):
  if z<=b[0]:
   t=(z-a[0])/(b[0]-a[0]);return tuple(a[j]*(1-t)+b[j]*t for j in (1,2,3))
 return profile[-1][1:]
def gauss(x,z,cx,cz,sx,sz):return math.exp(-((x-cx)/sx)**2-((z-cz)/sz)**2)
for j in range(rows+1):
 z=profile[0][0]+(profile[-1][0]-profile[0][0])*j/rows;rx,ry,cy=section(z)
 for i in range(segments):
  angle=math.tau*i/segments;x=rx*math.cos(angle);y=cy+ry*math.sin(angle);front=max(0,-math.sin(angle))**8
  relief=.025*gauss(x,z,0,1.721,.013,.016)+.010*gauss(x,z,0,1.746,.011,.028)
  relief+=.006*gauss(x,z,0,1.681,.027,.013)
  for s in (-1,1):
   relief-=.006*gauss(x,z,s*.03,1.746,.020,.010)
   relief+=.006*gauss(x,z,s*.03,1.761,.024,.005)
   relief+=.004*gauss(x,z,s*.05,1.716,.02,.015)
  verts.append((x,y-front*relief,z))
for j in range(rows):
 for i in range(segments):
  a=j*segments+i;b=j*segments+(i+1)%segments;faces.append((a,b,b+segments,a+segments))
faces.extend([tuple(reversed(range(segments))),tuple(rows*segments+i for i in range(segments))])
mesh=bpy.data.meshes.new('ReedContinuousFace');mesh.from_pydata(verts,[],faces);mesh.materials.append(source.data.materials[names.index('Skin')]);mesh.update()
head=bpy.data.objects.new('ReedContinuousFace',mesh);bpy.context.collection.objects.link(head)
head.vertex_groups.new(name='head').add(list(range(len(verts))),1,'REPLACE')
for p in mesh.polygons:p.use_smooth=True

def ellipsoid(name,center,scale,material,bone='head',n=24,r=12):
 bpy.ops.mesh.primitive_uv_sphere_add(segments=n,ring_count=r,location=center)
 ob=bpy.context.object;ob.name=name;ob.scale=scale;bpy.ops.object.transform_apply(location=True,rotation=False,scale=True)
 ob.data.materials.append(source.data.materials[names.index(material)]);ob.vertex_groups.new(name=bone).add(list(range(len(ob.data.vertices))),1,'REPLACE')
 for p in ob.data.polygons:p.use_smooth=True
 return ob
ears=[ellipsoid('ReedEar',(s*.079,.026,1.735),(.012,.014,.026),'Skin') for s in (-1,1)]
neck=ellipsoid('ReedNeck',(0,.022,1.60),(.044,.048,.079),'Skin','neck_01')
select(head)
for ob in [skin,neck]+ears:ob.select_set(True)
bpy.ops.object.join();skin=head
# Surface union welds the palm/finger roots, neck and ears where they meet.
reference=skin.copy();reference.data=skin.data.copy();bpy.context.collection.objects.link(reference);reference.name='ReedSkinWeightSource'
fuse(skin,.0025);transfer(skin,reference);reference.hide_set(True)
details=[]
for s in (-1,1):
 details.append(ellipsoid('ReedEye',(s*.03,-.042,1.746),(.014,.005,.0042),'EyeWhite'))
 details.append(ellipsoid('ReedIris',(s*.03,-.047,1.746),(.0035,.0018,.0034),'Iris'))
 details.append(ellipsoid('ReedBrow',(s*.031,-.045,1.761),(.021,.003,.0025),'Hair'))
details.append(ellipsoid('ReedLip',(0,-.060,1.684),(.025,.002,.002),'Mouth'))
details.append(ellipsoid('ReedMoustache',(0,-.060,1.702),(.028,.003,.005),'Hair'))

select(joined)
for ob in [rest,skin]+details:ob.select_set(True)
bpy.ops.object.join();ob=joined;ob.name='SK_Reed_Benchmark'
bm=bmesh.new();bm.from_mesh(ob.data);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(ob.data);bm.free()
select(ob)
ob.data.calc_loop_triangles()
if len(ob.data.loop_triangles)>45000:
 mod=ob.modifiers.new('Runtime triangle budget','DECIMATE');mod.ratio=45000/len(ob.data.loop_triangles);bpy.ops.object.modifier_apply(modifier=mod.name)
for layer in list(ob.data.uv_layers):ob.data.uv_layers.remove(layer)
ob.data.uv_layers.new(name='ReedAtlas')
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(angle_limit=math.radians(65),island_margin=.008);bpy.ops.object.mode_set(mode='OBJECT')

# Bake original material variation into a single atlas, reducing material sections.
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=8;scene.render.bake.margin=8
palette={'OliveWool':(.12,.115,.075),'Shirt':(.36,.30,.20),'Trouser':(.075,.061,.043),'Leather':(.061,.035,.019),'Skin':(.43,.255,.16),'HatFelt':(.18,.13,.073),'Brass':(.38,.37,.32)}
atlas=bpy.data.images.new('T_Reed_BaseColor',width=2048,height=2048,alpha=False)
orm=bpy.data.images.new('T_Reed_ORM',width=2048,height=2048,alpha=False);orm.colorspace_settings.name='Non-Color'
for packed,img in ((False,atlas),(True,orm)):
 for mat in ob.data.materials:
  key=mat.name.split('.')[0];base=palette.get(key,tuple(mat.diffuse_color[:3]));mat.use_nodes=True;nodes=mat.node_tree.nodes;nodes.clear();links=mat.node_tree.links
  out=nodes.new('ShaderNodeOutputMaterial');em=nodes.new('ShaderNodeEmission');links.new(em.outputs[0],out.inputs['Surface'])
  if packed:em.inputs['Color'].default_value=(1,.45 if key in ('Skin','Leather') else .32 if key=='Brass' else .86,.75 if key=='Brass' else 0,1)
  else:
   coord=nodes.new('ShaderNodeTexCoord');noise=nodes.new('ShaderNodeTexNoise');noise.inputs['Scale'].default_value=240 if key not in ('Skin','Hair') else 65;noise.inputs['Detail'].default_value=2;links.new(coord.outputs['Generated'],noise.inputs['Vector'])
   ramp=nodes.new('ShaderNodeValToRGB');ramp.color_ramp.elements[0].color=tuple(c*.87 for c in base)+(1,);ramp.color_ramp.elements[1].color=tuple(c*1.07 for c in base)+(1,);links.new(noise.outputs['Fac'],ramp.inputs[0]);links.new(ramp.outputs[0],em.inputs['Color'])
  target=nodes.new('ShaderNodeTexImage');target.image=img;nodes.active=target
 select(ob)
 bpy.ops.object.bake(type='EMIT')
 img.filepath_raw=str(OUT/(img.name+'.png'));img.file_format='PNG';img.save()
material=bpy.data.materials.new('ReedAtlas');material.use_nodes=True;nodes=material.node_tree.nodes;links=material.node_tree.links
bsdf=nodes.get('Principled BSDF');t=nodes.new('ShaderNodeTexImage');t.image=atlas;links.new(t.outputs['Color'],bsdf.inputs['Base Color'])
t=nodes.new('ShaderNodeTexImage');t.image=orm;split=nodes.new('ShaderNodeSeparateColor');links.new(t.outputs['Color'],split.inputs[0]);links.new(split.outputs[1],bsdf.inputs['Roughness']);links.new(split.outputs[2],bsdf.inputs['Metallic'])
ob.data.materials.clear();ob.data.materials.append(material)
for p in ob.data.polygons:p.material_index=0
# Normalize to at most four influences before attaching the existing rig.
select(ob);bpy.ops.object.vertex_group_limit_total(limit=4);bpy.ops.object.vertex_group_normalize_all(lock_active=False)
mod=ob.modifiers.new('Existing CountyLine rig','ARMATURE');mod.object=rig;ob.parent=rig;ob.matrix_parent_inverse=rig.matrix_world.inverted()
for item in list(bpy.context.scene.objects):
 if item not in (ob,rig):bpy.data.objects.remove(item,do_unlink=True)
select(ob);rig.select_set(True);bpy.context.view_layer.update()
assert 1.5<rig.dimensions.z<2.1 and 1.8<ob.dimensions.z<2.1,('Bind-pose scale mismatch',rig.dimensions[:],ob.dimensions[:])
bpy.ops.export_scene.fbx(filepath=str(OUT/'SK_Reed_Benchmark.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,use_armature_deform_only=False,axis_forward='-Y',axis_up='Z',bake_anim=False,mesh_smooth_type='FACE')
ob.data.calc_loop_triangles();stats={'vertices':len(ob.data.vertices),'triangles':len(ob.data.loop_triangles),'material_slots':len(ob.data.materials),'max_influences':max(len(v.groups) for v in ob.data.vertices),'texture_size':2048}
(OUT/'mesh-stats.json').write_text(json.dumps(stats,indent=2)+'\n')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ReedBenchmark.blend'))
bpy.ops.file.make_paths_relative()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'ReedBenchmark.blend'))
print('CL_REED_BENCHMARK_AUTHORED',stats)
