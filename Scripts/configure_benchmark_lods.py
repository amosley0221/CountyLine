"""Configure explicit benchmark LOD budgets; safe to rerun after import."""
import unreal
mesh=unreal.load_asset('/Game/Art/Characters/SK_Reed_Benchmark');assert mesh
# Keep full detail available on Android: forced LOD 1 showed hat-band artifacts
# in the close-up review. Measure on-device before stripping the base level.
minimum=mesh.get_editor_property('min_lod');minimum.set_editor_property('default',0);minimum.set_editor_property('per_platform',{'Android':0});mesh.set_editor_property('min_lod',minimum)
unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
sub=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
for name in ('SM_DrugstoreJoinery','SM_DrugstoreStock'):
 mesh=unreal.load_asset('/Game/Art/Town/DrugstoreBenchmark/'+name);assert mesh
 options=unreal.StaticMeshReductionOptions();options.auto_compute_lod_screen_size=False
 options.reduction_settings=[unreal.StaticMeshReductionSettings(percent_triangles=ratio,screen_size=size) for ratio,size in ((1.,1.),(.5,.35),(.2,.12))]
 assert sub.set_lods(mesh,options)==3,name
 unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
unreal.log('CL_BENCHMARK_LOD_CONFIGURATION_SUCCESS')
