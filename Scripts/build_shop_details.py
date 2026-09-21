"""Immutable shop glazing, worn dust, and Reed-only material revision."""
import unreal
T=unreal.AssetToolsHelpers.get_asset_tools();L=unreal.MaterialEditingLibrary
folder='/Game/Art/Town/ShopDetails'
def make(name,path=folder):
 if unreal.EditorAssetLibrary.does_asset_exist(path+'/'+name): return None
 return T.create_asset(name,path,unreal.Material,unreal.MaterialFactoryNew())
def constant(mat,value,prop):
 cls=unreal.MaterialExpressionConstant3Vector if isinstance(value,tuple) else unreal.MaterialExpressionConstant
 e=L.create_material_expression(mat,cls)
 e.set_editor_property('constant' if isinstance(value,tuple) else 'r',unreal.LinearColor(*value,1) if isinstance(value,tuple) else value)
 L.connect_material_property(e,'',prop)
def save(m):
 L.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m,only_if_is_dirty=False)
m=make('M_DisplayGlass')
if m:
 m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
 m.set_editor_property('two_sided',True)
 constant(m,(.12,.16,.17),unreal.MaterialProperty.MP_BASE_COLOR)
 constant(m,.14,unreal.MaterialProperty.MP_OPACITY)
 constant(m,.16,unreal.MaterialProperty.MP_ROUGHNESS)
 save(m)
m=make('M_WornRoadV2')
if m:
 m.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
 uv=L.create_material_expression(m,unreal.MaterialExpressionTextureCoordinate)
 pos=L.create_material_expression(m,unreal.MaterialExpressionWorldPosition)
 for prop,code,output in [
 (unreal.MaterialProperty.MP_BASE_COLOR,'''struct DirtNoise {
 float hash(float2 v) {return frac(sin(dot(v,float2(127.1,311.7)))*43758.5453);}
 float value(float2 v) {
  float2 i=floor(v),f=frac(v);f=f*f*(3-2*f);
  return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);
 }
};
DirtNoise n;
float wear=(n.value(P.xy*.008)-.5)*.035+(n.value(P.xy*.045)-.5)*.012;
return float3(.38,.29,.18)+wear;''',unreal.CustomMaterialOutputType.CMOT_FLOAT3),
 (unreal.MaterialProperty.MP_OPACITY_MASK,'''float2 edge=min(U,1-U);
 float ragged=(sin(P.x*.07)*sin(P.y*.043)+sin(P.y*.19)*.3)*.016;
 return smoothstep(.005,.065,min(edge.x,edge.y)+ragged);''',unreal.CustomMaterialOutputType.CMOT_FLOAT1)]:
  c=L.create_material_expression(m,unreal.MaterialExpressionCustom);c.set_editor_property('code',code);c.set_editor_property('output_type',output)
  inputs=[]
  for key in ['P','U']:
   item=unreal.CustomInput();item.set_editor_property('input_name',key);inputs.append(item)
  c.set_editor_property('inputs',inputs);L.connect_material_expressions(pos,'',c,'P');L.connect_material_expressions(uv,'',c,'U');L.connect_material_property(c,'',prop)
 constant(m,.96,unreal.MaterialProperty.MP_ROUGHNESS);save(m)
m=make('M_DisplayBacking')
if m:
 constant(m,(.32,.285,.22),unreal.MaterialProperty.MP_BASE_COLOR)
 constant(m,.94,unreal.MaterialProperty.MP_ROUGHNESS);save(m)
# Dedicated Reed materials avoid changing every civilian who shares generic clothing.
colors={'OliveWool':(.12,.115,.075),'Shirt':(.36,.30,.20),'Trouser':(.075,.061,.043),'Leather':(.061,.035,.019),'Skin':(.43,.255,.16),'HatFelt':(.18,.13,.073),'Brass':(.38,.37,.32)}
mesh=unreal.load_asset('/Game/Art/Characters/SK_Reed_Period')
slots=list(mesh.materials)
for key,color in colors.items():
 path='/Game/Art/Characters/Reed';name='M_Reed_'+key
 m=make(name,path)
 if m:
  constant(m,color,unreal.MaterialProperty.MP_BASE_COLOR)
  constant(m,.5 if key in ('Leather','Skin') else .35 if key=='Brass' else .92,unreal.MaterialProperty.MP_ROUGHNESS)
  if key=='Brass':constant(m,.75,unreal.MaterialProperty.MP_METALLIC)
  L.set_material_usage(m,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH);save(m)
 m=unreal.load_asset(path+'/'+name)
 for i,slot in enumerate(slots):
  if str(slot.material_slot_name).split('.')[0]==key:slot.material_interface=m;slots[i]=slot
mesh.materials=slots;unreal.EditorAssetLibrary.save_loaded_asset(mesh,only_if_is_dirty=False)
unreal.log('CL_SHOP_DETAILS_SUCCESS')
