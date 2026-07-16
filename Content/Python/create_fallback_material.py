"""Create the tiny code-authored lit colour parent for primitive models."""

import unreal


ASSET_PATH = "/Game/Generated/Fallback"
ASSET_NAME = "M_FallbackTintedLit"
OBJECT_PATH = f"{ASSET_PATH}/{ASSET_NAME}"


def main() -> None:
    material = unreal.EditorAssetLibrary.load_asset(OBJECT_PATH)
    is_new = not isinstance(material, unreal.Material)
    if is_new:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            ASSET_NAME, ASSET_PATH, unreal.Material, unreal.MaterialFactoryNew()
        )
    if not isinstance(material, unreal.Material):
        raise RuntimeError(f"Unable to create {OBJECT_PATH}")

    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    material.set_editor_property("used_with_instanced_static_meshes", True)

    # UE 5.7 can assert when deleting expressions from a loaded, already
    # compiled material in a commandlet. Existing assets only need the usage
    # flag refreshed; build the graph solely on first creation.
    if not is_new:
        unreal.EditorAssetLibrary.save_loaded_asset(material)
        unreal.log(f"[SolCity Assets] Updated {OBJECT_PATH}")
        return

    colour = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -180, 0
    )
    colour.set_editor_property("parameter_name", "Color")
    colour.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        colour, "RGB", unreal.MaterialProperty.MP_BASE_COLOR
    )

    roughness = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -180, 140
    )
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.82)
    unreal.MaterialEditingLibrary.connect_material_property(
        roughness, "", unreal.MaterialProperty.MP_ROUGHNESS
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    unreal.log(f"[SolCity Assets] Created {OBJECT_PATH}")


if __name__ == "__main__":
    main()
