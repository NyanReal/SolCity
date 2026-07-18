"""Import the authored perimeter railway kit into /Game/Art/Props."""

import os

import unreal


CONTENT_DIR = unreal.Paths.project_content_dir()
DESTINATION = "/Game/Art/Props"
ASSET_NAMES = (
    "SM_SolCity_DoubleTrack_01",
    "SM_SolCity_RailBridge_01",
    "SM_SolCity_CommuterTrain_01",
)


def import_asset(asset_name):
    asset_path = f"{DESTINATION}/{asset_name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"SolCity railway import: keeping existing {asset_name}")
        return
    source = os.path.join(CONTENT_DIR, "Art", "Props", asset_name + ".fbx")
    if not os.path.isfile(source):
        raise RuntimeError(f"Missing railway FBX: {source}")

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
        raise RuntimeError(f"Railway import failed: {asset_name}")
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    bounds = asset.get_bounds()
    unreal.log(f"SolCity railway import: {asset_name}, extent={bounds.box_extent}, origin={bounds.origin}")


for name in ASSET_NAMES:
    import_asset(name)

unreal.log("SolCity: imported double-track, railway bridge, and commuter train assets.")
