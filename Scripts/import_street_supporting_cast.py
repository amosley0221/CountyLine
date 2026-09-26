import runpy
from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()/'Scripts'
for script in ('build_shop_paints.py','import_supporting_cast.py'):
 runpy.run_path(str(root/script))
unreal.log('CL_STREET_SUPPORTING_CAST_IMPORTED')
