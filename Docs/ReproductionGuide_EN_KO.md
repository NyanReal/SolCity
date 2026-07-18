# Sol City Reproduction Guide

## English

### 1. Target result

Build a UE 5.7 C++ city-builder prototype with a peaceful Japanese-anime-background look and cats as citizens. Use an irregular procedural road hierarchy rather than a plain grid. Cars travel autonomously on a directed road graph and obey traffic lights, while cats use a separate sidewalk graph.

The project contains:

- A 36,000 × 36,000 cm city and camera zoom from 800 to 48,000 cm.
- Irregular arterial roads, a collector loop, curved branches, spline road meshes, and junction caps.
- Road-aware procedural lots with density and height biased toward the city center.
- Authored Blender/FBX buildings mixed with modular procedural buildings.
- A persistent 505 × 505 Landscape, a lowered riverbed, and one continuous UV-animated river surface.
- Modeled cats, cars, bridge, road pieces, junctions, and trees.
- Movable sunlight, blue ambient skylight, atmospheric fog, volumetric clouds, autonomous traffic, and signals.

### 2. Required software

- Windows 10 or 11
- Git
- Unreal Engine 5.7
- Visual Studio 2022 with Desktop development with C++ and the Windows SDK
- Blender 5.2 LTS
- Python 3.12
- Codex CLI when regenerating models through Blender MCP

### 3. Clone and open

```powershell
git clone https://github.com/NyanReal/SolCity.git
Set-Location SolCity
```

The checked-in Unreal assets, `.blend`, `.fbx`, preview PNG, and source textures are sufficient to build and run the prototype. Blender MCP and its Python environments are ignored by the main repository and are required only when models must be regenerated or edited.

### 4. Install Blender MCP

```powershell
git clone https://github.com/djeada/blender-mcp-server.git Tools/blender-mcp-server
py -3.12 -m venv Tools/blender-mcp-server/.mcp-runtime
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install --upgrade pip
& Tools/blender-mcp-server/.mcp-runtime/Scripts/python.exe -m pip install -e Tools/blender-mcp-server
```

Build the Blender add-on zip with Git Bash:

```bash
cd Tools/blender-mcp-server
bash scripts/build_addon_zip.sh
```

In Blender 5.2, open **Edit > Preferences > Add-ons > Install from Disk**, install `Tools/blender-mcp-server/dist/blender_mcp_bridge.zip`, enable **Blender MCP Bridge**, and confirm `Listening on 127.0.0.1:9876` in the 3D Viewport **MCP** sidebar.

Register the server with Codex, replacing the path if necessary:

```powershell
codex mcp add blender -- D:\github\SolCity\Tools\blender-mcp-server\.mcp-runtime\Scripts\blender-mcp-server.exe
codex mcp list
```

Restart Codex if the Blender tools are not visible. Before editing, inspect the connected scene and preserve any Blender session owned by someone else.

### 5. Blender asset workflow

Make actual Blender changes through MCP; writing an unexecuted Python file is not a completed modeling pass. Use metric units with `scale_length = 1.0` and keep ground contact near `Z = 0`.

Each authored building must have matching deliverables in `Content/Art/Buildings`:

```text
SM_SolCity_<Name>_01.blend
SM_SolCity_<Name>_01.fbx
SM_SolCity_<Name>_01_preview.png
```

Store gameplay props in `Content/Art/Props`. Export selected mesh objects only with applied unit scale, `axis_forward = -Y`, and `axis_up = Z`. Import multi-object assets into Unreal with **Combine Meshes**, lightmap UV generation, and automatic collision.

Preserve the proportions of every completed Blender-authored building. A completed building may use one uniform XYZ scale derived from the standard road-lane width so it fits the city scale, but never calculate or apply a separate Z scale from `target height / source mesh height`. Buildings that need variable height use real Blender-authored FBX modules stacked vertically; do not stretch a finished building mesh.

The variable-height module set is:

- `SM_SolCity_BuildingBase_01`
- `SM_SolCity_BuildingMiddle_01`
- `SM_SolCity_BuildingCrown_01`

Model the windows, frames, ledges, entrances, and roof details in these modules and use their authored materials. Export, preview, and clean-scene re-import them with the same checks as completed buildings.

