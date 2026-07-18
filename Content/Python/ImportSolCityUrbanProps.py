"""Focused import for the independently placeable Blender-authored urban prop kit."""

import os

import unreal


SOURCE = os.path.join(
    unreal.Paths.project_content_dir(), "Art", "Props", "SM_SolCity_UrbanProps_01.fbx"
)
DESTINATION = "/Game/Art/Props"
EXPECTED = (
    "SM_SolCity_Bench_01",
    "SM_SolCity_TrashBin_01",
    "SM_SolCity_StreetLamp_01",
    "SM_SolCity_Planter_01",
    "SM_SolCity_Bollard_01",
    "SM_SolCity_ParkingWheelStop_01",
)


def main():
    if not os.path.isfile(SOURCE):
        raise RuntimeError(f"Missing urban prop FBX: {SOURCE}")

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", SOURCE)
    task.set_editor_property("destination_path", DESTINATION)
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
    static_data.set_editor_property("combine_meshes", False)
    static_data.set_editor_property("generate_lightmap_u_vs", True)
    static_data.set_editor_property("auto_generate_collision", True)
    static_data.set_editor_property("transform_vertex_to_absolute", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    missing = []
    for mesh_name in EXPECTED:
        mesh = unreal.load_asset(f"{DESTINATION}/{mesh_name}")
        if not mesh:
            missing.append(mesh_name)
            continue
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    if missing:
        raise RuntimeError("Urban prop import missed: " + ", ".join(missing))
    unreal.log(f"SolCity: imported {len(EXPECTED)} independent urban prop meshes")


if __name__ == "__main__":
    main()
