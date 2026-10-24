# Voyage StaticMesh builder index

This project is the private Unreal Editor host for
`../New-VoyageStaticMeshAssets.ps1`. Normal callers use that public script and
the manifest-validated published host under `.tools/bin`; they do not compile or
inspect this source.

- [StaticMesh preparation and inspection](../static-mesh-assets.md): public
  manifest, result, failure, and validation contracts.
- [Toolchain maintenance](../toolchain-maintenance.md): deliberate publication
  and repair boundary.

`Publish-VoyageStaticMeshBuilder.ps1` is the deliberate tool-development path.
It pins the exact UE 5.8 editor identity and source fingerprint. Generated
assets, cook output, test fixtures, and logs stay in ignored `Tmp/`.
