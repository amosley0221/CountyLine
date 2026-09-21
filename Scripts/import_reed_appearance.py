import unreal
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
mats={}
for source in [ROOT/'SourceAssets/Authored/Characters/SK_Reed_Period.fbx']:
 skeletal=source.name.startswith('SK_')
 folder='/Game/Art/Characters' if skeletal else '/Game/Art/BendLateral'
 task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=folder
 task.destination_name=source.stem;task.automated=True;task.replace_existing=True;task.save=True
 task.factory=unreal.FbxFactory()
 opts=unreal.FbxImportUI();opts.automated_import_should_detect_type=False
 opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_animations=False
 opts.import_as_skeletal=skeletal
 opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else unreal.FBXImportType.FBXIT_STATIC_MESH
 if skeletal:
  opts.skeleton=unreal.load_asset('/Game/Mannequin/Character/Mesh/SK_Mannequin').get_editor_property('skeleton')
  assert opts.skeleton,'Existing animation skeleton missing'
  opts.create_physics_asset=False
  opts.skeletal_mesh_import_data.set_editor_property('update_skeleton_reference_pose',False)
  opts.skeletal_mesh_import_data.set_editor_property('use_t0_as_ref_pose',False)
 else:
  opts.static_mesh_import_data.combine_meshes=True
  opts.static_mesh_import_data.auto_generate_collision=False
 task.options=opts;TOOLS.import_asset_tasks([task])
 mesh=unreal.load_asset(folder+'/'+source.stem)
 assert mesh,'Failed to import '+source.name
 if skeletal:
  slots=list(mesh.materials)
  for i,slot in enumerate(slots):
   key=str(slot.get_editor_property('material_slot_name')).split('.')[0]
   if key in mats:
    unreal.MaterialEditingLibrary.set_material_usage(mats[key],unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
    unreal.EditorAssetLibrary.save_loaded_asset(mats[key])
    slot.set_editor_property('material_interface',mats[key]);slots[i]=slot
  mesh.materials=slots
  assert mesh.get_editor_property('skeleton')==opts.skeleton,'Import did not preserve the animation skeleton'
 else:
  for i,slot in enumerate(mesh.get_editor_property('static_materials')):
   key=str(slot.get_editor_property('material_slot_name')).split('.')[0]
   if key in mats:mesh.set_material(i,mats[key])
 unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
 unreal.log('CL_ART_IMPORTED '+mesh.get_path_name())
unreal.log('CL_VISUAL_IMPORT_SUCCESS')
