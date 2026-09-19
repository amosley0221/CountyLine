"""Export the project's licensed template rig for original clothing authoring."""
import unreal
from pathlib import Path

root = Path(unreal.Paths.project_dir()).resolve()
out = root / 'Saved' / 'ArtAuthoring'
out.mkdir(parents=True, exist_ok=True)
task = unreal.AssetExportTask()
task.object = unreal.load_asset('/Game/Mannequin/Character/Mesh/SK_Mannequin')
task.filename = str(out / 'MannequinRig.fbx')
task.automated = True
task.prompt = False
task.replace_identical = True
task.exporter = unreal.SkeletalMeshExporterFBX()
task.options = unreal.FbxExportOption()
task.options.ascii = False
task.options.level_of_detail = False
assert unreal.Exporter.run_asset_export_task(task), 'Rig export failed'
unreal.log('CL_RIG_EXPORT_SUCCESS')
