# MaterialPack support files

Normal callers use `../Export-VoyageMaterialPack.ps1`; they do not invoke files in
this directory directly.

`render_material_pack_preview.py` runs inside Blender 5.0+ and renders the one
material from the exporter's transient evidence GLB with the fixed
`voyage.material-sphere/1` recipe. The manifest-gated `VoyageMaterialLibrary`
binary owns staging, deterministic ZIP assembly and readback validation. The final
archive never contains the transient GLB or any geometry.

`-SourceTextures MetadataOnly|Reconstructable` controls source image payloads.
`MetadataOnly` is the compact default and preserves all source identities and
classification metadata without adding `source/` files. `Reconstructable` is
opt-in and adds only source images selected for skipped layered effects.

`-Profile Full|AnalysisCompact|Reconstructable` controls exchange quality. `Full`
is backward-compatible and keeps the original baked resolution. `AnalysisCompact`
forces `MetadataOnly`, preserves aspect ratio, never upscales, limits PBR images to
512x1024, stores BaseColor/Normal/ORM as lossless WebP, and renormalizes filtered
OpenGL tangent-space normals. `Reconstructable` keeps full resolution and requires
the selective `Reconstructable` source policy.

Normal callers use `../Export-VoyageMaterialBundle.ps1` for a batch analysis
archive. It invokes the same pack black box per material, then emits one
`<BundleName>.materialbundle.zip` with `bundle.json`, `materials/*.json`,
`previews/*.webp`, and content-addressed `textures/<sha256>.webp`. Identical final
encoded PBR payloads occur once; bundle validation rejects missing/orphan files,
hash drift, duplicate payloads, source pixels, geometry and invalid compact normals.
