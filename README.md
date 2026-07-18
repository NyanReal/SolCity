# Sol City

Unreal Engine 5.7 C++ city-builder simulation prototype. It creates a complete colorful city at runtime from engine primitives, so it can boot even before any `.uasset` setup.

## First run

1. Open `SolCity.uproject` in Unreal Engine 5.7 and let it compile.
2. Open `Content/Maps/SolCity` and start Play-in-Editor. The checked-in imagegen textures and materials are loaded automatically.
3. If you replace any source PNG, choose **Tools > Execute Python Script** and run `Content/Python/SetupSolCityAssets.py` to rebuild the imported assets.

## Controls

- `W A S D`: pan across the city
- Mouse wheel: zoom in/out
- Hold right mouse button and move horizontally: rotate

## Runtime systems

- Hierarchical arterial/collector/local roads with curved branches and an irregular ring instead of a uniform grid
- Variable-footprint, multi-part building massing
- Three authored FBX buildings mixed by lot role: corner retail on low commercial lots, mid-rise on medium lots, and stepped towers on tall central lots; all use automatic bounds-based scaling
- Lowered river surface with UV-panning water material
- Detailed bridge deck, abutments, twin piers, crossbeams, railing and posts
- Directed lane graph, autonomous cars, following distance and traffic-light stopping
- Six-phase two-axis traffic signals
- Separate sidewalk graph used exclusively by autonomous cat citizens

Algorithm rationale and sources are in `Docs/ProceduralCityResearch.md`.

The complete English/Korean setup and reproduction instructions, including Blender MCP installation, are in `Docs/ReproductionGuide_EN_KO.md`.
