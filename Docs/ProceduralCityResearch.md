# SolCity 절차적 도시·도로 생성 연구

## 결론

이 프로젝트에는 **계층형 목적지 그래프 + 비유클리드 비용 경로 + 텐서장 스트림라인**을 결합한 하이브리드 방식을 권장한다.

- 간선도로(arterial)는 인구 중심·도심·다리 후보를 연결하는 희소 그래프에서 먼저 만든다. 강, 곡률, 기존 도로 합류를 비용으로 반영해 도시의 큰 골격과 교량 위치를 결정한다.
- 집산도로(collector)와 생활도로(local)는 간선도로와 강변을 경계로 나뉜 구역 안에서 텐서장의 두 고유벡터 방향을 따라 스트림라인으로 만든다.
- 텐서장은 일률적인 직교 격자가 아니라 직선형, 방사형, 강변 평행형, 완만한 회전 노이즈를 공간적으로 혼합한다.
- Space colonization은 도시 전체의 주 알고리즘으로 쓰지 않고, 교외 가지길·공원 산책로·막다른 골목 후보를 만드는 보조 도구로 제한한다.
- Voronoi는 도로 그 자체보다 **지구 영향권, 공원·상업 중심 배정, 필지 초기 분할**에 사용한다.

이 선택은 도로 위 차량 경로 탐색에 필요한 연결성과 순환로를 보장하면서도, 화면에서 보았을 때 단순 격자 복제처럼 보이지 않는 통제 가능한 도시를 만든다.

## 1. 근거가 된 1차 자료

