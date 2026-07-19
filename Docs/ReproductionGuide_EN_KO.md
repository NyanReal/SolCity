# Sol City Reconstruction Guide / Sol City 리컨스트럭션 가이드

This document describes the implementation that is currently checked into the repository. The English and Korean sections intentionally contain the same operational information.

이 문서는 현재 저장소에 들어 있는 구현을 기준으로 작성한다. 영문과 한국어 섹션은 동일한 재구성 절차와 기준값을 설명한다.

---

## English

### 1. Current target

Reconstruct a UE 5.7 C++ city-builder prototype with a bright Japanese-anime-background look, a dense central business district, low-density outer neighborhoods, road and rail transport, and cat citizens.

The authoritative defaults are defined on `ASolCityGenerator`:

| Setting | Current default |
|---|---:|
| Seed | `71527` |
| City footprint | `144,000 x 144,000 cm` (1.44 x 1.44 km) |
| River width | `6,000 cm` |
| River surface | `Z = -90 cm` |
| Target buildings | `1,500` (clamped to `12..2,400`) |
| Runtime regeneration | Enabled in editor and at BeginPlay |

The generated result includes:

- An irregular hierarchy of 1-lane local roads, 2-lane collectors, and 4-lane arterials/bridges.
- Separate raised sidewalks and 20 cm curbs, with curb and sidewalk intervals removed inside intersections.
- A continuous river ribbon with retaining walls, riverside walks, railings, green banks, and bridge transitions.
- A double-track perimeter railway with two river crossings, truss rail bridges, two moving four-car commuter consists, and animated at-grade crossing barriers.
- Continuous district underlays plus slightly overlapping zoning tiles, eliminating the old dark grid gaps between grass cells.
- Road-derived city blocks and parcels rather than point-only building scatter.
- Pedestrian-only interior mews that fill eligible oversized outer blocks with inward-facing low-rise homes.
- Residential, commercial, park, parking, and courtyard land uses.
- Dense CBD massing, four authored mega-skyscraper landmarks, tapered towers, corner buildings, and connected twin towers.
- Wide one-story authored suburban houses and more trees in outer neighborhoods.
- Benches, bins, lamps, planters, bollards, wheel stops, billboards, rooftop equipment, cars, and detailed voxel cats.

The validated default-seed runtime snapshot from 2026-07-19 is:

```text
blocks=113 buildings=1500/1500 parks=8 parking=9 courtyards=9 interiorMews=16 interiorHomes=78
tapered=194 twinSkybridge=4 megaCBD=5 outerLowRise=754
authored suburban=244 authored megaCBD=4
billboards=48 skybridges=7
trees=1068 conifers=532 curbs=1366
railway segments=352 trackMeshes=342 bridgeMeshes=10 consists=2 trainCars=8
level crossings=22 barrierBases=44 barrierBooms=44
```

Counts can change when the seed, target count, city diameter, or available assets change. The invariants are successful target completion, four authored mega-CBD landmarks when their assets are available, visible skybridges, four billboard material variants, a closed perimeter railway with both river crossings bridged, paired animated barriers at every detected at-grade road crossing, and no HISM material-usage errors.

### 2. Required software and checkout

- Windows 10 or 11
- Git
- Unreal Engine 5.7
- Visual Studio 2022 with **Desktop development with C++** and a Windows SDK
- Blender 5.2 LTS
- Python 3.12 for the Blender MCP runtime
- Codex with the configured `blender` MCP server when models must be regenerated

```powershell
git clone https://github.com/NyanReal/SolCity.git
Set-Location SolCity
```

The checked-in `.uasset`, `.blend`, `.fbx`, preview PNG, and source texture files are sufficient to build and run the project. Blender MCP and the Unreal Python import scripts are needed when reconstructing or changing source assets.

The project enables:

- `ProceduralMeshComponent`
- `PythonScriptPlugin`
- `EditorScriptingUtilities`

The default map is `/Game/Maps/SolCity` and the default game mode is `/Script/SolCity.SolCityGameMode`.

### 3. Blender MCP setup

```powershell
git clone https://github.com/djeada/blender-mcp-server.git Tools/blender-mcp-server
py -3.12 -m venv Tools/blender-mcp-server/.mcp-runtime
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install --upgrade pip
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install -e Tools/blender-mcp-server
```

Build and install the Blender add-on:

```bash
cd Tools/blender-mcp-server
bash scripts/build_addon_zip.sh
```

Install `Tools/blender-mcp-server/dist/blender_mcp_bridge.zip` from **Edit > Preferences > Add-ons > Install from Disk**, enable **Blender MCP Bridge**, and confirm `Listening on 127.0.0.1:9876` in the viewport MCP sidebar.

Register the server with Codex, adjusting the repository path as needed:

```powershell
codex mcp add blender -- D:\github\SolCity\Tools\blender-mcp-server\.mcp-runtime\Scripts\blender-mcp-server.exe
codex mcp list
```

Always inspect the connected Blender scene before editing. Use a separate Blender process when ownership of an existing scene is uncertain.

### 4. Blender authoring and FBX validation

Follow `Docs/BlenderMCP_Modeling_Guide.md`. A Blender task is complete only after the scene was actually modified through MCP, the preview was inspected, and the FBX was re-imported into a clean Blender 5.2 scene.

Use metric units, `scale_length = 1.0`, ground contact at `Z = 0`, selected mesh export, `axis_forward = -Y`, and `axis_up = Z`. Store reusable creation and verification scripts in `Tools/blender-mcp-server/scripts/`.

Building deliverables belong in `Content/Art/Buildings/` and props in `Content/Art/Props/`. Each kit must keep matching names:

```text
SM_SolCity_<Name>_01.blend
SM_SolCity_<Name>_01.fbx
SM_SolCity_<Name>_01_preview.png
```

Current expansion kits:

