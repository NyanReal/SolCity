"""Create SolCity's native Unreal building materials and assign mesh slots.

This script is intentionally idempotent.  It rebuilds the native master graph,
updates deterministic material-instance parameters, and reapplies static-mesh
slot mappings without deleting imported FBX meshes or native material assets.

Run with Tools > Execute Python Script, or:
    UnrealEditor-Cmd.exe SolCity.uproject \
        -ExecutePythonScript=Content/Python/SetupSolCityBuildingMaterials.py
"""

from __future__ import annotations

import os
from collections import Counter

import unreal


BUILDING_DIR = "/Game/Art/Buildings"
MATERIAL_DIR = f"{BUILDING_DIR}/Materials"
MASTER_NAME = "M_SolCity_Building_Master"
MASTER_PATH = f"{MATERIAL_DIR}/{MASTER_NAME}"

DEFAULT_NORMAL_PATHS = (
    "/Engine/EngineMaterials/DefaultNormal.DefaultNormal",
    "/Engine/EngineMaterials/DefaultNormal",
)
DEFAULT_WHITE_PATHS = (
    "/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture",
    "/Engine/EngineResources/WhiteSquareTexture",
)


def surface(
    color,
    roughness,
    *,
    metallic=0.0,
    specular=0.5,
    emissive_color=(0.0, 0.0, 0.0),
    emissive_strength=0.0,
):
    """Declare one authored slot using values from its Blender source."""
    return {
        "BaseColor": color,
        "Metallic": metallic,
        "Roughness": roughness,
        "Specular": specular,
        "EmissiveColor": emissive_color,
        "EmissiveStrength": emissive_strength,
    }


# Material names deliberately match the imported FBX slot names.  Keeping this
# source-of-truth data beside BUILDING_SLOT_MAP makes additions deterministic.
MATERIAL_SPECS = {
    # SM_SolCity_MidRise_01
    "M_Concrete_Warm": surface((0.60, 0.55, 0.47), 0.72),
    "M_Stone_Charcoal": surface((0.055, 0.065, 0.075), 0.48),
    "M_Frame_Dark": surface((0.018, 0.024, 0.030), 0.25, metallic=0.55),
    "M_Glass_Blue": surface((0.025, 0.18, 0.26), 0.10, metallic=0.15, specular=0.6),
    "M_Glass_InteriorGlow": surface(
        (0.12, 0.24, 0.29),
        0.18,
        metallic=0.05,
        specular=0.6,
        emissive_color=(0.20, 0.48, 0.56),
        emissive_strength=0.45,
    ),
    "M_Accent_Terracotta": surface((0.76, 0.17, 0.055), 0.38, metallic=0.05),
    "M_Roof_Metal": surface((0.10, 0.12, 0.14), 0.28, metallic=0.78),
    "M_Sidewalk": surface((0.22, 0.24, 0.25), 0.88),
    "M_Planter_Green": surface((0.055, 0.20, 0.08), 0.72),
    "M_Sign_White": surface(
        (0.82, 0.92, 1.0),
        0.22,
        metallic=0.15,
        emissive_color=(0.45, 0.75, 1.0),
        emissive_strength=2.0,
    ),
    # SM_SolCity_CornerRetail_01 -- Phase 1 plan seed values.
    "M_Retail_Cream": surface((0.64, 0.50, 0.34), 0.70),
    "M_Retail_Brick": surface((0.42, 0.08, 0.045), 0.78),
    "M_Retail_Dark": surface((0.035, 0.03, 0.028), 0.27, metallic=0.55),
    "M_Retail_Brass": surface((0.55, 0.28, 0.055), 0.25, metallic=0.82),
    "M_Retail_Glass": surface((0.03, 0.16, 0.24), 0.10, metallic=0.14, specular=0.6),
    "M_Retail_GlassGlow": surface(
        (0.15, 0.24, 0.28),
        0.16,
        specular=0.6,
        emissive_color=(0.55, 0.28, 0.12),
        emissive_strength=0.55,
    ),
    "M_Retail_Awning": surface((0.72, 0.12, 0.045), 0.38),
    "M_Retail_Green": surface((0.07, 0.23, 0.07), 0.80),
    "M_Retail_Sign": surface(
        (1.0, 0.78, 0.35),
        0.20,
        metallic=0.25,
        emissive_color=(1.0, 0.25, 0.05),
        emissive_strength=2.8,
    ),
    # SM_SolCity_SteppedTower_01
    "M_Tower_Brick": surface((0.23, 0.075, 0.045), 0.78),
    "M_Tower_Stone": surface((0.16, 0.18, 0.18), 0.70),
    "M_Tower_Frame": surface((0.018, 0.026, 0.026), 0.24, metallic=0.62),
    "M_Tower_Glass": surface((0.015, 0.20, 0.18), 0.10, metallic=0.18, specular=0.6),
    "M_Tower_GlassGlow": surface(
        (0.08, 0.28, 0.24),
        0.16,
        specular=0.6,
        emissive_color=(0.18, 0.55, 0.42),
        emissive_strength=0.55,
    ),
    "M_Tower_Copper": surface((0.58, 0.19, 0.055), 0.32, metallic=0.45),
    "M_Tower_Plant": surface((0.05, 0.24, 0.09), 0.78),
    "M_Tower_Concrete": surface((0.46, 0.43, 0.37), 0.75),
    "M_Tower_Sign": surface(
        (0.75, 1.0, 0.88),
        0.20,
        metallic=0.15,
        emissive_color=(0.3, 1.0, 0.65),
        emissive_strength=2.4,
    ),
}