1. Parish와 Müller의 SIGGRAPH 2001 논문은 확장 L-system으로 도로를 성장시키고, `globalGoals`와 `localConstraints`를 분리한다. 인구 밀도는 간선도로 방향을, 물·공원·고도·기존 도로 교차는 지역 제약을 결정한다. 도로가 만든 블록은 긴 변을 기준으로 재귀 분할하여 필지로 만든다.  
   [Procedural Modeling of Cities — 논문 PDF](https://people.eecs.berkeley.edu/~sequin/CS285/PAPERS/Parish_Muller01.pdf), [ACM DOI](https://doi.org/10.1145/383259.383292)
2. Chen 등은 대칭·무추적 2×2 텐서장의 major/minor 고유벡터를 따라 hyperstreamline을 추적하고, 두 계열의 교차로 도로 그래프와 블록을 만든다. 직선·방사형 basis field, 브러시, 경계, 회전장, 노이즈, 밀도 지도를 혼합할 수 있고 간선/생활 도로장을 계층적으로 분리할 수 있다.  
   [Interactive Procedural Street Modeling — 저자 공개 PDF](https://peterwonka.net/Publications/pdfs/2008.SG.Chen.InteractiveProceduralStreetModeling.pdf), [ACM DOI](https://doi.org/10.1145/1399504.1360702)
3. Galin 등은 경사, 물, 숲, 곡률, 교량·터널 비용을 포함하는 가중 비등방성 최단 경로로 두 지점을 연결하고, 이산 경로를 piecewise clothoid로 평활화한다. 본 프로젝트는 평탄 지형이므로 이 모델을 강 회피/횡단, 완만한 선형, 기존 도로 합류 비용에 축소 적용한다.  
   [Procedural Generation of Roads — 저자 공개 PDF](https://perso.liris.cnrs.fr/eric.galin/Articles/2010-roads.pdf), [Wiley DOI](https://doi.org/10.1111/j.1467-8659.2009.01612.x)
4. Galin 등의 후속 연구는 도시·마을을 연결하는 비유클리드 기하 그래프를 계층별로 생성하고, 새 경로를 기존 경로에 합류시켜 고속·1차·2차 도로와 교차로를 함께 만든다. 도로 밀도와 패턴을 고수준에서 제어할 수 있다.  
   [Authoring Hierarchical Road Networks — Purdue 저자 PDF](https://www.cs.purdue.edu/cgvlab/www/resources/papers/Galin-Computer_Graphics_Forum-2011-Authoring_Hierarchical_Road_Networks.pdf), [Wiley DOI](https://doi.org/10.1111/j.1467-8659.2011.02055.x)
5. Fernandes와 Fernandes는 도로 생성에 space colonization의 attraction point를 적용한다. 점 분포가 직관적인 제어 수단이며 flow field, 인구, 지리, 경계 지도를 함께 사용할 수 있다.  
   [Space Colonisation for Procedural Road Generation — IEEE DOI](https://doi.org/10.1109/ITCGI.2018.8602928)
6. Unreal 공식 문서는 PCG가 편집기와 런타임 생성에 쓰이는 확장 가능한 프레임워크임을 설명하며, 5.7 API에는 spline 생성과 spline mesh spawn 설정이 제공된다. 본 설계에서는 핵심 도로 그래프는 결정론적 C++로 만들고 PCG는 건물·가로수·소품 배치에 사용한다.  
   [UE 5.7 PCG Framework](https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-framework-in-unreal-engine?lang=en-US), [UE 5.7 PCG API](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/PCG), [PCG Editor Mode](https://dev.epicgames.com/documentation/unreal-engine/pcg-editor-mode-in-unreal-engine?lang=en-US)

## 2. 후보 알고리즘 비교

| 방식 | 장점 | 약점 | SolCity에서의 판단 |
|---|---|---|---|
| 확장 L-system | 성장 순서와 도로 계층을 규칙으로 명시하기 쉽다. 전역 목표와 지역 제약을 분리하면 물·인구·교차 처리가 가능하다. | 기능이 늘면 상태와 규칙 추적이 어려워지고, 결과를 국소 편집하거나 원하는 큰 형태로 유도하기 어렵다. 트리 성장 편향을 별도 교차 규칙으로 상쇄해야 한다. | 핵심 구현으로 채택하지 않는다. `제안 → 지역 제약으로 수정/거절 → 그래프 삽입` 파이프라인 개념만 가져온다. |
| 텐서장 + 스트림라인 | 직교성이 있는 도시 블록을 만들면서도 방사형, 강변형, 휘어진 패턴을 연속적으로 혼합한다. 아트 디렉션과 국소 편집성이 가장 좋다. | 특이점, 짧은 도로, 지나친 직교성, 근접 평행선이 생길 수 있다. 그래프 평면화와 정리 단계가 필수다. | 생활도로 생성의 주 방식. 방향 회전 노이즈와 구역별 basis를 섞어 격자 반복을 방지한다. |
| Space colonization | attraction point가 곧 수요이므로 직관적이다. 유기적인 가지와 밀도 변화, 장애물 회피를 쉽게 만든다. | 기본 결과가 수목처럼 가지치기된 형태라 순환로와 사거리 비율이 부족하다. 차량 네트워크에는 후처리로 loop closure가 필요하다. | 공원길·교외 cul-de-sac·보행길의 보조 생성기로 사용한다. |
| Voronoi 단독 도로 | 구현이 단순하고 셀을 모두 둘러 연결성이 높다. 불규칙 블록을 즉시 얻는다. | 균일 난수 seed면 벌집처럼 보여 도시 도로의 계층·방향성·역사적 성장감이 약하다. 도로 폭과 기능 계층도 자연스럽게 나오지 않는다. | 도로 생성기로는 사용하지 않는다. 가중 seed의 영향권/지구 배정과 필지 보조선에 사용한다. |
| 간선-생활 계층 + 비용 경로 | 도시의 큰 연결, 교량, 도로 폭, 교통 중요도가 처음부터 그래프 속성으로 남는다. 기존 경로 합류로 자연스러운 Y/T 접속을 만든다. | 이것만으로 블록 내부를 채우면 희소하고 교외 도로처럼 보인다. 목적지와 비용 가중치 설계가 필요하다. | 간선도로의 주 방식으로 채택하고 텐서장 생활도로와 결합한다. |

## 3. 권장 생성 파이프라인

### 3.1 입력 필드

월드 XY를 고정 해상도의 2D 필드로 표현한다. 처음 구현은 128×128 또는 256×256이면 충분하다.

- `WaterMask`: 강 내부 1, 육지 0. 강 표면은 지면보다 낮지만 도로 계획은 2D 마스크로 처리한다.
- `Population`: 도심·역·상업 중심의 Gaussian 합. 중심 하나가 아니라 3~6개의 크기 다른 봉우리를 둔다.
- `ParkMask`: 큰 공원과 수변 녹지. 일반 차량 도로는 높은 비용, 보행로는 낮은 비용으로 해석한다.
- `DevelopmentSuitability`: 물가 이격, 공원 접근, 간선도로 접근을 반영한 건설 적합도.
- `OrientationField`: 텐서 `T=(a,b)`로 저장한다. 이는 행렬 `[a b; b -a]`, 즉 `a=R cos(2θ)`, `b=R sin(2θ)`에 해당한다. 각도를 직접 평균하지 말고 `(a,b)`를 가중합해야 180도 방향 동치와 보간 불연속을 피할 수 있다.

모든 난수는 `FRandomStream(CitySeed)`와 단계별 파생 seed를 사용한다. 에디터 재생성, 저장/불러오기, 자동 테스트에서 같은 seed가 같은 그래프를 만들어야 한다.

### 3.2 도시 중심과 지구

1. 강 위를 제외한 고밀도 후보를 weighted Poisson disk sampling으로 8~20개 뽑는다.
2. 가장 큰 1~2개를 도심, 나머지를 주거·상업·공원 중심으로 태깅한다.
3. 중심점의 가중 Voronoi 영역은 도로가 아니라 `DistrictId`, 건물 높이, 지붕색, 주민/차량 밀도 결정에 쓴다.
4. 인접 후보 그래프는 완전 그래프를 쓰지 않는다. 작은 중심 수에서는 Delaunay를 구현하거나, 모든 쌍을 검사해 relative-neighborhood/Gabriel 조건으로 희소화해도 충분하다.

### 3.3 간선도로

중심 후보 그래프의 각 간선에 다음 비용을 계산한다.

```text
edge_cost = length
          + water_crossing_penalty
          + curvature_penalty
          + park_penalty
          + low_population_penalty
          - existing_road_merge_bonus
          - bridge_candidate_bonus
```

1. 최소 신장 트리로 모든 중요 중심의 기본 연결을 보장한다.
2. 우회율 `graphDistance/euclideanDistance`가 큰 중심 쌍에 15~30%의 추가 간선을 넣어 순환 간선망을 만든다.
3. 각 간선은 2D 비용 격자에서 방향 상태를 포함한 A*로 구체 경로를 찾는다. 상태는 `(Cell, PrevDirectionBin)`으로 두어 급회전을 벌점화한다.
4. 기존 간선도로 가까이에서는 합류 보너스를 주되, 평행 중복은 금지한다.
5. 강 횡단 후보는 강 양안의 법선 방향과 가까운 짧은 구간에 한정한다. 일반 셀의 물 비용은 무한대에 가깝게 두고, 후보 통로에서만 유한한 교량 비용을 허용한다.
6. 결과 polyline은 Ramer–Douglas–Peucker로 미세 떨림을 제거한 뒤 Catmull–Rom 또는 UE spline tangent로 완만하게 만든다. 급격한 overshoot로 강둑이나 블록을 침범하지 않도록 평활 후 다시 제약을 검사한다.

평탄한 도시에 Galin 2010의 터널·경사 모델 전체를 구현할 필요는 없다. 다만 “표면 도로와 교량을 같은 비용 탐색에서 비교한다”는 구조는 그대로 유효하다. 교량은 짧고 강에 가깝게 직교하며 양쪽 진입부가 충분히 곧은 후보를 선호한다.

### 3.4 구역별 텐서장

간선도로와 강 경계가 나눈 각 구역에 다음 basis를 섞는다.

- `Regular`: 지구 주방향. 간선도로의 평균 tangent 또는 아트 디렉터의 방향을 따른다.
- `Radial`: 광장·역·랜드마크 주변. 중심을 향한 방향과 접선 방향을 만든다.
- `Boundary`: 강변과 공원 외곽에서는 경계 tangent를 우선해 도로가 물과 나란히 흐르게 한다.
- `RotationNoise`: 저주파 2D noise를 ±8~18도로 변환한다. minor 방향에는 약간 다른 회전을 주어 모든 교차가 정확한 90도가 되는 것을 피한다.

가중치는 거리 감쇠와 지구 mask로 결정한다. 서로 다른 지구 사이에 억지로 평균하지 않고, 간선도로를 비대칭 경계로 삼아 도로 끝점을 반대편 지구의 seed로 전달하는 방식이 안정적이다.

### 3.5 생활도로 스트림라인 추적

각 지구에서 major/minor 고유벡터 계열을 따로 추적한다.

```text
Trace(seed, family):
    p = seed
    dir = EigenDirection(T(p), family)
    while inside_district and length < MaxLength:
        dir = ChooseSignClosestToPrevious(dir)
        pNext = RK4(p, dir, StepSize)
        if crosses_water_or_park: stop
        if intersects_road: split-and-snap; stop
        if endpoint_near_node: snap; stop
        if too_close_parallel_to_road: stop
        append pNext
```

실용 파라미터 시작값(1 UU = 1 cm):

| 항목 | 간선도로 | 집산도로 | 생활도로 |
|---|---:|---:|---:|
| 평균 블록/seed 간격 | 18,000~28,000 cm | 9,000~15,000 cm | 5,000~9,000 cm |
| spline 표본 step | 800~1,500 cm | 500~1,000 cm | 400~800 cm |
| 차도 폭 | 1,200~1,800 cm | 800~1,200 cm | 600~900 cm |
| 최소 교차각 | 35° | 30° | 25° |
| node snap 반경 | 1,500 cm | 1,000 cm | 700 cm |

seed는 규칙 격자에 놓지 않는다. 높은 `Population`에서 가중 Poisson sampling하고, 기존 도로 양옆에 교대로 발아시킨다. 스트림라인 간 최소 분리 거리 `dsep`는 인구 밀도가 높을수록 작게 하며, 한 계열을 모두 만든 뒤 교차점과 빈 영역을 seed로 다른 계열을 만든다.

### 3.6 그래프 정리와 품질 보증

생성 직후의 선 모음은 차량 경로 그래프로 바로 쓰면 안 된다. 다음 순서로 정리한다.

1. 모든 선분 교차를 찾아 node를 만들고 edge를 분할한다. 공간 해시 또는 uniform grid로 후보 수를 줄인다.
2. 너무 가까운 node를 병합한다. 교량과 지상도로는 Z layer가 다르므로 교차해도 연결하지 않는다.
3. 길이가 임계치보다 짧고 막다른 edge인 경우 삭제한다. 단, 의도적 cul-de-sac와 건물 진입로는 태그로 보존한다.
4. degree-2 node 중 거의 직선인 것은 합쳐 spline component 수를 줄인다.
5. 모든 주거 블록이 도로에 접하는지 확인한다.
6. 간선 그래프가 하나의 연결 성분인지 확인하고, 생활도로 성분은 가장 가까운 간선/집산도로에 보정 연결한다.
7. 차량용 edge에 `RoadClass`, `LaneCount`, `SpeedLimit`, `OneWay`, `BridgeLayer`, `Length`, `SplineDistanceRange`를 기록한다.

지루한 격자 방지를 위한 정량 검사는 다음과 같다.

- 전체 교차로의 4-way 비율이 75%를 넘으면 rotation noise 또는 방사형 지구 가중치를 높인다.
- 방향 히스토그램이 정확히 두 bin에 85% 이상 몰리면 지구 주방향을 더 다양하게 한다.
- 블록 면적 변동계수(CV)가 너무 낮으면 seed 간격과 지구별 `dsep`를 변화시킨다.
- 순환도 `E - V + C`가 너무 낮으면 우회율이 큰 node 쌍에 loop closure edge를 추가한다.
- 다리가 없거나 과도하면 강 횡단 비용과 교량 후보 간 최소 거리를 조절한다.

### 3.7 블록, 인도, 필지, 건물

1. 정리된 평면 그래프에 half-edge를 만들고 각 node의 outgoing edge를 각도순으로 정렬한다.
2. directed edge의 “가장 왼쪽 다음 edge”를 따라 face를 순회한다. 가장 큰 외부 face, 강 내부 face, 지나치게 작은 face는 버린다.
3. face polygon을 차도 폭 + 인도 폭만큼 inset하여 건축 가능한 블록을 만든다.
4. 블록은 도로에 면한 긴 변을 우선하는 재귀 split로 필지화한다. 매 split 위치에 ±10~20% jitter를 주고, 최소 frontage/depth를 지킨다.
5. 삼각형·오목 코너·수변의 불량 필지는 억지로 건물을 채우지 말고 소공원, 자전거 주차, 카페 테라스, 고양이 놀이터로 바꾼다.
6. 건물 footprint는 필지 경계를 그대로 복제하지 말고 전면 setback, 옆마당, 코너 chamfer를 둔다. 높이는 지구 밀도와 간선 접근성에 따라 연속적으로 변화시키고 인접 건물과 약간 상관된 난수를 쓴다.

이 단계가 “도로만 휘었지만 건물은 같은 간격의 박스”가 되는 문제를 막는다. 건물 외형은 footprint extrusion, 1~3단 setback, 지붕 유형, imagegen 파사드 texture atlas 조합만으로도 선명한 일본 애니메이션 배경풍을 충분히 낼 수 있다.

## 4. UE 5.7 C++ 적용 구조

핵심 생성기는 UObject와 Actor 생명주기에 묶지 않은 순수 데이터/알고리즘 모듈로 두는 편이 테스트와 비동기 생성에 유리하다.

```cpp
struct FCityRoadNode
{
    int32 Id;
    FVector2D Position;
    TArray<int32> EdgeIds;
};

struct FCityRoadEdge
{
    int32 Id;
    int32 From;
    int32 To;
    ERoadClass RoadClass;
    TArray<FVector2D> Polyline;
    bool bBridge;
    int32 Layer;
};

struct FCityBlock
{
    int32 Id;
    int32 DistrictId;
    TArray<FVector2D> Boundary;
};
```

권장 클래스 책임:

- `FCityFieldGrid`: 물/인구/공원/텐서 샘플과 bilinear lookup.
- `FCityCenterGenerator`: weighted Poisson center와 지구 속성 생성.
- `FArterialGraphGenerator`: 후보 희소화, MST, loop 추가, 방향 A*.
- `FTensorStreetGenerator`: basis 합성, RK4 streamline 추적.
- `FRoadGraphPlanarizer`: 교차 분할, snap, merge, 연결성 검증.
- `FCityBlockExtractor`: half-edge face 추출, inset, lot split.
- `ACityPresentationActor`: 결과 데이터를 `USplineComponent`/`USplineMeshComponent`, HISM, PCG 입력으로 표현.
- `UCityRoadGraphSubsystem`: 차량·신호·보행 시스템에 불변 graph snapshot 제공.

생성은 작업 스레드에서 `FVector2D`, 배열, 해시만 다루고, UObject/Component 생성은 game thread에서 한다. 도로 spline 하나마다 Actor를 만들지 말고 지구 또는 chunk 단위 presentation actor 아래에 component를 묶는다. 반복되는 건물·나무·표지판은 ISM/HISM 또는 PCG Static Mesh Spawner로 배치한다.

PCG에는 다음 속성을 넘기면 충분하다.

- 블록/필지 point: `DistrictId`, `LotArea`, `Frontage`, `BuildingHeight`, `RoofType`, `PaletteId`.
- 도로 sample point: `RoadClass`, `Side`, `DistanceAlongRoad`, `IntersectionDistance`.
- 수변 point: `RiverDistance`, `BankSide`, `BridgeDistance`.

PCG는 건물 variant, 가로수, 벤치, 가로등, 화단을 배치한다. 도로의 논리 그래프와 신호등 연결은 C++가 소유해야 재생성 순서나 PCG graph 변경이 교통 시뮬레이션을 깨뜨리지 않는다.

## 5. 차량·보행·신호등에 필요한 생성 결과

- 각 교차로에는 접근 lane과 출구 lane을 연결하는 turn arc를 만든다. 도로 centerline만으로 차량을 움직이면 교차로에서 순간 회전한다.
- 3-way/4-way node에는 conflict set을 계산하고, 4-way 간선 교차로부터 우선 신호등을 설치한다. 작은 생활도로 T 접속은 양보 표지로 남겨 도시가 신호등으로 과밀해지는 것을 피한다.
- 보행 그래프는 각 도로 edge 양쪽의 offset sidewalk와 횡단보도로 만든다. `bBridge` edge에는 보행 가능한 교량 sidewalk를 별도 생성한다.
- 주민은 보행 그래프와 블록 내부 plaza/park waypoint만 사용하게 하며, 차도 polygon은 Nav Modifier로 높은 비용 또는 금지 영역으로 둔다.
- 건물 entrance는 가장 가까운 sidewalk edge에 연결한다. 필지가 도로에 접하는지 검사하는 이유도 이 연결을 보장하기 위해서다.

## 6. 구현 순서와 최소 완성 기준

### 1단계: 데이터와 디버그 뷰

- 입력 field, 중심점, 도로 graph, 고정 seed 재현성.
- `DrawDebugLine` 또는 단순 spline로 간선/생활/교량을 색상 구분.
- 최소 완성: 강을 가로지르는 통제된 다리 2~4개, 연결된 간선망, 서로 다른 방향을 가진 지구 3개 이상.

### 2단계: 텐서 생활도로와 블록

- major/minor streamline, 교차 분할, snap/merge, face 추출.
- 최소 완성: 물 위 일반 도로 0개, 면적이 유효한 블록 90% 이상, 4-way 교차 비율 75% 이하.

### 3단계: 필지와 건물

- inset sidewalk, lot split, footprint extrusion, 지구별 높이/색 변화.
- 최소 완성: 모든 건물 entrance가 보행 그래프에 연결되고, 불량 필지는 공원/광장으로 치환.

### 4단계: 교통

- lane graph, turn arc, 신호 phase, 차량 A*, 보행 graph.
- 최소 완성: 임의 spawn/destination 100쌍이 경로를 찾고, 차량이 물·인도에 진입하지 않으며, 신호 교차로에서 상충 movement가 동시에 진행되지 않음.

## 7. 채택하지 말아야 할 단순화

- 전체 맵을 같은 셀 크기의 격자로 채운 뒤 몇 칸을 비우는 방식.
- 균일 Voronoi edge 전체를 동일 폭 도로로 바꾸는 방식.
- 건물 중심점을 규칙 격자에 뿌리고 회전만 무작위화하는 방식.
- 도로 spline의 시각 geometry만 만들고 별도 위상 graph를 만들지 않는 방식.
- 강 위 모든 도로 교차를 자동 다리로 승격하는 방식.
- PCG graph를 교통 시뮬레이션의 원본 데이터로 사용하는 방식.

이 설계의 핵심은 “불규칙하게 보이기”가 아니라 **서로 다른 공간 규칙을 가진 지구들이 계층적인 도로로 연결되었다는 구조**다. 간선도로는 목적을 갖고 이어지고, 생활도로는 지구의 방향과 경계를 따르며, 필지와 건물은 실제 도로 접면에서 파생되어야 결과가 자연스럽다.
