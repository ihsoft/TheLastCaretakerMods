# Voyage StaticMesh validator

This project is the implementation of `../Test-VoyageStaticMesh.ps1`.
Normal callers use the public wrapper as a black box. `Publish.ps1` is the
deliberate development-only publisher and consumes the canonical CUE4Parse and
VoyageModelExporter bundles without changing them.

- [StaticMesh preparation and inspection](../static-mesh-assets.md): public
  request, report, exit-code, and representation contracts.
- [Toolchain maintenance](../toolchain-maintenance.md): deliberate publication
  and repair boundary.

The canonical CUE4Parse dependency is the reviewed `ihsoft/CUE4Parse` fork
commit that retains ray-tracing proxy data. Keep both that commit and its
upstream base explicit in published manifests.
Facts belong in `inspection.json`; caller-selected policy belongs in
`validation.json`. Never add mod-specific expected values.
