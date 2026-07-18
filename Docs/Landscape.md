# Sol City Landscape

The city floor is a persistent Unreal Engine Landscape rather than a grid of cube instances.

- Resolution: 505 x 505 vertices
- Layout: 4 x 4 components, 2 x 2 sections per component, 63 quads per section
- Coverage: 36,000 x 36,000 cm (about 71.43 cm per quad)
- River: the same deterministic centerline used by the runtime city generator carves a smooth bed to -230 cm; water renders at -90 cm
- Water: one continuous procedural ribbon with 192 longitudinal segments, replacing overlapping box tiles

After changing the city diameter or river parameters, rebuild and save the map with:

```text
UnrealEditor.exe SolCity.uproject -ExecutePythonScript=Content/Python/BuildSolCityLandscape.py
```
