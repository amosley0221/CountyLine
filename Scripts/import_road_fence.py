"""Update the authored Bend entrance fence and approach ground only."""
import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
for asset_name in ("SM_BendFence", "SM_BendGround"):
    task=unreal.AssetImportTask()
    task.filename=str(root/('SourceAssets/Authored/BendLateral/'+asset_name+'.fbx'))
    task.destination_path='/Game/Art/BendLateral'
    task.destination_name=asset_name
    task.automated=True;task.replace_existing=True;task.save=True
    task.factory=unreal.FbxFactory()
    opts=unreal.FbxImportUI();opts.automated_import_should_detect_type=False
    opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_animations=False
    opts.import_as_skeletal=False;opts.mesh_type_to_import=unreal.FBXImportType.FBXIT_STATIC_MESH
    opts.static_mesh_import_data.combine_meshes=True;opts.static_mesh_import_data.auto_generate_collision=False
    task.options=opts
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh=unreal.load_asset('/Game/Art/BendLateral/'+asset_name)
    assert mesh
    for i,slot in enumerate(mesh.get_editor_property('static_materials')):
        name=str(slot.get_editor_property('material_slot_name')).split('.')[0]
        material=unreal.load_asset('/Game/Art/Materials/M_CL_'+name)
        assert material, name
        mesh.set_material(i,material)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
    unreal.log('CL_ROAD_FENCE_IMPORTED')
