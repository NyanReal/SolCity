"""Import the split level-crossing barrier base and animated boom meshes."""

import os

import unreal


CONTENT_DIR = unreal.Paths.project_content_dir()
DESTINATION = "/Game/Art/Props"
ASSET_NAMES = (
    "SM_SolCity_LevelCrossingBarrierBase_01",
    "SM_SolCity_LevelCrossingBarrierBoom_01",
)
SIGNAL_MATERIAL_NAME = "M_Crossing_WarningSignal"


def import_asset(asset_name):
    asset_path = f"{DESTINATION}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"SolCity crossing import: keeping existing {asset_name}")
        return

    source = os.path.join(CONTENT_DIR, "Art", "Props", asset_name + ".fbx")
    if not os.path.isfile(source):
        raise RuntimeError(f"Missing level-crossing FBX: {source}")

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("combine_meshes", True)
    static_data.set_editor_property("generate_lightmap_u_vs", True)
    static_data.set_editor_property("auto_generate_collision", True)

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", False)
    task.set_editor_property("save", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    asset = unreal.load_asset(asset_path)
    if not isinstance(asset, unreal.StaticMesh):
        raise RuntimeError(f"Level-crossing import failed: {asset_name}")
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    bounds = asset.get_bounds()
    unreal.log(f"SolCity crossing import: {asset_name}, extent={bounds.box_extent}, origin={bounds.origin}")


for name in ASSET_NAMES:
    import_asset(name)


def create_signal_material():
    """Create a HISM-compatible green/red lamp driven by custom data float 0."""
    asset_path = f"{DESTINATION}/{SIGNAL_MATERIAL_NAME}"
    material = unreal.EditorAssetLibrary.load_asset(asset_path)
    created_new = material is None
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            SIGNAL_MATERIAL_NAME,
            DESTINATION,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    if not isinstance(material, unreal.Material):
        raise RuntimeError(f"{asset_path} exists but is not a Material")

    material.set_editor_property("used_with_instanced_static_meshes", True)
    if created_new:
        signal_state = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionPerInstanceCustomData, -620, 20
        )
        signal_state.set_editor_property("data_index", 0)
        signal_state.set_editor_property("const_default_value", 0.0)

        green = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant3Vector, -620, -160
        )
        green.set_editor_property("constant", unreal.LinearColor(0.005, 0.32, 0.012, 1.0))
        red = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant3Vector, -620, -70
        )
        red.set_editor_property("constant", unreal.LinearColor(0.42, 0.004, 0.006, 1.0))
        signal_color = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionLinearInterpolate, -330, -70
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(green, "", signal_color, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(red, "", signal_color, "B")
        unreal.MaterialEditingLibrary.connect_material_expressions(signal_state, "", signal_color, "Alpha")

        emission_strength = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -330, 90
        )
        emission_strength.set_editor_property("r", 7.0)
        emissive = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionMultiply, -90, 20
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(signal_color, "", emissive, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(emission_strength, "", emissive, "B")

        roughness = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, -90, 170
        )
        roughness.set_editor_property("r", 0.22)
        unreal.MaterialEditingLibrary.connect_material_property(
            signal_color, "", unreal.MaterialProperty.MP_BASE_COLOR
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
        )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


create_signal_material()

unreal.log("SolCity: imported split level-crossing assets and built the green/red HISM signal material.")
