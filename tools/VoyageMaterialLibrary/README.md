# Voyage material library -> GLB, preview or MaterialPack

`../Export-VoyageMaterialsGlb.ps1` accepts a batch of exact material package
identities and produces **one self-contained GLB**. It does not change the game,
install containers, edit models, or invoke Unreal. Outputs remain under ignored
`artifacts/`; extracted textures/materials are not Git source inputs.

`../Export-VoyageMaterialPreview.ps1` accepts the same exact material identities,
builds lightweight sample-panel GLB evidence with the selected material mode, and
renders that reconstructed/approximate material result to one PNG through Blender.
Source-only texture artifacts are not embedded and original decoded textures are
not written separately.

`../Export-VoyageMaterialPack.ps1` exports one exact material to one
`<MaterialName>.materialpack.zip`. The archive is a material-only handoff: it has
no model, sample panel or GLB, and contains a fixed WebP sphere preview, final
bounded PBR maps, machine-readable reconstruction metadata, and a conservative
selection of source images for skipped layered effects.

## Normal use (Windows PowerShell 5.1)

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Get-VoyageAssetJson.ps1 -ListPackages -AsJson
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Export-VoyageMaterialsGlb.ps1 `
  -MaterialsFile tools/VoyageMaterialLibrary/socket-materials.example.json `
  -MaterialMode PbrApproximation `
  -OutputPath artifacts/material-export/my-materials.glb
```

Rendered material preview:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Export-VoyageMaterialPreview.ps1 `
  -MaterialsFile tools/VoyageMaterialLibrary/socket-materials.example.json `
  -MaterialMode BakeReconstructed `
  -OutputPath artifacts/material-export/my-material-preview.png
```

Material-only exchange:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Export-VoyageMaterialPack.ps1 `
  -Material /Game/AssetSets/CoreModularPieces/Decorations/Materials/MI_Deco_Ext_DamagedMetal `
  -MaterialMode BakeReconstructed `
  -SourceTextures MetadataOnly `
  -OutputPath artifacts/material-export/MI_Deco_Ext_DamagedMetal.materialpack.zip
