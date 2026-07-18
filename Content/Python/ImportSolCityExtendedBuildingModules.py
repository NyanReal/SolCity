"""Import only the optional corner, rooftop, and skybridge building kits."""

import os

import unreal


DESTINATION = "/Game/Art/Buildings"
KITS = {
    "SM_SolCity_CornerBuildingModules_01": (
        "SM_SolCity_CornerBase_01",
        "SM_SolCity_CornerMiddle_01",
        "SM_SolCity_CornerCrown_01",
    ),
    "SM_SolCity_RooftopModules_01": (
        "SM_SolCity_RooftopHVAC_01",
        "SM_SolCity_RooftopWaterTank_01",
        "SM_SolCity_RooftopSolar_01",
        "SM_SolCity_RooftopAntenna_01",
        "SM_SolCity_RooftopPergola_01",
        "SM_SolCity_RooftopHelipad_01",
        "SM_SolCity_RooftopWarningBeacon_01",
        "SM_SolCity_SkybridgeConnector_01",
    ),
}


for kit_name, expected_meshes in KITS.items():
    source = os.path.join(
        unreal.Paths.project_content_dir(), "Art", "Buildings", kit_name + ".fbx"
    )
    if not os.path.isfile(source):
        raise RuntimeError(f"Missing authored building kit: {source}")

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", DESTINATION)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)

    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", False)
    options.set_editor_property(
        "mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH
    )
    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("combine_meshes", False)
    static_data.set_editor_property("generate_lightmap_u_vs", True)
    static_data.set_editor_property("auto_generate_collision", True)
    static_data.set_editor_property("transform_vertex_to_absolute", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    missing = []
    for mesh_name in expected_meshes:
        mesh = unreal.load_asset(f"{DESTINATION}/{mesh_name}")
        if not isinstance(mesh, unreal.StaticMesh):
            missing.append(mesh_name)
            continue
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)

    if missing:
        raise RuntimeError(f"{kit_name} import missed: {', '.join(missing)}")
    unreal.log(f"SOLCITY_EXTENDED_KIT_READY {kit_name} meshes={len(expected_meshes)}")
