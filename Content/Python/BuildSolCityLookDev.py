"""Create the deterministic Unreal look-development level used for visual parity.

The level is deliberately isolated from SolCity's gameplay map and runtime lighting.
Run this script from Tools > Execute Python Script or with UnrealEditor-Cmd.

Baseline capture settings:
  - Output: 1600 x 900 PNG, ``SolCity_CornerRetail_lookdev_before.png``
  - Camera: full-frame 36 mm sensor, 53 mm focal length
  - Transform: (3500, -4000, 2400) cm, aimed at (-50, -30, 680) cm
  - Exposure: Manual, physical-camera exposure disabled, compensation 0 EV
  - Key: Directional Light, 3.0 lux, 4.0 degree source angle
  - Fill: Rect Light, 65000 lm, neutral-cool, 1400 x 1800 cm
  - Rim: Rect Light, 45000 lm, warm, 1000 x 1400 cm
  - Ambient: Sky Light, intensity 0.18, lower hemisphere dark gray
  - Reflection: Sphere Reflection Capture, 6000 cm radius

Use ``-SolCityLookDevStage=legacy`` for actor-level legacy material overrides and
``-SolCityLookDevStage=native`` (the safe default) for current mesh defaults.  The
two modes produce matching ``_before`` and ``_after`` captures without modifying
the StaticMesh asset.
"""

from __future__ import annotations

import os
import re

import unreal


LEVEL_PATH = "/Game/Maps/SolCityLookDev"
BUILDING_PATH = "/Game/Art/Buildings/SM_SolCity_CornerRetail_01"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"
BASIC_MATERIAL_PATH = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"
LOOKDEV_MATERIAL_DIR = "/Game/Art/Buildings/Materials"
BACKGROUND_MATERIAL_PATH = f"{LOOKDEV_MATERIAL_DIR}/M_SolCity_LookDev_Backdrop"

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
CAPTURE_DIR = os.path.join(PROJECT_DIR, "Saved", "VisualParity")
CAPTURE_FILENAMES = {
    "legacy": "SolCity_CornerRetail_lookdev_before.png",
    "native": "SolCity_CornerRetail_lookdev_after.png",
}

CAMERA_LOCATION = unreal.Vector(3500.0, -4000.0, 2400.0)
CAMERA_TARGET = unreal.Vector(-50.0, -30.0, 680.0)
BUILDING_LOCATION = unreal.Vector(0.0, 0.0, 0.0)


def _load(path: str):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"Required asset is missing: {path}")
    return asset


def _set(obj, name: str, value, required: bool = True) -> bool:
    """Set an editor property while producing a useful UE-version error."""
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as exc:
        message = f"SolCity LookDev: could not set {obj}.{name}: {exc}"
        if required:
            raise RuntimeError(message) from exc
        unreal.log_warning(message)
        return False


def _component(actor, component_class):
    component = actor.get_component_by_class(component_class)
    if not component:
        raise RuntimeError(
            f"{actor.get_name()} has no {component_class.get_name()} component"
        )
    return component


def _ensure_backdrop_material():
    """Create the project-local, unlit neutral-gray diagnostic background."""
    unreal.EditorAssetLibrary.make_directory(LOOKDEV_MATERIAL_DIR)
    material = unreal.load_asset(BACKGROUND_MATERIAL_PATH)
    if material and not isinstance(material, unreal.Material):
        raise RuntimeError(
            f"{BACKGROUND_MATERIAL_PATH} exists but is not a Material"
        )
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_SolCity_LookDev_Backdrop",
            LOOKDEV_MATERIAL_DIR,
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )
        if not material:
            raise RuntimeError(
                f"Could not create neutral material: {BACKGROUND_MATERIAL_PATH}"
            )
        color = unreal.MaterialEditingLibrary.create_material_expression(
            material, unreal.MaterialExpressionVectorParameter, -260, 0
        )
        color.set_editor_property("parameter_name", "BackdropColor")
        color.set_editor_property(
            "default_value", unreal.LinearColor(0.18, 0.18, 0.18, 1.0)
        )
        if not unreal.MaterialEditingLibrary.connect_material_property(
            color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        ):
            raise RuntimeError("Could not connect neutral backdrop emissive color")
    material.set_editor_property(
        "shading_model", unreal.MaterialShadingModel.MSM_UNLIT
    )
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("two_sided", True)
    unreal.MaterialEditingLibrary.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError(f"Could not save {BACKGROUND_MATERIAL_PATH}")
    return material


