# Sol City Reproduction Guide

## English

### 1. Target result

Reproduce a UE 5.7 C++ city-builder prototype with a peaceful, vividly colored Japanese-anime-background look and cats as citizens.

The expected result includes:

- A 36,000 x 36,000 cm city with mouse-wheel zoom from 800 to 48,000 cm.
- An irregular procedural road hierarchy instead of a plain grid.
- Spline roads, junction caps, modeled bridges, and continuous road connections.
- Road-aware procedural lots with density and height biased toward the city center.
- Three completed Blender/FBX buildings mixed evenly with Blender-authored modular buildings.
- A persistent 505 x 505 Landscape, a lowered riverbed, and one continuous UV-animated river surface.
- Autonomous cars on a directed road graph, six-phase traffic lights, and cats on a separate sidewalk graph.
- A bright daytime environment with readable blue ambient light, fog, volumetric clouds, and restrained post-processing.

### 2. Required software and project checkout

- Windows 10 or 11
- Git
- Unreal Engine 5.7
- Visual Studio 2022 with **Desktop development with C++** and a Windows SDK
- Blender 5.2 LTS
- Python 3.12 for Blender MCP
- Codex CLI when regenerating or editing Blender assets through MCP

```powershell
git clone https://github.com/NyanReal/SolCity.git
Set-Location SolCity
```

The checked-in Unreal assets, `.blend`, `.fbx`, preview PNG files, and source textures are sufficient to build and run the project. Blender MCP is required only when a model must be regenerated or edited.

Enable **Python Script Plugin** and **Editor Scripting Utilities** in Unreal before running the project setup scripts.

### 3. Install Blender MCP

```powershell
git clone https://github.com/djeada/blender-mcp-server.git Tools/blender-mcp-server
py -3.12 -m venv Tools/blender-mcp-server/.mcp-runtime
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install --upgrade pip
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install -e Tools/blender-mcp-server
```

Build the Blender add-on zip in Git Bash:

```bash
cd Tools/blender-mcp-server
bash scripts/build_addon_zip.sh
```

In Blender, open **Edit > Preferences > Add-ons > Install from Disk**, install `Tools/blender-mcp-server/dist/blender_mcp_bridge.zip`, and enable **Blender MCP Bridge**. Confirm `Listening on 127.0.0.1:9876` in the 3D Viewport **MCP** sidebar.

Register the server with Codex, changing the path when necessary:

```powershell
codex mcp add blender -- D:\github\SolCity\Tools\blender-mcp-server\.mcp-runtime\Scripts\blender-mcp-server.exe
codex mcp list
```

Restart Codex if the Blender tools are not visible. Inspect the connected scene before editing and do not replace a Blender session owned by another task.

### 4. Blender modeling and FBX rules

Make real Blender changes through MCP. Writing an unexecuted Python file is not a completed modeling pass. Use metric units with `scale_length = 1.0` and keep ground contact near `Z = 0`.

Each completed building in `Content/Art/Buildings` must have matching deliverables:

```text
SM_SolCity_<Name>_01.blend
SM_SolCity_<Name>_01.fbx
SM_SolCity_<Name>_01_preview.png
```

Gameplay props belong in `Content/Art/Props`. Export selected mesh objects only, apply unit scale, and use `axis_forward = -Y` and `axis_up = Z`. Import multi-object assets with **Combine Meshes**, generated lightmap UVs, and automatic collision.

Create and revise cats, cars, bridges, road pieces, junctions, and trees through the same Blender MCP workflow. Follow `Docs/BlenderMCP_Modeling_Guide.md` and record each changed `.blend`, `.fbx`, preview, dimensions, axis convention, and validation result in `Docs/BlenderMCP_Asset_Log.md`.

Preserve the proportions of every completed Blender building. If a city-scale correction is required, derive one uniform XYZ scale from the standard road-lane width. Never calculate a separate Z scale from `target height / source mesh height`.

Variable-height buildings must stack these authored FBX modules instead of stretching a completed mesh:

- `SM_SolCity_BuildingBase_01`
- `SM_SolCity_BuildingMiddle_01`
- `SM_SolCity_BuildingCrown_01`

