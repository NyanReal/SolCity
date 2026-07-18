# Blender-to-Unreal Visual Parity Implementation Plan

## 1. Objective

Improve the appearance of Blender-authored SolCity buildings in Unreal Engine while preserving the current stylized art direction, gameplay readability, and procedural city workflow.

The implementation should recover the material separation, edge highlights, emissive signage, and contact depth visible in the Blender previews without attempting to make the gameplay scene identical to a dark studio render.

## 2. Current Findings

The geometry import is broadly correct. The main visual gap comes from the rendering pipeline around the geometry:

1. Blender Principled BSDF materials are imported as instances of Unreal's `FBXLegacyPhongSurfaceMaterial`.
2. The FBX material path represents diffuse, specular, shininess, and emissive values rather than Blender's metallic/roughness PBR model.
3. The Blender preview uses a dark world, a controlled three-light setup, AgX color management, and a close 53 mm camera.
4. The Unreal gameplay scene uses strong daylight, a bright movable Sky Light, atmospheric fill, fog, and a distant top-down camera.
5. Building materials currently use flat colors with little or no normal, roughness, or macro-surface variation.
6. Thin bevels, frames, and trim become sub-pixel details at the default city-camera distance and are softened by temporal anti-aliasing.

## 3. Success Criteria

The work is complete when all of the following are true:

- Brass, dark metal, glass, masonry, awnings, and signage remain visually distinct in direct light and shade.
- Metallic and roughness values are controlled by native Unreal materials rather than the legacy FBX material conversion.
- `SOL CITY` and `SOL MARKET` signage has readable emissive output without clipping into a solid white shape.
- Window and trim recesses retain visible contact depth at the normal gameplay camera distance.
- The gameplay scene remains bright and readable but no longer washes out building material differences.
- Reimporting an FBX does not destroy or replace the authored Unreal material setup.
- The process is repeatable for all current and future Blender MCP building assets.
- Before/after captures exist for both a controlled look-development view and the normal gameplay view.

## 4. Scope

### In scope

- A native Unreal master material for stylized buildings.
- Material instances for the existing authored building material slots.
- Deterministic material-slot assignment during FBX import or reimport.
- A controlled Unreal look-development scene or capture setup.
- Gameplay lighting, exposure, ambient occlusion, contact-shadow, bloom, and color-grading adjustments.
- Optional low-cost normal and roughness variation where flat colors remain visibly artificial.
- Documentation and verification captures.

### Out of scope

- Remodeling the existing buildings unless a geometry defect is discovered during validation.
- Reproducing the Blender studio background in normal gameplay.
- Replacing the overall daytime SolCity art direction with a night scene.
- Introducing expensive transparent architectural glass by default.
- Large changes to city generation, traffic simulation, or camera controls unrelated to visual parity.

## 5. Implementation Phases

### Phase 0: Establish a reproducible baseline

- [ ] Capture the current `SM_SolCity_CornerRetail_01` in the normal gameplay camera.
- [ ] Capture the same asset in a controlled Unreal look-development setup.
- [ ] Match the Blender preview camera as closely as practical: 53 mm focal length, three-quarter view, similar framing, and similar target height.
- [ ] Record the active Unreal renderer, GI, reflection, anti-aliasing, exposure, Sky Light, Directional Light, fog, and post-process settings.
- [ ] Check the persistent level and runtime for duplicate atmosphere, cloud, Directional Light, Sky Light, fog, or post-process actors.
- [ ] Save the baseline images under a stable review location, using `_before` suffixes.

Acceptance gate: the team can compare the same asset under a controlled view and the gameplay view without relying on memory or mismatched framing.

### Phase 1: Replace legacy FBX materials with a native Unreal PBR pipeline

#### 1.1 Create the master material

Create `M_SolCity_Building_Master` with the following parameters:

- `BaseColor`
- `Metallic`
- `Roughness`
- `Specular`
- `EmissiveColor`
- `EmissiveStrength`
- `NormalTexture` and `NormalStrength`
- `RoughnessTexture` and `RoughnessTextureStrength`
- `MacroVariationTexture`, `MacroVariationScale`, and `MacroVariationStrength`
- Static switches for normal, roughness, macro variation, and emissive features

The default blend mode should remain `Opaque`. Stylized blue windows should initially remain opaque and reflective; translucent glass should be evaluated separately because it has different sorting, reflection, and performance costs.

#### 1.2 Create deterministic material instances

Create native Unreal material instances for every authored slot. Initial values for the corner retail building should be seeded from the Blender source:

| Material | Metallic | Roughness | Emissive strength | Notes |
|---|---:|---:|---:|---|
| `M_Retail_Cream` | 0.00 | 0.70 | 0.00 | Warm masonry/plaster |
| `M_Retail_Brick` | 0.00 | 0.78 | 0.00 | Matte terracotta wall |
| `M_Retail_Dark` | 0.55 | 0.27 | 0.00 | Dark metal and roof details |
| `M_Retail_Brass` | 0.82 | 0.25 | 0.00 | Reflective gold/brass trim |
| `M_Retail_Glass` | 0.14 | 0.10 | 0.00 | Stylized opaque blue glass |
| `M_Retail_GlassGlow` | 0.00 | 0.16 | 0.55 | Interior-lit window variant |
| `M_Retail_Awning` | 0.00 | 0.38 | 0.00 | Terracotta awning |
| `M_Retail_Green` | 0.00 | 0.80 | 0.00 | Planter foliage |
| `M_Retail_Sign` | 0.25 | 0.20 | 2.80 | Sign color multiplied by emissive strength |

These values are starting points, not final cross-renderer matches. Unreal values should be visually calibrated in the look-development setup because Blender Eevee/AgX and Unreal do not produce numerically identical results.

#### 1.3 Make import and reimport safe

- [ ] Stop treating automatically generated FBX materials as final production materials.
- [ ] Add an idempotent Unreal Python setup script for creating/updating the master material and instances.
- [ ] Maintain a data-driven mapping from mesh asset and material-slot name to Unreal material instance.
- [ ] Assign materials to static-mesh slots after import so the result does not depend on FBX material conversion.
- [ ] Change the building import process so FBX reimport preserves the native material assets and reapplies slot mappings.
- [ ] Do not delete unrelated or manually authored material assets.
- [ ] Log missing, renamed, and duplicate material slots as errors or explicit warnings.

Suggested implementation locations:

- `Content/Python/SetupSolCityBuildingMaterials.py`
- `Content/Python/SetupSolCityAssets.py`
- `Content/Art/Buildings/Materials/`

Acceptance gate: deleting and reimporting a test static mesh produces the same native Unreal material assignments without recreating legacy Phong materials as the production result.

### Phase 2: Build a controlled Unreal look-development setup

- [ ] Create a small look-development map or reusable editor capture setup.
- [ ] Use a neutral dark or mid-gray background that does not contribute excessive color.
- [ ] Use one Directional Light and controlled key/fill lighting to reveal bevels and material response.
- [ ] Add a Reflection Capture where appropriate, even if Lumen reflections remain enabled, and verify that metal has a useful environment to reflect.
- [ ] Use a 53 mm camera and match the Blender three-quarter framing.
- [ ] Add an unbound Post Process Volume with explicit manual exposure and documented values.
- [ ] Capture `_lookdev_before` and `_lookdev_after` images at identical resolution and camera transform.

The look-development map is a diagnostic reference. Its lighting should not be copied blindly into the gameplay level.

Acceptance gate: the Unreal look-development capture shows clear brass, glass, matte wall, dark metal, and emissive separation before gameplay lighting is adjusted.

### Phase 3: Tune gameplay lighting and post processing

Apply changes in small, measurable passes:

1. Reduce excessive ambient fill before increasing contrast or darkening base colors.
2. Tune the Sky Light intensity, indirect-lighting intensity, lower-hemisphere color, and real-time capture behavior.
3. Verify the Directional Light angle and source angle so facade details produce useful shadows without harsh aliasing.
4. Add or tune contact shadows for thin awnings, frames, ledges, and rooftop elements.
5. Add moderate ambient occlusion for local depth while avoiding dirty halos.
6. Set explicit manual exposure in a Post Process Volume or camera instead of relying only on the project-wide auto-exposure switch.
7. Adjust color contrast and saturation conservatively after lighting is stable.
8. Tune bloom last so signs glow but retain letter shapes.
9. Recheck fog and volumetric scattering because they can reduce distant contrast.

The current runtime environment creation in `SolCityGameMode.cpp` should be made auditable and preferably data-driven. Environment actors should not be spawned unconditionally when an equivalent authored actor already exists in the level.

Acceptance gate: gameplay remains sunny and colorful, shaded facades stay readable, and material distinctions survive both direct sun and shadow.

### Phase 4: Add surface variation only where needed

After native PBR materials and lighting are working, evaluate whether flat materials still appear artificial at gameplay distance.

- [ ] Add subtle, tileable roughness variation to plaster, brick, dark roof metal, and awnings.
- [ ] Add restrained normal detail to masonry and painted surfaces.
- [ ] Add low-frequency macro color variation to large wall areas.
- [ ] Keep variation strength low enough to preserve the clean stylized aesthetic.
- [ ] Share textures across building materials to control memory and shader permutations.
- [ ] Validate mip behavior and temporal stability from the default camera distance.