```

The filename is fixed by the material identity. Output must be a fresh path below
repository `artifacts/`. One call exports exactly one material; callers that need
several packs invoke the same black-box entry point once per identity. As with GLB
and preview export, the caller must explicitly choose `PbrApproximation` or
`BakeReconstructed`. `-SourceTextures MetadataOnly|Reconstructable` independently
controls source pixel payloads and defaults to compact `MetadataOnly`.

## MaterialPack schema and payload policy

Schema `1` stores the Unreal identity and parent chain, render state, PBR factors
and files, named colors/scalars/switches, resolved texture bindings, UV evidence,
bake operations, warnings, skipped effects, game/tool provenance, and a complete
file inventory with hashes and dimensions. `preview.webp` uses the fixed
`voyage.material-sphere/1` Blender recipe at 768x768. Base color and emission use
lossless WebP; normal and ORM remain lossless PNG with OpenGL normal convention
and R/G/B = occlusion/roughness/metallic.

A missing PBR image means the scalar/factor is authoritative; uniform placeholder
textures are not invented. Every referenced source texture has metadata, including
its roles, package identity, skipped-effect relationship, reconstructable-candidate
classification and selection reason. `MetadataOnly` never adds `source/` payloads.
In `Reconstructable`, source pixels are included only when an explicit semantic
parameter identifies a known skipped effect (damage, mask, world/object projection,
rust, dirt, wear, height, POM or a similar bounded category), it is not explicitly
disabled, and it is not a default/fallback or merely an inherited texture-name
binding. Selected source masks use lossless WebP; selected source normals use PNG.
Active/baked inputs are not duplicated. Multiple identities with identical stored
bytes share one file.

The pack is assembled deterministically and then reopened by the published binary.
Readback rejects unsafe/case-colliding ZIP paths, unreferenced files, geometry,
missing sections, mismatched hashes/dimensions, invalid PBR conventions, and source
payloads without an inclusion reason. A temporary sample GLB exists only inside
the fresh staging directory to feed Blender and is deleted after successful ZIP
validation; it is never archived. No Python runtime is required.

This format remains a bounded cooked-parameter reconstruction, not recovered
Unreal shader bytecode. A small number of meaningful 4K skipped-effect inputs can
still dominate archive size; their inclusion is deliberate and visible in
`sourceTextures`, rather than an implicit archive of all parent dependencies.

The preview tool requires the same explicit `-MaterialMode` choice as GLB export.
It invokes the published material exporter, omits the reconstructed mode's source
artifact archive, retains only images actually bound to the sample materials, and
renders the resulting GLB with Blender 5.0 or newer. The sibling `.png.evidence/`
contains the lightweight source GLB, exporter report, hashes and Blender log.
Wide batch previews use at most six columns. Every material is rendered in
isolation with the same local camera and fixed lighting recipe before its tile is
placed into the contact sheet; edge materials therefore receive the same lighting
as center materials. Labels alternate within their tiles: even 1-based list
positions are above and odd positions below.
This is a material preview of the documented bounded reconstruction, not a view of
raw texture dependencies and not an Unreal-rendered shader.
The GLB machine manifest marks `sourceArtifactPolicy: omitted-for-preview`; bake
inputs remain named and provenance-bound in the texture records, while the large
source-only image archive is intentionally absent.

`-MaterialMode` is deliberately required. If the user's request does not select
one, the caller must ask rather than silently choosing:

- `PbrApproximation` embeds only supported active PBR bindings.
- `BakeReconstructed` applies supported cooked-parameter recipes, embeds only
  active/baked PBR outputs, and records every referenced source texture as
  metadata without copying its source pixels. This is not Unreal engine-executed
  baking because cooked packages do not retain the editor graph.

Consume the catalog tool's returned `packageListPath`; do not discover its cache
internals. `M_` and `MI_` are useful naming filters, not proof of export type.
Exporter resolves and checks Material/MaterialInstance types. Convert catalog
`Voyage/Content/.../MI_Name.uasset` to `/Game/.../MI_Name`; preserve other mounts.
Only exact virtual packages are accepted, with no `.uasset`/object suffix,
wildcards or fragments. Parent/default parameter references resolve automatically;
only dependencies with a supported active PBR use are decoded.

`-MaterialsFile` is a UTF-8 JSON array of 1..128 unique strings. Use it for batch
calls through `powershell.exe -File`, which does not pass string arrays reliably.
Within a PowerShell session, `-Materials @('/Game/.../M_A','/Game/.../MI_B')` is
also supported. Single `-Materials /Game/.../MI_A` works with `-File`.
Optional `-GameRoot` defaults to the repository's configured Steam installation.

Output must be a **fresh `.glb` below repository `artifacts/`**. The sibling
`<output>.evidence/` contains fingerprint, request, published-tool manifest,
diagnostics and `export-report.json`. Existing output/evidence is never overwritten.
Returns one compact JSON with GLB path/hash, material/image counts, number of
PBR-textured materials, failed-texture count and detailed report path. Invalid
material, missing parent, mapping mismatch or invalid GLB fails the batch. A
texture decode failure yields an explicit `partial-textures` result and its
reason; it is not reported as a complete texture extraction.

GLB root `extras.materialPipeline` is the authoritative downstream-agent
contract. Schema `voyage.material-pipeline/1` records requested mode and fidelity,
all bake operations and output image indices, generated image transforms, source
artifacts with their consumers, and unresolved layers. Source-artifact images may
be intentionally unbound to glTF PBR texture slots; their manifest references,
hashes and dimensions are validated independently.

## What is transferred

- CUE4Parse reads inherited parameters, scalar/vector overrides, switches and
  referenced textures from stock containers only. Game, mapping and tool hashes
  gate the operation. The GLB also records source identities, parent chains,
  extracted parameters, approximation warnings and texture provenance in extras.
- In `PbrApproximation`, CUE4Parse-Conversion/AssetRipper decodes only selected Texture2D dependencies.
  **Every embedded PNG must be used by an exported material.** Unknown,
  ambiguous and disabled maps are not decoded or archived; their paths and
  omission reasons appear in `SkippedTextures`. There is no archive-all mode.
  Normal blue reconstruction is done by the upstream decoder; only the PBR-bound
  green-inverted glTF/OpenGL variant is embedded, not a second raw copy. Color
  maps marked linear are encoded to sRGB for color slots; data maps are not
  gamma-encoded. A known zero emission strength omits the emission map entirely.
- In `BakeReconstructed`, known recipes consume their source textures in memory
  and embed only the resulting active PBR variants. Every referenced Texture2D
  remains separately addressable through `materialPipeline.sourceArtifacts` with
  `Disposition: metadata-only`, its consumers and any available decoded hash/
  dimensions. Source pixels are not copied into the GLB by default.
- SharpGLTF constructs/writes/reads back glTF 2.0. One named, UV-mapped 1m sample
  panel per requested material ensures materials survive ordinary Blender import.
  These panels are samples, not extracted game geometry. Assign imported materials
  to your own mesh. Atlas/trim textures still require suitable UVs on that mesh.
- Unambiguous base-color, normal, emissive and explicitly named ORM dependencies
  get **heuristic** PBR bindings. ORM means R=AO, G=roughness, B=metallic. Simple
  named metallic/roughness/tint/emission values are approximated; tint's explicit
  disabled switch is honored. Unknown or ambiguous maps are omitted.
  Mask/translucent modes use base-color alpha only, not the Unreal opacity graph.

This is **not engine-executed shader graph recovery**. `BakeReconstructed` applies
only declared known recipes; world-aligned projection, unknown multi-layer blends,
damage/wetness formulas, runtime parameters, vertex effects, refraction, custom
shaders and glyph/atlas logic are not reproduced. A material may therefore
import as a flat swatch; unsupported texture paths/parameters remain only as
machine-readable unresolved layers; their texture identities and consumers remain
metadata-only in reconstructed mode. "Used" means a known supported use
in the reconstructed PBR approximation, not proof of the original shader wiring.
The raw shader bytecode and executable shader graph are not bundled. Unsupported
texture types/decoders are reported; HDR-to-PNG quantization is disclosed per image.
Largest available mip is used, not necessarily the original author's source.
No automatic resizing: used maps retain their decoded mip resolution. Keep
batches reasonable; decoding and GLB assembly are in memory. Schema3 records
the decoded-source hash and hashes/indices of the actual used image variants;
the raw decoded image need not be embedded. A dependency is retained once per
needed representation, not once per material reference. Generic importers may
discard diagnostic extras during re-export. This tool does not claim exact
in-game visual equivalence.

## Dependencies and one-time publish

We reuse [CUE4Parse/FModel](https://github.com/FabianFG/CUE4Parse) material parsing
and texture conversion, not a new texture decoder. FModel documents material plus
texture export, but not the required batch-material-library GLB interface:
[FModel guide](https://github.com/4sval/FModel/wiki/Getting-Started).
[Unreal's glTF exporter](https://dev.epicgames.com/documentation/unreal-engine/how-the-gltf-exporter-handles-unreal-engine-content)
can bake editor materials; this is not a verified replacement for extracting
Voyage's cooked assets. The local conversion GLTF mesh writer assigns white
materials, so it is not sufficient by itself.

Requirements: .NET 10; published reviewed `.tools/bin/CUE4Parse` parser bundle,
Oodle beside it; matching reviewed Voyage mappings; pinned clean `.tools/CUE4Parse`
checkout for **publishing only**. Versioned NuGet dependencies are in
`packages.lock.json`. CUE4Parse conversion sources/resources compile unchanged
against a private copy of the canonical parser; shared binaries are not replaced.
Upstream sources remain Apache-2.0 with their upstream notices; SharpGLTF is MIT.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/VoyageMaterialLibrary/Publish.ps1
.tools/bin/VoyageMaterialLibrary/VoyageMaterialLibrary.exe --self-test artifacts/material-export/fresh-tests
python tools/glb/verify_material_library.py artifacts/material-export/my-materials.glb
python tools/glb/inspect_glb.py artifacts/material-export/my-materials.glb
```

