# Blender MCP 모델링 지침

이 문서는 다른 Codex 대화에서도 SolCity용 Blender 모델을 같은 방식으로 생성하기 위한 프로젝트 표준이다. Blender 조작에는 `djeada/blender-mcp-server` 기반의 `blender` MCP를 사용한다.

## 1. 현재 구성

- Blender: `C:\Program Files\Blender Foundation\Blender 5.2\blender.exe`
- MCP 서버: `Tools/blender-mcp-server/`
- 서버 실행 파일: `Tools/blender-mcp-server/.mcp-runtime/Scripts/blender-mcp-server.exe`
- Blender 애드온: `Blender MCP Bridge` (`blender_mcp_bridge`)
- 브리지 주소: `127.0.0.1:9876`
- Codex MCP 이름: `blender`
- 건물 결과물: `Content/Art/Buildings/`

Codex 전역 설정에는 다음과 같은 형태로 서버가 등록되어 있다.

```text
codex mcp add blender -- D:\github\SolCity\Tools\blender-mcp-server\.mcp-runtime\Scripts\blender-mcp-server.exe
```

이미 설치된 서버와 애드온을 매 작업마다 다시 설치하지 않는다. 먼저 등록 및 연결 상태를 확인한다.

## 2. 작업 시작 절차

1. `codex mcp list`에서 `blender`가 `enabled`인지 확인한다.
2. 현재 대화에 `blender_*` 도구가 노출되어 있는지 확인한다. 전역 MCP를 방금 등록한 대화라면 새 대화 또는 Codex 재시작 후 도구가 표시될 수 있다.
3. Blender 프로세스와 포트 사용 여부를 확인한다.

```powershell
Get-Process blender -ErrorAction SilentlyContinue
Test-NetConnection -ComputerName 127.0.0.1 -Port 9876
```

4. 사용자가 열어 둔 Blender 씬은 임의로 초기화하거나 종료하지 않는다. 작업 소유권이 불명확하면 별도의 Blender 5.2 인스턴스를 사용한다.
5. 작업용 인스턴스에서는 `Blender MCP Bridge` 애드온을 활성화하고 MCP 패널이 `Listening on 127.0.0.1:9876` 상태인지 확인한다.

애드온을 활성화한 작업용 인스턴스 시작 스크립트는 다음 파일을 재사용할 수 있다.

```text
Tools/blender-mcp-server/scripts/start_mcp_52.py
```

## 3. MCP 사용 원칙

모델링 요청은 반드시 MCP 호출로 Blender에 반영한다. Python 파일만 작성하고 완료로 처리하지 않는다.

권장 흐름:

1. `blender_scene_get_info`로 연결과 현재 씬을 확인한다.
2. 단순 편집은 구조화된 `blender_object_*`, `blender_material_*` 도구를 사용한다.
3. 절차적 건물처럼 작업량이 큰 경우 `blender_python_exec`를 사용한다.
4. 생성 후 다시 `blender_scene_get_info`, `blender_scene_list_objects`, `blender_material_list`로 결과를 확인한다.
5. 시간이 오래 걸리는 베이크나 시뮬레이션만 `blender_python_exec_async`를 사용한다.

주요 도구:

- 씬 조회: `blender_scene_get_info`, `blender_scene_list_objects`
- 오브젝트 조작: `blender_object_create`, `blender_object_translate`, `blender_object_rotate`, `blender_object_scale`
- 재질: `blender_material_create`, `blender_material_assign`, `blender_material_set_color`
- 저장·생성 스크립트: `blender_python_exec`
- 렌더: `blender_render_still`
- 내보내기: `blender_export_fbx`, `blender_export_gltf`, `blender_export_obj`
- 복구: `blender_history_undo`, `blender_history_redo`

`blender_python_exec`에 전달하는 스크립트는 마지막에 JSON 직렬화 가능한 `__result__`를 설정해야 한다.

```python
__result__ = {
    "asset": asset_name,
    "mesh_objects": mesh_count,
    "dimensions_m": [width, depth, height],
    "blend": blend_path,
    "fbx": fbx_path,
    "preview": preview_path,
}
```

MCP 응답 자체가 성공이어도 응답 본문의 `error` 값이 채워질 수 있다. 반드시 `error`, `stderr`, `timed_out`, `cancelled`를 확인한다.

## 4. 모델링 표준

### 좌표와 크기

- 단위계: Metric
- 길이 단위: Meter
- `scale_length`: `1.0`
- 지면: `Z=0`
- 건물 중심과 피벗은 가능한 한 월드 원점 근처에 둔다.
- Unreal 기준 전방축 내보내기: `-Y`, 상단축: `Z`
- 모든 치수는 프롬프트 또는 결과에 미터 단위로 기록한다.

### 명명

- 정적 메시 건물: `SM_SolCity_<Type>_<Number>`
- 재질: `M_<AssetOrType>_<Surface>`
- 미리보기 카메라와 조명: `Preview_*`
- 생성 파일의 기본 이름을 `.blend`, `.fbx`, `_preview.png`에서 동일하게 유지한다.

예시:

```text
SM_SolCity_SteppedTower_01.blend
SM_SolCity_SteppedTower_01.fbx
SM_SolCity_SteppedTower_01_preview.png
```

### 건물 구성

디자인 지시가 부족하면 다음을 합리적인 기본값으로 사용한다.

- SolCity의 현대 도시 환경과 어울리는 읽기 쉬운 실루엣
- 1층 출입구 또는 상업 파사드
- 반복 가능한 창호 모듈과 구조 리듬
- 옥상 파라펫, 설비, 태양광 패널 또는 간판 중 최소 하나
- 전면과 측면 모두 비어 보이지 않게 구성
- 미리보기에서 형태를 읽을 수 있는 3/4 카메라 구도
- 같은 요청에서 복수 건물을 만들 때는 높이, 매스, 색상, 용도를 분명히 다르게 설계

