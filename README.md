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
- 중심부는 조밀하고 외곽 2개 링은 건물 점유율·크기·높이가 점진적으로 감소하는 24,000uu 폭 도시
- 7종 외벽과 붉은 기와·청록 금속·옥상정원 3종 지붕을 섞은 박스 조합 저층 건물
- 옥상 구조물, 발광색 창문, 공원과 가로수
- 평면 강과 모든 횡단 도로의 교량
- `M_WaterAnimated`가 있으면 Panner 기반 UV 흐름, 없으면 청록색 수면 움직임 폴백
- 교차로별 남북/동서 2상 신호등
- 차선 주행, 교차로 랜덤 회전, 적신호 정지, 앞차 안전거리 유지 자동차
- 차체·보닛·트렁크·유리 객실·범퍼·4개 바퀴/휠캡·전후 등화로 구성한 일반 도심형 소형 승용차
- 도로와 강을 피하고 건물을 감지하며 걷는 고양이 시민
- 고양이의 다리 교차 움직임, 몸 바운스, 꼬리 흔들기
- 런타임 낮 조명, SkyLight, SkyAtmosphere, 옅은 원경 안개

주요 월드 설정은 `ACityWorldActor`의 `GridHalfExtent`, `RoadSpacing`, `RoadWidth`, `RiverX`, `RiverHalfWidth`, `VehicleCount`, `CitizenCount`에서 조절할 수 있습니다.

카메라는 마우스 휠로 `1,400~21,600uu` 범위를 거리 비례 방식으로 부드럽게 이동하며, 최대 줌아웃 범위는 초기 버전의 3배입니다.

## 외부 imagegen 큐

개별 이미지 생성 병목을 피하기 위해 모든 생성 요청은 `ExternalImageRequests.md`에 분리했습니다. 기본 환경·건물·물·아틀라스 10종과 시각 검수에서 추가된 잔디 무봉제 수정본·수목 잎사귀·교량 상판 3종까지, 현재 13종 모두 생성과 UE 임포트가 완료됐습니다. 이후 필요한 텍스처도 같은 체크박스 큐와 매니페스트에 계속 추가할 수 있습니다.

추가 건물 다양화 요청에서는 사용자의 지시에 따라 외부 큐를 사용하지 않고 built-in imagegen을 직접 실행했습니다. 카페·아파트·타운하우스 외벽 3종과 금속·옥상정원 지붕 2종은 `DIRECT-014~018`로 매니페스트에만 등록되며, 외부 요청 문서의 상태와 독립적으로 임포트됩니다.

외부 생성 작업자의 계약:

1. `생성 요청 상태: [x]`, `작업 할당됨: [ ]`, `생성 완료: [ ]`인 항목을 하나씩 가져갑니다.
2. 생성을 시작하기 전에 `작업 할당됨`을 `[x]`로 바꿔 중복 작업을 막습니다.
3. 항목에 적힌 정확한 경로로 2048×2048 PNG를 저장합니다.
4. `생성 완료` 체크박스를 `[x]`로 바꿉니다.

감시기 실행:

```powershell
# 파일 도착 상태만 한 번 동기화
.\Tools\Watch-ImageRequests.ps1 -Once

# 계속 감시하고, 새 파일이 생기면 UE 임포트와 재질 생성까지 수행
.\Tools\Watch-ImageRequests.ps1 -Import
```

임포트 스크립트는 PNG를 `/Game/Generated/Textures`에 넣고 `/Game/Generated/Materials` 아래에 타일링 재질을 생성합니다. 물 재질에는 자동 UV Panner가 연결됩니다. 월드 코드는 생성 재질을 자동 탐지해 색상 폴백보다 우선 사용하며, 새 결과가 아직 없으면 기존 재질로 계속 실행됩니다.

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
