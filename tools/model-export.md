# Voyage model export

Use `Export-VoyageModelGlb.ps1` to export one exact stock `StaticMesh` package
or one Blueprint's default static-mesh assembly to a self-contained GLB. The
wrapper reads current stock containers, validates its published exporter and
returns a compact result plus paths to detailed ignored evidence.

## Normal call

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\Export-VoyageModelGlb.ps1 `
  -Asset '/Game/AssetSets/Vehicles/Gyrocopter/Mesh/Gyro_MooringPlate' `
  -MaterialMode PbrApproximation `
  -OutputPath 'Tmp/model-export/gyro-mooring-plate.glb'
```

`-Asset` is one exact virtual package path without `.uasset`, object suffix,
wildcard, or fragment. Convert `Voyage/Content/.../SM_Name.uasset` to
`/Game/.../SM_Name`; preserve other mount names.

Choose `-MaterialMode` explicitly:

- `PbrApproximation` transfers supported active glTF PBR bindings.
- `BakeReconstructed` applies supported cooked-parameter recipes and records
  unresolved layers and source identities. It is not Unreal shader baking.

`-GeometryMode Ordinary` is the default. Use `-GeometryMode Nanite` only when
the untrimmed Nanite representation matters. That mode is limited to direct
static meshes and fails instead of silently falling back when the complete
cluster and attribute contract cannot be decoded.

The output must be a fresh `.glb` below `Tmp/`, or below ignored `artifacts/`
only when a continuing consumer justifies retention. Its sibling evidence
directory contains the fingerprint, request, report, omissions, tool manifest,
and detailed log. Consume the compact result and omission summary first.

## Representation boundary

The exporter preserves static mesh sections, component hierarchy, default
relative transforms, supported vertex attributes, material slot overrides, and
bounded PBR material data. It does not establish runtime Blueprint behavior,
construction-script effects, animation, dynamic visibility, skeletal content,
widgets, Niagara, collision, audio, or exact Unreal shader fidelity.

Use the GLB as an editable static/default reference. The omissions report is
part of the result: do not synthesize missing textures or present a successful
structural export as gameplay or visual-parity evidence.

## Failure and maintenance

If the wrapper fails, inspect its compact error and returned evidence path
before opening exporter source. A mapping or published-binary failure follows
the relevant guide in [tool index](tool-index.md). For deliberate source changes,
use `VoyageModelExporter/Publish.ps1`, then validate through the public wrapper.
