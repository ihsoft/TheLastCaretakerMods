# Voyage material export

Choose the output needed, then invoke the corresponding public wrapper. These
tools read current stock containers and expose bounded cooked-material
reconstruction; none recovers the original Unreal shader graph.

| Need | Entry point | Output |
| --- | --- | --- |
| Assignable material samples | `Export-VoyageMaterialsGlb.ps1` | One GLB containing sample panels |
| Visual comparison | `Export-VoyageMaterialPreview.ps1` | One PNG contact sheet |
| Exchange one material | `Export-VoyageMaterialPack.ps1` | One `.materialpack.zip` |
| Analyze 1..128 materials | `Export-VoyageMaterialBundle.ps1` | One compact `.materialbundle.zip` |

Every call requires an explicit `-MaterialMode`:

- `PbrApproximation` transfers supported active PBR bindings.
- `BakeReconstructed` applies supported cooked-parameter recipes and keeps
  unsupported layers explicit. It is not engine-executed baking.

When the requested representation is not established, ask which mode is wanted.
Material identities are exact virtual package paths without `.uasset`, object
suffixes, wildcards, or fragments. For batch calls through
`powershell.exe -File`, use `-MaterialsFile` with a UTF-8 JSON array of unique
strings.

## GLB and preview

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\Export-VoyageMaterialsGlb.ps1 `
  -MaterialsFile tools\VoyageMaterialLibrary\socket-materials.example.json `
  -MaterialMode PbrApproximation `
  -OutputPath artifacts\material-export\socket-materials.glb

powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\Export-VoyageMaterialPreview.ps1 `
  -MaterialsFile tools\VoyageMaterialLibrary\socket-materials.example.json `
  -MaterialMode BakeReconstructed `
  -OutputPath artifacts\material-export\socket-materials.png
```

The GLB contains sample geometry so ordinary importers retain its materials;
that geometry is not extracted game content. The preview renders the bounded
reconstruction in Blender and is not an Unreal-rendered shader result. Both
produce sibling ignored evidence with provenance, reports, and diagnostics.

## MaterialPack and bundle

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\Export-VoyageMaterialPack.ps1 `
  -Material '/Game/Path/MI_Material' `
  -MaterialMode BakeReconstructed `
  -Profile Full `
  -SourceTextures MetadataOnly `
  -OutputPath artifacts\material-export\MI_Material.materialpack.zip

powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\Export-VoyageMaterialBundle.ps1 `
  -MaterialsFile tools\VoyageMaterialLibrary\socket-materials.example.json `
  -BundleName MaterialCandidates `
  -MaterialMode BakeReconstructed `
  -OutputPath artifacts\material-export\MaterialCandidates.materialbundle.zip
```

The pack contains no model or GLB. It carries one material's preview, bounded
PBR maps, reconstruction metadata, and its validated inventory. Choose its
profile by consumer:

- `Full` keeps full-resolution final PBR payloads and defaults to
  `MetadataOnly` source textures.
- `AnalysisCompact` bounds final PBR images for comparison and requires
  `MetadataOnly`.
- `Reconstructable` keeps full resolution and includes only source images
  selected for known skipped effects; it requires `Reconstructable` source
  policy.

The bundle is the batch analysis format. It always uses `AnalysisCompact`,
stores final PBR payloads once by content identity, and excludes source pixels
and geometry.

All outputs must be fresh paths below ignored `artifacts/`; the wrappers refuse
unsafe names and existing outputs. A missing image means the recorded scalar or
factor is authoritative. Unsupported layered effects and ambiguous textures
remain explicit metadata rather than invented payloads.

## Result and failure boundary

Consume the wrapper's compact status, output hash, counts, warnings, and report
path. A texture decode problem produces an explicit partial result; it is not a
complete extraction. Read detailed evidence only for the reported material or
failure. Inspect source after a reproducible wrapper failure or for explicit
tool development. Source changes are published through
`VoyageMaterialLibrary/Publish.ps1` and then exercised through the public wrapper.