Acceptance gate: large surfaces no longer look perfectly uniform, but the building still reads as a clean stylized asset rather than a photorealistic one.

### Phase 5: Improve gameplay-distance readability

- [ ] Compare the default top-down camera against a slightly lower review angle.
- [ ] Verify that important trim widths and bevels occupy enough pixels at the normal zoom level.
- [ ] Consider modestly widening only the most important silhouette and facade accents if material and lighting changes are insufficient.
- [ ] Avoid globally enlarging every small detail, which would make close views visually heavy.
- [ ] Confirm that temporal anti-aliasing does not erase thin emissive letters or metal trim during camera movement.

Geometry changes require a new Blender preview, FBX export, and clean Blender FBX reimport validation under the existing Blender MCP modeling workflow.

Acceptance gate: the building retains its design hierarchy at both close inspection and normal city-management distance.

## 6. Verification Matrix

| Test | Controlled look-development view | Gameplay view |
|---|---|---|
| Material slot assignment | All native instances assigned | All native instances assigned |
| Brass response | Metallic reflections and narrow highlights | Remains gold and distinct from painted trim |
| Glass response | Dark blue reflections remain visible | Does not become a flat cyan rectangle |
| Masonry | Matte and color-stable | Not washed to white in direct sun |
| Dark roof/frame | Retains highlight response | Does not crush to featureless black |
| Emissive signs | Letter shapes retained with halo | Readable without white clipping |
| Contact depth | Frames and awnings cast local shadows | Recesses remain visible at default zoom |
| Temporal stability | No shimmer while orbiting | No disappearing trim during movement |
| Performance | Shader complexity reviewed | Target frame time maintained |

## 7. Regression and Safety Checks

- Reimport each authored building into a clean test destination and verify material-slot names.
- Confirm that combined-mesh import still preserves all intended material sections.
- Confirm that material scripts are idempotent and safe to rerun.
- Confirm that instanced static-mesh and spline usage flags remain valid where required.
- Verify that runtime lighting is not duplicated after level reload, PIE restart, or packaged startup.
- Test direct sun, full shadow, fogged distance, and emissive/bloom conditions.
- Compare screenshots using identical camera transforms, exposure, resolution, and scalability settings.
- Record any renderer-specific differences that cannot be matched numerically.

## 8. Deliverables

- Native Unreal building master material.
- Material instances for all existing Blender-authored building slots.
- Idempotent Unreal Python material/setup script.
- Updated FBX import/reimport material assignment workflow.
- Controlled Unreal look-development setup.
- Tuned gameplay environment and post-process settings.
- Before/after captures for `SM_SolCity_CornerRetail_01` and at least one additional building.
- Updated Blender MCP asset log or art-direction documentation with the approved Unreal material conventions.

## 9. Recommended Execution Order

1. Baseline captures and duplicate-light audit.
2. Native Unreal master material.
3. Corner retail material instances and deterministic assignment.
4. Controlled look-development validation.
5. Extend the pipeline to the other authored buildings.
6. Gameplay Sky Light, exposure, contact-shadow, AO, fog, and bloom tuning.
7. Optional texture/normal/roughness variation.
8. Gameplay-distance readability review.
9. Final reimport, performance, and screenshot regression pass.

This order isolates material-translation problems from lighting problems and avoids compensating for incorrect materials by over-tuning the entire world.

## 10. Implementation Status (2026-07-18)

Implementation has started through the first three phases:

- Phase 0: `/Game/Maps/SolCityLookDev` and `Content/Python/BuildSolCityLookDev.py` provide a repeatable 53 mm, 1600 x 900, manual-exposure review setup. Matching legacy/native captures are written to `Saved/VisualParity` without changing the production mesh assignments.
- Phase 1: `M_SolCity_Building_Master` and 28 native material instances cover the exact authored slots on MidRise (10), CornerRetail (9), and SteppedTower (9). `SetupSolCityBuildingMaterials.py` validated 28/28 assignments with zero errors on two consecutive UE 5.7 runs.
- Reimport safety: FBX material creation is disabled for the three authored buildings. Both the dedicated material script and the main asset setup reapply native materials by slot name after an in-place import.
- Phase 2: the controlled scene uses a neutral studio, 53 mm cine camera, reflection capture, manual exposure, and fixed lighting so material changes can be compared independently of gameplay lighting.
- Phase 3: runtime environment actors are reused when already present and spawned only when missing. The shared setup now applies manual exposure, restrained bloom and color tuning, ambient occlusion, and Directional Light contact-shadow settings.

Still pending are the normal gameplay before/after capture pair, a second-building capture, the optional Phase 4 surface textures, and the Phase 5 gameplay-distance readability and performance review.
