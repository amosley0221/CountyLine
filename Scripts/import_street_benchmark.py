"""Import only the original street kit. Does not regenerate maps or character assets."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve();lib=unreal.MaterialEditingLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
folder='/Game/Art/Town/StreetKit'
colors={'Oak':(.20,.135,.078),'Iron':(.055,.062,.059),'Bark':(.16,.135,.10),'Leaf':(.13,.205,.075),'LeafLight':(.25,.32,.12),'LeafGold':(.32,.34,.14),'Canvas':(.64,.58,.43),'CanvasGreen':(.14,.21,.16),'Terracotta':(.34,.16,.09),'Glass':(.19,.24,.20)}
mats={}
for name,color in colors.items():
 if unreal.EditorAssetLibrary.does_asset_exist(folder+'/M_Street'+name):
  mats[name]=unreal.load_asset(folder+'/M_Street'+name);continue
 mat=unreal.load_asset(folder+'/M_Street'+name) if unreal.EditorAssetLibrary.does_asset_exist(folder+'/M_Street'+name) else tools.create_asset('M_Street'+name,folder,unreal.Material,unreal.MaterialFactoryNew())
 lib.delete_all_material_expressions(mat);mat.set_editor_property('two_sided',name.startswith('Leaf') or name.startswith('Canvas'))
 c=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector);c.set_editor_property('constant',unreal.LinearColor(*color,1))
 lib.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
 rough=lib.create_material_expression(mat,unreal.MaterialExpressionConstant);rough.set_editor_property('r',.65 if name=='Iron' else .85)
 lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
 if name.startswith('Leaf'):
  mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
  lib.connect_material_property(c,'',unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
 if name=='Iron':
  metal=lib.create_material_expression(mat,unreal.MaterialExpressionConstant);metal.set_editor_property('r',.65);lib.connect_material_property(metal,'',unreal.MaterialProperty.MP_METALLIC)
 lib.recompile_material(mat);unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False);mats[name]=mat
for source in sorted((root/'SourceAssets/Authored/StreetBenchmark').glob('*.fbx')):
 task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=folder;task.destination_name=source.stem;task.automated=True;task.replace_existing=True;task.save=True;task.factory=unreal.FbxFactory()
 opts=unreal.FbxImportUI();opts.automated_import_should_detect_type=False;opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_animations=False;opts.import_as_skeletal=False;opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
 opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False;task.options=opts;tools.import_asset_tasks([task])
 mesh=unreal.load_asset(folder+'/'+source.stem);assert mesh,source
 bounds=mesh.get_bounds().box_extent
 if source.stem=='SM_StreetBench': assert 85<bounds.x<110,('Bench units',bounds)
 if source.stem=='SM_StreetCottonwood': assert 220<bounds.z<450,('Tree units',bounds)
 for i,slot in enumerate(mesh.get_editor_property('static_materials')):
  key=str(slot.get_editor_property('material_slot_name')).split('.')[0];assert key in mats,key;mesh.set_material(i,mats[key])
 unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
 unreal.log('CL_STREET_IMPORTED '+source.stem)
unreal.log('CL_STREET_IMPORT_SUCCESS')