Windows, frames, entrances, ledges, and roof details are modeled into these modules. Do not rebuild their walls from cubes or apply the old `M_AnimeFacade` window-wall texture to boxes.

Keep FBX material-slot names stable. Blender materials are authoring references; production rendering uses native Unreal material instances assigned by slot name.

Forward-axis rules:

- Road spline meshes are longitudinally subdivided and Unreal `+X` forward.
- The car mesh is authored nose-forward in Blender `-Y`; apply only the mesh-relative visual yaw correction while actor movement and sensing remain Unreal `+X` forward.
- The cat mesh is also authored in Blender local `-Y`; apply a mesh-relative `+90 degrees` yaw so the visible cat follows the sidewalk tangent while navigation remains actor-local `+X`.

Model bridge decks, piers, railings, and approach transitions carefully in Blender. The deck must align with the road graph and lane width, piers must clear the river path, and the approaches must not leave visible road or Landscape gaps.

For every exported model, inspect the rendered preview and re-import the FBX into a clean Blender 5.2 scene. Verify mesh count, non-degenerate bounds, ground contact, dimensions, forward direction, and error-free import.

### 5. Unreal asset and material setup

Source textures are stored in `Content/Art/Source`:

- `T_AnimeGrass.png`
- `T_AnimeAsphalt.png`
- `T_AnimeWater.png`
- `T_AnimeFacade.png` retained only as a source/legacy asset

ImageGen outputs may replace the first three textures while retaining their filenames. Run these scripts from **Tools > Execute Python Script** in order:

1. `Content/Python/SetupSolCityAssets.py`
2. `Content/Python/TuneSolCityEnvironment.py`
3. `Content/Python/ImportSolCityBuildingModules.py`
4. `Content/Python/ImportSolCityProps.py`
5. `Content/Python/CreateSolCitySplineMaterials.py`
6. `Content/Python/FinalizeSolCityMaterials.py`
7. `Content/Python/BuildSolCityLandscape.py`

`SetupSolCityAssets.py` imports the three completed buildings with `import_materials = False` and calls `SetupSolCityBuildingMaterials.py`. The material script creates or updates:

- `/Game/Art/Buildings/Materials/M_SolCity_Building_Master`
- 28 native material instances for the exact authored slots
- MidRise `10/10`, CornerRetail `9/9`, and SteppedTower `9/9` deterministic assignments

The master is opaque and exposes base color, metallic, roughness, specular, emissive, normal, roughness-texture, and macro-variation controls. Missing, renamed, duplicate, and unmapped slots are logged. Re-running the script updates the same assets and assignments without recreating FBX legacy materials.

Run `Content/Python/SetupSolCityBuildingMaterials.py` separately whenever building FBXs or their slots change. Its `import_or_reimport_buildings()` helper performs an in-place FBX import with material creation disabled, then restores the native slot mappings.

`TuneSolCityEnvironment.py` reduces the fluorescent grass appearance using a muted green tint, high roughness, and low specular response. Run it again whenever the grass base material is regenerated.

### 6. World generation and runtime behavior

- Generate an asymmetrical arterial spine, collector loop, and curved district branches.
- Render roads with `USplineComponent` and `USplineMeshComponent`; use junction meshes to cover branch seams and spline endpoints.
- Sample lots deterministically near roads, reject road/river/building overlaps, vary footprints and rotations, and increase density toward the center.
- Mix all three completed authored buildings evenly and deterministically. Do not hide a type behind a rare style, height, or random-chance gate.
- Use uniform lane-width-derived scaling for completed buildings and HISM-stacked Base/Middle/Crown meshes for variable-height buildings.
- Use the persistent Landscape instead of ground tiles. The current Landscape uses 4 x 4 components, 2 x 2 sections per component, and 63 quads per section, producing 505 x 505 vertices across 36,000 cm.
- Carve the riverbed to `-230 cm`, place the water surface at `-90 cm`, and render the river as one continuous UV-animated ribbon.
- Keep cars on a directed two-lane graph with speed limits, following distance, red-light stops, and correct actor-forward alignment.
- Keep cats off the road on a separate sidewalk waypoint graph and verify their visual forward direction against movement.
- Use six traffic-signal phases across two conflicting movement axes.
- Place modeled trees outside roads, river clearance, bridge approaches, and occupied building lots.
- Support `W A S D` panning, mouse-wheel zoom, and right-mouse horizontal rotation.

