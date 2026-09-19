"""Read-only content checks using the Unreal Python commandlet."""
import unreal

blend = unreal.load_asset('/Game/Mannequin/Animations/ThirdPerson_IdleRun_2D')
for index, parameter in enumerate(blend.get_editor_property('blend_parameters')):
    unreal.log('CL_BLEND_AXIS %s name=%s min=%s max=%s' % (index, parameter.get_editor_property('display_name'), parameter.get_editor_property('min'), parameter.get_editor_property('max')))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Maps/L_JailOffice')
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
count = 0
for actor in actors:
    if actor.get_class().get_name() == 'CLJailOffice':
        for part in actor.get_components_by_class(unreal.StaticMeshComponent):
            assert part.get_editor_property('static_mesh'), 'Missing mesh: ' + part.get_name()
            assert part.get_material(0), 'Missing material: ' + part.get_name()
            count += 1
assert count > 80, 'Office geometry was not generated'
unreal.log('CL_CONTENT_PASS: %d mesh components with materials' % count)
