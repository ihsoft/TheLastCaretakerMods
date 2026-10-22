# Voyage model exporter index

This project exports one exact stock `StaticMesh` or one Blueprint's default
static-mesh assembly to GLB. Use the public wrapper as a black box:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\Export-VoyageModelGlb.ps1 -Asset '/Game/Path/Asset' -MaterialMode PbrApproximation -OutputPath 'Tmp/model-export/asset.glb'
```

- [Model export](../model-export.md): asset identity, material and geometry
  modes, output evidence, and representation limits.
- [GLB tools](../glb-tools.md): inspect or preview the resulting file.
- [Toolchain maintenance](../toolchain-maintenance.md): publication boundary.

Run the wrapper before reading source. This directory's `Publish.ps1` is only
for deliberate tool development.
