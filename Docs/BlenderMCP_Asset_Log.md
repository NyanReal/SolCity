# SolCity Blender MCP 에셋·스플라인 도로 작업 기록

작업일: 2026-07-18
대상: Unreal Engine 5.7 / Blender 5.2.0 LTS

## 작업 환경

- Blender MCP: `djeada/blender-mcp-server`
- Codex MCP 이름: `blender`
- 실행 파일: `Tools/blender-mcp-server/.mcp-runtime/Scripts/blender-mcp-server.exe`
- Blender 브리지: `127.0.0.1:9876`
- 생성 호출: `Tools/blender-mcp-server/scripts/create_props_via_mcp.py`
- Blender 생성 본문: `Tools/blender-mcp-server/scripts/create_solcity_props.py`
- FBX 클린 재임포트 검증: `Tools/blender-mcp-server/scripts/verify_solcity_props_fbx.py`
- UE 임포트: `Content/Python/ImportSolCityProps.py`

기존 `.venv`는 삭제된 Microsoft Store Python 3.11 경로를 참조해 실행할 수 없었다. 프로젝트 내부 `.mcp-runtime`을 Python 3.12로 새로 만들고 서버를 editable 설치한 뒤 `codex mcp`의 `blender` 등록도 이 경로로 교체했다. 모델 생성 전후 `blender_scene_get_info`와 `blender_python_exec`를 실제 MCP 세션에서 호출했다.

## 생성 에셋

모든 에셋은 같은 이름의 편집 가능한 `.blend`, UE용 `.fbx`, `_preview.png`를 `Content/Art/Props/`에 둔다. FBX는 `axis_forward=-Y`, `axis_up=Z`, meter 단위, 메시만 선택 내보내기했다. UE에서는 `Combine Meshes`, 라이트맵 UV 생성, 자동 충돌 생성을 사용한다.

| 에셋 | 크기(m) | 메시 수 | 재질 수 | 용도 |
|---|---:|---:|---:|---|
| `SM_SolCity_Cat_01` | 0.586 × 0.950 × 0.810 | 19 | 5 | 보도 네트워크를 걷는 고양이 시민 |
| `SM_SolCity_AutonomousCar_01` | 1.900 × 3.555 × 1.641 | 13 | 6 | 도로 그래프를 주행하는 고양이 귀 자율주행차 |
| `SM_SolCity_RoadSpline_01` | 10.200 × 4.000 × 0.260 | 8 | 4 | 스플라인 변형용 세분화 도로 단면, UE에서 +X 전방 |
| `SM_SolCity_RoadJunction_01` | 11.000 × 11.000 × 0.350 | 29 | 4 | T/십자 분기 이음매를 덮는 횡단보도 교차부 캡 |
| `SM_SolCity_Bridge_01` | 12.200 × 36.000 × 6.860 | 83 | 5 | 교대, 4개 교각, 가로보, 종거더, 보행로, 난간, 조명 포함 도시 교량 |
| `SM_SolCity_Tree_01` | 2.130 × 1.843 × 4.116 | 11 | 4 | 가로수 HISM 산포 |

클린 Blender 5.2 재임포트에서 여섯 FBX 모두 메시 수가 0보다 크고 바운드가 유효했으며 예외가 없었다. 바닥 기준 에셋인 도로·교차부·교량·나무의 최소 Z는 0m이고, 고양이는 0.015m, 자동차는 바퀴 하단 기준 0.15m이다.

## UE 스플라인 도로 처리

Epic의 Blueprint Spline 방식과 동일한 컴포넌트를 C++ 생성기에 적용했다.

1. 도로 폴리라인마다 `USplineComponent`를 만들고 각 제어점을 `CurveClamped`로 설정한다.
2. 인접 제어점 한 쌍마다 `USplineMeshComponent`를 하나씩 만들며 시작/끝 위치와 arrive/leave tangent를 공유한다.
3. Blender 도로 메시를 X-forward로 가져오고 `SetForwardAxis(ESplineMeshAxis::X)`를 사용한다.
4. 도로 등급별 폭은 스플라인 메시의 시작/끝 횡방향 스케일로 적용한다.
5. 차량용 `RoadSegments`와 고양이용 보도 웨이포인트는 시각 스플라인과 분리해 기존 시뮬레이션 위상을 유지한다.
6. 폴리라인 끝점과 신호 교차점에는 `SM_SolCity_RoadJunction_01`을 중복 제거 후 배치해 T/십자 분기 틈을 덮는다.
7. 강을 건너는 일반 구간은 폴리라인을 분리하고, 유일한 횡단 구간은 별도 authored bridge 메시와 bridge navigation edge가 담당한다.

참고한 Epic 공식 문서:

- [Blueprint Spline Components Overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/blueprint-spline-components-overview-in-unreal-engine)
- [Blueprint Spline Mesh Component Property Reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/blueprint-spline-mesh-component-property-reference-in-unreal-engine)
- [USplineComponent API (UE 5.7)](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/USplineComponent?application_version=5.7)
- [USplineMeshComponent API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/USplineMeshComponent)

## 런타임 연결

- `ASolCityCatCitizen`: authored cat 메시가 있으면 엔진 primitive fallback을 숨긴다.
- `ASolCityAutonomousVehicle`: authored car 메시가 있으면 cube body/cabin fallback을 숨긴다.
- 자동차 모델의 Blender 전방 `-Y`는 시각 메시 컴포넌트에 Yaw `+90°`를 적용해 UE 액터 전방 `+X`와 정렬한다. 액터 루트를 별도로 두므로 주행, 전방 차량 감지, 신호 판단 좌표계는 영향을 받지 않는다.
- `ASolCityGenerator`: authored road, junction, bridge, tree를 생성자에서 로드한다.
- 나무는 강·도로면·건물 footprint를 피하면서 도로에서 4.3~8.6m 떨어진 보행 영역에 결정론적으로 산포한다.
- 교량 FBX의 주행면 Z=4.55m를 `BridgeDeckZ`에 맞춰 전체 구조물의 높이를 정렬한다.
- `M_AnimeAsphalt`에는 Instanced Static Mesh와 Spline Mesh 셰이더 사용 플래그를 함께 저장한다.

## 검증 결과

- Blender MCP 생성: 성공, `stderr` 없음, `timed_out=false`, `cancelled=false`
- Blender 5.2 FBX 클린 재임포트: 6/6 성공
- UE 5.7 FBX 임포트 및 24개 관련 에셋 validation: 성공
- `SolCityEditor Win64 Development` 빌드: 성공
- 오프스크린 게임 실행: 치명 오류 없음, spline material usage 경고 없음
- 최종 시각 검증: `Saved/Screenshots/WindowsEditor/HighresScreenshot00003.png`
