"""Import only authored visual-pass assets; never regenerate gameplay maps."""
import unreal
from pathlib import Path

ROOT=Path(unreal.Paths.project_dir()).resolve()
TOOLS=unreal.AssetToolsHelpers.get_asset_tools()
COLORS={
 'Soil':(.36,.27,.16), 'RoadDust':(.52,.42,.28), 'Rut':(.29,.22,.13),
 'Clay':(.27,.21,.12), 'Water':(.075,.105,.074), 'Timber':(.21,.15,.086),
 'Endgrain':(.31,.24,.14), 'Grass':(.30,.30,.13), 'DryGrass':(.49,.40,.20),
 'Leaf':(.18,.24,.09), 'LeafLight':(.31,.35,.14), 'Rock':(.35,.32,.24),
 'BottleGlass':(.052,.095,.041), 'PaperLabel':(.60,.54,.38), 'Iron':(.08,.07,.05),
 'OliveWool':(.115,.13,.073),'BrownWool':(.14,.086,.044),'Waistcoat':(.105,.067,.037),
 'Shirt':(.51,.44,.29),'Linen':(.66,.62,.48),'Trouser':(.09,.075,.048),
 'Leather':(.048,.03,.017),'Skin':(.43,.255,.145),'Hair':(.045,.033,.024),
 'HatFelt':(.22,.17,.10),'HatBand':(.074,.049,.025),'Brass':(.41,.31,.13),
 'EyeWhite':(.54,.49,.40),'Iris':(.04,.03,.02),'Mouth':(.19,.079,.044),
}
mats={}
for name,color in COLORS.items():
 path='/Game/Art/Materials/M_CL_'+name
 mat=unreal.load_asset(path)
 if mat:
  mats[name]=mat
  continue
 if not mat:mat=TOOLS.create_asset('M_CL_'+name,'/Game/Art/Materials',unreal.Material,unreal.MaterialFactoryNew())
 unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
 mat.set_editor_property('two_sided',True)
 constant=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector)
 constant.set_editor_property('constant',unreal.LinearColor(*color,1))
 noise=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionNoise)
 noise.set_editor_property('scale',0.18 if name in ['OliveWool','BrownWool','Waistcoat','Shirt','Trouser'] else .04)
 noise.set_editor_property('quality',1);noise.set_editor_property('levels',2);noise.set_editor_property('output_min',.70);noise.set_editor_property('output_max',1.08)
 multiply=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionMultiply)
 unreal.MaterialEditingLibrary.connect_material_expressions(constant,'',multiply,'A')
 unreal.MaterialEditingLibrary.connect_material_expressions(noise,'',multiply,'B')
 unreal.MaterialEditingLibrary.connect_material_property(multiply,'',unreal.MaterialProperty.MP_BASE_COLOR)
 rough=unreal.MaterialEditingLibrary.create_material_expression(mat,unreal.MaterialExpressionConstant)
 rough.set_editor_property('r',.22 if name in ['Water','BottleGlass'] else .48 if name in ['Leather','Brass','EyeWhite','Iris'] else .94)
 unreal.MaterialEditingLibrary.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
 unreal.MaterialEditingLibrary.recompile_material(mat)
 unreal.EditorAssetLibrary.save_loaded_asset(mat)
 mats[name]=mat

for source in sorted((ROOT/'SourceAssets'/'Authored').rglob('*.fbx')):
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
