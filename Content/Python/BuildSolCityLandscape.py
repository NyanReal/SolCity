"""Rebuild the persistent UE Landscape used by the Sol City runtime map."""

import unreal


LEVEL_PATH = "/Game/Maps/SolCity"


def main():
    level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not level_editor.load_level(LEVEL_PATH):
        raise RuntimeError(f"Could not load {LEVEL_PATH}")

    if not unreal.SolCityLandscapeLibrary.rebuild_sol_city_landscape(36000.0, 2400.0, -90.0):
        raise RuntimeError("Sol City Landscape rebuild failed")
    if not level_editor.save_current_level():
        raise RuntimeError("Could not save the Sol City level")
    unreal.log("SolCity: persistent 505x505 Landscape and carved riverbed saved.")


if __name__ == "__main__":
    main()