def _spawn(actor_subsystem, actor_class, label: str, location, rotation=None):
    actor = actor_subsystem.spawn_actor_from_class(
        actor_class, location, rotation or unreal.Rotator()
    )
    if not actor:
        raise RuntimeError(f"Could not spawn {actor_class} for {label}")
    actor.set_actor_label(label)
    actor.tags = ["SolCityLookDev", label]
    return actor


def _look_at(start, target):
    return unreal.MathLibrary.find_look_at_rotation(start, target)


def _make_studio(actor_subsystem):
    cube = _load(CUBE_PATH)
    basic_material = _load(BASIC_MATERIAL_PATH)
    background_material = _ensure_backdrop_material()

    floor = actor_subsystem.spawn_actor_from_object(
        cube, unreal.Vector(0.0, 0.0, -10.0), unreal.Rotator()
    )
    floor.set_actor_label("LookDev_NeutralFloor")
    floor.tags = ["SolCityLookDev", "NeutralStudio"]
    floor.set_actor_scale3d(unreal.Vector(80.0, 80.0, 0.20))
    floor_component = _component(floor, unreal.StaticMeshComponent)
    floor_component.set_material(0, basic_material)
    _set(floor_component, "cast_shadow", False)

    # A two-sided unlit studio enclosure is more robust than a finite card for
    # an oblique camera: every ray outside the floor hits the same neutral gray.
    wall = actor_subsystem.spawn_actor_from_object(
        cube,
        unreal.Vector(0.0, 0.0, 0.0),
        unreal.Rotator(),
    )
    wall.set_actor_label("LookDev_NeutralBackdrop")
    wall.tags = ["SolCityLookDev", "NeutralStudio"]
    wall.set_actor_scale3d(unreal.Vector(300.0, 300.0, 300.0))
    wall_component = _component(wall, unreal.StaticMeshComponent)
    # The project-local unlit material prevents the subject from casting an
    # enormous near-black shadow across the diagnostic background.
    wall_component.set_material(0, background_material)
    _set(wall_component, "cast_shadow", False)
    wall_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)


def _capture_stage() -> str:
    """Read ``-SolCityLookDevStage=legacy|native``; default safely to native."""
    command_line = unreal.SystemLibrary.get_command_line()
    match = re.search(
        r"(?:^|\s)-SolCityLookDevStage=(legacy|native)(?:\s|$)",
        command_line,
        flags=re.IGNORECASE,
    )
    return match.group(1).lower() if match else "native"


def _make_building(actor_subsystem, stage: str):
    mesh = _load(BUILDING_PATH)
    building = actor_subsystem.spawn_actor_from_object(
        mesh, BUILDING_LOCATION, unreal.Rotator()
    )
    if not building:
        raise RuntimeError(f"Could not spawn {BUILDING_PATH}")
    building.set_actor_label("LookDev_SM_SolCity_CornerRetail_01")
    building.tags = ["SolCityLookDev", "ParitySubject", f"{stage.title()}MaterialStage"]
    building.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))

    if stage == "legacy":
        component = _component(building, unreal.StaticMeshComponent)
        static_materials = mesh.get_editor_property("static_materials")
        missing = []
        for material_index, slot in enumerate(static_materials):
            slot_name = str(slot.get_editor_property("material_slot_name"))
            legacy_path = f"/Game/Art/Buildings/{slot_name}"
            legacy_material = unreal.load_asset(legacy_path)
            if not legacy_material:
                missing.append(f"{slot_name} -> {legacy_path}")
                continue
            # Actor override only: the StaticMesh asset and its native defaults
            # are never modified by the baseline capture.
            component.set_material(material_index, legacy_material)
        if missing:
            raise RuntimeError(
                "Missing legacy LookDev materials: " + ", ".join(missing)
            )
    return building


