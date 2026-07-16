# Sol City 외부 이미지 생성 요청 큐

이 문서는 외부 이미지 생성 작업자와 UE 임포트 감시기가 함께 쓰는 큐입니다.

- 외부 생성기는 `생성 요청 상태: [x]`, `작업 할당됨: [ ]`, `생성 완료: [ ]`인 항목만 가져갑니다.
- 작업을 가져가면 이미지 생성을 시작하기 전에 즉시 `작업 할당됨`을 `[x]`로 바꿉니다. `작업 할당됨: [x]`인 항목은 다른 작업자가 가져가거나 처리하지 않습니다.
- 결과는 항목의 `저장 경로`에 **PNG, sRGB, 2048×2048**로 저장합니다.
- 파일 저장 후 `생성 완료`를 `[x]`로 바꿉니다.
- `Tools/Watch-ImageRequests.ps1`은 파일을 감시하고, UE 임포트가 끝나면 `UE 임포트 완료`를 `[x]`로 바꿉니다.
- 모든 타일 텍스처는 네 방향으로 완전히 seamless해야 하며, 글자·로고·워터마크가 없어야 합니다.
- 공통 아트 디렉션: **일본 애니메이션 배경화 같은 선명하고 깨끗한 파스텔 색감, 맑고 따뜻한 낮, 평화롭고 아늑한 소도시, 손으로 채색한 듯한 단순한 형태와 은은한 질감. 특정 작품이나 작가를 모사하지 않는 독창적 스타일.**

---

## REQ-001 잔디 바닥 타일

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_GroundGrass_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_GroundGrass_Anime`

```text
Use case: stylized-concept
Asset type: seamless tileable game texture for a UE city-builder ground plane
Primary request: short, tidy urban park grass with very subtle hand-painted blade clusters and rare tiny clover shapes
Style/medium: original peaceful Japanese-animation-background-inspired painted texture; vivid clean pastel color; simplified forms
Composition/framing: orthographic top-down surface, evenly distributed detail, no horizon, no focal point
Lighting/mood: neutral diffuse daylight baked as little as possible
Color palette: fresh spring green, soft mint and a restrained warm yellow-green
Constraints: perfectly seamless on all four edges; uniform density; albedo only; no objects; no flowers; no text; no logo; no watermark
Avoid: photorealism, strong shadows, dark patches, perspective, repeated obvious motifs
```

## REQ-002 도로 아스팔트 타일

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_RoadAsphalt_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_RoadAsphalt_Anime`

```text
Use case: stylized-concept
Asset type: seamless tileable game texture for UE roads
Primary request: clean fine-grain city asphalt without painted lane markings
Style/medium: original hand-painted anime-background-inspired game texture; crisp and simplified
Composition/framing: orthographic top-down, even micro texture, no focal point
Lighting/mood: neutral diffuse daylight
Color palette: soft blue-charcoal and slightly warm gray, bright enough for a peaceful city
Constraints: perfectly seamless on all four edges; albedo only; no cracks; no lane lines; no drains; no text; no watermark
Avoid: photorealism, grunge, oil stains, dramatic shadows, perspective
```

## REQ-003 보도 타일

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_Sidewalk_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_Sidewalk_Anime`

```text
Use case: stylized-concept
Asset type: seamless tileable game texture for pedestrian sidewalks
Primary request: neat square sidewalk pavers with fine joints and extremely light surface variation
Style/medium: original peaceful anime-background-inspired painted texture; clean readable shapes
Composition/framing: orthographic top-down, regular grid aligned to image edges
Lighting/mood: neutral diffuse daylight
Color palette: warm cream gray with pale peach and cool lavender-gray variation
Constraints: perfectly seamless; albedo only; evenly sized paving grid; no curb; no objects; no text; no logo; no watermark
Avoid: damage, dirt, weeds, strong shadows, perspective, photorealism
```

## REQ-004 강물 UV 애니메이션 타일

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_RiverWater_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_WaterAnimated`