Publishing requires SDK/NuGet access and may require outside-sandbox permission.
It validates the pinned conversion checkout and canonical parser, builds only
this separate tool, compares private parser DLL hashes and writes its own bundle
manifest. Normal export validates publication and source hashes, never invokes
`dotnet run`, restore, publish, or build. A source change requires explicit publish.
PowerShell launch uses hidden native child with captured stdout/stderr. Decoders'
temporary native files, if needed, are confined to the fresh evidence directory.

The GLB validator requires Python3.10+ and Pillow, decodes/verifies every embedded PNG,
checks texture hashes/dimensions against provenance and material identities. For
schema2 it also rejects orphan images/textures and a texture on disabled emission.
Schema1 legacy archives remain readable, without the used-only guarantee. The
ordinary GLB inspector additionally checks geometry/ranges/hierarchy (NumPy).
Self-tests cover input rejection, normal convention, ambiguous binding, scalar
factors, skipped-resource non-decoding, used-only images, disabled emission,
parameter-only materials and SharpGLTF roundtrip. Real batch verification covers
the three socket materials in the example on Steam25191271 / UE5.8; import/render
checked in Blender2.80. This is extraction/import evidence, not gameplay validation
or proof of arbitrary material fidelity. Future game fingerprints require matching
reviewed mappings and fresh evidence; older snapshot outputs are not reused.
