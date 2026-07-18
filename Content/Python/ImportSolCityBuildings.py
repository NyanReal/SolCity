"""Incrementally import only missing authored FBX buildings.

This entry point is safe to run while the project is open because it neither
deletes nor recreates existing textures, materials, maps, or building meshes.
"""

import SetupSolCityAssets


SetupSolCityAssets.import_authored_buildings(replace_existing=False)
SetupSolCityAssets.enable_instanced_material_usage()