Always inspect the rendered preview and re-import the FBX into a clean Blender 5.2 scene. Verify mesh count, non-degenerate bounds, ground contact, dimensions, and error-free import.

The road spline mesh must be longitudinally subdivided and X-forward in Unreal. The car mesh is authored nose-forward in Blender `-Y`; the vehicle actor applies only a visual yaw correction so its mesh follows the Unreal actor's `+X` travel direction without rotating sensing logic.

The authored cat also faces Blender local `-Y`; apply a mesh-relative yaw offset of `+90°` so it faces the sidewalk path tangent while leaving pedestrian navigation in actor-local `+X`.

### 6. Texture and Unreal asset setup

Source textures are stored in `Content/Art/Source`:

- `T_AnimeGrass.png`
- `T_AnimeAsphalt.png`
- `T_AnimeWater.png`
- `T_AnimeFacade.png`

ImageGen outputs may replace them while retaining the filenames. Enable Python Script Plugin and Editor Scripting Utilities, then run these scripts from **Tools > Execute Python Script** in order:

1. `Content/Python/SetupSolCityAssets.py`
2. `Content/Python/TuneSolCityEnvironment.py`
3. `Content/Python/ImportSolCityBuildingModules.py`
4. `Content/Python/ImportSolCityProps.py`
5. `Content/Python/CreateSolCitySplineMaterials.py`
6. `Content/Python/FinalizeSolCityMaterials.py`
7. `Content/Python/BuildSolCityLandscape.py`

`TuneSolCityEnvironment.py` reduces the grass material's fluorescent appearance with a muted green tint, high roughness, and low specular response. Run it again whenever the base grass material is regenerated. `T_AnimeFacade` and the old box-facade material are not used as procedural-building walls; the modular FBX meshes provide modeled windows, ledges, and authored materials.

The final script saves the persistent Landscape in `/Game/Maps/SolCity`. It uses 4 × 4 components, 2 × 2 sections per component, and 63 quads per section, producing 505 × 505 vertices over 36,000 cm. The riverbed is carved to -230 cm and the water surface is at -90 cm.

### 7. Runtime implementation

- Generate an asymmetrical arterial spine, collector loop, and curved district branches.
- Render roads with `USplineComponent` and `USplineMeshComponent`; cover branch seams with junction meshes.
- Sample lots deterministically near roads, reject road/river/building overlaps, vary footprints and rotations, and increase density toward the center.
- Preserve completed authored FBX building proportions and use only one lane-width-derived uniform XYZ scale when city-scale adjustment is necessary. Never scale Z independently.
- Build variable-height procedural buildings by stacking `SM_SolCity_BuildingBase_01`, repeated `SM_SolCity_BuildingMiddle_01`, and `SM_SolCity_BuildingCrown_01` as HISM instances. Do not generate procedural walls from `CubeMesh`, and do not apply the old `M_AnimeFacade` window-wall texture to boxes.
- Distribute all three authored building types evenly and deterministically across eligible lots; do not hide any type behind rare style, height, or random-chance gates.
- Use a persistent Landscape instead of ground tiles and render the river as one continuous procedural ribbon with UV animation.
- Keep cars on a directed two-lane graph with speed limits, following distance, traffic-light stops, and correct `+X` actor-forward alignment.
- Keep cats on a separate sidewalk waypoint graph and verify that their visual forward direction matches movement.
- Use six traffic-signal phases across two conflicting movement axes.

### 8. Environment and lighting

Configure the movable Directional Light to match the editor transform: Location `(0, 0, 8000)`, Rotation X/Roll `-180°`, Y/Pitch `-161°`, Z/Yaw `193°`, and Scale `(2.5, 2.5, 2.5)`. Use intensity `3.4` with Atmosphere Sun Light enabled.

Use a movable, real-time captured Sky Light with a blue ambient contribution and a non-black lower hemisphere so shadowed facades remain readable. Add Exponential Height Fog with volumetric fog enabled. Add a Volumetric Cloud actor using UE 5.7's built-in simple volumetric-cloud material.

