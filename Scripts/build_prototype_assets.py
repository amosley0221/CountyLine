"""Run with UE's Python commandlet after compiling the CountyLine module.

Creates only prototype-owned materials and L_JailOffice. Re-running deliberately
rebuilds that generated map; place authored future levels under different names.
"""
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
colors = {
    "Wood": (0.14, 0.095, 0.055), "Ink": (0.025, 0.022, 0.018),
    "Paper": (0.86, 0.77, 0.60), "Plaster": (0.42, 0.41, 0.34),
    "Brick": (0.24, 0.11, 0.08), "Brass": (0.40, 0.29, 0.12),
    "Iron": (0.055, 0.063, 0.067), "Ledger": (0.13, 0.18, 0.12),
    "Glass": (0.09, 0.16, 0.20), "Light": (1.0, 0.86, 0.58),
}
for name, rgb in colors.items():
    path = "/Game/Prototype/Materials/M_" + name
    mat = unreal.load_asset(path)
    if not mat:
        mat = tools.create_asset("M_" + name, "/Game/Prototype/Materials", unreal.Material, unreal.MaterialFactoryNew())
        color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector)
        color.set_editor_property("constant", unreal.LinearColor(*rgb, 1.0))
        unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
        rough = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant)
        rough.set_editor_property("r", 0.85)
        unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
        if name in ("Light", "Glass"):
            unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mat)

level = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level.new_level("/Game/Maps/L_JailOffice")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
office_class = unreal.load_class(None, "/Script/CountyLine.CLJailOffice")
office = actors.spawn_actor_from_class(office_class, unreal.Vector(0, 0, 0))
office.set_actor_label("Jail office — generated graybox")
start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-290, -110, 100))
start.set_actor_label("Reed — office entrance")
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/CountyLine.CLPrototypeGameMode"))
level.save_current_level()
unreal.log("CL_ASSETS_SUCCESS: L_JailOffice saved")
