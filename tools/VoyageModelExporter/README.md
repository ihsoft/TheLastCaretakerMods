# Voyage model / Blueprint assembly -> GLB

`../Export-VoyageModelGlb.ps1` exports one exact stock Voyage `StaticMesh` package
or one Blueprint's default SCS static-mesh assembly to a self-contained GLB. It
preserves mesh sections, vertex normals/tangents, UV channels, meaningful vertex colors,
component hierarchy and default relative transforms. Material inheritance,
selected textures and bounded glTF PBR bindings use the same policy as
[`../VoyageMaterialLibrary/README.md`](../VoyageMaterialLibrary/README.md).
An all-zero Unreal vertex-color stream is exported as neutral white because
glTF multiplies `COLOR_0` into base color; preserving that placeholder as black
would incorrectly suppress otherwise valid textures.

This is a read-only extractor. It does not change the game, install a container,
run Unreal, execute Blueprint construction scripts or write tracked game data.
All output stays below ignored `artifacts/`.

## Normal use (Windows PowerShell 5.1)

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Export-VoyageModelGlb.ps1 `
  -Asset '/Game/Blueprints/Modules/Utility/BP_Module_Fabricator' `
  -MaterialMode BakeReconstructed `
  -OutputPath 'artifacts/model-export/fabricator.glb'

powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Export-VoyageModelGlb.ps1 `
  -Asset '/Game/ModulesAnimations/Fabricator/Meshes/SM_Fabricator_Exterior_A_01' `
  -MaterialMode PbrApproximation `
  -OutputPath 'artifacts/model-export/fabricator-exterior.glb'
```

`-MaterialMode` is deliberately required. If the user's request does not choose
a representation, the caller must ask instead of silently selecting one:

- `PbrApproximation` embeds only supported active PBR bindings and does not
  reconstruct layered inputs.
- `BakeReconstructed` applies supported cooked-parameter recipes and embeds every
  decodable referenced `Texture2D` as a machine-indexed source artifact. It is
  not engine-executed baking because cooked packages do not retain the editor
  expression graph required by Unreal's material baker.

`-Asset` is one exact virtual package path, without `.uasset`, object suffix,
wildcard or fragment. Convert `Voyage/Content/.../SM_Name.uasset` to
`/Game/.../SM_Name`. `-GameRoot` is optional and defaults to the configured Steam
installation.

Output must be a fresh `.glb` below repository `artifacts/`. The sibling
`<output>.evidence/` contains the fingerprint, request, published-tool manifest,
captured log, `export-report.json` and `material-omissions.md`. The Markdown file
names deferred effects, their cooked texture paths, parameter names and relevant
damage/rust controls without copying omitted texture payloads. Existing output/evidence is never
overwritten. The compact result reports source kind, nodes, mesh instances,
unique source meshes, materials, images, failed textures, omitted-component
kinds, GLB hash, report path and omissions path.

The GLB root `extras.materialPipeline` is the authoritative machine interface
for downstream agents. Schema `voyage.material-pipeline/1` records the requested
mode, fidelity, every bake operation and output image index, every generated
image transform, all source texture artifacts with their GLB image indices and
consumers, and unresolved layers. Material and image indices are zero-based glTF
logical indices. `material-omissions.md` is only a human-readable projection.

## Supported representation

- Direct `StaticMesh`: highest ordinary render LOD, or an explicit reported
  Nanite fallback only when no ordinary render LOD exists.
- Blueprint: serialized `USimpleConstructionScript` hierarchy and default
  `StaticMeshComponent` templates. Repeated meshes remain separate component
  instances. Per-component material overrides are honored by slot index.
- Geometry uses the reviewed CUE4Parse conversion DTO/writer conventions:
  centimeters to meters and Unreal axes/handedness to glTF Y-up.
- Material output is approximate glTF metallic/roughness PBR. Only textures with
  a supported active binding are decoded and embedded. Every embedded image has
  provenance and a material consumer.
- In `BakeReconstructed`, a single exact `ColorMask` plus enabled `Red Mask`, `Green Mask` or `Blue Mask`
  controls and matching `MaskedColor` values is composited into Base Color as
  `BaseColor * lerp(white, MaskedColor, channel)`. This cheap bake is explicitly
  reported as a shader approximation. Its input textures remain separately
  embedded as source artifacts even though the baked result is the active PBR image.

## What GLB cannot preserve here

The evidence report lists these rather than silently claiming parity:

- Blueprint construction-script behavior, timelines, runtime transforms,
  spawning, state/visibility changes and gameplay logic;
- skeletal animation, morphs, spline deformation, Niagara, decals, widget/UI
  rendering, lights, audio, collisions and child-actor contents;
- exact high-resolution Nanite data when ordinary render LOD is used;
- Unreal shader graphs beyond the bounded masked-color rule: layered rust/damage,
  world/object-space projection, UV math, vertex effects, refraction, render targets and runtime dynamic
  material parameters.

Compensation is task-specific:

1. Use the GLB as the editable static/default reference, not runtime proof.
2. Preserve separate component nodes so authored animation can be rebuilt for
   moving lids, screens or emitters.
3. Replace Niagara, screen widgets and dynamic decals with authored equivalents
   or keep them as native game components when building a mod.
4. For important shader appearance, use an explicit per-material bake or a
   reviewed alias/binding added from concrete evidence. Do not guess ambiguous
   maps or archive unused textures.
5. If Nanite silhouette/detail matters, add a separately validated Nanite mode;
   do not describe ordinary LOD0 as the original high-resolution source.

## Publish and validation

Normal export consumes the manifest-validated published binary and never runs
`dotnet run`, restore, build or publish. Explicit tool development only:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/VoyageModelExporter/Publish.ps1
python tools/glb/inspect_glb.py artifacts/model-export/fabricator.glb
```

Publishing requires the pinned clean CUE4Parse conversion checkout, canonical
parser bundle and .NET 10. It writes `.tools/bin/VoyageModelExporter` plus a
source/binary manifest. The normal wrapper validates those hashes, current game
fingerprint and reviewed UE 5.8 mapping before mounting stock containers only.

Validated real sample: `/Game/Blueprints/Modules/Utility/BP_Module_Fabricator`
on Steam build `25191271`, executable SHA-256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
It produced 39 nodes, 13 mesh instances from 11 unique meshes, 19,657 triangles,
14 used materials and 20 images with zero texture failures. Structural readback
and Blender 5.2.2 import/render passed. This validates the bounded static/default
representation, not exact in-game shader or runtime behavior.
