"""Opaque, inexpensive shop-specific paint finishes in world centimetres."""
import unreal
T=unreal.AssetToolsHelpers.get_asset_tools();L=unreal.MaterialEditingLibrary
for name,color in {'Sage':(.095,.16,.12),'Ochre':(.27,.18,.075),'Oxide':(.23,.08,.045),'Slate':(.085,.12,.15),'Umber':(.12,.085,.054)}.items():
 path='/Game/Art/Town/ShopDetails';asset='M_ShopPaint'+name
 if unreal.EditorAssetLibrary.does_asset_exist(path+'/'+asset):continue
 m=T.create_asset(asset,path,unreal.Material,unreal.MaterialFactoryNew())
 p=L.create_material_expression(m,unreal.MaterialExpressionWorldPosition)
 n=L.create_material_expression(m,unreal.MaterialExpressionVertexNormalWS)
 c=L.create_material_expression(m,unreal.MaterialExpressionCustom)
 c.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
 c.set_editor_property('code','''float2 uv=abs(N.x)>abs(N.y)?P.yz:P.xz;
 float grain=sin(uv.x*.13+sin(uv.y*.45))*.012;
 float weather=sin(uv.x*.008)*sin(uv.y*.016)*.018;
 return max(float3(%s)+grain+weather,.005);'''%(','.join(map(str,color))))
 inputs=[]
 for key in ('P','N'):
  i=unreal.CustomInput();i.set_editor_property('input_name',key);inputs.append(i)
 c.set_editor_property('inputs',inputs);L.connect_material_expressions(p,'',c,'P');L.connect_material_expressions(n,'',c,'N');L.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
 r=L.create_material_expression(m,unreal.MaterialExpressionConstant);r.set_editor_property('r',.78);L.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
 L.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
unreal.log('CL_SHOP_PAINTS_SUCCESS')
