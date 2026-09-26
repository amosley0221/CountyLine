import unreal
from pathlib import Path
ROOT=Path(unreal.Paths.project_dir()).resolve()
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
mats={}
# Dedicated supporting-cast materials; no changes to Reed or Salazar.
colors={'OliveWool':(.12,.13,.075),'BrownWool':(.15,.095,.05),'Waistcoat':(.09,.065,.045),'Shirt':(.38,.32,.22),'Linen':(.49,.46,.37),'Trouser':(.095,.083,.06),'Leather':(.065,.037,.021),'Skin':(.43,.265,.17),'Hair':(.043,.028,.019),'HatFelt':(.18,.15,.10),'HatBand':(.067,.046,.026),'Brass':(.4,.32,.16),'EyeWhite':(.52,.48,.4),'Iris':(.035,.028,.02),'Mouth':(.18,.075,.043)}
L=unreal.MaterialEditingLibrary
for name,color in colors.items():
 path='/Game/Art/Characters/Supporting';asset='M_Supporting_'+name
 mat=unreal.load_asset(path+'/'+asset) if unreal.EditorAssetLibrary.does_asset_exist(path+'/'+asset) else None
 if not mat:
  mat=TOOLS.create_asset(asset,path,unreal.Material,unreal.MaterialFactoryNew())
  mat.set_editor_property('two_sided',True)
  c=L.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector);c.set_editor_property('constant',unreal.LinearColor(*color,1));L.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
  r=L.create_material_expression(mat,unreal.MaterialExpressionConstant);r.set_editor_property('r',.48 if name in ('Skin','Leather','Brass') else .88);L.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
  L.set_material_usage(mat,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH);L.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat)
 mats[name]=mat
for source in [ROOT/('SourceAssets/Authored/Characters/SK_'+name+'_Period.fbx') for name in ('Pruitt','NorthLaneResident')]:
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