| Kit | Internal meshes | Validated bounds |
|---|---|---|
| `SM_SolCity_SuburbanHouses_01` | `SM_SolCity_SuburbanHouse_01..04` | each `17.250 x 12.798 x 6.640 m` |
| `SM_SolCity_Conifers_01` | `SM_SolCity_Conifer_01` | `4.972 x 5.100 x 7.568 m` |
|  | `SM_SolCity_Conifer_02` | `4.192 x 4.300 x 9.585 m` |
| `SM_SolCity_VoxelCat_01` | one mesh | `1.250 x 1.896 x 1.198 m` |
| `SM_SolCity_DoubleTrack_01` | one double-track module | `12.000 x 8.250 x 0.590 m` |
| `SM_SolCity_RailBridge_01` | one double-track truss module | `12.000 x 9.100 x 6.330 m` |
| `SM_SolCity_CommuterTrain_01` | one reusable commuter car | `34.190 x 3.430 x 5.360 m` |
| `SM_SolCity_LevelCrossingBarrierBase_01` | one reusable cabinet, warning light, and crossbuck assembly | `0.924 x 0.860 x 2.832 m` |
| `SM_SolCity_LevelCrossingBarrierBoom_01` | one independently rotating hinge-pivot boom | `6.880 x 0.380 x 0.460 m` |
| `SM_SolCity_MegaSkyscrapers_01` | `SM_SolCity_MegaGlassCurtainwall_01` | `48.000 x 42.425 x 225.000 m` |
|  | `SM_SolCity_MegaNewYorkSetback_01` | `52.000 x 44.700 x 228.500 m` |
|  | `SM_SolCity_MegaGeometricTwist_01` | `49.889 x 52.336 x 223.500 m` |
|  | `SM_SolCity_MegaPodiumCrown_01` | `56.000 x 54.500 x 227.500 m` |

All listed meshes validate with non-degenerate bounds after clean FBX re-import. Grounded assets have `minZ = 0`; the barrier boom intentionally spans `Z = -0.23..0.23 m` around its hinge origin. The repository-wide commit-friendly preview gallery is `Content/Art/PreviewGallery/`; it currently contains copies of all 25 `_preview.png` files while the authoritative previews remain beside their source assets.

Import the house, conifer, and mega-tower kits with **Combine Meshes = false**. Import the voxel cat with **Combine Meshes = true**. Generate lightmap UVs and automatic collision.

Variable-height procedural buildings use authored modules:

- `SM_SolCity_BuildingBase_01`
- `SM_SolCity_BuildingMiddle_01`
- `SM_SolCity_BuildingCrown_01`
- `SM_SolCity_CornerBase_01`
- `SM_SolCity_CornerMiddle_01`
- `SM_SolCity_CornerCrown_01`

Optional roof and landmark modules include HVAC, water tank, solar, antenna, pergola, helipad, warning beacon, and skybridge connector meshes. Completed ordinary buildings retain uniform XYZ scaling. Mega-CBD landmarks are the deliberate exception: XY scale fits the parcel while vertical scale is kept at least `0.86` so their skyline height remains monumental.

The car and cat FBXs use the Blender `-Y` forward convention. Their Unreal mesh components apply a relative `-90 degree` yaw while actor navigation remains Unreal `+X` forward. The active cat asset is `SM_SolCity_VoxelCat_01` at runtime scale `0.62`.

### 5. Unreal asset reconstruction

Checked-in Unreal assets normally require no setup. To reconstruct assets from source, run the scripts from **Tools > Execute Python Script** or with `UnrealEditor-Cmd.exe` in this order:

1. `Content/Python/SetupSolCityAssets.py`
2. `Content/Python/SetupSolCitySurfaceAssets.py`
3. `Content/Python/ImportSolCityBuildingModules.py`
4. `Content/Python/ImportSolCityExtendedBuildingModules.py`
5. `Content/Python/ImportSolCityProps.py`
6. `Content/Python/ImportSolCityUrbanProps.py`
7. `Content/Python/ImportSolCityExpansionAssets.py`
8. `Content/Python/ImportSolCityRailwayAssets.py`
9. `Content/Python/ImportSolCityLevelCrossingAssets.py`
10. `Content/Python/SetupSolCityBillboard.py`
11. `Content/Python/CreateSolCitySplineMaterials.py`
12. `Content/Python/SetupSolCityBuildingMaterials.py`
13. `Content/Python/FinalizeSolCityMaterials.py`
14. `Content/Python/TuneSolCityEnvironment.py`

`ImportSolCityExpansionAssets.py` imports 11 meshes: four houses, two conifers, four mega towers, and one voxel cat. It also enables instanced-static-mesh usage on imported base materials.

`ImportSolCityRailwayAssets.py` imports the double-track, rail-bridge, and commuter-train FBXs and creates the railway/train materials under `/Game/Art/Props`. The script is idempotent and skips assets that already exist. UE 5.7 unattended Interchange can terminate with a `SlateApplication.h` assertion after an FBX has already been saved; prefer **Tools > Execute Python Script**, or rerun the commandlet one asset/process at a time until all three `.uasset` files exist.

`ImportSolCityLevelCrossingAssets.py` imports the barrier base and separate hinge-origin boom, enables combined meshes, lightmap UV generation, and collision, and creates `M_Crossing_WarningSignal`. The signal material reads HISM custom-data float `0`: `0` is emissive green and `1` is emissive red. The generator overrides only the authored warning-light slot at runtime, so the source mesh remains reusable. The source `.blend`, `.fbx`, and preview PNG files remain beside the imported props.

`SetupSolCityBillboard.py` imports the billboard, four language-free ad textures, one two-sided HISM-compatible master, and four swappable material instances. The mesh uses frame slot `0` and ad slot `1`. The script currently contains `PROJECT = Path(r"D:\github\SolCity")`; change that value when the checkout is elsewhere.

Source textures are under `Content/Art/Source/`. Runtime textures and materials are under `/Game/Art/Generated`, `/Game/Art/Materials`, `/Game/Art/Buildings`, and `/Game/Art/Props`.

`Config/DefaultGame.ini` force-cooks maps, materials, generated art, props, and buildings because several runtime assets are loaded by string path.

#### Landscape note

The current runtime generator is authoritative for the 1.44 km ground, zoning surfaces, river, banks, walls, and distant-ground transition. `USolCityLandscapeLibrary` defaults to `144000, 6000, -90`, but `Content/Python/BuildSolCityLandscape.py` still passes the legacy `36000, 2400, -90` arguments. Do not run that script unchanged for the current city. If a persistent Landscape must be rebuilt, update the call to:

```python
unreal.SolCityLandscapeLibrary.rebuild_sol_city_landscape(144000.0, 6000.0, -90.0)
```

### 6. Current procedural generation

`ASolCityGenerator::RegenerateCity()` runs in this order:

1. Clear generated components and reset deterministic state.
2. Create HISM and spline instance groups.
3. Generate ground, river, banks, walls, walks, rails, and distant transitions.
4. Generate the complete road hierarchy.
5. Generate the perimeter railway, rail bridges, trains, and detected at-grade crossing barriers.
6. Generate curb and sidewalk strips after all roads are known.
7. Generate district surfaces.
8. Reserve and place traffic furniture, including billboards.
9. Extract blocks, subdivide parcels, reserve open spaces, generate interior mews, and place buildings.
10. Place the authored road bridge.
11. Place broadleaf and conifer trees while respecting railway clearance.
12. Finalize HISM trees and emit validation counts.

Road geometry uses a 360 cm lane ruler:

| Class | Marked lanes | Road width | Sidewalk width per side |
|---|---:|---:|---:|
| Local | 1 | `360 cm` | `160 cm` |
| Collector | 2 | `720 cm` | `240 cm` |
| Arterial / bridge | 4 | `1,440 cm` | `320 cm` |

Road surface is at `Z = 8 cm`; sidewalk top is `Z = 23 cm`; curb width is `20 cm`. Curbs and sidewalks are not created inline with each road. The generator first records all road segments, finds 2D segment intersections, subtracts an intersection-clearance interval from each edge strip, and creates only the visible remainder. This prevents curbs from crossing intersections.

The zoning system creates continuous residential underlays first, then residential, commercial, park, or parking cells. Cell dimensions overlap their pitch by 16 cm, so the distant ground cannot appear as dark seams between cells.

`SolCityLotLayout` extracts closed blocks from road centerlines and subdivides street-front parcels. Building placement rejects city-edge, river, road, railway, occupied-building, billboard, park, parking, and courtyard overlaps. Open/disconnected road graphs receive deterministic frontage-lot fallback placement rather than city-wide point scatter.

Before ordinary frontage placement, eligible oversized outer blocks receive a 150 cm pedestrian mews from an existing roadside sidewalk into the block. One-story homes are tested on both sides with their authored `-Y` entrance facing the mews. Polygon corners, river samples, roads, railway, existing occupancy, and candidate OBB overlaps are rejected. The path, entrance spurs, pedestrian waypoint pair, and reserved corridor are committed only when at least two homes place successfully.

The railway is a deterministic closed ellipse inside the city edge. Segment count follows circumference and is clamped to `96..520`. Both crossings of the meandering river use the truss bridge mesh at a `420 cm` running surface, with smooth `1,900 cm` approaches down to the `59 cm` ground-running surface. Two four-car consists run in opposite directions on opposite tracks. Every car samples its own cumulative route distance, so the cars keep the authored length plus an `80 cm` coupler gap and align independently to curves and bridge grades. Position stays on the authored polyline while a centered chord supplies the tangent; a world-up quaternion removes segment-boundary yaw snaps, reverse twitches, and the loop-seam discontinuity. The eight moving cars are ordinary movable `UStaticMeshComponent` objects, not HISM instances, so each car updates its own render bounds and cannot be incorrectly removed by a stale hierarchical cluster. Default speed is `1,800 cm/s` (`64.8 km/h`). Buildings, open-space props, mews, billboards, trees, conifers, and traffic furniture test their real rotated footprints against railway clearance, and `GetRailwayPathWaypoints()` exposes the closed route.

At-grade crossing generation intersects ground-level railway and road segments in 2D, rejects rail bridges, road bridges, parallel segments, duplicate intersections within `900 cm`, and vertical separation above `140 cm`. Each accepted crossing places one barrier on each roadway approach, with the cabinet outside the curb and a separately instanced boom directed toward the road center. Boom X scale expands only when required for wider roads. The lamps change from green to red `12,000 cm` before arrival, while the boom deliberately waits until `9,000 cm` before arrival to start closing. It stays red and closed until the last of the four cars plus `1,200 cm` clearance has passed. The boom then opens at normalized speed `0.75/s`, and the lamps return to green only after it reaches the fully raised `82 degree` angle. These values are editable under **Sol City > Railway > Level Crossing**.

Procedural building styles are:

- Setback mid-rise.
- Authored corner modules with an L-shaped fallback.
- Tapered tower whose requested setback rectangles shrink monotonically.
- Courtyard cluster.
- Authored suburban house or low-rise row-house fallback.
- Symmetric twin towers with one or two elevated skybridges.
- Super-scale CBD tower with a broad podium and monotonic setbacks.

Before ordinary frontage lots consume the center, the generator reserves large central block interiors for four authored mega-skyscraper variants. Additional eligible CBD lots can use the procedural mega style. Large roofs receive appropriate helipads, water tanks, HVAC, solar, antenna/pergola units, and warning beacons.

Known implementation limitation: tapered floors use one uniform XY mesh scale but calculate each setback offset from the requested X/Y rectangle. When parcel and module aspect ratios differ, requested-rectangle containment does not guarantee that the upper module is supported by the actual lower module bounds. This is the cause of the visible partial overhang in some towers; actual lower/upper mesh support rectangles must be used before this style is considered structurally valid.

Outer neighborhoods use wider/deeper parcels, reduced building plates, four colored one-story house variants, and higher tree preference. Up to 38 percent of eligible outer-neighborhood tree placements use one of the two conifers.

Billboards are placed before buildings so their sites remain visible. At the current diameter the target is 48. Both road sides are sampled, the opposite side receives a 180-degree yaw correction, placement Z matches the ground zone, and the four language-free materials rotate evenly.

### 7. Simulation, camera, and environment

Cars use a directed road graph with lane speed limits, acceleration/braking, following distance, red-light stopping, and six traffic-signal phases across two conflicting axes. Cats use a separate pedestrian waypoint graph generated from the visible sidewalk strips and never intentionally traverse vehicle lanes.

Camera defaults:

| Setting | Value |
|---|---:|
| Initial spring arm | `11,000 cm` |
| City zoom range | `800..48,000 cm` |
| Wheel zoom step | `2,500 cm` |
| Zoom interpolation | `5.0` |
| Pan speed | `5,000 cm/s` |
| City pitch | `-80..-25 degrees` |
| Free-flight speed | `6,000 cm/s` |
| Free-flight speed range | `500..30,000 cm/s` |
| Free-flight pitch | `-89..89 degrees` |

