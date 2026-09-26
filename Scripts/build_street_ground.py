"""Opaque continuous dirt finish for the current authored town and road layout.

World-space road coverage is shared by soil and road meshes, so intersections
and mesh boundaries cannot reveal masked holes or mismatched edge colors.
Create a new revision rather than editing an already loaded material graph.
"""
import unreal

PATH = '/Game/Art/Town/StreetFinishes'
NAME = 'M_CountyGroundV3'
# Center XY and half extent XY in centimetres; includes the four home paths.
ROADS = [
    (-3300,4300,4250,300), (-5650,1750,250,2550),
    (-7250,1750,150,2550), (-1770,2125,115,1350),
    (-2200,4100,310,475), (-2200,-100,310,3850),
    (-4100,-800,3500,300), (-3900,-100,1200,380),
    (-5500,4675,70,300), (-3650,4750,70,300),
    (-1250,4675,70,300), (400,4775,70,300),
    (3800,-800,4900,275), (-850,-350,285,625),
]
if not unreal.EditorAssetLibrary.does_asset_exist(PATH+'/'+NAME):
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME,PATH,unreal.Material,unreal.MaterialFactoryNew())
    lib = unreal.MaterialEditingLibrary
    pos = lib.create_material_expression(mat,unreal.MaterialExpressionWorldPosition)
    code = '''
struct GroundNoise {
 float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
 float value(float2 p) {
  float2 i=floor(p), f=frac(p); f=f*f*(3-2*f);
  return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);
 }
};
GroundNoise n;
float coverage=-100000;
'''
    for x,y,hx,hy in ROADS:
        code += f'coverage=max(coverage,min({hx}.0-abs(P.x-({x}.0)),{hy}.0-abs(P.y-({y}.0))));\n'
    code += '''
float broad=n.value(P.xy*.003);
float grit=n.value(P.xy*.12);
float breakup=(n.value(P.xy*.028)-.5)*18;
float road=smoothstep(-12,48,coverage+breakup);
float3 soil=float3(.29,.225,.145)+(broad-.5)*.045+(grit-.5)*.025;
float3 dust=float3(.365,.285,.185)+(broad-.5)*.027+(grit-.5)*.014;
return lerp(soil,dust,road);
'''
    c=lib.create_material_expression(mat,unreal.MaterialExpressionCustom)
    c.set_editor_property('code',code)
    c.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    item=unreal.CustomInput();item.set_editor_property('input_name','P');c.set_editor_property('inputs',[item])
    lib.connect_material_expressions(pos,'',c,'P')
    lib.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
    rough=lib.create_material_expression(mat,unreal.MaterialExpressionConstant);rough.set_editor_property('r',.96)
    lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
unreal.log('CL_CONTINUOUS_GROUND_SUCCESS')
