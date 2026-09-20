"""Create original, centimetre-scaled procedural town materials; no map edits."""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.MaterialEditingLibrary
folder = '/Game/Art/Town/Materials'
projection = '''
float3 n = abs(N);
float2 uv = n.z > 0.6 ? P.xy : (n.y > n.x ? P.xz : P.yz);
'''
patterns = {
    'Siding': ('''
float edge = min(frac(uv.y/18),1-frac(uv.y/18));
float joint = 1-smoothstep(.01,.045,edge);
float wear = sin(uv.x*.11)*sin(uv.y*.9)*.008;
return lerp(float3(.46,.445,.37)+wear,float3(.19,.18,.145),joint*.55);
''', .9),
    'Brick': ('''
float row = floor(uv.y / 7.5);
float2 cell = float2(uv.x / 25.0 + fmod(abs(row),2.0)*0.5, uv.y / 7.5);
float2 edge = min(frac(cell),1-frac(cell));
float2 aa = max(fwidth(cell),0.002);
float joint = 1-smoothstep(0.028-aa.x,0.028+aa.x,edge.x)*smoothstep(0.065-aa.y,0.065+aa.y,edge.y);
float grain = frac(sin(dot(floor(cell),float2(12.9898,78.233)))*43758.5453);
float3 brick = lerp(float3(.19,.067,.037),float3(.32,.135,.072),grain);
return lerp(brick,float3(.25,.225,.175),joint*.85);
''', .9),
    'Wood': ('''
float board = floor(uv.y/16.0);
float grain = sin(uv.x*.16+sin(uv.y*.75)*2)*.012;
float shade = frac(sin(board*37.3)*43758.5);
float joint = 1-smoothstep(.015,.055,min(frac(uv.y/16),1-frac(uv.y/16)));
return lerp(float3(.16,.105,.055)+shade*.035+grain,float3(.045,.029,.015),joint*.7);
''', .88),
    'Metal': ('''
float seam = 1-smoothstep(.008,.035,min(frac(uv.x/45),1-frac(uv.x/45)));
float weather = sin(uv.y*.045)*sin(uv.x*.012)*.012;
return float3(.065,.071,.065)+weather+seam*.035;
''', .68),
    'Stone': ('''
float row = floor(uv.y/28);
float2 c = float2(uv.x/65+fmod(abs(row),2)*.5,uv.y/28);
float2 e = min(frac(c),1-frac(c));
float joint = 1-smoothstep(.012,.032,e.x)*smoothstep(.018,.045,e.y);
float fleck=sin(uv.x*1.5)*sin(uv.y*1.7)*.012;
return lerp(float3(.47,.43,.34)+fleck,float3(.24,.22,.18),joint*.55);
''', .92),
    'Glass': ('return float3(.055,.095,.11);', .23),
}
for name, (code, roughness) in patterns.items():
    asset = 'M_Town'+name
    mat = unreal.load_asset(folder+'/'+asset) if unreal.EditorAssetLibrary.does_asset_exist(folder+'/'+asset) else None
    if not mat:
        mat = tools.create_asset(asset,folder,unreal.Material,unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    pos = lib.create_material_expression(mat,unreal.MaterialExpressionWorldPosition)
    normal = lib.create_material_expression(mat,unreal.MaterialExpressionVertexNormalWS)
    custom = lib.create_material_expression(mat,unreal.MaterialExpressionCustom)
    custom.set_editor_property('code',projection+code)
    custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs=[]
    for key in ('P','N'):
        item=unreal.CustomInput()
        item.set_editor_property('input_name',key)
        inputs.append(item)
    custom.set_editor_property('inputs',inputs)
    assert lib.connect_material_expressions(pos,'',custom,'P')
    assert lib.connect_material_expressions(normal,'',custom,'N')
    assert lib.connect_material_property(custom,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough = lib.create_material_expression(mat,unreal.MaterialExpressionConstant)
    rough.set_editor_property('r',roughness)
    lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
unreal.log('CL_TOWN_MATERIALS_SUCCESS')