```text
Use case: stylized-concept
Asset type: seamless tileable game texture for a horizontally panned UE river material
Primary request: calm clear river surface with sparse soft curved ripple streaks designed to look natural while UV-panning
Style/medium: original bright anime-background-inspired hand-painted water texture; clean graphic shapes
Composition/framing: orthographic top-down, directional flow from left to right, no shore, no objects
Lighting/mood: sunny peaceful daylight with restrained highlights
Color palette: turquoise, sky blue, pale cyan and a few soft white highlights
Constraints: perfectly seamless on all four edges; albedo only; low-frequency ripples; no fish; no plants; no foam clusters; no text; no watermark
Avoid: ocean waves, photorealism, caustic perspective, strong glare, dark water
```

## REQ-005 따뜻한 계열 건물 외벽

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_FacadeWarm_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_FacadeWarm_Anime`

```text
Use case: stylized-concept
Asset type: tileable game texture for low-poly box building facades
Primary request: cheerful low-rise urban facade module with simple repeated windows, subtle stucco, and narrow trim
Style/medium: original peaceful Japanese-animation-background-inspired painted facade texture; crisp simplified architecture
Composition/framing: straight orthographic front elevation; repetition must tile horizontally and vertically
Lighting/mood: neutral warm daylight with minimal baked shadow
Color palette: peach, apricot, warm cream, coral accents, pale teal glass
Constraints: perfectly seamless; front-facing flat texture; no doors; no signs; no readable text; no logos; no people; no watermark
Avoid: perspective, deep shadows, photorealism, broken windows, grime
```

## REQ-006 시원한 계열 건물 외벽

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_FacadeCool_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_FacadeCool_Anime`

```text
Use case: stylized-concept
Asset type: tileable game texture for low-poly box building facades
Primary request: friendly low-rise urban facade module with simple repeated windows, painted panels, and narrow trim
Style/medium: original bright anime-background-inspired painted facade texture; clean simplified architecture
Composition/framing: straight orthographic front elevation; repetition must tile horizontally and vertically
Lighting/mood: neutral clear daylight with minimal baked shadow
Color palette: powder blue, mint, pale lavender, warm white, soft amber window accents
Constraints: perfectly seamless; no doors; no signs; no readable text; no logos; no people; no watermark
Avoid: perspective, deep shadows, photorealism, grime, cyberpunk neon
```

## REQ-007 지붕 타일

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_RoofTile_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_RoofTile_Anime`

```text
Use case: stylized-concept
Asset type: seamless tileable game texture for flat and gently sloped low-poly roofs
Primary request: neat small roof shingles with subtle painted variation
Style/medium: original anime-background-inspired hand-painted texture; simple and cheerful
Composition/framing: orthographic top-down, regular rows aligned for seamless tiling
Lighting/mood: neutral diffuse daylight
Color palette: muted coral red, warm terracotta and pale rose highlights
Constraints: perfectly seamless; albedo only; no moss; no gutters; no objects; no text; no watermark
Avoid: photorealism, heavy weathering, dramatic shadow, perspective
```

## REQ-008 공원 산책로 타일

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_ParkPath_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_ParkPath_Anime`

```text
Use case: stylized-concept
Asset type: seamless tileable game texture for park walking paths
Primary request: clean compacted fine gravel with tiny soft pebbles and gentle painted variation
Style/medium: original peaceful anime-background-inspired game texture; bright and simplified
Composition/framing: orthographic top-down, even detail, no focal point
Lighting/mood: neutral warm daylight
Color palette: pale sand, warm cream, light ochre and a hint of peach
Constraints: perfectly seamless; albedo only; no footprints; no leaves; no objects; no text; no watermark
Avoid: mud, large stones, photorealism, high contrast, perspective
```

## REQ-009 고양이 얼굴 아틀라스

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_CatFaceAtlas_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_CatFaceAtlas_Anime`

```text
Use case: stylized-concept
Asset type: game texture atlas for simple low-poly cat citizen faces
Primary request: a clean 4 by 4 atlas of sixteen front-facing cute cat facial markings, each centered in an equal square cell
Style/medium: original cheerful anime-inspired flat painted game texture; simple dot eyes, tiny nose and minimal whisker marks
Composition/framing: exact 4x4 grid, identical face scale and alignment, flat orthographic front view
Color palette: cream, ginger, gray, charcoal, calico and soft pastel accent colors
Constraints: square cells; no grid lines; only facial features and fur markings; no body; no background scene; no text; no logo; no watermark
Avoid: realistic fur, complex shading, cropped faces, accessories, uneven cell sizes
```

## REQ-010 상점 아이콘 아틀라스

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_ShopIconAtlas_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_ShopIconAtlas_Anime`