Use `W A S D` to pan, RMB drag to rotate/change pitch, the wheel to zoom, and `F` to toggle free flight. `T` toggles railway time: the two train consists pause in place and resume from the same cumulative route distance while water, road traffic, and the rest of the city continue. Crossing barriers keep evaluating the stopped consist and finish opening or closing, so a train paused in the approach zone remains safely protected. In free flight use `Q/E` for down/up and the wheel to change speed. Returning from free flight restores the saved city transform, arm rotation, current arm length, and desired zoom.

The runtime creates or reuses Sky Atmosphere, directional sun, Sky Light, Exponential Height Fog, Volumetric Cloud, and an unbound Post Process Volume. It uses manual exposure, AO `0.45`, AO radius `120`, AO power `1.2`, saturation `0.97`, contrast `1.02`, bloom `0.25`, and bloom threshold `1.4`. A large collisionless, shadowless plane at `Z = -280 cm` hides the finite-world edge behind height fog.

### 8. Build and verification

Build the editor target:

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" `
  SolCityEditor Win64 Development "$PWD\SolCity.uproject" `
  -WaitMutex -NoHotReloadFromIDE
```

Run a headless BeginPlay generation smoke test:

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$PWD\SolCity.uproject" -game -nullrhi -nosound -unattended `
  -stdout -FullStdOutLogOutput "-ExecCmds=quit"
