# Voyage material library index

This project backs the public material GLB, preview, MaterialPack, and bundle
exporters. Start with the operation-specific wrapper; all require an explicit
`PbrApproximation` or `BakeReconstructed` material mode.

- [Material export](../material-export.md): choose an entry point, profile, and
  source-texture policy; interpret its compact result and omissions.
- [Model and material routing](../model-and-material-tools.md): choose between
  model, material, and GLB operations.
- [Toolchain maintenance](../toolchain-maintenance.md): publication and binary
  repair boundaries.

Run the wrapper before reading source. This directory's `Publish.ps1` is only
for deliberate tool development.