### 7. Environment, lighting, and visual parity

When no authored equivalent exists, the runtime creates the environment actor. When one already exists, it is reused so level reloads and PIE do not duplicate atmosphere, sun, Sky Light, fog, clouds, or post-process volumes.

Directional Light settings:

- Location `(0, 0, 8000)`
- Rotation Roll/X `-180 degrees`, Pitch/Y `-161 degrees`, Yaw/Z `193 degrees`
- Scale `(2.5, 2.5, 2.5)`
- Intensity `3.4`, indirect intensity `1.05`
- Source angle `1.0`, contact-shadow length `0.035`, contact-shadow intensity `0.65`
- Movable, Atmosphere Sun Light enabled, cloud shadows enabled

Use a movable real-time Sky Light with blue ambient contribution and a non-black lower hemisphere so shaded facades remain readable. Use Sky Atmosphere, Exponential Height Fog with volumetric fog, and UE 5.7's built-in simple Volumetric Cloud material.

The unbound runtime Post Process Volume uses manual exposure with physical-camera exposure disabled, exposure bias `0.0`, AO intensity `0.45`, AO radius `120`, AO power `1.2`, saturation `0.97`, contrast `1.02`, bloom intensity `0.25`, and bloom threshold `1.4`.

A finite Landscape exposes the black world beyond its edge. Keep the large low distant-ground plane below and outside the playable Landscape, disable its collision and shadows, and use height fog to blend it into the horizon. Gameplay and navigation remain on the Landscape.

For controlled material comparison, run `Content/Python/BuildSolCityLookDev.py` in both modes:

```powershell
$Editor = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
& $Editor "$PWD\SolCity.uproject" -ExecutePythonScript="$PWD\Content\Python\BuildSolCityLookDev.py" -SolCityLookDevStage=legacy -unattended -nop4 -nosplash -DDC-ForceMemoryCache -NoZenLoader
& $Editor "$PWD\SolCity.uproject" -ExecutePythonScript="$PWD\Content\Python\BuildSolCityLookDev.py" -SolCityLookDevStage=native -unattended -nop4 -nosplash -DDC-ForceMemoryCache -NoZenLoader
```

The isolated `/Game/Maps/SolCityLookDev` scene uses a 53 mm full-frame camera, fixed three-light setup, reflection capture, neutral unlit background, and manual exposure. It writes matching 1600 x 900 captures to:

```text
Saved/VisualParity/SolCity_CornerRetail_lookdev_before.png
Saved/VisualParity/SolCity_CornerRetail_lookdev_after.png
```

Legacy mode uses actor-level overrides only; it does not replace the production mesh assignments.