```

Inspect `Saved/Logs/SolCity.log` for:

- `SolCity parcel layout`
- `SolCity expansion authored buildings`
- `SolCity urban props`
- `SolCity authored modules`
- `SolCity streetscape`
- `SolCity perimeter railway`
- `SolCity level crossings`

There must be no `missing bUsedWithInstancedStaticMeshes`, `Default Material will be used`, Python traceback, assertion, or fatal error.

Visual PIE verification:

- No dark grid lines between green district cells.
- Curbs and sidewalks stop before intersection road surfaces.
- Local, collector, and arterial roads read as distinct widths and markings.
- Four authored mega-tower silhouettes are present in the CBD.
- Twin towers have clearly visible elevated connectors.
- Outer neighborhoods contain wide roofed one-story houses in several colors.
- Billboards face the road and visibly rotate four non-verbal ads.
- Conifers are mixed among broadleaf trees.
- The voxel cat has visible eyes, ears, muzzle, whiskers, paws, and segmented tail and follows sidewalk direction.
- The perimeter double track is closed, both river crossings use rail bridges, approaches are continuous, and two four-car commuter consists move in opposite directions on opposite tracks.
- Oversized outer blocks that receive interior homes also receive a narrow pedestrian mews, and the house entrances face it.
- Every tapered upper module is contained by the actual supporting module bounds, not only by its requested parcel rectangle.
- River water, green bank, retaining wall, promenade, rail, bridge, and concrete transitions do not leave open gaps.
- Every ground-level road/rail crossing has two cabinets outside the road edge. Its lamps turn red before boom motion begins, the separate booms remain down through the fourth car, and the lamps return to green only when the booms are fully raised.
- Trains rotate continuously through every polyline corner and the loop seam without a brief reverse-yaw twitch.
- Train bodies and shadows remain visible together from every camera angle because each moving car owns dynamic component bounds rather than a static HISM cluster bound.

For each regenerated Blender asset, also inspect its `_preview.png` and repeat the clean Blender 5.2 FBX import validation before accepting it.

---

## 한국어

### 1. 현재 목표 결과

밝은 일본 애니메이션 배경풍의 UE 5.7 C++ 도시 빌더 프로토타입을 재구성한다. 도심에는 고밀도 CBD가 있고 외곽에는 저밀도 주거지가 있으며, 도로·철도 교통과 고양이 시민이 별도 네트워크를 이용한다.

`ASolCityGenerator`의 권위 기본값은 다음과 같다.

| 설정 | 현재 기본값 |
|---|---:|
| 시드 | `71527` |
| 도시 영역 | `144,000 x 144,000 cm` (1.44 x 1.44 km) |
| 강 폭 | `6,000 cm` |
| 수면 높이 | `Z = -90 cm` |
| 목표 건물 수 | `1,500` (`12..2,400` 범위) |
| 런타임 재생성 | 에디터와 BeginPlay에서 활성화 |

현재 결과에는 다음 요소가 포함된다.

- 1차로 골목길, 2차로 집산도로, 4차로 간선도로와 교량으로 구성된 비정형 도로 계층.
- 차도보다 높은 별도 인도와 20cm 경석. 교차로 내부에는 경석과 인도를 생성하지 않는다.
- 하나로 이어진 강, 옹벽, 강변 산책로, 난간, 녹지 둔치와 교량 진입부.
- 도시 외곽을 닫힌 경로로 순환하는 복선 철도, 강을 두 번 건너는 트러스 철교, 운행하는 4량 통근열차 편성 2개와 움직이는 평면 건널목 차단기.
- 연속 지면 받침과 서로 약간 겹치는 용도지역 셀. 예전 녹지 격자 틈은 제거됐다.
- 포인트 산포가 아니라 도로에서 추출한 도시 블록과 대지 필지에 따른 건물 배치.
- 큰 외곽 블록 내부를 보행 전용 좁은 길과 길을 향한 저층 주택으로 채우는 mews 배치.
- 주거·상업·공원·주차장·중정 용도지역.
- 고밀도 CBD, authored 초대형 마천루 4종, 상부가 좁아지는 타워, 코너 건물과 공중 연결 쌍둥이 건물.
- 외곽의 넓은 단층 주택과 풍부한 수목.
- 벤치, 쓰레기통, 가로등, 화분, 볼라드, 주차 스토퍼, 광고판, 옥상 설비, 차량과 상세 복셀 고양이.

2026-07-19 기본 시드 검증 결과는 다음과 같다.

```text
blocks=113 buildings=1500/1500 parks=8 parking=9 courtyards=9 interiorMews=16 interiorHomes=78
tapered=194 twinSkybridge=4 megaCBD=5 outerLowRise=754
authored suburban=244 authored megaCBD=4
billboards=48 skybridges=7
trees=1068 conifers=532 curbs=1366
railway segments=352 trackMeshes=342 bridgeMeshes=10 consists=2 trainCars=8
level crossings=22 barrierBases=44 barrierBooms=44
```

시드, 목표 건물 수, 도시 크기나 로드 가능한 에셋이 바뀌면 정확한 수치는 달라질 수 있다. 목표 건물 수 달성, authored 초대형 CBD 4종, 보이는 공중 연결부, 광고 재질 4종, 강 양쪽 횡단부에 철교가 있는 닫힌 외곽 순환 철도, 감지된 모든 평면 도로 교차점의 양방향 차단기와 HISM 재질 오류 0건을 핵심 불변 조건으로 본다.

### 2. 필수 소프트웨어와 체크아웃

- Windows 10 또는 11
- Git
- Unreal Engine 5.7
- **Desktop development with C++**와 Windows SDK가 설치된 Visual Studio 2022
- Blender 5.2 LTS
- Blender MCP 런타임용 Python 3.12
- 모델을 다시 만들 때 사용할 `blender` MCP가 설정된 Codex

```powershell
git clone https://github.com/NyanReal/SolCity.git
Set-Location SolCity
```

저장소의 `.uasset`, `.blend`, `.fbx`, 프리뷰 PNG와 원본 텍스처만으로 프로젝트를 빌드하고 실행할 수 있다. 원본 에셋을 재구성하거나 수정할 때만 Blender MCP와 Unreal Python 임포트 스크립트가 필요하다.

프로젝트는 다음 플러그인을 사용한다.

- `ProceduralMeshComponent`
- `PythonScriptPlugin`
- `EditorScriptingUtilities`

기본 맵은 `/Game/Maps/SolCity`, 기본 게임 모드는 `/Script/SolCity.SolCityGameMode`다.

### 3. Blender MCP 설정

```powershell
git clone https://github.com/djeada/blender-mcp-server.git Tools/blender-mcp-server
py -3.12 -m venv Tools/blender-mcp-server/.mcp-runtime
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install --upgrade pip
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install -e Tools/blender-mcp-server
```

Blender 애드온을 빌드하고 설치한다.

```bash
cd Tools/blender-mcp-server
bash scripts/build_addon_zip.sh
```

**Edit > Preferences > Add-ons > Install from Disk**에서 `Tools/blender-mcp-server/dist/blender_mcp_bridge.zip`을 설치하고 **Blender MCP Bridge**를 활성화한다. 뷰포트 MCP 사이드바에 `Listening on 127.0.0.1:9876`이 표시되는지 확인한다.

저장소 위치에 맞게 경로를 조정해 Codex에 서버를 등록한다.

```powershell
codex mcp add blender -- D:\github\SolCity\Tools\blender-mcp-server\.mcp-runtime\Scripts\blender-mcp-server.exe
codex mcp list
```

편집하기 전에 연결된 Blender 장면을 반드시 검사한다. 기존 장면의 소유 작업이 불명확하면 별도 Blender 프로세스를 사용한다.

### 4. Blender 제작과 FBX 검증

`Docs/BlenderMCP_Modeling_Guide.md`를 따른다. MCP로 실제 장면을 변경하고, 프리뷰를 직접 검사하고, FBX를 깨끗한 Blender 5.2 장면에 재임포트해야 제작이 완료된 것이다.

미터 단위, `scale_length = 1.0`, `Z = 0` 지면 접촉, 선택 메시 내보내기, `axis_forward = -Y`, `axis_up = Z`를 사용한다. 재사용 가능한 생성·검증 스크립트는 `Tools/blender-mcp-server/scripts/`에 저장한다.

건물은 `Content/Art/Buildings/`, 프랍은 `Content/Art/Props/`에 두고 세 결과물 이름을 맞춘다.

```text
SM_SolCity_<Name>_01.blend
SM_SolCity_<Name>_01.fbx
SM_SolCity_<Name>_01_preview.png
```

현재 확장 에셋은 다음과 같다.

| 키트 | 내부 메시 | 검증 치수 |
|---|---|---|
| `SM_SolCity_SuburbanHouses_01` | `SM_SolCity_SuburbanHouse_01..04` | 각각 `17.250 x 12.798 x 6.640 m` |
| `SM_SolCity_Conifers_01` | `SM_SolCity_Conifer_01` | `4.972 x 5.100 x 7.568 m` |
|  | `SM_SolCity_Conifer_02` | `4.192 x 4.300 x 9.585 m` |
| `SM_SolCity_VoxelCat_01` | 단일 메시 | `1.250 x 1.896 x 1.198 m` |
| `SM_SolCity_DoubleTrack_01` | 복선 모듈 1개 | `12.000 x 8.250 x 0.590 m` |
| `SM_SolCity_RailBridge_01` | 복선 트러스 철교 모듈 1개 | `12.000 x 9.100 x 6.330 m` |
| `SM_SolCity_CommuterTrain_01` | 재사용 통근열차 차량 1량 | `34.190 x 3.430 x 5.360 m` |
| `SM_SolCity_LevelCrossingBarrierBase_01` | 제어함·경고등·크로스벅 일체형 본체 1개 | `0.924 x 0.860 x 2.832 m` |
| `SM_SolCity_LevelCrossingBarrierBoom_01` | 힌지 원점에서 독립 회전하는 차단봉 1개 | `6.880 x 0.380 x 0.460 m` |
| `SM_SolCity_MegaSkyscrapers_01` | `SM_SolCity_MegaGlassCurtainwall_01` | `48.000 x 42.425 x 225.000 m` |
|  | `SM_SolCity_MegaNewYorkSetback_01` | `52.000 x 44.700 x 228.500 m` |
|  | `SM_SolCity_MegaGeometricTwist_01` | `49.889 x 52.336 x 223.500 m` |
|  | `SM_SolCity_MegaPodiumCrown_01` | `56.000 x 54.500 x 227.500 m` |

모든 메시가 clean FBX 재임포트에서 정상 바운드를 통과했다. 지면형 에셋은 `minZ = 0`이고, 차단봉만 힌지 원점 중심으로 의도적으로 `Z = -0.23..0.23 m` 범위를 가진다. 저장소 전체의 커밋 가능한 프리뷰 갤러리는 `Content/Art/PreviewGallery/`이며, 현재 `_preview.png` 25개를 모두 복사해 두었다. 원본 프리뷰는 각 소스 에셋 옆에 그대로 유지한다.

주택·침엽수·초대형 타워 키트는 **Combine Meshes = false**, 복셀 고양이는 **Combine Meshes = true**로 임포트한다. 라이트맵 UV와 자동 충돌을 생성한다.

가변 높이 절차형 건물은 다음 authored 모듈을 사용한다.

- `SM_SolCity_BuildingBase_01`
- `SM_SolCity_BuildingMiddle_01`
- `SM_SolCity_BuildingCrown_01`
- `SM_SolCity_CornerBase_01`
- `SM_SolCity_CornerMiddle_01`
- `SM_SolCity_CornerCrown_01`

선택 옥상·랜드마크 모듈에는 HVAC, 물탱크, 태양광, 안테나, 퍼골라, 헬기장, 고도 경고등과 스카이브리지 연결부가 있다. 일반 완성 건물은 XYZ 균일 스케일을 유지한다. 초대형 CBD 랜드마크는 예외로, XY는 필지에 맞추되 수직 스케일을 최소 `0.86`으로 유지해 웅장한 높이를 보존한다.

차량과 고양이 FBX는 Blender `-Y` 전방 규칙을 사용한다. Unreal 메시 컴포넌트에는 상대 yaw `-90도`를 적용하고 액터 내비게이션은 Unreal `+X` 전방을 유지한다. 현재 고양이 에셋은 `SM_SolCity_VoxelCat_01`, 런타임 스케일은 `0.62`다.

### 5. Unreal 에셋 재구성

체크인된 Unreal 에셋을 사용할 때는 별도 설정이 필요 없다. 원본에서 에셋을 재구성할 때 **Tools > Execute Python Script** 또는 `UnrealEditor-Cmd.exe`로 다음 순서에 따라 실행한다.

1. `Content/Python/SetupSolCityAssets.py`
2. `Content/Python/SetupSolCitySurfaceAssets.py`
3. `Content/Python/ImportSolCityBuildingModules.py`
4. `Content/Python/ImportSolCityExtendedBuildingModules.py`
5. `Content/Python/ImportSolCityProps.py`
6. `Content/Python/ImportSolCityUrbanProps.py`
7. `Content/Python/ImportSolCityExpansionAssets.py`
8. `Content/Python/ImportSolCityRailwayAssets.py`
9. `Content/Python/ImportSolCityLevelCrossingAssets.py`
10. `Content/Python/SetupSolCityBillboard.py`
11. `Content/Python/CreateSolCitySplineMaterials.py`
12. `Content/Python/SetupSolCityBuildingMaterials.py`
13. `Content/Python/FinalizeSolCityMaterials.py`
14. `Content/Python/TuneSolCityEnvironment.py`

`ImportSolCityExpansionAssets.py`는 주택 4, 침엽수 2, 초대형 타워 4, 복셀 고양이 1개로 총 11개 메시를 임포트한다. 가져온 기본 재질에는 Instanced Static Mesh 사용 플래그도 설정한다.

`ImportSolCityRailwayAssets.py`는 복선, 철교와 통근열차 FBX를 임포트하고 `/Game/Art/Props`에 철도·열차 재질을 만든다. 스크립트는 재실행 가능하며 이미 존재하는 에셋은 건너뛴다. UE 5.7 무인 Interchange는 FBX 저장을 마친 직후 `SlateApplication.h` assertion으로 종료될 수 있다. **Tools > Execute Python Script** 실행을 우선하고, commandlet을 써야 한다면 세 `.uasset`이 모두 생길 때까지 한 프로세스당 한 에셋씩 재실행한다.

`ImportSolCityLevelCrossingAssets.py`는 차단기 본체와 힌지 원점을 가진 분리 차단봉을 임포트하고 Combine Meshes·라이트맵 UV·충돌을 설정하며 `M_Crossing_WarningSignal`을 만든다. 신호 재질은 HISM custom-data float `0`을 읽으며 `0`은 발광 초록색, `1`은 발광 빨간색이다. 생성기는 런타임에 authored 경고등 슬롯만 이 재질로 오버라이드하므로 원본 메시의 재사용성을 유지한다. 원본 `.blend`, `.fbx`, 프리뷰 PNG는 임포트된 프랍 옆에 유지한다.

`SetupSolCityBillboard.py`는 광고판, 언어 없는 광고 텍스처 4종, 양면/HISM 호환 마스터 재질과 교체 가능한 인스턴스 4종을 만든다. 메시의 프레임은 슬롯 `0`, 광고면은 슬롯 `1`이다. 현재 스크립트에는 `PROJECT = Path(r"D:\github\SolCity")`가 있으므로 다른 위치에 체크아웃했다면 이 값을 수정한다.

원본 텍스처는 `Content/Art/Source/`, 런타임 에셋은 `/Game/Art/Generated`, `/Game/Art/Materials`, `/Game/Art/Buildings`, `/Game/Art/Props`에 있다.

일부 런타임 에셋은 문자열 경로로 로드하므로 `Config/DefaultGame.ini`가 맵, 재질, 생성 에셋, 프랍과 건물 폴더를 강제 쿠킹한다.

#### Landscape 주의사항

현재 1.44km 지면, 용도지역, 강, 둔치, 옹벽과 원거리 지면 연결의 권위 구현은 런타임 생성기다. `USolCityLandscapeLibrary`의 기본값은 `144000, 6000, -90`이지만 `Content/Python/BuildSolCityLandscape.py`는 아직 예전 `36000, 2400, -90` 인자를 전달한다. 현재 도시에서 이 스크립트를 그대로 실행하지 않는다. 지속 Landscape를 다시 만들 필요가 있다면 호출을 다음처럼 바꾼다.

```python
unreal.SolCityLandscapeLibrary.rebuild_sol_city_landscape(144000.0, 6000.0, -90.0)
```

### 6. 현재 절차 생성 구조

`ASolCityGenerator::RegenerateCity()`는 다음 순서로 동작한다.

1. 생성 컴포넌트와 결정적 상태 초기화.
2. HISM과 스플라인 인스턴스 그룹 생성.
3. 지면, 강, 둔치, 옹벽, 산책로, 난간과 원거리 전환부 생성.
4. 전체 도로 계층 생성.
5. 외곽 순환 철도, 철교, 열차와 감지된 평면 건널목 차단기 생성.
6. 모든 도로가 확정된 뒤 경석과 인도 스트립 생성.
7. 용도지역 지면 생성.
8. 광고판을 포함한 교통·도시 프랍의 위치 선점과 배치.
9. 블록 추출, 필지 분할, 공공 공간 선점, 내부 mews와 건물 배치.
10. authored 도로 교량 배치.
11. 철도 이격을 지키며 활엽수와 침엽수 배치.
12. HISM 트리 완성과 검증 수치 출력.

도로는 360cm 차로 폭을 기준으로 한다.

| 등급 | 표시 차로 수 | 차도 폭 | 한쪽 인도 폭 |
|---|---:|---:|---:|
| 골목길 | 1 | `360 cm` | `160 cm` |
| 집산도로 | 2 | `720 cm` | `240 cm` |
| 간선도로 / 교량 | 4 | `1,440 cm` | `320 cm` |

차도면은 `Z = 8 cm`, 인도 상면은 `Z = 23 cm`, 경석 폭은 `20 cm`다. 경석과 인도는 도로를 만들 때 즉시 끝까지 생성하지 않는다. 모든 도로 선분을 기록한 뒤 2D 교차점을 찾고, 각 스트립에서 교차로 여유 구간을 빼고 남은 부분만 생성한다. 따라서 교차로를 가로지르는 경석이 생기지 않는다.

용도지역은 먼저 연속 주거 지면 받침을 만들고 그 위에 주거·상업·공원·주차 셀을 배치한다. 셀 크기는 피치보다 16cm 크게 겹치므로 셀 사이로 원거리 지면이 검은 선처럼 노출되지 않는다.

`SolCityLotLayout`은 도로 중심선에서 닫힌 블록을 추출하고 도로를 향한 필지로 나눈다. 도시 경계, 강, 도로, 철도, 기존 건물, 광고판, 공원, 주차장과 중정의 충돌을 거부한다. 닫히지 않은 도로 지역은 도시 전체 포인트 산포가 아니라 결정적인 도로 전면 필지 폴백으로 채운다.

일반 도로 전면 배치 전에 조건을 만족하는 큰 외곽 블록에는 기존 도로 인도에서 블록 내부로 들어가는 폭 150cm의 보행 전용 mews를 만든다. 양쪽 단층 주택은 authored `-Y` 출입구가 mews를 향하도록 배치한다. 블록 모서리, 강의 시작·중앙·끝 표본, 도로, 철도, 기존 점유와 후보 OBB 충돌을 검사한다. 주택이 최소 2채 이상 성공한 경우에만 본선·현관 연결 보도, 보행 웨이포인트 쌍과 예약 회랑을 확정한다.

철도는 도시 경계 안쪽의 결정적인 닫힌 타원 경로다. 선분 수는 둘레에 비례하며 `96..520` 범위다. 굽은 강을 지나는 두 횡단부는 주행면 `420 cm`의 트러스 철교를 사용하고, 길이 `1,900 cm`의 완만한 진입 경사로 지상 주행면 `59 cm`에 연결한다. 4량 편성 2개가 서로 반대 선로에서 반대 방향으로 운행한다. 각 차량은 루프 누적 거리를 별도로 샘플링하므로 authored 차량 길이에 `80 cm` 연결 간격을 더한 간격을 유지하면서 곡선과 철교 경사에 개별 정렬된다. 위치는 기존 폴리라인 위에 유지하되 중심 차분 chord로 접선을 만들고 world-up 쿼터니언 회전을 사용하므로 선분 경계의 yaw 스냅, 미세 역회전과 루프 이음새 불연속이 없다. 움직이는 차량 8량은 HISM 인스턴스가 아니라 일반 movable `UStaticMeshComponent`이며, 차량마다 자체 렌더 Bounds를 갱신하므로 오래된 계층 클러스터 때문에 카메라 컬링에서 빠지지 않는다. 기본 속도는 `1,800 cm/s`(`64.8 km/h`)다. 건물·개방 공간 프랍·mews·광고판·나무·침엽수·교통 시설은 실제 회전 바닥 형상으로 철도 이격을 검사한다. 닫힌 경로는 `GetRailwayPathWaypoints()`로 제공한다.

평면 건널목 생성기는 지상 철도와 도로 선분을 2D로 교차 검사한다. 철교·도로교·평행 선분·`900 cm` 안의 중복 교차점과 높이 차가 `140 cm`를 넘는 입체교차는 제외한다. 유효한 건널목마다 도로 양쪽 접근부에 차단기 하나씩을 두며, 본체는 경석 밖에 놓이고 별도 HISM 차단봉은 도로 중앙을 향한다. 넓은 도로에서는 필요한 만큼 차단봉 X 길이만 늘린다. 열차가 `12,000 cm` 앞에 오면 먼저 초록등이 빨간등으로 바뀌고, `9,000 cm` 앞에서 차단봉이 내려가기 시작한다. 4번째 차량 뒤로 `1,200 cm`가 더 지날 때까지 빨간색과 닫힘을 유지한 뒤 normalized speed `0.75/s`로 올라간다. 차단봉이 완전 상승 각도 `82도`에 도달해야 다시 초록색이 된다. 값은 **Sol City > Railway > Level Crossing**에서 조정할 수 있다.

절차형 건물 스타일은 다음과 같다.

- 셋백 중층 건물.
- authored 코너 모듈과 L자 폴백.
- 요청 셋백 사각형이 단조 감소하는 테이퍼 타워.
- 중정 클러스터.
- authored 외곽 주택과 저층 연립 폴백.
- 한두 개의 공중 연결부가 있는 대칭 쌍둥이 타워.
- 넓은 포디엄과 단조 감소 셋백을 가진 초대형 CBD 타워.

일반 필지가 중심부를 채우기 전에 큰 도심 블록 내부를 authored 초대형 마천루 4종의 랜드마크 필지로 선점한다. 추가 조건을 만족하는 CBD 필지는 절차형 초대형 스타일을 사용할 수 있다. 면적이 큰 지붕에는 헬기장, 물탱크, HVAC, 태양광, 안테나·퍼골라와 고도 경고등을 배치한다.

현재 알려진 제한: 테이퍼 층은 메시 XY에 단일 균등 배율을 적용하지만 셋백 오프셋은 요청 X/Y 사각형으로 계산한다. 필지와 모듈 종횡비가 다르면 요청 사각형이 아래층 안에 있어도 실제 상층 메시가 실제 하층 메시 지지 범위를 벗어날 수 있다. 화면에서 보인 일부 타워의 허공 걸침 원인이며, 이 스타일을 구조적으로 정상으로 보기 전에 실제 상·하층 메시 지지 사각형으로 계산을 바꿔야 한다.

외곽 주거지는 더 넓고 깊은 필지, 축소된 건축판, 색이 다른 단층 주택 4종과 높은 수목 선호도를 사용한다. 외곽의 적합한 수목 배치 중 최대 약 38%는 침엽수 2종 중 하나를 사용한다.

광고판은 건물보다 먼저 배치해 가려지지 않게 필지를 선점한다. 현재 도시 크기에서 목표는 48개다. 도로 양쪽을 검사하고 반대쪽은 yaw를 180도 보정하며 지면 높이에 맞춘다. 언어 없는 광고 재질 4종을 균등하게 돌려 쓴다.

### 7. 시뮬레이션, 카메라와 환경

차량은 방향성 도로 그래프에서 제한 속도, 가감속, 앞차 간격, 적신호 정지와 두 충돌 축에 대한 6단계 신호 주기를 사용한다. 고양이는 보이는 인도 스트립에서 만든 별도 보행 웨이포인트 그래프만 사용하며 의도적으로 차로를 통과하지 않는다.

카메라 기본값은 다음과 같다.

| 설정 | 값 |
|---|---:|
| 초기 스프링암 | `11,000 cm` |
| 도시 줌 범위 | `800..48,000 cm` |
| 휠 줌 단계 | `2,500 cm` |
| 줌 보간 속도 | `5.0` |
| 화면 이동 속도 | `5,000 cm/s` |
| 도시 시점 피치 | `-80..-25도` |
| 자유 비행 속도 | `6,000 cm/s` |
| 자유 비행 속도 범위 | `500..30,000 cm/s` |
| 자유 비행 피치 | `-89..89도` |

`W A S D`로 화면 이동, RMB 드래그로 회전과 피치 변경, 휠로 줌, `F`로 자유 비행을 전환한다. `T`는 철도 시간 토글이다. 두 열차 편성은 현재 누적 경로 거리에서 정지하고 다시 `T`를 누르면 그 위치에서 운행을 재개하며, 물·도로 교통과 나머지 도시는 계속 움직인다. 차단기는 정지한 편성 위치를 계속 판정하며 열림·닫힘 동작을 끝까지 수행하므로 접근 구간에 멈춘 열차도 안전하게 차단된다. 자유 비행에서는 `Q/E`로 하강·상승하고 휠로 속도를 바꾼다. 자유 비행을 끝내면 저장한 도시 Transform, 스프링암 회전, 현재 길이와 목표 줌을 복원한다.

런타임은 Sky Atmosphere, 방향광, Sky Light, Exponential Height Fog, Volumetric Cloud와 무한 Post Process Volume을 생성하거나 기존 액터를 재사용한다. 수동 노출, AO `0.45`, AO 반경 `120`, AO Power `1.2`, 채도 `0.97`, 대비 `1.02`, Bloom `0.25`, Bloom Threshold `1.4`를 사용한다. `Z = -280 cm`의 매우 큰 충돌·그림자 없는 평면과 높이 안개로 유한 지형 밖 경계를 가린다.

### 8. 빌드와 검증

Editor 타깃을 빌드한다.

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" `
  SolCityEditor Win64 Development "$PWD\SolCity.uproject" `
  -WaitMutex -NoHotReloadFromIDE
```

