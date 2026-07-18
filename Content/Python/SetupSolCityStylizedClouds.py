"""Create SolCity's project-owned, stylized volumetric-cloud material instance.

The parent remains Epic's simple volumetric-cloud material.  This instance keeps
the physically integrated cloud lighting while art-directing the pattern toward
large, readable masses with restrained high-frequency breakup.
"""

import unreal


SOURCE = "/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst"
DESTINATION_DIR = "/Game/Art/Environment"
ASSET_NAME = "MI_SolCity_StylizedClouds"
ASSET_PATH = f"{DESTINATION_DIR}/{ASSET_NAME}"


def set_scalar(instance, name, value):
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        instance, name, value
    )


def set_vector(instance, name, rgba):
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance, name, unreal.LinearColor(*rgba)
    )


def main():
    source = unreal.load_asset(SOURCE)
    if not source:
        raise RuntimeError(f"Unable to load cloud parent: {SOURCE}")

    instance = unreal.load_asset(ASSET_PATH)
    if not instance:
        instance = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
            ASSET_NAME, DESTINATION_DIR, source
        )
    if not instance:
        raise RuntimeError(f"Unable to create {ASSET_PATH}")

    # Large-scale layout: fewer, larger silhouettes and more open blue sky.
    set_scalar(instance, "Layout_CloudGlobalScale", 640.0)
    set_scalar(instance, "Cloud_GlobalCoverage", -0.32)
    set_scalar(instance, "Cloud_GlobalDensity", 0.0062)

    # Preserve one broad Worley layer while almost removing the two detail bands.
    set_vector(instance, "Noise_Strength", (0.62, 0.018, 0.004, 1.55))
    set_vector(instance, "Noise_Bias", (0.57, 0.90, 0.90, 0.0))
    set_vector(instance, "Noise1_Coordinates", (2.4, 2.4, 4.8, 5.0))
    set_vector(instance, "Noise2_Coordinates", (95.0, 95.0, 110.0, -3.0))
    set_vector(instance, "Noise3_Coordinates", (80.0, 80.0, 65.0, 4.0))

    # Soft warm tops, cool readable undersides, and deliberately mild anisotropy.
    set_vector(instance, "Cloud_AlbedoColor", (1.0, 0.975, 0.92, 0.42))
    set_vector(instance, "Multiscatter_Controls", (0.82, 0.42, 0.28, 1.0))
    set_vector(instance, "Phase_Controls", (0.58, 0.24, 0.48, 1.0))

    # Slow, single-direction drift; the alpha keeps evolution subtle.
    set_vector(instance, "Layout_WindControls", (0.28, 0.10, 0.12, 0.12))
    set_scalar(instance, "StormClouds", 0.0)

    unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    unreal.log(f"Sol City: stylized cloud instance ready at {ASSET_PATH}")


if __name__ == "__main__":
    main()
