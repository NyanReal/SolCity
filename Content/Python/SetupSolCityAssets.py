"""Import the generated PNG sources and build the four runtime materials.

Run once in Unreal Editor with Tools > Execute Python Script. Re-running is safe:
existing generated textures/materials are replaced in-place.
"""

import os
import importlib.util
import unreal


SOURCE_DIR = os.path.join(unreal.Paths.project_content_dir(), "Art", "Source")
TEXTURE_DIR = "/Game/Art/Generated"
MATERIAL_DIR = "/Game/Art/Materials"
BUILDING_DIR = "/Game/Art/Buildings"

ASSETS = {
    "AnimeGrass": "T_AnimeGrass.png",
    "AnimeAsphalt": "T_AnimeAsphalt.png",
    "AnimeWater": "T_AnimeWater.png",
    "AnimeFacade": "T_AnimeFacade.png",
}

AUTHORED_BUILDINGS = (
    "SM_SolCity_MidRise_01",
    "SM_SolCity_CornerRetail_01",
    "SM_SolCity_SteppedTower_01",
)


def ensure_startup_level():
    level_path = "/Game/Maps/SolCity"
    if unreal.EditorAssetLibrary.does_asset_exist(level_path):
        return
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_editor.new_level(level_path):
        raise RuntimeError(f"Could not create startup level: {level_path}")
    level_editor.save_current_level()


def import_texture(asset_key, filename):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE_DIR, filename))
    task.set_editor_property("destination_path", TEXTURE_DIR)
    task.set_editor_property("destination_name", "T_" + asset_key)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(f"{TEXTURE_DIR}/T_{asset_key}")
    if not texture:
        raise RuntimeError(f"Texture import failed: {filename}")
    texture.set_editor_property("filter", unreal.TextureFilter.TF_DEFAULT)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def import_authored_buildings(replace_existing=True):
    # The FBX contains hundreds of named detail objects. Import them as one
    # renderable building mesh. Incremental mode leaves loaded/editor-owned
    # assets untouched and imports only missing authored buildings.
    for asset_name in AUTHORED_BUILDINGS:
        asset_path = f"{BUILDING_DIR}/{asset_name}"
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            if not replace_existing:
                continue
        source = os.path.join(unreal.Paths.project_content_dir(), "Art", "Buildings", asset_name + ".fbx")
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", source)
        task.set_editor_property("destination_path", BUILDING_DIR)
        task.set_editor_property("destination_name", asset_name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        # Preserve the native UE material library and deterministic slot
        # overrides. Imported FBX materials are legacy authoring data only.
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        static_data = options.get_editor_property("static_mesh_import_data")
        static_data.set_editor_property("combine_meshes", True)
        static_data.set_editor_property("generate_lightmap_u_vs", True)
        task.set_editor_property("options", options)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        mesh = unreal.load_asset(asset_path)
        if not mesh:
            raise RuntimeError(f"Authored building FBX import failed: {asset_name}")
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)


def apply_native_building_materials():
    """Load the dedicated building material setup without relying on sys.path."""
    script_path = os.path.join(
        unreal.Paths.project_content_dir(), "Python", "SetupSolCityBuildingMaterials.py"
    )
    spec = importlib.util.spec_from_file_location("solcity_building_materials", script_path)
    if not spec or not spec.loader:
        raise RuntimeError(f"Could not load native building material setup: {script_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.main()


def configure_material(material, asset_key, texture, is_water=False):
    """Rebuild a generated material in-place so existing map references survive."""
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -120, 0
    )
    sample.set_editor_property("parameter_name", "SurfaceTexture")
    sample.set_editor_property("texture", texture)

    if is_water:
        texcoord = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionTextureCoordinate, -620, 0
        )
        panner = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionPanner, -390, 0
        )
        panner.set_editor_property("speed_x", 0.035)
        panner.set_editor_property("speed_y", 0.012)
        unreal.MaterialEditingLibrary.connect_material_expressions(texcoord, "", panner, "Coordinate")
        unreal.MaterialEditingLibrary.connect_material_expressions(panner, "", sample, "Coordinates")

    base_color_expression = sample
    base_color_output = "RGB"
    if asset_key == "AnimeGrass":
        ground_tint = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionVectorParameter, -120, -170
        )
        ground_tint.set_editor_property("parameter_name", "GroundTint")
        # Reduce green and overall luminance without flattening the painted
        # source texture into a single color.
        ground_tint.set_editor_property("default_value", unreal.LinearColor(0.40, 0.20, 0.35, 1.0))
        tinted = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionMultiply, 130, 0
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(sample, "RGB", tinted, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(ground_tint, "", tinted, "B")
        base_color_expression = tinted
        base_color_output = ""

    unreal.MaterialEditingLibrary.connect_material_property(
        base_color_expression, base_color_output, unreal.MaterialProperty.MP_BASE_COLOR
    )
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -120, 220
    )
    roughness.set_editor_property("r", 0.22 if is_water else (0.9 if asset_key == "AnimeGrass" else 0.78))
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    if asset_key == "AnimeGrass":
        specular = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionConstant, 100, 220
        )
        specular.set_editor_property("r", 0.2)
        unreal.MaterialEditingLibrary.connect_material_property(
            specular, "", unreal.MaterialProperty.MP_SPECULAR
        )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def make_material(asset_key, texture, is_water=False):
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material_path = f"{MATERIAL_DIR}/M_{asset_key}"
    material = unreal.load_asset(material_path)
    if not material:
        material = asset_tools.create_asset(
            "M_" + asset_key,
            MATERIAL_DIR,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
    return configure_material(material, asset_key, texture, is_water)


def make_distant_ground_material():
    """Build a flat, fog-friendly horizon material without stretched texture UVs."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material_path = f"{MATERIAL_DIR}/M_DistantGround"
    material = unreal.load_asset(material_path)
    if not material:
        material = asset_tools.create_asset(
            "M_DistantGround",
            MATERIAL_DIR,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -180, 0
    )
    color.set_editor_property("parameter_name", "DistantGroundColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.15, 0.23, 0.16, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        color, "", unreal.MaterialProperty.MP_BASE_COLOR
    )
    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant, -180, 160
    )
    roughness.set_editor_property("r", 0.95)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def enable_instanced_material_usage():
    for directory in (MATERIAL_DIR, BUILDING_DIR):
        for asset_path in unreal.EditorAssetLibrary.list_assets(directory, recursive=True, include_folder=False):
            asset = unreal.load_asset(asset_path)
            if isinstance(asset, unreal.Material):
                asset.set_editor_property("used_with_instanced_static_meshes", True)
                asset.set_editor_property("used_with_spline_meshes", True)
                unreal.EditorAssetLibrary.save_loaded_asset(asset)


def main():
    imported = {key: import_texture(key, filename) for key, filename in ASSETS.items()}
    for key, texture in imported.items():
        make_material(key, texture, is_water=(key == "AnimeWater"))
    make_distant_ground_material()
    import_authored_buildings()
    apply_native_building_materials()
    enable_instanced_material_usage()
    ensure_startup_level()
    unreal.log("Sol City: imagegen textures and materials are ready.")


if __name__ == "__main__":
    main()