A finite Landscape otherwise exposes the black world below the horizon. Place a large, low distant-ground plane outside and beneath the playable Landscape, give it the muted ground material, disable collision and shadows, and let the height fog blend it into the horizon. This plane is only a visual background; gameplay and navigation remain on the Landscape.

### 9. Build and verify

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" SolCityEditor Win64 Development "$PWD\SolCity.uproject" -WaitMutex -NoHotReloadFromIDE
```

Open `SolCity.uproject`, load `/Game/Maps/SolCity`, and use Play-in-Editor. Controls:

- `W A S D`: pan
- Mouse wheel: zoom
- Hold right mouse button and move horizontally: rotate

Verify that the Landscape has no tile gaps, the distant ground and fog remove the black perimeter, clouds render correctly, grass is muted rather than fluorescent, and shadowed facades retain blue ambient light. Also verify continuous river and road seams, bridge placement, original authored-building proportions, modular floor stacking, vehicle orientation and red-light stops, and cat sidewalk movement and facing.

---

## 한국어 번역

### 1. 재현 목표

평화로운 일본 애니메이션 배경풍의 UE 5.7 C++ 시티빌더 프로토타입을 만들며 시민은 고양이로 구성한다. 단순 격자 대신 비정형 절차적 도로 계층을 사용한다. 자동차는 방향성 도로 그래프에서 자율주행하며 신호를 지키고, 고양이는 별도의 보도 그래프를 이용한다.

프로젝트 구성은 다음과 같다.

- 36,000 × 36,000cm 도시와 800~48,000cm 카메라 줌 범위
- 비정형 간선도로, 순환 집산도로, 곡선 분기, 스플라인 도로 메시, 교차로 캡
- 도로를 기준으로 배치하고 도심에 밀도와 높이가 집중되는 절차적 필지
- Blender/FBX 완성형 건물과 모듈식 절차 건물의 혼합
- 505 × 505 영구 Landscape, 낮아진 강바닥, 하나로 이어진 UV 애니메이션 강 표면
- 모델링된 고양이, 자동차, 교량, 도로 조각, 교차로, 나무
- 이동형 태양광, 푸른 앰비언트 스카이라이트, 대기 포그, 볼류메트릭 클라우드, 자율 교통과 신호

### 2. 필요한 소프트웨어

- Windows 10 또는 11
- Git
- Unreal Engine 5.7
- Desktop development with C++와 Windows SDK를 설치한 Visual Studio 2022
- Blender 5.2 LTS
- Python 3.12
- Blender MCP로 모델을 재생성할 때 Codex CLI

### 3. 복제 및 실행 준비

```powershell
git clone https://github.com/NyanReal/SolCity.git
Set-Location SolCity
```

저장소의 Unreal 에셋, `.blend`, `.fbx`, 미리보기 PNG, 원본 텍스처만으로 빌드와 실행이 가능하다. Blender MCP와 Python 환경은 메인 저장소에서 제외되며 모델을 다시 만들거나 편집할 때만 설치한다.

### 4. Blender MCP 설치

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

Blender 5.2에서 **Edit > Preferences > Add-ons > Install from Disk**를 열어 `Tools/blender-mcp-server/dist/blender_mcp_bridge.zip`을 설치하고 **Blender MCP Bridge**를 활성화한다. 3D Viewport의 **MCP** 사이드바에서 `Listening on 127.0.0.1:9876`을 확인한다.

필요하면 실제 복제 경로로 바꾸어 Codex에 서버를 등록한다.

```powershell
codex mcp add blender -- D:\github\SolCity\Tools\blender-mcp-server\.mcp-runtime\Scripts\blender-mcp-server.exe
codex mcp list
```

Blender 도구가 보이지 않으면 Codex를 다시 시작한다. 편집 전 연결된 장면을 확인하고 다른 사람이 사용 중인 Blender 세션은 유지한다.

### 5. Blender 에셋 작업 방식

실제 Blender 변경은 MCP를 통해 수행한다. 실행하지 않은 Python 파일만 작성한 상태는 완료로 보지 않는다. 미터법, `scale_length = 1.0`, 지면 접점 `Z = 0` 근처를 기준으로 한다.

완성형 건물은 `Content/Art/Buildings`에 이름이 일치하는 파일을 둔다.

```text
SM_SolCity_<Name>_01.blend
SM_SolCity_<Name>_01.fbx
SM_SolCity_<Name>_01_preview.png
```

게임플레이 소품은 `Content/Art/Props`에 둔다. 선택한 메시 오브젝트만 단위 스케일을 적용해 `axis_forward = -Y`, `axis_up = Z`로 내보낸다. Unreal에서는 **Combine Meshes**, 라이트맵 UV 생성, 자동 충돌을 사용한다.

Blender에서 완성한 건물은 원본 비율을 유지한다. 도시 스케일에 맞출 필요가 있으면 표준 도로 차선 폭에서 계산한 하나의 균일 XYZ 스케일은 사용할 수 있지만, `목표 높이 / 원본 메시 높이`로 Z만 따로 계산하거나 늘리지 않는다. 높이가 달라져야 하는 건물은 완성형 메시를 잡아 늘리는 대신 실제 Blender 제작 FBX 모듈을 층층이 쌓는다.

가변 높이 모듈은 다음 3종이다.

- `SM_SolCity_BuildingBase_01`
- `SM_SolCity_BuildingMiddle_01`
- `SM_SolCity_BuildingCrown_01`

창문, 프레임, 돌출부, 출입구, 옥상 디테일은 이 모듈에 직접 모델링하고 제작된 머티리얼을 사용한다. 완성형 건물과 같은 기준으로 내보내기, 미리보기, 깨끗한 장면 재임포트를 검증한다.

렌더 미리보기를 확인하고 FBX를 깨끗한 Blender 5.2 장면에 다시 가져온다. 메시 수, 정상 바운드, 지면 접점, 치수, 오류 없는 임포트를 확인한다.

도로 스플라인 메시는 길이 방향으로 충분히 분할하고 Unreal에서 X-forward가 되게 한다. 자동차 메시는 Blender `-Y` 방향이 전면이며, 차량 액터는 감지 로직을 돌리지 않고 시각 메시의 yaw만 보정해 Unreal 액터의 `+X` 주행 방향과 맞춘다.

완성형 고양이도 Blender 로컬 `-Y`가 전방이므로 메시 상대 yaw를 `+90°` 보정한다. 보행자 내비게이션은 액터 로컬 `+X`를 유지하면서 시각 메시만 보도 경로 접선을 바라보게 한다.

### 6. 텍스처 및 Unreal 에셋 설정

원본 텍스처는 `Content/Art/Source`에 있다.

- `T_AnimeGrass.png`
- `T_AnimeAsphalt.png`
- `T_AnimeWater.png`
- `T_AnimeFacade.png`

같은 파일명을 유지하면 ImageGen 결과물로 교체할 수 있다. Python Script Plugin과 Editor Scripting Utilities를 활성화한 뒤 **Tools > Execute Python Script**에서 다음 순서로 실행한다.

1. `Content/Python/SetupSolCityAssets.py`
2. `Content/Python/TuneSolCityEnvironment.py`
3. `Content/Python/ImportSolCityBuildingModules.py`
4. `Content/Python/ImportSolCityProps.py`
5. `Content/Python/CreateSolCitySplineMaterials.py`
6. `Content/Python/FinalizeSolCityMaterials.py`
7. `Content/Python/BuildSolCityLandscape.py`

`TuneSolCityEnvironment.py`는 채도를 누른 녹색 틴트, 높은 러프니스, 낮은 스페큘러로 잔디의 형광 느낌을 줄인다. 기본 잔디 머티리얼을 다시 생성했다면 이 스크립트도 다시 실행한다. `T_AnimeFacade`와 기존 박스 파사드 머티리얼은 절차 건물 벽에 사용하지 않는다. 모듈 FBX 자체에 모델링된 창문·돌출부와 제작된 머티리얼을 사용한다.

마지막 스크립트는 `/Game/Maps/SolCity`에 영구 Landscape를 저장한다. 구성은 컴포넌트 4 × 4, 컴포넌트당 섹션 2 × 2, 섹션당 63쿼드이며 36,000cm 영역에 505 × 505 버텍스를 만든다. 강바닥은 -230cm, 수면은 -90cm다.

### 7. 런타임 구현

- 비대칭 간선도로, 순환 집산도로, 곡선 지구 분기를 생성한다.
- 도로는 `USplineComponent`와 `USplineMeshComponent`로 렌더하고 교차로 메시로 분기 이음매를 덮는다.
- 필지는 도로 주변에서 결정론적으로 샘플링하고 도로·강·건물 겹침을 제외하며, 대지 크기와 회전을 변화시키고 도심 밀도를 높인다.
- 완성형 FBX 건물은 원본 비율을 유지하고, 도시 크기 보정이 필요할 때만 차선 폭에서 계산한 하나의 균일 XYZ 스케일을 사용한다. Z만 별도로 스케일하지 않는다.
- 가변 높이 절차 건물은 `SM_SolCity_BuildingBase_01`, 반복되는 `SM_SolCity_BuildingMiddle_01`, `SM_SolCity_BuildingCrown_01`을 HISM 인스턴스로 쌓는다. `CubeMesh`로 절차 벽을 만들거나 박스에 기존 `M_AnimeFacade` 창문 벽 텍스처를 적용하지 않는다.
- 완성형 건물 3종은 사용 가능한 필지에 균등하고 결정론적으로 분배한다. 특정 종류를 드문 스타일, 높이 조건, 낮은 무작위 확률 뒤에 숨기지 않는다.
- 지면 타일 대신 영구 Landscape를 사용하고 강은 UV 애니메이션이 있는 하나의 연속 리본 메시로 만든다.
- 자동차는 제한속도, 앞차 간격, 신호 정지, `+X` 액터 전방 정렬을 갖춘 방향성 왕복 2차선 그래프를 이용한다.
- 고양이는 별도의 보도 웨이포인트 그래프를 이용하며 시각적 전방이 이동 방향과 맞는지 확인한다.
- 충돌하는 두 이동 축에 대해 6단계 신호 주기를 사용한다.

### 8. 환경과 조명

이동형 Directional Light는 에디터 표시 기준으로 Location `(0, 0, 8000)`, Rotation X/Roll `-180°`, Y/Pitch `-161°`, Z/Yaw `193°`, Scale `(2.5, 2.5, 2.5)`를 사용한다. 강도는 `3.4`이며 Atmosphere Sun Light를 활성화한다.

그늘진 건물 면이 읽히도록 푸른 앰비언트가 있는 이동형 실시간 캡처 Sky Light와 검지 않은 Lower Hemisphere를 사용한다. Exponential Height Fog를 추가하고 볼류메트릭 포그를 활성화한다. Volumetric Cloud 액터에는 UE 5.7 내장 심플 볼류메트릭 클라우드 머티리얼을 적용한다.

유한한 Landscape만 두면 수평선 아래의 검은 월드가 드러난다. 플레이 Landscape 바깥과 아래에 크고 낮은 원경 지면 평면을 놓고 채도를 낮춘 지면 머티리얼을 적용하며, 충돌과 그림자를 끈 뒤 높이 포그로 수평선에 섞이게 한다. 이 평면은 시각적 배경일 뿐 게임플레이와 내비게이션은 Landscape에서만 처리한다.

### 9. 빌드 및 검증

```powershell
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" SolCityEditor Win64 Development "$PWD\SolCity.uproject" -WaitMutex -NoHotReloadFromIDE
```

`SolCity.uproject`를 열고 `/Game/Maps/SolCity`를 로드한 뒤 Play-in-Editor를 실행한다. 조작법은 다음과 같다.

- `W A S D`: 화면 이동
- 마우스 휠: 줌
- 마우스 오른쪽 버튼을 누른 채 수평 이동: 회전

Landscape 타일 틈이 없는지, 원경 지면과 포그가 검은 외곽을 가리는지, 클라우드가 정상적으로 보이는지, 잔디가 형광색이 아닌 차분한 색인지, 그늘진 건물에 푸른 앰비언트가 남는지 확인한다. 강과 도로 이음매, 교량 배치, 완성형 건물의 원본 비율, 모듈 층 쌓기, 자동차 방향과 적신호 정지, 고양이의 보도 이동과 바라보는 방향도 함께 확인한다.
