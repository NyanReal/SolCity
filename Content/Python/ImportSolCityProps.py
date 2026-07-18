"""Import Blender MCP-authored SolCity props as combined Unreal static meshes."""

import os

import unreal


SOURCE_DIR = os.path.join(unreal.Paths.project_content_dir(), "Art", "Props")
DESTINATION = "/Game/Art/Props"
ASSETS = (
    "SM_SolCity_Cat_01",
    "SM_SolCity_AutonomousCar_01",
    "SM_SolCity_RoadSpline_01",
    "SM_SolCity_RoadJunction_01",
    "SM_SolCity_Bridge_01",
    "SM_SolCity_Tree_01",
)


def import_prop(asset_name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(SOURCE_DIR, asset_name + ".fbx"))
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)

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
    task.set_editor_property("options", options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh = unreal.load_asset(f"{DESTINATION}/{asset_name}")
    if not mesh:
        raise RuntimeError(f"SolCity prop import failed: {asset_name}")
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    return mesh


def enable_material_usage():
    for asset_path in unreal.EditorAssetLibrary.list_assets(DESTINATION, recursive=True, include_folder=False):
        asset = unreal.load_asset(asset_path)
        if isinstance(asset, unreal.Material):
            asset.set_editor_property("used_with_instanced_static_meshes", True)
            asset.set_editor_property("used_with_spline_meshes", True)
            unreal.EditorAssetLibrary.save_loaded_asset(asset)


for name in ASSETS:
    import_prop(name)
enable_material_usage()
unreal.log("SolCity: Blender MCP prop set imported and material usages enabled.")
