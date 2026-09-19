"""Original restrained idle and conversation motion on the existing Epic rig."""
import bpy, math
from pathlib import Path
from mathutils import Quaternion, Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'SourceAssets'/'Authored'/'Motion'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(ROOT/'Saved'/'ArtAuthoring'/'MannequinRig.fbx'))
rig = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
for ob in list(bpy.context.scene.objects):
    if ob.type != 'ARMATURE': bpy.data.objects.remove(ob, do_unlink=True)
rig.animation_data_clear()
scene = bpy.context.scene
scene.render.fps = 30
scene.frame_start, scene.frame_end = 1, 181

def world_rotation(name, axis, degrees):
    # Convert a world-space axis into this bone's reference local frame.
    pb = rig.pose.bones[name]
    basis = (rig.matrix_world @ pb.bone.matrix_local).to_quaternion()
    local_axis = basis.inverted() @ Vector(axis)
    pb.rotation_quaternion = Quaternion(local_axis, math.radians(degrees))

for name, speaking in [('A_FieldIdle', False), ('A_SalazarSpeaking', True)]:
    rig.animation_data_create()
    rig.animation_data.action = bpy.data.actions.new(name)
    for frame in range(1, 182, 3):
        scene.frame_set(frame)
        phase = (frame-1)/180 * math.tau
        breath = math.sin(phase)
        gesture = (1-math.cos(phase))*0.5 if speaking else 0
        for pb in rig.pose.bones:
            pb.rotation_mode = 'QUATERNION'
            pb.rotation_quaternion = Quaternion()
            pb.location = (0, 0, 0)
            pb.scale = (1, 1, 1)
        # Lower the reference A-pose arms; keep the hands clear of the coat.
        world_rotation('upperarm_l', (0,1,0), 26 - 4*gesture)
        world_rotation('upperarm_r', (0,1,0), -26 + 3*gesture)
        world_rotation('lowerarm_l', (1,0,0), -6 - 38*gesture)
        world_rotation('lowerarm_r', (1,0,0), -6 - 8*gesture)
        world_rotation('spine_02', (1,0,0), .5*breath)
        world_rotation('spine_03', (0,0,1), .65*breath)
        world_rotation('neck_01', (1,0,0), -1 + (2.5*math.sin(phase*2)*gesture if speaking else .5*breath))
        world_rotation('head', (0,0,1), 2*math.sin(phase)*gesture if speaking else .7*breath)
        for pb in rig.pose.bones:
            pb.keyframe_insert(data_path='rotation_quaternion', frame=frame, group=pb.name)
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')), use_selection=True,
        object_types={'ARMATURE'}, add_leaf_bones=False, bake_anim=True,
        bake_anim_use_all_actions=False, bake_anim_use_nla_strips=False,
        bake_anim_simplify_factor=0, axis_forward='-Z', axis_up='Y')
    rig.animation_data.action.use_fake_user = True
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'FieldMotion.blend'))
print('CL_FIELD_MOTION_AUTHORED')
