"""Import original field motion/sound; preserve the gameplay map and base rig."""
from pathlib import Path
import unreal
root = Path(unreal.Paths.project_dir()).resolve()
tools = unreal.AssetToolsHelpers.get_asset_tools()
skeleton = unreal.load_asset('/Game/Mannequin/Character/Mesh/SK_Mannequin').get_editor_property('skeleton')
for source in sorted((root/'SourceAssets/Authored/Motion').glob('*.fbx')):
    task = unreal.AssetImportTask()
    task.filename = str(source); task.destination_path = '/Game/Art/Animations'
    task.destination_name = source.stem; task.automated = True; task.replace_existing = True; task.save = True
    task.factory = unreal.FbxFactory()
    opts = unreal.FbxImportUI()
    opts.automated_import_should_detect_type = False
    opts.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    opts.import_mesh = False; opts.import_animations = True; opts.skeleton = skeleton
    task.options = opts
    tools.import_asset_tasks([task])
    anim = unreal.load_asset(task.destination_path+'/'+source.stem)
    assert isinstance(anim,unreal.AnimSequence), source.name
    # Animation-only FBX promotes the exported rig object to a root track with
    # a 100x unit scale. These original clips are in-place; the root must remain
    # identity while the already-centimetre child tracks remain untouched.
    count = unreal.AnimationLibrary.get_num_keys(anim)
    controller = anim.get_editor_property('controller')
    assert controller.set_bone_track_keys('root', [unreal.Vector(0,0,0)]*count,
        [unreal.Quat(0,0,0,1)]*count, [unreal.Vector(1,1,1)]*count)
    unreal.EditorAssetLibrary.save_loaded_asset(anim, only_if_is_dirty=False)
idle = unreal.load_asset('/Game/Art/Animations/A_FieldIdle')
path = '/Game/Art/Animations/BS_Reed_FieldLocomotion'
blend = unreal.load_asset(path) or unreal.EditorAssetLibrary.duplicate_asset('/Game/Mannequin/Animations/ThirdPerson_IdleRun_2D',path)
samples = list(blend.get_editor_property('sample_data'))
replaced = 0
for i,sample in enumerate(samples):
    if abs(sample.get_editor_property('sample_value').x)<1:
        sample.set_editor_property('animation',idle); samples[i]=sample; replaced+=1
assert replaced, 'No idle sample found'
blend.set_editor_property('sample_data',samples)
unreal.EditorAssetLibrary.save_loaded_asset(blend, only_if_is_dirty=False)
for source in sorted((root/'SourceAssets/Authored/Audio').glob('*.wav')):
    task = unreal.AssetImportTask()
    task.filename = str(source); task.destination_path = '/Game/Audio/Field'
    task.destination_name = source.stem; task.automated = True; task.replace_existing = True; task.save = True
    tools.import_asset_tasks([task])
    sound = unreal.load_asset(task.destination_path+'/'+source.stem)
    assert isinstance(sound,unreal.SoundWave), source.name
    sound.set_editor_property('looping',not source.stem.startswith('S_Step'))
    unreal.EditorAssetLibrary.save_loaded_asset(sound,only_if_is_dirty=False)
unreal.log('CL_FIELD_IMMERSION_IMPORT_SUCCESS')
