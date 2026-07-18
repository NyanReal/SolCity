"""Import the expanded suburban, vegetation, cat, and mega-CBD asset sets."""

import os

import unreal


CONTENT_DIR = unreal.Paths.project_content_dir()

KITS = (
    {
        "source": os.path.join(CONTENT_DIR, "Art", "Buildings", "SM_SolCity_SuburbanHouses_01.fbx"),
        "destination": "/Game/Art/Buildings",
        "combine": False,
        "expected": tuple(f"SM_SolCity_SuburbanHouse_{index:02d}" for index in range(1, 5)),
    },
    {
        "source": os.path.join(CONTENT_DIR, "Art", "Props", "SM_SolCity_Conifers_01.fbx"),
        "destination": "/Game/Art/Props",
        "combine": False,
        "expected": ("SM_SolCity_Conifer_01", "SM_SolCity_Conifer_02"),
    },
    {
        "source": os.path.join(CONTENT_DIR, "Art", "Buildings", "SM_SolCity_MegaSkyscrapers_01.fbx"),
        "destination": "/Game/Art/Buildings",
        "combine": False,
        "expected": (
            "SM_SolCity_MegaGlassCurtainwall_01",
            "SM_SolCity_MegaNewYorkSetback_01",
            "SM_SolCity_MegaGeometricTwist_01",
            "SM_SolCity_MegaPodiumCrown_01",
        ),
    },
    {
        "source": os.path.join(CONTENT_DIR, "Art", "Props", "SM_SolCity_VoxelCat_01.fbx"),
        "destination": "/Game/Art/Props",
        "destination_name": "SM_SolCity_VoxelCat_01",
        "combine": True,
        "expected": ("SM_SolCity_VoxelCat_01",),
    },
)


def make_options(combine_meshes):
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", False)
    options.set_editor_property("import_materials", True)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    static_data = options.get_editor_property("static_mesh_import_data")
    static_data.set_editor_property("combine_meshes", combine_meshes)
    static_data.set_editor_property("generate_lightmap_u_vs", True)
    static_data.set_editor_property("auto_generate_collision", True)
    return options


def import_kit(spec):
    if not os.path.isfile(spec["source"]):
        raise RuntimeError(f"Missing SolCity expansion FBX: {spec['source']}")
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", spec["source"])
    task.set_editor_property("destination_path", spec["destination"])
    if spec.get("destination_name"):
        task.set_editor_property("destination_name", spec["destination_name"])
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", make_options(spec["combine"]))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    missing = []
    for asset_name in spec["expected"]:
        asset = unreal.load_asset(f"{spec['destination']}/{asset_name}")
        if not isinstance(asset, unreal.StaticMesh):
            missing.append(asset_name)
            continue
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
    if missing:
        raise RuntimeError("SolCity expansion import missed: " + ", ".join(missing))


def enable_instancing_material_usage():
    for destination in {spec["destination"] for spec in KITS}:
        for asset_path in unreal.EditorAssetLibrary.list_assets(destination, recursive=False, include_folder=False):
            asset = unreal.load_asset(asset_path)
            if isinstance(asset, unreal.Material) and not asset.get_editor_property("used_with_instanced_static_meshes"):
                asset.set_editor_property("used_with_instanced_static_meshes", True)
                unreal.EditorAssetLibrary.save_loaded_asset(asset)


for kit in KITS:
    import_kit(kit)
enable_instancing_material_usage()
unreal.log("SolCity: imported 11 expansion meshes (4 houses, 2 conifers, 4 mega towers, 1 voxel cat).")