BUILDING_SLOT_MAP = {
    f"{BUILDING_DIR}/SM_SolCity_MidRise_01": {
        name: name
        for name in (
            "M_Concrete_Warm",
            "M_Stone_Charcoal",
            "M_Frame_Dark",
            "M_Glass_Blue",
            "M_Glass_InteriorGlow",
            "M_Accent_Terracotta",
            "M_Roof_Metal",
            "M_Sidewalk",
            "M_Planter_Green",
            "M_Sign_White",
        )
    },
    f"{BUILDING_DIR}/SM_SolCity_CornerRetail_01": {
        name: name
        for name in (
            "M_Retail_Cream",
            "M_Retail_Brick",
            "M_Retail_Dark",
            "M_Retail_Brass",
            "M_Retail_Glass",
            "M_Retail_GlassGlow",
            "M_Retail_Awning",
            "M_Retail_Green",
            "M_Retail_Sign",
        )
    },
    f"{BUILDING_DIR}/SM_SolCity_SteppedTower_01": {
        name: name
        for name in (
            "M_Tower_Brick",
            "M_Tower_Stone",
            "M_Tower_Frame",
            "M_Tower_Glass",
            "M_Tower_GlassGlow",
            "M_Tower_Copper",
            "M_Tower_Plant",
            "M_Tower_Concrete",
            "M_Tower_Sign",
        )
    },
}


def _load_first(paths):
    for path in paths:
        asset = unreal.load_asset(path)
        if asset:
            return asset
    return None


def _create_expression(material, expression_class, x, y, **properties):
    expression = unreal.MaterialEditingLibrary.create_material_expression(
        material, expression_class, x, y
    )
    if not expression:
        raise RuntimeError(f"Could not create {expression_class} in {material.get_name()}")
    for name, value in properties.items():
        expression.set_editor_property(name, value)
    return expression


def _connect(source, source_output, target, target_input):
    output_names = (source_output,) if source_output else ("", "Result")
    for output_name in output_names:
        if unreal.MaterialEditingLibrary.connect_material_expressions(
            source, output_name, target, target_input
        ):
            return
    raise RuntimeError(
        f"Could not connect {source.get_name()}.{source_output} to "
        f"{target.get_name()}.{target_input}"
    )


