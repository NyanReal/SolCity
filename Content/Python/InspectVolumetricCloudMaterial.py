"""Log editable parameters exposed by Unreal's simple volumetric-cloud instance."""

import unreal


ASSET = "/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst"


def main():
    material = unreal.load_asset(ASSET)
    if not material:
        raise RuntimeError(f"Unable to load {ASSET}")

    library = unreal.MaterialEditingLibrary
    unreal.log_warning(f"CLOUD_ASSET class={material.get_class().get_name()} path={material.get_path_name()}")
    for name in library.get_scalar_parameter_names(material):
        value = library.get_material_instance_scalar_parameter_value(material, name)
        unreal.log_warning(f"CLOUD_SCALAR {name}={value}")
    for name in library.get_vector_parameter_names(material):
        value = library.get_material_instance_vector_parameter_value(material, name)
        unreal.log_warning(f"CLOUD_VECTOR {name}={value}")
    for name in library.get_texture_parameter_names(material):
        value = library.get_material_instance_texture_parameter_value(material, name)
        unreal.log_warning(f"CLOUD_TEXTURE {name}={value.get_path_name() if value else 'None'}")


if __name__ == "__main__":
    main()