### 8. Build and verify

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" SolCityEditor Win64 Development -Project="$PWD\SolCity.uproject" -WaitMutex -NoHotReloadFromIDE
```

Open `SolCity.uproject`, load `/Game/Maps/SolCity`, and start Play-in-Editor. Verify:

- The mouse wheel zooms through the full range and camera controls remain responsive.
- Landscape edges, distant ground, fog, and clouds produce no black perimeter.
- Grass is muted rather than fluorescent and shaded facades retain ambient sky color.
- River and spline roads are continuous; junctions and bridge approaches have no visible seams.
- Bridge piers and deck align with the river and road lanes.
- Completed buildings retain their original proportions and all three types appear in a balanced mix.
- Modular buildings use correctly scaled Base/Middle/Crown floors rather than stretched boxes.
- Native material slots survive a repeated setup or FBX reimport.
- Cars face their travel direction, maintain lanes, and stop at red lights.
- Cats remain on sidewalks and face their movement direction.
- The LookDev legacy/native captures use identical framing and exposure.

Run the building material setup twice when validating idempotence; both runs should report `28/28` assigned slots and zero errors.

---

## 한국어

### 1. 재현 목표

평화롭고 선명한 일본 애니메이션 배경풍의 UE 5.7 C++ 시티빌더 프로토타입을 재현한다. 시민은 고양이다.

최종 결과에는 다음 내용이 포함된다.

- 36,000 x 36,000cm 규모의 도시와 800~48,000cm 마우스 휠 줌 범위
- 단순 격자가 아닌 비정형 절차적 도로 계층
- 스플라인 도로, 교차로 캡, 모델링된 교량과 자연스럽게 이어지는 도로 연결부
- 도로를 기준으로 배치되고 중심부로 갈수록 밀도와 높이가 증가하는 절차적 필지
- 완성형 Blender/FBX 건물 3종과 Blender 제작 적층 모듈 건물의 균형 잡힌 혼합
- 505 x 505 영구 Landscape, 낮아진 강바닥과 하나로 이어진 UV 애니메이션 수면
- 방향성 도로 그래프를 주행하는 자동차, 6단계 신호등과 별도 보도 그래프를 이용하는 고양이
- 푸른 앰비언트가 살아 있는 밝은 주간 조명, 포그, 볼류메트릭 클라우드와 절제된 후처리

### 2. 필수 소프트웨어와 프로젝트 받기

- Windows 10 또는 11
- Git
- Unreal Engine 5.7
- **Desktop development with C++**와 Windows SDK를 설치한 Visual Studio 2022
- Blender 5.2 LTS
- Blender MCP용 Python 3.12
- Blender 모델을 MCP로 재생성하거나 수정할 때 사용할 Codex CLI

```powershell
git clone https://github.com/NyanReal/SolCity.git
Set-Location SolCity
```

저장소에 포함된 Unreal 에셋, `.blend`, `.fbx`, 미리보기 PNG와 원본 텍스처만으로 프로젝트를 빌드하고 실행할 수 있다. Blender MCP는 모델을 다시 만들거나 수정할 때만 필요하다.

프로젝트 셋업 스크립트를 실행하기 전에 Unreal에서 **Python Script Plugin**과 **Editor Scripting Utilities**를 활성화한다.

### 3. Blender MCP 설치

```powershell
git clone https://github.com/djeada/blender-mcp-server.git Tools/blender-mcp-server
py -3.12 -m venv Tools/blender-mcp-server/.mcp-runtime
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install --upgrade pip
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install -e Tools/blender-mcp-server
```

Git Bash에서 Blender 애드온 ZIP을 만든다.

```bash
cd Tools/blender-mcp-server
bash scripts/build_addon_zip.sh
```

Blender에서 **Edit > Preferences > Add-ons > Install from Disk**를 열어 `Tools/blender-mcp-server/dist/blender_mcp_bridge.zip`을 설치하고 **Blender MCP Bridge**를 활성화한다. 3D Viewport의 **MCP** 사이드바에서 `Listening on 127.0.0.1:9876`을 확인한다.

필요하면 실제 경로에 맞게 수정해 Codex에 서버를 등록한다.

```powershell
codex mcp add blender -- D:\github\SolCity\Tools\blender-mcp-server\.mcp-runtime\Scripts\blender-mcp-server.exe
codex mcp list
```

Blender 도구가 보이지 않으면 Codex를 다시 시작한다. 수정 전 연결된 장면을 확인하고 다른 작업이 사용하는 Blender 세션을 교체하지 않는다.

### 4. Blender 모델링과 FBX 규칙

실제 Blender 변경은 MCP를 통해 실행한다. 실행되지 않은 Python 파일만 작성한 상태는 모델링 완료로 보지 않는다. 미터 단위와 `scale_length = 1.0`을 사용하고 지면 접점은 `Z = 0` 근처에 둔다.

`Content/Art/Buildings`의 완성형 건물은 같은 이름의 결과물 3개를 가져야 한다.

```text
SM_SolCity_<Name>_01.blend
SM_SolCity_<Name>_01.fbx
SM_SolCity_<Name>_01_preview.png
```

게임플레이 소품은 `Content/Art/Props`에 둔다. 선택한 메시 오브젝트만 내보내고 단위 스케일을 적용하며 `axis_forward = -Y`, `axis_up = Z`를 사용한다. 여러 오브젝트로 구성된 에셋은 **Combine Meshes**, 라이트맵 UV 생성과 자동 충돌 설정으로 임포트한다.

고양이, 자동차, 교량, 도로 조각, 교차로와 나무도 같은 Blender MCP 절차로 생성하고 수정한다. `Docs/BlenderMCP_Modeling_Guide.md`를 따르며 변경한 `.blend`, `.fbx`, 미리보기, 치수, 축 규칙과 검증 결과를 `Docs/BlenderMCP_Asset_Log.md`에 기록한다.

완성형 Blender 건물은 원본 비율을 유지한다. 도시 스케일 보정이 필요하면 표준 도로 차로 폭에서 계산한 하나의 균일한 XYZ 스케일만 사용한다. `목표 높이 / 원본 메시 높이`로 Z만 별도 계산하지 않는다.

높이가 변하는 건물은 완성 메시를 늘이지 않고 다음 FBX 모듈을 적층한다.

- `SM_SolCity_BuildingBase_01`
- `SM_SolCity_BuildingMiddle_01`
- `SM_SolCity_BuildingCrown_01`

창문, 프레임, 출입구, 돌출부와 옥상 디테일은 모듈에 직접 모델링한다. 벽을 큐브로 다시 만들거나 기존 `M_AnimeFacade` 창문 벽 텍스처를 박스에 적용하지 않는다.

FBX 머티리얼 슬롯 이름을 안정적으로 유지한다. Blender 머티리얼은 제작 기준이며 실제 게임 렌더링은 슬롯 이름으로 할당된 Unreal 네이티브 머티리얼 인스턴스를 사용한다.

포워드 축 규칙은 다음과 같다.

- 도로 스플라인 메시는 길이 방향으로 충분히 분할하고 Unreal `+X`가 전방이 되게 한다.
- 자동차 메시는 Blender `-Y`가 앞이다. 이동과 감지 로직은 Unreal 액터 `+X`를 유지하고 시각 메시의 상대 yaw만 보정한다.
- 고양이 메시도 Blender 로컬 `-Y`가 앞이다. 내비게이션은 액터 로컬 `+X`를 유지하고 메시 상대 yaw에 `+90도`를 적용해 보도 접선 방향을 바라보게 한다.

교량 상판, 교각, 난간과 진입부 전환을 Blender에서 신경 써서 모델링한다. 상판은 도로 그래프와 차로 폭에 맞아야 하며 교각은 강 경로를 침범하지 않아야 한다. 진입부에는 도로나 Landscape 틈이 보이면 안 된다.

모든 모델은 렌더 미리보기를 확인하고 깨끗한 Blender 5.2 장면에 FBX를 다시 임포트한다. 메시 수, 정상적인 바운드, 지면 접점, 치수, 전방 방향과 오류 없는 임포트를 확인한다.

### 5. Unreal 에셋과 머티리얼 셋업

원본 텍스처는 `Content/Art/Source`에 있다.

- `T_AnimeGrass.png`
- `T_AnimeAsphalt.png`
- `T_AnimeWater.png`
- `T_AnimeFacade.png`: 원본 및 레거시 용도로만 유지

ImageGen 결과로 앞의 텍스처 3개를 교체할 때는 파일명을 유지한다. **Tools > Execute Python Script**에서 다음 순서로 실행한다.

1. `Content/Python/SetupSolCityAssets.py`
2. `Content/Python/TuneSolCityEnvironment.py`
3. `Content/Python/ImportSolCityBuildingModules.py`
4. `Content/Python/ImportSolCityProps.py`
5. `Content/Python/CreateSolCitySplineMaterials.py`
6. `Content/Python/FinalizeSolCityMaterials.py`
7. `Content/Python/BuildSolCityLandscape.py`

`SetupSolCityAssets.py`는 완성형 건물 3종을 `import_materials = False`로 임포트하고 `SetupSolCityBuildingMaterials.py`를 호출한다. 머티리얼 스크립트는 다음 에셋을 생성하거나 갱신한다.

- `/Game/Art/Buildings/Materials/M_SolCity_Building_Master`
- 실제 제작 슬롯에 대응하는 네이티브 머티리얼 인스턴스 28개
- MidRise `10/10`, CornerRetail `9/9`, SteppedTower `9/9`의 결정적 슬롯 할당

마스터 머티리얼은 Opaque이며 Base Color, Metallic, Roughness, Specular, Emissive, Normal, Roughness Texture와 Macro Variation 설정을 제공한다. 누락, 이름 변경, 중복과 미매핑 슬롯을 로그로 남긴다. 스크립트를 다시 실행해도 FBX 레거시 머티리얼을 만들지 않고 동일 에셋과 할당을 갱신한다.

건물 FBX나 슬롯이 바뀌면 `Content/Python/SetupSolCityBuildingMaterials.py`를 별도로 다시 실행한다. `import_or_reimport_buildings()`는 머티리얼 생성을 끈 채 FBX를 제자리 임포트한 뒤 네이티브 슬롯 매핑을 복원한다.

`TuneSolCityEnvironment.py`는 채도를 낮춘 녹색 틴트, 높은 Roughness와 낮은 Specular로 형광 같은 잔디를 완화한다. 잔디 기본 머티리얼을 다시 생성했다면 이 스크립트도 다시 실행한다.

### 6. 월드 생성과 런타임 동작

- 비대칭 간선도로, 순환 집산도로와 곡선 지구 분기를 생성한다.
- 도로는 `USplineComponent`와 `USplineMeshComponent`로 렌더링하고 교차로 메시로 분기 이음매와 스플라인 끝을 덮는다.
- 필지는 도로 근처에서 결정적으로 샘플링하고 도로·강·건물 겹침을 제외한다. 발자국 크기와 회전을 변화시키고 중심부 밀도를 높인다.
- 완성형 건물 3종을 균형 있고 결정적으로 섞는다. 특정 건물을 희귀 스타일, 높이 조건이나 낮은 무작위 확률 뒤에 숨기지 않는다.
- 완성형 건물은 차로 폭 기준의 균일 스케일을 사용하고 높이 가변 건물은 Base/Middle/Crown HISM을 적층한다.
- 지면 타일 대신 영구 Landscape를 사용한다. 현재 Landscape는 컴포넌트 4 x 4, 컴포넌트당 섹션 2 x 2, 섹션당 63쿼드로 36,000cm 영역에 505 x 505 버텍스를 만든다.
- 강바닥은 `-230cm`, 수면은 `-90cm`에 두고 강은 하나로 이어진 UV 애니메이션 리본으로 렌더링한다.
- 자동차는 제한 속도, 앞차 간격, 적신호 정지와 올바른 액터 전방 정렬이 있는 방향성 2차선 그래프를 이용한다.
- 고양이는 차도 밖의 별도 보도 웨이포인트 그래프를 사용하며 시각 전방과 이동 방향이 일치해야 한다.
- 충돌하는 두 이동 축에 대해 6단계 신호 주기를 사용한다.
- 나무는 도로, 강 여유 영역, 교량 진입부와 이미 사용 중인 건물 필지를 피해 배치한다.
- `W A S D` 화면 이동, 마우스 휠 줌과 마우스 오른쪽 버튼 수평 이동 회전을 지원한다.

### 7. 환경, 조명과 비주얼 패리티

동일한 제작 액터가 없을 때만 런타임에서 환경 액터를 생성한다. 기존 액터가 있으면 재사용해 레벨 재로딩과 PIE에서 대기, 태양, Sky Light, 포그, 구름과 Post Process Volume이 중복되지 않게 한다.

Directional Light 설정은 다음과 같다.

- 위치 `(0, 0, 8000)`
- 회전 Roll/X `-180도`, Pitch/Y `-161도`, Yaw/Z `193도`
- 스케일 `(2.5, 2.5, 2.5)`
- 강도 `3.4`, 간접광 강도 `1.05`
- Source Angle `1.0`, Contact Shadow Length `0.035`, Contact Shadow Intensity `0.65`
- Movable, Atmosphere Sun Light와 구름 그림자 활성화

그늘진 건물 면이 읽히도록 푸른 앰비언트가 있는 이동형 실시간 Sky Light와 검지 않은 Lower Hemisphere를 사용한다. Sky Atmosphere, 볼류메트릭 포그가 활성화된 Exponential Height Fog와 UE 5.7 내장 심플 Volumetric Cloud 머티리얼을 사용한다.

무한 범위 런타임 Post Process Volume은 물리 카메라 노출을 끈 Manual Exposure, 노출 보정 `0.0`, AO 강도 `0.45`, AO 반경 `120`, AO Power `1.2`, Saturation `0.97`, Contrast `1.02`, Bloom 강도 `0.25`, Bloom Threshold `1.4`를 사용한다.

유한한 Landscape 바깥에는 검은 월드가 보일 수 있다. 플레이 영역 바깥과 아래에 크고 낮은 원경 지면 평면을 유지하고 충돌과 그림자를 끈다. Height Fog로 수평선에 섞이게 하며 게임플레이와 내비게이션은 Landscape에서만 처리한다.

제어된 머티리얼 비교는 `Content/Python/BuildSolCityLookDev.py`를 두 모드로 실행한다.

```powershell
$Editor = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
& $Editor "$PWD\SolCity.uproject" -ExecutePythonScript="$PWD\Content\Python\BuildSolCityLookDev.py" -SolCityLookDevStage=legacy -unattended -nop4 -nosplash -DDC-ForceMemoryCache -NoZenLoader
& $Editor "$PWD\SolCity.uproject" -ExecutePythonScript="$PWD\Content\Python\BuildSolCityLookDev.py" -SolCityLookDevStage=native -unattended -nop4 -nosplash -DDC-ForceMemoryCache -NoZenLoader
```

분리된 `/Game/Maps/SolCityLookDev` 장면은 53mm 풀프레임 카메라, 고정 3점 조명, Reflection Capture, 중립 Unlit 배경과 Manual Exposure를 사용한다. 동일한 1600 x 900 캡처를 다음 위치에 저장한다.

```text
Saved/VisualParity/SolCity_CornerRetail_lookdev_before.png
Saved/VisualParity/SolCity_CornerRetail_lookdev_after.png
```

Legacy 모드는 액터 단위 오버라이드만 사용하며 실제 Static Mesh의 네이티브 할당을 바꾸지 않는다.

### 8. 빌드와 검증

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" SolCityEditor Win64 Development -Project="$PWD\SolCity.uproject" -WaitMutex -NoHotReloadFromIDE
```

