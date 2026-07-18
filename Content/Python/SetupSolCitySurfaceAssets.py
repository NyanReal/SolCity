"""Import SolCity road/sidewalk sources and build the surface material library.

This focused entry point avoids reimporting authored buildings. Run it from
Tools > Execute Python Script or UnrealEditor-Cmd after changing surface art.
"""

import SetupSolCityAssets


def main():
    imported = {
        "AnimeGrass": SetupSolCityAssets.unreal.load_asset(
            f"{SetupSolCityAssets.TEXTURE_DIR}/T_AnimeGrass"
        ),
        "AnimeAsphalt": SetupSolCityAssets.unreal.load_asset(
            f"{SetupSolCityAssets.TEXTURE_DIR}/T_AnimeAsphalt"
        ),
        "AnimeSidewalkPavers": SetupSolCityAssets.import_texture(
            "AnimeSidewalkPavers",
            SetupSolCityAssets.ASSETS["AnimeSidewalkPavers"],
        ),
    }
    for key in ("AnimeGrass", "AnimeAsphalt"):
        if not imported[key]:
            imported[key] = SetupSolCityAssets.import_texture(
                key, SetupSolCityAssets.ASSETS[key]
            )
    SetupSolCityAssets.make_material(
        "AnimeSidewalkPavers", imported["AnimeSidewalkPavers"]
    )
    SetupSolCityAssets.make_city_surface_library(imported)
    SetupSolCityAssets.unreal.log(
        "SolCity: hierarchical roads, sidewalk pavers and ground-zone materials are ready."
    )


if __name__ == "__main__":
    main()