def _connect_property(source, source_output, material_property):
    output_names = (source_output,) if source_output else ("", "Result")
    for output_name in output_names:
        if unreal.MaterialEditingLibrary.connect_material_property(
            source, output_name, material_property
        ):
            return
    raise RuntimeError(f"Could not connect {source.get_name()} to {material_property}")


def ensure_master_material():
    """Create or rebuild the opaque native PBR master material."""
    unreal.EditorAssetLibrary.make_directory(MATERIAL_DIR)
    master = unreal.load_asset(MASTER_PATH)
    if master and not isinstance(master, unreal.Material):
        raise RuntimeError(f"{MASTER_PATH} exists but is not a Material")
    if master:
        # DeleteAllMaterialExpressions asserts on a previously saved/rooted UE
        # material in 5.7 commandlets. The graph is a versioned contract: reuse
        # it on subsequent runs and update all instance values below.
        master.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
        master.set_editor_property("used_with_instanced_static_meshes", True)
        unreal.EditorAssetLibrary.save_loaded_asset(master)
        unreal.log(f"SolCity materials: reused {MASTER_PATH}")
        return master
    if not master:
        master = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            MASTER_NAME, MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not master:
        raise RuntimeError(f"Could not create {MASTER_PATH}")

    unreal.MaterialEditingLibrary.delete_all_material_expressions(master)
    master.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    master.set_editor_property("used_with_instanced_static_meshes", True)

    # Core PBR parameters.
    base_color = _create_expression(
        master,
        unreal.MaterialExpressionVectorParameter,
        -1050,
        -420,
        parameter_name="BaseColor",
        default_value=unreal.LinearColor(0.5, 0.5, 0.5, 1.0),
    )
    metallic = _create_expression(
        master,
        unreal.MaterialExpressionScalarParameter,
        300,
        -40,
        parameter_name="Metallic",
        default_value=0.0,
    )
    roughness = _create_expression(
        master,
        unreal.MaterialExpressionScalarParameter,
        -480,
        80,
        parameter_name="Roughness",
        default_value=0.65,
    )
    specular = _create_expression(
        master,
        unreal.MaterialExpressionScalarParameter,
        300,
        200,
        parameter_name="Specular",
        default_value=0.5,
    )
    _connect_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    _connect_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)

    # Roughness texture hook. Lerp keeps the scalar value authoritative at a
    # strength of zero, while allowing gradual art-direction changes later.
    white = _load_first(DEFAULT_WHITE_PATHS)
    roughness_texture = _create_expression(
        master,
        unreal.MaterialExpressionTextureSampleParameter2D,
        -1040,
        80,
        parameter_name="RoughnessTexture",
    )
    if white:
        roughness_texture.set_editor_property("texture", white)
    roughness_multiply = _create_expression(
        master, unreal.MaterialExpressionMultiply, -260, 80
    )
    _connect(roughness, "", roughness_multiply, "A")
    _connect(roughness_texture, "R", roughness_multiply, "B")
    roughness_strength = _create_expression(
        master,
        unreal.MaterialExpressionScalarParameter,
        -480,
        250,
        parameter_name="RoughnessTextureStrength",
        default_value=0.0,
    )
    roughness_lerp = _create_expression(
        master, unreal.MaterialExpressionLinearInterpolate, -40, 80
    )
    _connect(roughness, "", roughness_lerp, "A")
    _connect(roughness_multiply, "", roughness_lerp, "B")
    _connect(roughness_strength, "", roughness_lerp, "Alpha")
    use_roughness = _create_expression(
        master,
        unreal.MaterialExpressionStaticSwitchParameter,
        160,
        80,
        parameter_name="UseRoughnessTexture",
        default_value=False,
    )
    _connect(roughness_lerp, "", use_roughness, "True")
    _connect(roughness, "", use_roughness, "False")
    _connect_property(use_roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # Low-frequency color variation hook using regular UVs. Texture assets can
    # be shared later without changing the material-instance contract.
    texcoord = _create_expression(
        master, unreal.MaterialExpressionTextureCoordinate, -1080, -220
    )
    macro_scale = _create_expression(
        master,
        unreal.MaterialExpressionScalarParameter,
        -1080,
        -80,
        parameter_name="MacroVariationScale",
        default_value=0.05,
    )
    macro_texture = _create_expression(
        master,
        unreal.MaterialExpressionTextureSampleParameter2D,
        -610,
        -250,
        parameter_name="MacroVariationTexture",
    )
    if white:
        macro_texture.set_editor_property("texture", white)
    # UE 5.7's Python material graph API does not expose an output pin for a
    # Multiply used as UV data. Keep the scale parameter in the stable master
    # contract and use regular UV0 until Phase 4 supplies the shared texture
    # and its calibrated scale implementation.
    # UMaterialGraphNode shortens the Python-visible Coordinates pin to UVs.
    _connect(texcoord, "", macro_texture, "UVs")
    macro_tinted = _create_expression(master, unreal.MaterialExpressionMultiply, -360, -350)
    _connect(base_color, "", macro_tinted, "A")
    _connect(macro_texture, "RGB", macro_tinted, "B")
    macro_strength = _create_expression(
        master,
        unreal.MaterialExpressionScalarParameter,
        -340,
        -170,
        parameter_name="MacroVariationStrength",
        default_value=0.0,
    )
    macro_lerp = _create_expression(
        master, unreal.MaterialExpressionLinearInterpolate, -100, -390
    )
    _connect(base_color, "", macro_lerp, "A")
    _connect(macro_tinted, "", macro_lerp, "B")
    _connect(macro_strength, "", macro_lerp, "Alpha")
    use_macro = _create_expression(
        master,
        unreal.MaterialExpressionStaticSwitchParameter,
        130,
        -390,
        parameter_name="UseMacroVariation",
        default_value=False,
    )
    _connect(macro_lerp, "", use_macro, "True")
    _connect(base_color, "", use_macro, "False")
    _connect_property(use_macro, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Opaque normal-map hook. UE 5.7 does not expose FlattenNormal to Python,
    # so blend from the tangent-space default normal to retain a useful,
    # deterministic NormalStrength control.
    normal_texture = _create_expression(
        master,
        unreal.MaterialExpressionTextureSampleParameter2D,
        -840,
        430,
        parameter_name="NormalTexture",
    )
    default_normal = _load_first(DEFAULT_NORMAL_PATHS)
    if default_normal:
        normal_texture.set_editor_property("texture", default_normal)
    normal_texture.set_editor_property(
        "sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL
    )
    normal_strength = _create_expression(
        master,
        unreal.MaterialExpressionScalarParameter,
        -840,
        600,
        parameter_name="NormalStrength",
        default_value=1.0,
    )
    default_normal_vector = _create_expression(
        master,
        unreal.MaterialExpressionConstant3Vector,
        -610,
        650,
        constant=unreal.LinearColor(0.0, 0.0, 1.0, 1.0),
    )
    normal_lerp = _create_expression(
        master, unreal.MaterialExpressionLinearInterpolate, -360, 480
    )
    _connect(default_normal_vector, "", normal_lerp, "A")
    _connect(normal_texture, "RGB", normal_lerp, "B")
    _connect(normal_strength, "", normal_lerp, "Alpha")
    use_normal = _create_expression(
        master,
        unreal.MaterialExpressionStaticSwitchParameter,
        -80,
        480,
        parameter_name="UseNormalTexture",
        default_value=False,
    )
    _connect(normal_lerp, "", use_normal, "True")
    _connect(default_normal_vector, "", use_normal, "False")
    _connect_property(use_normal, "", unreal.MaterialProperty.MP_NORMAL)

    # Emissive is explicitly switchable so non-emissive building slots do not
    # carry an unnecessary shader path.
    emissive_color = _create_expression(
        master,
        unreal.MaterialExpressionVectorParameter,
        -620,
        850,
        parameter_name="EmissiveColor",
        default_value=unreal.LinearColor(0.0, 0.0, 0.0, 1.0),
    )
    emissive_strength = _create_expression(
        master,
        unreal.MaterialExpressionScalarParameter,
        -620,
        1020,
        parameter_name="EmissiveStrength",
        default_value=0.0,
    )
    emissive_multiply = _create_expression(
        master, unreal.MaterialExpressionMultiply, -350, 880
    )
    _connect(emissive_color, "", emissive_multiply, "A")
    _connect(emissive_strength, "", emissive_multiply, "B")
    emissive_off = _create_expression(
        master,
        unreal.MaterialExpressionConstant3Vector,
        -330,
        1060,
        constant=unreal.LinearColor(0.0, 0.0, 0.0, 1.0),
    )
    use_emissive = _create_expression(
        master,
        unreal.MaterialExpressionStaticSwitchParameter,
        -80,
        900,
        parameter_name="UseEmissive",
        default_value=False,
    )
    _connect(emissive_multiply, "", use_emissive, "True")
    _connect(emissive_off, "", use_emissive, "False")
    _connect_property(use_emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    unreal.MaterialEditingLibrary.recompile_material(master)
    unreal.EditorAssetLibrary.save_loaded_asset(master)
    unreal.log(f"SolCity materials: rebuilt {MASTER_PATH}")
    return master


def ensure_material_instances(master):
    """Create/update all native material instances without deleting assets."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    instances = {}
    for material_name in sorted(MATERIAL_SPECS):
        path = f"{MATERIAL_DIR}/{material_name}"
        instance = unreal.load_asset(path)
        if instance and not isinstance(instance, unreal.MaterialInstanceConstant):
            raise RuntimeError(f"{path} exists but is not a MaterialInstanceConstant")
        if not instance:
            instance = asset_tools.create_asset(
                material_name,
                MATERIAL_DIR,
                unreal.MaterialInstanceConstant,
                unreal.MaterialInstanceConstantFactoryNew(),
            )
        if not instance:
            raise RuntimeError(f"Could not create {path}")

        spec = MATERIAL_SPECS[material_name]
        edit = unreal.MaterialEditingLibrary
        edit.set_material_instance_parent(instance, master)
        edit.set_material_instance_vector_parameter_value(
            instance, "BaseColor", unreal.LinearColor(*spec["BaseColor"], 1.0)
        )
        edit.set_material_instance_scalar_parameter_value(instance, "Metallic", spec["Metallic"])
        edit.set_material_instance_scalar_parameter_value(instance, "Roughness", spec["Roughness"])
        edit.set_material_instance_scalar_parameter_value(instance, "Specular", spec["Specular"])
        edit.set_material_instance_vector_parameter_value(
            instance,
            "EmissiveColor",
            unreal.LinearColor(*spec["EmissiveColor"], 1.0),
        )
        edit.set_material_instance_scalar_parameter_value(
            instance, "EmissiveStrength", spec["EmissiveStrength"]
        )
        edit.set_material_instance_scalar_parameter_value(instance, "NormalStrength", 1.0)
        edit.set_material_instance_scalar_parameter_value(
            instance, "RoughnessTextureStrength", 0.0
        )
        edit.set_material_instance_scalar_parameter_value(instance, "MacroVariationScale", 0.05)
        edit.set_material_instance_scalar_parameter_value(
            instance, "MacroVariationStrength", 0.0
        )
        edit.set_material_instance_static_switch_parameter_value(
            instance, "UseNormalTexture", False
        )
        edit.set_material_instance_static_switch_parameter_value(
            instance, "UseRoughnessTexture", False
        )
        edit.set_material_instance_static_switch_parameter_value(
            instance, "UseMacroVariation", False
        )
        edit.set_material_instance_static_switch_parameter_value(
            instance, "UseEmissive", spec["EmissiveStrength"] > 0.0
        )
        edit.update_material_instance(instance)
        unreal.EditorAssetLibrary.save_loaded_asset(instance)
        instances[material_name] = instance
    unreal.log(f"SolCity materials: created/updated {len(instances)} native instances")
    return instances


def _slot_name(static_material):
    imported_name = str(static_material.get_editor_property("imported_material_slot_name"))
    if imported_name and imported_name != "None":
        return imported_name
    return str(static_material.get_editor_property("material_slot_name"))


def assign_building_materials(instances, strict=True):
    """Apply native instances by FBX slot name and report mapping defects."""
    results = {}
    errors = []
    for mesh_path, slot_mapping in BUILDING_SLOT_MAP.items():
        mesh = unreal.load_asset(mesh_path)
        if not mesh:
            message = f"Missing authored building mesh: {mesh_path}"
            unreal.log_error(message)
            errors.append(message)
            continue

        static_materials = list(mesh.get_editor_property("static_materials"))
        slot_names = [_slot_name(slot) for slot in static_materials]
        duplicates = sorted(name for name, count in Counter(slot_names).items() if count > 1)
        if duplicates:
            message = f"Duplicate material slots on {mesh_path}: {duplicates}"
            unreal.log_error(message)
            errors.append(message)

        missing = sorted(set(slot_mapping) - set(slot_names))
        if missing:
            message = f"Missing/renamed material slots on {mesh_path}: {missing}"
            unreal.log_error(message)
            errors.append(message)

        unknown = sorted(set(slot_names) - set(slot_mapping))
        if unknown:
            unreal.log_warning(f"Unmapped imported slots on {mesh_path}: {unknown}")

        assigned = []
        for index, slot_name in enumerate(slot_names):
            instance_name = slot_mapping.get(slot_name)
            if not instance_name:
                continue
            instance = instances.get(instance_name)
            if not instance:
                message = f"Missing native instance {instance_name} for {mesh_path}:{slot_name}"
                unreal.log_error(message)
                errors.append(message)
                continue
            mesh.set_material(index, instance)
            assigned.append((index, slot_name, instance.get_path_name()))

        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        results[mesh_path] = {
            "slot_count": len(slot_names),
            "assigned_count": len(assigned),
            "slots": slot_names,
            "assignments": assigned,
        }
        unreal.log(
            f"SolCity materials: {mesh_path} assigned {len(assigned)}/{len(slot_names)} slots"
        )

    if strict and errors:
        raise RuntimeError("Native building material assignment failed:\n" + "\n".join(errors))
    return results, errors


def import_or_reimport_buildings():
    """Reimport authored FBXs in place, then restore native slot mappings.

    Native materials live in their own folder and are never deleted. FBX
    material creation is disabled; imported slot names remain the lookup key.
    """
    for mesh_path in BUILDING_SLOT_MAP:
        asset_name = mesh_path.rsplit("/", 1)[-1]
        source = os.path.join(
            unreal.Paths.project_content_dir(), "Art", "Buildings", asset_name + ".fbx"
        )
        if not os.path.isfile(source):
            raise RuntimeError(f"Missing building FBX: {source}")

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
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
        static_data = options.get_editor_property("static_mesh_import_data")
        static_data.set_editor_property("combine_meshes", True)
        static_data.set_editor_property("generate_lightmap_u_vs", True)
        task.set_editor_property("options", options)
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        if not unreal.load_asset(mesh_path):
            raise RuntimeError(f"Authored building FBX import failed: {asset_name}")

    master = ensure_master_material()
    instances = ensure_material_instances(master)
    return assign_building_materials(instances, strict=True)


def main():
    master = ensure_master_material()
    instances = ensure_material_instances(master)
    results, errors = assign_building_materials(instances, strict=True)
    assigned = sum(result["assigned_count"] for result in results.values())
    slots = sum(result["slot_count"] for result in results.values())
    unreal.log(
        f"SolCity materials complete: master=1 instances={len(instances)} "
        f"meshes={len(results)} assigned={assigned}/{slots} errors={len(errors)}"
    )
    return results


if __name__ == "__main__":
    main()
