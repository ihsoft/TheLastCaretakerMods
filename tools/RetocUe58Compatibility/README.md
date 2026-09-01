# retoc UE 5.8 compatibility build

Voyage's UE 5.8 cooked legacy packages always serialize
`FObjectImport.PackageName`. For filtered cooked packages Unreal writes
`ObjectName` as the placeholder value; filtering does not remove the field.

Upstream retoc `0.1.5` skips that field when `FilterEditorOnly` is set. It then
reads every following import at the wrong offset and can panic while resolving
an invalid outer index. The same bug makes its `to-legacy` output unsuitable
for combining with packages cooked by UE 5.8.

`../Build-RetocUe58Compatibility.ps1` copies an exact reviewed upstream source,
asserts the `legacy_asset.rs` SHA-256, applies the two narrow read/write layout
corrections, builds `retoc_cli`, and writes a compatibility manifest. Source,
Cargo output, and the executable belong below ignored `artifacts/`.

```powershell
.\tools\Build-RetocUe58Compatibility.ps1 `
  -OutputRoot '.\artifacts\tools\retoc-ue58-compatibility'
```

The tool intentionally keeps retoc's `UE5_7` profile name: current Voyage
still uses the same TOC, container-header, and object-version values. The
compatibility change is specifically the legacy `FObjectImport` layout. Recheck
the asserted upstream hash and Unreal's `ObjectResource.cpp` serializer before
using this builder with another retoc or engine version.
