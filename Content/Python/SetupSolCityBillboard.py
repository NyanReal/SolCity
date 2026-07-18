"""Import the SolCity billboard and create four swappable ad material instances.

Run from Unreal Editor's Python console. The FBX uses Combine Meshes=False and
its material slot order is Frame (0), M_Billboard_Ad (1).
"""

from pathlib import Path

import unreal


PROJECT = Path(r"D:\github\SolCity")
FBX_FILE = PROJECT / "Content/Art/Props/SM_SolCity_Billboard_01.fbx"
SOURCE_DIR = PROJECT / "Content/Art/Source"
MESH_DIR = "/Game/Art/Props"
TEXTURE_DIR = "/Game/Art/Source"
MATERIAL_DIR = "/Game/Art/Props/Materials"
MASTER_NAME = "M_SolCity_Billboard_Ad"
INSTANCE_NAMES = [f"MI_SolCity_Billboard_Ad_{index:02d}" for index in range(1, 5)]


def import_billboard():
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.static_mesh_import_data.set_editor_property("combine_meshes", False)
    ui.static_mesh_import_data.set_editor_property("convert_scene", True)
    ui.static_mesh_import_data.set_editor_property("convert_scene_unit", True)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(FBX_FILE))
    task.set_editor_property("destination_path", MESH_DIR)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("options", ui)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])


def import_texture(path):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", TEXTURE_DIR)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"{TEXTURE_DIR}/{path.stem}")


def create_master():
    path = f"{MATERIAL_DIR}/{MASTER_NAME}"
    master = unreal.EditorAssetLibrary.load_asset(path)
    created_new = master is None
    if not master:
        master = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            MASTER_NAME, MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not isinstance(master, unreal.Material):
        raise RuntimeError(f"{path} exists but is not a Material")
    master.set_editor_property("used_with_instanced_static_meshes", True)
    master.set_editor_property("two_sided", True)
    # UE 5.7 may assert when delete_all_material_expressions is called on an
    # already-loaded rooted material. Preserve the existing graph and only
    # author expressions when creating this master for the first time.
    if created_new:
        sample = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionTextureSampleParameter2D, -260, 0
        )
        sample.set_editor_property("parameter_name", "AdTexture")
        roughness = unreal.MaterialEditingLibrary.create_material_expression(
            master, unreal.MaterialExpressionConstant, -40, 180
        )
        roughness.set_editor_property("r", 0.38)
        unreal.MaterialEditingLibrary.connect_material_property(
            sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
        )
        unreal.MaterialEditingLibrary.connect_material_property(
            roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
        )
    unreal.MaterialEditingLibrary.recompile_material(master)
    unreal.EditorAssetLibrary.save_loaded_asset(master)
    return master


def create_instances(master, textures):
    instances = []
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, texture in zip(INSTANCE_NAMES, textures):
        path = f"{MATERIAL_DIR}/{name}"
        instance = unreal.EditorAssetLibrary.load_asset(path)
        if not instance:
            instance = tools.create_asset(
                name,
                MATERIAL_DIR,
                unreal.MaterialInstanceConstant,
                unreal.MaterialInstanceConstantFactoryNew(),
            )
        if not isinstance(instance, unreal.MaterialInstanceConstant):
            raise RuntimeError(f"{path} exists but is not a MaterialInstanceConstant")
        instance.set_editor_property("parent", master)
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            instance, "AdTexture", texture
        )
        unreal.EditorAssetLibrary.save_loaded_asset(instance)
        instances.append(instance)
    return instances


import_billboard()
textures = [import_texture(SOURCE_DIR / f"T_SolCity_Ad_{index:02d}.png") for index in range(1, 5)]
if any(texture is None for texture in textures):
    raise RuntimeError("One or more SolCity ad textures failed to import")
master = create_master()
instances = create_instances(master, textures)
mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/SM_SolCity_Billboard_01")
if not isinstance(mesh, unreal.StaticMesh):
    raise RuntimeError("Billboard Static Mesh import failed")
if mesh.get_num_sections(0) < 2:
    raise RuntimeError("Billboard does not expose the required Frame + Ad sections")
mesh.set_material(1, instances[0])
unreal.EditorAssetLibrary.save_loaded_asset(mesh)

unreal.log(
    "SOLCITY_BILLBOARD_READY mesh=/Game/Art/Props/SM_SolCity_Billboard_01 "
    "instances=" + ",".join(f"{MATERIAL_DIR}/{name}" for name in INSTANCE_NAMES)
)
