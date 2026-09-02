# retoc UE 5.8 compatibility build

Voyage's UE 5.8 cooked legacy packages always serialize
`FObjectImport.PackageName`. For filtered cooked packages Unreal writes
`ObjectName` as the placeholder value; filtering does not remove the field.

The reviewed upstream commit
`885a8dae740cb1ce1e41ff2e74f67f9f0c118237`
(`v0.1.5-2-g885a8da`, 2026-08-13) skips that field when `FilterEditorOnly` is
set. It then reads every following import at the wrong offset and can panic
while resolving an invalid outer index. The same bug makes its `to-legacy`
output unsuitable for combining with packages cooked by UE 5.8.

The reviewed fork commit
`234f4e5dcc7b9c2d7a0c8d3a79586a4168266723` contains the two narrow read/write
layout corrections. `../Build-RetocUe58Compatibility.ps1` asserts that exact
clean commit, its Git description, and the patched `legacy_asset.rs` SHA-256;
then it copies and builds the fork without altering it. The compatibility
manifest records both the fork commit and its upstream base. Source, Cargo
output, and the executable belong below ignored `artifacts/`.

```powershell
.\tools\Build-RetocUe58Compatibility.ps1 `
  -OutputRoot '.\artifacts\tools\retoc-ue58-compatibility'
```

The tool intentionally keeps retoc's `UE5_7` profile name: current Voyage
still uses the same TOC, container-header, and object-version values. The
compatibility change is specifically the legacy `FObjectImport` layout. Recheck
the asserted fork/source hash and Unreal's `ObjectResource.cpp` serializer before
using this builder with another retoc or engine version.
