"""Apply the art-direction material tuning without rebuilding the map.

Run with Tools > Execute Python Script after SetupSolCityAssets.py, or whenever
the grass source texture is replaced. The existing material is edited in-place.
"""

import os
import sys

import unreal


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPT_DIR not in sys.path:
    sys.path.insert(0, SCRIPT_DIR)

import SetupSolCityAssets as setup


def main():
    material = unreal.load_asset(f"{setup.MATERIAL_DIR}/M_AnimeGrass")
    texture = unreal.load_asset(f"{setup.TEXTURE_DIR}/T_AnimeGrass")
    if not material or not texture:
        raise RuntimeError("Run SetupSolCityAssets.py before tuning the environment.")

    setup.configure_material(material, "AnimeGrass", texture, is_water=False)
    setup.make_distant_ground_material()
    unreal.log(
        "Sol City: grass tuning and the fog-friendly distant-ground material are ready."
    )


if __name__ == "__main__":
    main()