```text
Use case: stylized-concept
Asset type: game texture atlas for small storefront signs
Primary request: a clean 4 by 4 atlas of sixteen friendly pictogram-only shop icons such as bread, tea cup, flower, book, fish, fruit, cake and bicycle
Style/medium: original peaceful anime-background-inspired flat painted icons; bold simple silhouettes
Composition/framing: exact 4x4 grid, one centered icon per equal square cell, generous padding
Color palette: vivid harmonious pastels on warm off-white cell backgrounds
Constraints: no grid lines; no letters; no words; no numbers; no logos; no trademarks; no watermark
Avoid: photorealism, tiny details, gradients, uneven cell sizes, repeated icons
```

## REQ-011 잔디 바닥 타일 무봉제 수정본

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_GroundGrass_Anime_v2.png`
- UE 재질: `/Game/Generated/Materials/M_GroundGrass_Anime_v2`

```text
Use case: stylized-concept
Asset type: corrected seamless tileable game texture for a UE city-builder ground and park plane
Primary request: short, tidy urban grass with extremely subtle hand-painted blade clusters and rare tiny clover shapes; repair the previous result by removing every horizontal band, mirrored strip, density transition and edge seam
Style/medium: original peaceful Japanese-animation-background-inspired painted texture; vivid clean pastel color; simplified forms
Composition/framing: orthographic top-down surface with perfectly uniform density and visual frequency across the entire canvas; no horizon and no focal point
Lighting/mood: neutral diffuse daylight with no directional baked lighting
Color palette: fresh spring green, soft mint and restrained warm yellow-green
Constraints: demonstrably seamless on all four edges; the texture must also remain visually continuous when repeated in a 3x3 grid; uniform density; albedo only; no objects; no flowers; no text; no logo; no watermark
Avoid: any horizontal or vertical band, mirrored seam, center line, border frame, photorealism, strong shadows, dark patches, perspective, obvious repeated motifs
```

## REQ-012 수목 잎사귀 타일

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_TreeFoliage_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_TreeFoliage_Anime`

```text
Use case: stylized-concept
Asset type: seamless tileable albedo texture for simple cylindrical and spherical low-poly tree crowns
Primary request: dense clusters of small rounded leaves painted as clean overlapping shapes, readable from an elevated city-builder camera without distinct branches
Style/medium: original peaceful Japanese-animation-background-inspired painted foliage; crisp, cheerful and simplified
Composition/framing: evenly distributed top-down foliage surface, no single leaf cluster as a focal point, no sky or gaps to a background
Lighting/mood: neutral soft daylight with only very subtle painted value variation
Color palette: emerald green, fresh mint, spring green and sparse pale yellow-green highlights
Constraints: perfectly seamless on all four edges; albedo only; uniform coverage; no trunk; no flowers; no fruit; no text; no logo; no watermark
Avoid: photorealism, black shadows, individual realistic branches, autumn colors, perspective, obvious tiling motifs
```

## REQ-013 교량 상판 타일

- 생성 요청 상태: [x]
- 작업 할당됨: [x]
- 생성 완료: [x]
- UE 임포트 완료: [x]
- 저장 경로: `Content/Generated/SourceTextures/T_BridgeDeck_Anime.png`
- UE 재질: `/Game/Generated/Materials/M_BridgeDeck_Anime`

```text
Use case: stylized-concept
Asset type: seamless tileable game texture for a small-city road bridge deck and pedestrian edge strips
Primary request: clean warm painted concrete with very fine aggregate and sparse soft brush variation, designed to complement blue-gray asphalt and cream sidewalks
Style/medium: original peaceful Japanese-animation-background-inspired hand-painted surface; bright, tidy and simplified
Composition/framing: orthographic top-down, even low-frequency texture with no focal point
Lighting/mood: neutral clear daylight with no baked directional shadow
Color palette: warm light stone, cream, pale peach-gray and restrained cool-gray flecks
Constraints: perfectly seamless on all four edges; albedo only; no lane markings; no expansion-joint line; no cracks; no stains; no text; no logo; no watermark
Avoid: photorealism, grime, dramatic shadow, perspective, cobblestones, repeated obvious motifs
```