BeginPlay 자동 생성 스모크 테스트를 실행한다.

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "$PWD\SolCity.uproject" -game -nullrhi -nosound -unattended `
  -stdout -FullStdOutLogOutput "-ExecCmds=quit"
```

`Saved/Logs/SolCity.log`에서 다음 로그를 확인한다.

- `SolCity parcel layout`
- `SolCity expansion authored buildings`
- `SolCity urban props`
- `SolCity authored modules`
- `SolCity streetscape`
- `SolCity perimeter railway`
- `SolCity level crossings`

`missing bUsedWithInstancedStaticMeshes`, `Default Material will be used`, Python traceback, assertion 또는 fatal error가 없어야 한다.

PIE 시각 검증 항목:

- 녹지 셀 사이에 검은 격자선이 없다.
- 경석과 인도가 교차로 차도 전에 끝난다.
- 골목길·집산도로·간선도로의 폭과 차선 표시가 구분된다.
- CBD에 authored 초대형 타워 4종의 서로 다른 실루엣이 있다.
- 쌍둥이 타워의 공중 연결부가 명확히 보인다.
- 외곽에 넓은 지붕 단층 주택이 여러 색으로 배치된다.
- 광고판이 도로를 향하고 언어 없는 광고 4종을 순환한다.
- 활엽수 사이에 침엽수가 섞인다.
- 복셀 고양이의 눈, 귀, 주둥이, 수염, 발과 마디형 꼬리가 보이며 인도 방향을 따라간다.
- 외곽 복선 철도가 닫힌 경로이고 강을 건너는 두 곳 모두 철교이며, 진입 경사가 끊기지 않고 4량 통근열차 편성 2개가 서로 반대 선로·반대 방향으로 움직인다.
- 내부 주택이 생긴 큰 외곽 블록에는 좁은 보행 mews도 있고 주택 출입구가 그 길을 향한다.
- 테이퍼 상층 모듈이 요청 필지 사각형뿐 아니라 실제 하층 모듈 지지 범위 안에도 들어간다.
- 강물, 녹지 둔치, 옹벽, 산책로, 난간, 교량과 콘크리트 전환부 사이에 열린 틈이 없다.
- 모든 지상 도로·철도 교차점에 도로 밖 본체 2개가 있다. 차단봉 동작 전에 빨간등이 켜지고 4번째 차량이 지날 때까지 내려가 있으며, 완전히 올라간 뒤에만 초록등으로 돌아온다.
- 열차가 모든 폴리라인 모서리와 루프 이음새를 지날 때 순간적인 역방향 yaw 튐 없이 연속 회전한다.
- 움직이는 각 차량이 동적 컴포넌트 Bounds를 가지므로 모든 카메라 각도에서 열차 본체와 그림자가 함께 보인다.

Blender 에셋을 다시 만들었다면 `_preview.png`를 검사하고 clean Blender 5.2 FBX 재임포트 검증도 반복한 뒤 결과를 승인한다.