def _make_lighting(actor_subsystem):
    key_rotation = unreal.Rotator(-38.0, -35.0, 0.0)
    key = _spawn(
        actor_subsystem,
        unreal.DirectionalLight,
        "LookDev_Key_Directional",
        unreal.Vector(0.0, 0.0, 2600.0),
        key_rotation,
    )
    key_component = _component(key, unreal.DirectionalLightComponent)
    _set(key_component, "mobility", unreal.ComponentMobility.MOVABLE)
    _set(key_component, "intensity", 3.0)
    _set(key_component, "light_color", unreal.Color(255, 232, 210, 255))
    _set(key_component, "light_source_angle", 4.0)
    _set(key_component, "indirect_lighting_intensity", 1.0)
    _set(key_component, "contact_shadow_length", 0.08, required=False)

    fill_location = unreal.Vector(-2600.0, -2600.0, 1700.0)
    fill = _spawn(
        actor_subsystem,
        unreal.RectLight,
        "LookDev_Fill_Rect",
        fill_location,
        _look_at(fill_location, CAMERA_TARGET),
    )
    fill_component = _component(fill, unreal.RectLightComponent)
    _set(fill_component, "mobility", unreal.ComponentMobility.MOVABLE)
    _set(fill_component, "intensity_units", unreal.LightUnits.LUMENS)
    _set(fill_component, "intensity", 65000.0)
    _set(fill_component, "light_color", unreal.Color(190, 215, 255, 255))
    _set(fill_component, "source_width", 1400.0)
    _set(fill_component, "source_height", 1800.0)
    _set(fill_component, "attenuation_radius", 9000.0)
    _set(fill_component, "cast_shadows", False)

    rim_location = unreal.Vector(2300.0, 2100.0, 2200.0)
    rim = _spawn(
        actor_subsystem,
        unreal.RectLight,
        "LookDev_Rim_Rect",
        rim_location,
        _look_at(rim_location, CAMERA_TARGET),
    )
    rim_component = _component(rim, unreal.RectLightComponent)
    _set(rim_component, "mobility", unreal.ComponentMobility.MOVABLE)
    _set(rim_component, "intensity_units", unreal.LightUnits.LUMENS)
    _set(rim_component, "intensity", 45000.0)
    _set(rim_component, "light_color", unreal.Color(255, 188, 145, 255))
    _set(rim_component, "source_width", 1000.0)
    _set(rim_component, "source_height", 1400.0)
    _set(rim_component, "attenuation_radius", 8000.0)
    _set(rim_component, "cast_shadows", False)

    skylight = _spawn(
        actor_subsystem,
        unreal.SkyLight,
        "LookDev_Ambient_SkyLight",
        unreal.Vector(0.0, 0.0, 1800.0),
    )
    sky_component = _component(skylight, unreal.SkyLightComponent)
    _set(sky_component, "mobility", unreal.ComponentMobility.MOVABLE)
    _set(sky_component, "intensity", 0.18)
    _set(sky_component, "lower_hemisphere_is_black", True)
    _set(
        sky_component,
        "lower_hemisphere_color",
        unreal.LinearColor(0.025, 0.030, 0.040, 1.0),
    )
    _set(sky_component, "real_time_capture", False)

    reflection = _spawn(
        actor_subsystem,
        unreal.SphereReflectionCapture,
        "LookDev_ReflectionCapture",
        unreal.Vector(0.0, 0.0, 900.0),
    )
    reflection_component = _component(
        reflection, unreal.SphereReflectionCaptureComponent
    )
    _set(reflection_component, "influence_radius", 6000.0)
    _set(reflection_component, "brightness", 1.0)


