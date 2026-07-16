# Sol City

UE 5.7로 만든, 고양이 주민이 사는 평화로운 소형 시티빌더 시뮬레이션입니다. 일본 애니메이션 배경화에서 느껴지는 맑은 낮과 선명한 파스텔 색감을 독창적인 로우폴리/박스 모델링으로 표현합니다.

프로젝트는 별도 맵 에셋 없이 `/Engine/Maps/Entry`에서 바로 부팅됩니다. 도로, 보도, 강, 다리, 건물, 창문, 공원, 나무, 신호등, 자동차와 고양이를 C++이 절차 생성하므로 외부 텍스처가 아직 도착하지 않아도 플레이할 수 있습니다.

## 바로 실행

1. UE 5.7에서 `SolCity.uproject`를 엽니다.
2. C++ 모듈 빌드 요청이 나오면 빌드합니다.
3. Play를 누릅니다.

명령줄 빌드:

```powershell
.\Tools\Build-Editor.ps1
```

조작:

- `WASD` 또는 화면 가장자리: 카메라 이동
- 마우스 휠: 줌 인/아웃
- `Q` / `E`: 카메라 회전
- `Home`: 카메라 초기화
- `H`: 도움말 표시 전환

## 구현된 시뮬레이션

- HISM 기반의 넓은 평면 타일과 격자형 도로망
- 박스 조합 저층 건물, 옥상 구조물, 발광색 창문, 공원과 가로수
- 평면 강과 모든 횡단 도로의 교량
- `M_WaterAnimated`가 있으면 Panner 기반 UV 흐름, 없으면 청록색 수면 움직임 폴백
- 교차로별 남북/동서 2상 신호등
- 차선 주행, 교차로 랜덤 회전, 적신호 정지, 앞차 안전거리 유지 자동차
- 도로와 강을 피하고 건물을 감지하며 걷는 고양이 시민
- 고양이의 다리 교차 움직임, 몸 바운스, 꼬리 흔들기
- 런타임 낮 조명, SkyLight, SkyAtmosphere, 옅은 원경 안개

주요 월드 설정은 `ACityWorldActor`의 `GridHalfExtent`, `RoadSpacing`, `RoadWidth`, `RiverX`, `RiverHalfWidth`, `VehicleCount`, `CitizenCount`에서 조절할 수 있습니다.

## 외부 imagegen 큐

개별 이미지 생성 병목을 피하기 위해 모든 생성 요청은 `ExternalImageRequests.md`에 분리했습니다. 현재 필요한 10개 항목은 `생성 요청 상태: [x]`, `생성 완료: [ ]`로 큐에 들어 있습니다.

외부 생성 작업자의 계약:

1. 요청 상태가 체크되고 완료되지 않은 항목을 하나씩 처리합니다.
2. 항목에 적힌 정확한 경로로 2048×2048 PNG를 저장합니다.
3. `생성 완료` 체크박스를 `[x]`로 바꿉니다.

감시기 실행:

```powershell
# 파일 도착 상태만 한 번 동기화
.\Tools\Watch-ImageRequests.ps1 -Once

# 계속 감시하고, 새 파일이 생기면 UE 임포트와 재질 생성까지 수행
.\Tools\Watch-ImageRequests.ps1 -Import
```

임포트 스크립트는 PNG를 `/Game/Generated/Textures`에 넣고 `/Game/Generated/Materials` 아래에 타일링 재질을 생성합니다. 물 재질에는 자동 UV Panner가 연결됩니다. 월드 코드는 생성 재질을 자동 탐지해 색상 폴백보다 우선 사용합니다.

큐/임포트 관련 파일:

- `ExternalImageRequests.md`: 사람이 읽고 체크하는 생성 큐와 최종 프롬프트
- `Config/GeneratedAssetManifest.json`: 파일명, 재질명, 타일링의 기계 판독 계약
- `Tools/Watch-ImageRequests.ps1`: 생성 결과 감시 및 선택적 UE 임포트
- `Content/Python/sync_generated_assets.py`: UE 텍스처 임포트와 재질 그래프 생성

## 코드 구조

- `Source/SolCity/World`: 절차 도시, 강, 건물, 조명
- `Source/SolCity/Traffic`: 도로 그래프, 차량, 신호등
- `Source/SolCity/Citizens`: 고양이 모델과 보행 시뮬레이션
- `Source/SolCity/Camera`: 줌/팬/회전 카메라
- `Source/SolCity/Game`: 빈 맵에서도 전체 데모를 띄우는 부트스트랩
- `Source/SolCity/UI`: 인구와 조작 도움말 HUD

## 검증

개발 기준 엔진은 `C:\Program Files\Epic Games\UE_5.7`입니다. `SolCityEditor Win64 Development` 전체 빌드와 `/Engine/Maps/Entry` 헤드리스 게임 부팅을 기준으로 검증합니다.
