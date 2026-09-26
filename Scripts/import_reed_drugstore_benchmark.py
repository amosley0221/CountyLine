"""Import only the new visual benchmark; preserve map, skeleton and other cast."""
import unreal, json
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();T=unreal.AssetToolsHelpers.get_asset_tools();L=unreal.MaterialEditingLibrary
folder='/Game/Art/Characters/ReedBenchmark'
textures={}
for name in ('T_Reed_BaseColor','T_Reed_ORM'):
 task=unreal.AssetImportTask();task.filename=str(root/'SourceAssets/Authored/ReedBenchmark'/(name+'.png'));task.destination_path=folder;task.automated=True;task.replace_existing=True;task.save=True;T.import_asset_tasks([task])
 texture=unreal.load_asset(folder+'/'+name);assert texture,name
 texture.set_editor_property('srgb',name.endswith('BaseColor'))
 if name.endswith('ORM'):texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_MASKS)
 unreal.EditorAssetLibrary.save_loaded_asset(texture);textures[name]=texture
path=folder+'/M_ReedAtlasV1'
mat=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if not mat:
 mat=T.create_asset('M_ReedAtlasV1',folder,unreal.Material,unreal.MaterialFactoryNew());mat.set_editor_property('two_sided',True)
 for name,props in [('T_Reed_BaseColor',[('RGB',unreal.MaterialProperty.MP_BASE_COLOR)]),('T_Reed_ORM',[('G',unreal.MaterialProperty.MP_ROUGHNESS),('B',unreal.MaterialProperty.MP_METALLIC)])]:
  e=L.create_material_expression(mat,unreal.MaterialExpressionTextureSample);e.set_editor_property('texture',textures[name])
  if name.endswith('ORM'):e.set_editor_property('sampler_type',unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
  for output,prop in props:assert L.connect_material_property(e,output,prop)
 L.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH);L.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
task=unreal.AssetImportTask();task.filename=str(root/'SourceAssets/Authored/ReedBenchmark/SK_Reed_Benchmark.fbx');task.destination_path='/Game/Art/Characters';task.destination_name='SK_Reed_Benchmark';task.automated=True;task.replace_existing=True;task.save=True;task.factory=unreal.FbxFactory()
opts=unreal.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_animations=False;opts.import_as_skeletal=True;opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH
opts.skeleton=unreal.load_asset('/Game/Mannequin/Character/Mesh/SK_Mannequin').get_editor_property('skeleton');opts.create_physics_asset=False
opts.skeletal_mesh_import_data.set_editor_property('update_skeleton_reference_pose',False);opts.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose',False)
opts.skeletal_mesh_import_data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
task.options=opts;T.import_asset_tasks([task]);mesh=unreal.load_asset('/Game/Art/Characters/SK_Reed_Benchmark');assert mesh
slots=list(mesh.materials)
for i,slot in enumerate(slots):slot.material_interface=mat;slots[i]=slot
mesh.materials=slots
assert mesh.get_editor_property('skeleton')==opts.skeleton
sub=unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
assert sub.regenerate_lod(mesh,3,False,False),'Could not generate skeletal LODs'
unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
unreal.log('CL_REED_BENCHMARK_LODS '+str([(i,sub.get_num_verts(mesh,i),sub.get_num_sections(mesh,i)) for i in range(sub.get_lod_count(mesh))]))

folder='/Game/Art/Town/DrugstoreBenchmark'
colors={'Sage':(.095,.16,.12),'Stone':(.43,.40,.33),'Oak':(.19,.125,.064),'Iron':(.045,.052,.047),'Brass':(.38,.29,.13),'Amber':(.14,.063,.019),'Paper':(.63,.58,.46),'Glass':(.045,.085,.095)}
mats={}
for name,color in colors.items():
 path=folder+'/M_Drugstore'+name;m=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
 if not m:
  m=T.create_asset('M_Drugstore'+name,folder,unreal.Material,unreal.MaterialFactoryNew())
  c=L.create_material_expression(m,unreal.MaterialExpressionConstant3Vector);c.set_editor_property('constant',unreal.LinearColor(*color,1));L.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
  r=L.create_material_expression(m,unreal.MaterialExpressionConstant);r.set_editor_property('r',.27 if name in ('Amber','Glass') else .6 if name in ('Iron','Brass') else .83);L.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
  L.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
 mats[name]=m
for name in ('SM_DrugstoreJoinery','SM_DrugstoreStock'):
 task=unreal.AssetImportTask();task.filename=str(root/'SourceAssets/Authored/DrugstoreBenchmark'/(name+'.fbx'));task.destination_path=folder;task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True;task.factory=unreal.FbxFactory()
 opts=unreal.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_animations=False;opts.import_as_skeletal=False;opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH;opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False
 task.options=opts;T.import_asset_tasks([task]);mesh=unreal.load_asset(folder+'/'+name);assert mesh
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):mesh.set_material(i,mats[str(slot.material_slot_name).split('.')[0].split('_')[0]])
 unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
unreal.log('CL_REED_DRUGSTORE_BENCHMARK_IMPORTED')

import runpy
runpy.run_path(str(root/'Scripts/configure_benchmark_lods.py'))