def _make_camera(actor_subsystem):
    camera = _spawn(
        actor_subsystem,
        unreal.CineCameraActor,
        "LookDev_Camera_53mm",
        CAMERA_LOCATION,
        _look_at(CAMERA_LOCATION, CAMERA_TARGET),
    )
    cine_component = _component(camera, unreal.CineCameraComponent)
    _set(cine_component, "current_focal_length", 53.0)
    filmback = cine_component.get_editor_property("filmback")
    _set(filmback, "sensor_width", 36.0)
    _set(filmback, "sensor_height", 20.25)
    _set(cine_component, "filmback", filmback)
    _set(cine_component, "current_aperture", 8.0)
    _set(cine_component, "focus_settings", unreal.CameraFocusSettings(
        focus_method=unreal.CameraFocusMethod.DISABLE
    ))
    _set(camera, "auto_activate_for_player", unreal.AutoReceiveInput.PLAYER0, required=False)
    return camera


def _make_post_process(actor_subsystem):
    volume = _spawn(
        actor_subsystem,
        unreal.PostProcessVolume,
        "LookDev_PostProcess_ManualExposure",
        unreal.Vector(0.0, 0.0, 0.0),
    )
    _set(volume, "unbound", True)
    _set(volume, "priority", 100.0)
    settings = volume.get_editor_property("settings")
    _set(settings, "override_auto_exposure_method", True)
    _set(settings, "auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    _set(settings, "override_auto_exposure_apply_physical_camera_exposure", True)
    _set(settings, "auto_exposure_apply_physical_camera_exposure", False)
    _set(settings, "override_auto_exposure_bias", True)
    _set(settings, "auto_exposure_bias", 0.0)
    _set(settings, "override_bloom_intensity", True)
    _set(settings, "bloom_intensity", 0.25)
    _set(settings, "override_ambient_occlusion_intensity", True)
    _set(settings, "ambient_occlusion_intensity", 0.65)
    _set(settings, "override_ambient_occlusion_radius", True)
    _set(settings, "ambient_occlusion_radius", 120.0)
    _set(settings, "override_color_saturation", True)
    _set(settings, "color_saturation", unreal.Vector4(1.0, 1.0, 1.0, 1.0))
    _set(settings, "override_color_contrast", True)
    _set(settings, "color_contrast", unreal.Vector4(1.03, 1.03, 1.03, 1.0))
    _set(volume, "settings", settings)
    return volume


def _configure_world():
    unreal_editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = unreal_editor.get_editor_world()
    if not world:
        raise RuntimeError("No editor world is available")
    world_settings = world.get_world_settings()
    # Prevent SolCityGameMode from populating this diagnostic map during PIE/game.
    base_game_mode = unreal.load_class(None, "/Script/Engine.GameModeBase")
    _set(world_settings, "default_game_mode", base_game_mode)
    _set(world_settings, "force_no_precomputed_lighting", True)
    return world


def _capture(camera, stage: str):
    os.makedirs(CAPTURE_DIR, exist_ok=True)
    capture_path = os.path.join(CAPTURE_DIR, CAPTURE_FILENAMES[stage])
    # Automation screenshots do not reliably replace an existing comparison
    # file, so make idempotent reruns explicit.
    if os.path.isfile(capture_path):
        os.remove(capture_path)
    task = unreal.AutomationLibrary.take_high_res_screenshot(
        1600, 900, capture_path, camera=camera
    )
    unreal.log(f"SolCity LookDev: {stage} capture requested: {capture_path}")
    return task


def main():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    stage = _capture_stage()

    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        if not level_editor.load_level(LEVEL_PATH):
            raise RuntimeError(f"Could not load existing level: {LEVEL_PATH}")
        for actor in actor_subsystem.get_all_level_actors():
            actor_subsystem.destroy_actor(actor)
    elif not level_editor.new_level(LEVEL_PATH):
        raise RuntimeError(f"Could not create level: {LEVEL_PATH}")

    _configure_world()
    _make_studio(actor_subsystem)
    _make_building(actor_subsystem, stage)
    _make_lighting(actor_subsystem)
    camera = _make_camera(actor_subsystem)
    _make_post_process(actor_subsystem)

    if not level_editor.save_current_level():
        raise RuntimeError(f"Could not save level: {LEVEL_PATH}")

    _capture(camera, stage)
    unreal.log(
        f"SolCity LookDev: deterministic {stage} level saved at {LEVEL_PATH}; "
        "focal length=53mm; capture=1600x900."
    )


if __name__ == "__main__":
    main()
