"""Import only the authored modular building FBXs into /Game/Art/Buildings."""

import os

import unreal


DESTINATION = "/Game/Art/Buildings"
MODULES = (
    "SM_SolCity_BuildingBase_01",
    "SM_SolCity_BuildingMiddle_01",
    "SM_SolCity_BuildingCrown_01",
)


for asset_name in MODULES:
    source = os.path.join(unreal.Paths.project_content_dir(), "Art", "Buildings", asset_name + ".fbx")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
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
    static_data.set_editor_property("transform_vertex_to_absolute", True)
    task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    mesh = unreal.load_asset(f"{DESTINATION}/{asset_name}")
    if not mesh:
        raise RuntimeError(f"Building module import failed: {asset_name}")
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    bounds = mesh.get_bounds()
    unreal.log(f"Imported {asset_name}: extent={bounds.box_extent}, origin={bounds.origin}")
