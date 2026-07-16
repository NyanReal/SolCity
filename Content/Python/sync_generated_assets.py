"""Import completed Sol City image requests and build simple runtime materials.

Run inside Unreal Editor 5.7. The script is intentionally idempotent: existing
textures/materials are updated instead of duplicated.
"""

from __future__ import annotations

import json
import re
from pathlib import Path

import unreal


PROJECT_DIR = Path(unreal.Paths.project_dir()).resolve()
MANIFEST_PATH = PROJECT_DIR / "Config" / "GeneratedAssetManifest.json"
QUEUE_PATH = PROJECT_DIR / "ExternalImageRequests.md"
TEXTURE_PATH = "/Game/Generated/Textures"
MATERIAL_PATH = "/Game/Generated/Materials"


def log(message: str) -> None:
    unreal.log(f"[SolCity Assets] {message}")


def import_texture(entry: dict) -> unreal.Texture2D | None:
    source = PROJECT_DIR / entry["source"]
    if not source.exists():
        return None

    object_path = f"{TEXTURE_PATH}/{entry['texture_name']}"
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        texture = unreal.EditorAssetLibrary.load_asset(object_path)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([
            make_import_task(source, entry["texture_name"], replace=True)
        ])
        texture = unreal.EditorAssetLibrary.load_asset(object_path)
    else:
        task = make_import_task(source, entry["texture_name"], replace=False)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = unreal.EditorAssetLibrary.load_asset(object_path)

    if isinstance(texture, unreal.Texture2D):
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
        texture.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
        texture.set_editor_property("filter", unreal.TextureFilter.TF_DEFAULT)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
        return texture
    return None


def make_import_task(source: Path, destination_name: str, replace: bool) -> unreal.AssetImportTask:
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", TEXTURE_PATH)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", replace)
    task.set_editor_property("replace_existing_settings", False)
    task.set_editor_property("save", True)
    return task


def get_or_create_material(entry: dict, texture: unreal.Texture2D) -> unreal.Material:
    material_object_path = f"{MATERIAL_PATH}/{entry['material_name']}"
    material = None
    if unreal.EditorAssetLibrary.does_asset_exist(material_object_path):
        material = unreal.EditorAssetLibrary.load_asset(material_object_path)
    if not isinstance(material, unreal.Material):
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            entry["material_name"], MATERIAL_PATH, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not isinstance(material, unreal.Material):
        raise RuntimeError(f"Could not create {material_object_path}")

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("used_with_instanced_static_meshes", True)
    is_unlit = bool(entry.get("unlit", False))
    material.set_editor_property(
        "shading_model",
        unreal.MaterialShadingModel.MSM_UNLIT
        if is_unlit
        else unreal.MaterialShadingModel.MSM_DEFAULT_LIT,
    )

    texcoord = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureCoordinate, -760, -40
    )
    tiling = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -760, 90
    )
    tiling.set_editor_property("parameter_name", "Tiling")
    tiling.set_editor_property("default_value", float(entry.get("tiling", 1.0)))

    multiply = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -560, -20
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(texcoord, "", multiply, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(tiling, "", multiply, "B")

    uv_output = multiply
    if entry.get("animated", False):
        panner = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionPanner, -360, -20
        )
        panner.set_editor_property("speed_x", 0.018)
        panner.set_editor_property("speed_y", 0.006)
        unreal.MaterialEditingLibrary.connect_material_expressions(multiply, "", panner, "Coordinate")
        uv_output = panner

    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -100, -20
    )
    sample.set_editor_property("parameter_name", "BaseTexture")
    sample.set_editor_property("texture", texture)
    unreal.MaterialEditingLibrary.connect_material_expressions(uv_output, "", sample, "UVs")
    if is_unlit:
        unreal.MaterialEditingLibrary.connect_material_property(
            sample, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
    else:
        unreal.MaterialEditingLibrary.connect_material_property(
            sample, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
        )

    emissive_strength = float(entry.get("emissive_strength", 0.0))
    if emissive_strength > 0.0:
        emissive = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionScalarParameter, -100, 280
        )
        emissive.set_editor_property("parameter_name", "EmissiveStrength")
        emissive.set_editor_property("default_value", emissive_strength)
        emissive_multiply = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionMultiply, 120, 80
        )
        unreal.MaterialEditingLibrary.connect_material_expressions(sample, "RGB", emissive_multiply, "A")
        unreal.MaterialEditingLibrary.connect_material_expressions(emissive, "", emissive_multiply, "B")
        unreal.MaterialEditingLibrary.connect_material_property(
            emissive_multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )

    if not is_unlit:
        roughness = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionScalarParameter, -100, 180
        )
        roughness.set_editor_property("parameter_name", "Roughness")
        roughness.set_editor_property("default_value", 0.32 if entry.get("animated", False) else 0.82)
        unreal.MaterialEditingLibrary.connect_material_property(
            roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
        )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def mark_imported(request_id: str) -> None:
    if not QUEUE_PATH.exists():
        return
    content = QUEUE_PATH.read_text(encoding="utf-8")
    section_pattern = re.compile(
        rf"(##\s+{re.escape(request_id)}\b.*?)(?=\n##\s+REQ-|\Z)", re.DOTALL
    )
    match = section_pattern.search(content)
    if not match:
        return
    section = match.group(1)
    section = re.sub(r"(- UE 임포트 완료:\s*)\[[ xX]\]", r"\1[x]", section, count=1)
    content = content[: match.start(1)] + section + content[match.end(1) :]
    QUEUE_PATH.write_text(content, encoding="utf-8")


def main() -> None:
    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    imported_count = 0
    for entry in manifest["assets"]:
        texture = import_texture(entry)
        if texture is None:
            log(f"Waiting for {entry['id']}: {entry['source']}")
            continue
        get_or_create_material(entry, texture)
        mark_imported(entry["id"])
        imported_count += 1
        log(f"Imported {entry['id']} as {entry['material_name']}")
    log(f"Sync complete: {imported_count}/{len(manifest['assets'])} available")


if __name__ == "__main__":
    main()