`SolCity.uproject`를 열고 `/Game/Maps/SolCity`를 로드해 Play-in-Editor를 실행한다. 다음 내용을 검증한다.

- 마우스 휠이 전체 범위에서 줌을 수행하고 카메라 조작이 정상인지 확인한다.
- Landscape 가장자리, 원경 지면, 포그와 구름을 통해 검은 외곽이 보이지 않는지 확인한다.
- 잔디가 형광색이 아니며 그늘진 건물 면에 앰비언트 스카이 컬러가 남는지 확인한다.
- 강과 스플라인 도로가 연속적이며 교차로와 교량 진입부에 이음매가 없는지 확인한다.
- 교각과 상판이 강과 도로 차로에 맞게 정렬되는지 확인한다.
- 완성형 건물이 원본 비율을 유지하고 3종 모두 균형 있게 섞이는지 확인한다.
- 모듈 건물이 늘어난 박스가 아니라 올바른 크기의 Base/Middle/Crown 층으로 구성되는지 확인한다.
- 셋업 반복 실행이나 FBX 재임포트 후에도 네이티브 머티리얼 슬롯이 유지되는지 확인한다.
- 자동차가 이동 방향을 바라보고 차로를 유지하며 적신호에 정지하는지 확인한다.
- 고양이가 보도를 벗어나지 않고 이동 방향을 바라보는지 확인한다.
- LookDev legacy/native 캡처의 구도와 노출이 동일한지 확인한다.

반복 실행 안전성을 검증할 때 건물 머티리얼 셋업을 두 번 실행한다. 두 실행 모두 할당 슬롯 `28/28`, 오류 0을 출력해야 한다.
