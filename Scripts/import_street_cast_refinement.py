"""Import the scoped street/character revision without regenerating the map."""
import runpy
from pathlib import Path
import unreal

scripts=Path(unreal.Paths.project_dir()).resolve()/'Scripts'
for name in ('build_street_ground.py','import_reed_appearance.py','build_shop_details.py'):
    runpy.run_path(str(scripts/name))
unreal.log('CL_STREET_CAST_REFINEMENT_IMPORTED')