### 재질

- 기본적으로 Principled BSDF를 사용한다.
- 한 건물에 지나치게 많은 재질을 만들지 않는다.
- 유리는 지나친 투명 효과 대신 게임용으로 읽기 쉬운 어두운 청록색 또는 청색 표면으로 표현할 수 있다.
- 발광 간판은 `Emission Color`와 낮은 수준의 `Emission Strength`를 사용한다.
- 최종 렌더에서 노출 과다, 완전한 검정, 뒤집힌 노멀을 확인한다.

## 5. 저장과 내보내기

각 모델은 다음 세 파일을 반드시 만든다.

1. 편집 가능한 Blender 원본 `.blend`
2. Unreal용 `.fbx`
3. 검수용 `_preview.png`

FBX 권장 설정:

```python
bpy.ops.export_scene.fbx(
    filepath=fbx_path,
    use_selection=True,
    object_types={"MESH"},
    apply_unit_scale=True,
    apply_scale_options="FBX_SCALE_UNITS",
    use_mesh_modifiers=True,
    add_leaf_bones=False,
    bake_anim=False,
    axis_forward="-Y",
    axis_up="Z",
)
```

내보내기 전에는 카메라와 미리보기 조명을 선택에서 제외하고 에셋 컬렉션의 메시만 선택한다.

상세 조립본이 수백 개 오브젝트로 구성된 경우 Unreal 가져오기에서 하나의 건물로 사용할 때 `Combine Meshes`를 권장한다. 실제 런타임 에셋으로 최적화하라는 요청이 있으면 재질별 병합, LOD, 충돌 메시, 라이트맵 UV를 별도 처리한다.

## 6. 필수 검증

### 미리보기 검수

생성된 PNG를 실제로 열어 확인한다. 다음 항목을 본다.

- 건물이 프레임 안에 모두 들어오는가
- 지면 아래로 내려간 오브젝트가 없는가
- 창호, 발코니, 간판이 벽과 심하게 교차하지 않는가
- 조명이 형태를 읽기에 충분한가
- 재질 색상과 발광이 의도대로 보이는가

### FBX 재가져오기

Blender 5.2의 `--background --factory-startup` 상태에서 FBX를 새 씬에 가져온다. 최소한 다음을 출력하고 확인한다.

- 메시 오브젝트 수가 0보다 큰가
- 바운딩 박스가 예상 크기인가
- 최소 Z가 지면에서 크게 벗어나지 않는가
- 가져오기 과정에서 예외가 없는가

기존 검증 예제:

```text
Tools/blender-mcp-server/scripts/verify_solcity_fbx.py
```

검증에 실패하면 결과물을 전달하기 전에 생성 스크립트를 수정하고 MCP 호출부터 다시 실행한다.

## 7. 안전 규칙

- 사용자가 연 Blender 프로세스를 허락 없이 종료하지 않는다.
- 모델 생성 전에 기존 씬을 비우려면 반드시 작업용 별도 인스턴스인지 확인한다.
- `blender_python_exec`는 Blender 안에서 Python을 실행하므로 저장 경로와 삭제 대상을 명시적으로 제한한다.
- 프로젝트 루트나 사용자 홈을 재귀 삭제하지 않는다.
- 외부 에셋, HDRI, 텍스처 또는 생성형 3D 서비스를 다운로드하려면 먼저 사용자 요청 범위인지 확인한다.
- 생성 완료 후 Codex가 시작한 숨김 Blender 인스턴스는 저장 여부를 확인한 뒤 정리한다.

## 8. 문제 해결

### MCP 서버가 보이지 않음

- `codex mcp list`에서 `blender` 등록을 확인한다.
- 현재 대화가 MCP 등록 전에 시작됐다면 Codex를 재시작하거나 새 대화를 연다.
- 전역 설정을 매번 중복 등록하지 않는다.

### 포트 9876이 열리지 않음

- 애드온 활성화 여부를 확인한다.
- `start_mcp_52.py`에서 `bridge._ensure_server_running()`을 실행한다.
- 다른 Blender 인스턴스가 9876을 사용 중인지 확인한다.
- 하나의 포트에는 하나의 Bridge만 실행한다.

### Blender 5.2 렌더 엔진 오류

Blender 5.2에서는 EEVEE 엔진 값을 다음처럼 사용한다.

```python
scene.render.engine = "BLENDER_EEVEE"
```

`BLENDER_EEVEE_NEXT`는 Blender 5.2에서 유효하지 않을 수 있다.

### 스크립트 경로가 거부됨

애드온은 `script_path`를 승인된 루트로 제한할 수 있다. 프로젝트의 검토된 스크립트를 읽어 내용 전체를 `code`로 전달하거나, 애드온 설정의 `Approved Script Roots`를 명시적으로 구성한다.

## 9. 다른 대화에서 사용할 요청 예시

```text
AGENTS.md와 Docs/BlenderMCP_Modeling_Guide.md를 따라 Blender MCP로
SolCity용 4층 코너 상가를 만들어줘.
Blender 원본, Unreal용 FBX, 미리보기 PNG를 만들고 FBX 재가져오기까지 검증해줘.
```

```text
Blender MCP를 사용해서 기존 SM_SolCity_MidRise_01.blend를 열고
옥상 간판을 제거한 뒤 야간 발광 창문 비율을 늘려줘.
기존 파일은 덮어쓰지 말고 _NightVariant 이름으로 저장해줘.
```
