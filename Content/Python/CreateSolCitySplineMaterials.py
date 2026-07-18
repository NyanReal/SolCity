"""Create spline-compatible road slot materials and bind them to the road mesh."""

import unreal


DESTINATION = "/Game/Art/Props/SplineMaterials"
ROAD_MESH = "/Game/Art/Props/SM_SolCity_RoadSpline_01"
PALETTE = {
    "M_Road_Asphalt": (0.105, 0.14, 0.18, 0.86),
    "M_Road_Sidewalk": (0.67, 0.68, 0.62, 0.90),
    "M_Road_Curb": (0.92, 0.85, 0.70, 0.78),
    "M_Road_Line": (1.0, 0.82, 0.30, 0.65),
}


def create_material(source_name, values):
    asset_name = source_name.replace("M_Road_", "M_SplineRoad_")
    path = f"{DESTINATION}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, DESTINATION, unreal.Material, unreal.MaterialFactoryNew()
    )
    material.set_editor_property("used_with_spline_meshes", True)
    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -260, 0
    )
    color.set_editor_property("constant", unreal.LinearColor(values[0], values[1], values[2], 1.0))
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -260, 180
    )
    roughness.set_editor_property("r", values[3])
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


mesh = unreal.load_asset(ROAD_MESH)
if not mesh:
    raise RuntimeError("Road spline mesh is missing")
runtime_materials = {name: create_material(name, values) for name, values in PALETTE.items()}
for index, slot in enumerate(mesh.get_editor_property("static_materials")):
    slot_name = str(slot.get_editor_property("material_slot_name"))
    if slot_name in runtime_materials:
        mesh.set_material(index, runtime_materials[slot_name])
unreal.EditorAssetLibrary.save_loaded_asset(mesh)
unreal.log("SolCity: spline-compatible road slot materials assigned.")
