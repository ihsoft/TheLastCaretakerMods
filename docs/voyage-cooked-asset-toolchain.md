# Voyage cooked-asset toolchain

This document records the accepted contracts and limits of the cooked-asset
workflow. Commands and parameters remain owned by `tools/README.md`.

## Operating rule

Use the repository scripts as black boxes first. Pick the operation from the
tool-routing table, run the documented interface, and consume its diagnostics,
manifest, summary, and output. Do not spend an ordinary task re-reading the
implementation or reconstructing the dependency chain.

Investigate internals only when a tool exits non-zero, crashes or opens a crash
dialog, hangs, rejects valid-looking inputs, returns an unexpected result, or
lacks a documented operation required by the task. Diagnose from the outside
in: inputs and fingerprint, paths and exact dependency commits, mapping gate,
tool log/manifest, minimal reproducer, then source.

## Accepted checkpoint

The accepted UE 5.8 checkpoint is:

- retoc fork `ihsoft/retoc` at `234f4e5`;
- jmap fork `ihsoft/jmap` at `4f88d8a`;
- UAssetAPI fork `ihsoft/UAssetAPI` at `6b5ead3`;
- UAssetGUI fork `ihsoft/UAssetGUI` at `e362030`, using that UAssetAPI commit;
- unmodified CUE4Parse upstream at
  `ec6595e46448a817ac21ea9bde01caa48f80a420`;
- canonical compact GUI executable at `.tools/bin/UAssetGUI.exe`, published by
  `tools/Publish-UAssetGuiBinary.ps1`, size `12,704,947` bytes and SHA-256
  `EABEDAF875A743E9B02E3381A07031559088EA9F352023A4D549A27FAF830C01`;
- canonical retoc at `.tools/bin/retoc.exe`, size `6,643,200` bytes and SHA-256
  `CF6E0A47F343A169413BE46EB750F3441F174D334AC0CAA14962F3F47BA93C1E`;
- canonical jmap dumper at `.tools/bin/jmap_dumper.exe`, size `9,472,000` bytes
  and SHA-256
  `75E693C2ACD22BB63671EF368C8608931CEF59E8F6F441AFFA3F7A72A3D11543`;
- canonical UAssetAPI at `.tools/bin/UAssetAPI/UAssetAPI.dll`, size `4,209,664`
  bytes and SHA-256
  `637500ACEE5565B1C2F4B026B87F3C37E71CDCF92ABCA2D21DDF85615F0798FE`;
- canonical managed CUE4Parse at `.tools/bin/CUE4Parse/CUE4Parse.dll`, size
  `4,025,344` bytes and SHA-256
  `F304981BAD4C53D209DFDABA9EB65A01D825572E543A04914BEBFD3538DCF4FD`.

For Steam build `25056839`, game UE `5.8.1`, executable SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`,
the reviewed mapping is
`mappings/Voyage/steam-25056839-ue5.8.1/Voyage-25056839.usmap`.
Its sibling manifest owns the generator identity, file hash, validation
evidence, and invalidation condition.

Do not infer that a newer fork checkout, similarly named binary, or mapping at
another path has the same guarantees. Builders and tests should use or verify
these identities explicitly.

## Operations

Use the wrappers listed in `tools/README.md`:

- fingerprint the installation with `Get-VoyageBuildFingerprint.ps1`;
- create and validate mappings with `New-VoyageMappings.ps1` and
  `Test-VoyageMappings.ps1`;
- cache an asset as versioned JSON with `Get-VoyageAssetJson.ps1`;
- extract an exact loose package with `Extract-VoyagePackage.ps1`;
- inspect packages and reflection with `Inspect-VoyageAsset.ps1`;
- publish and stress the exact GUI with `Publish-UAssetGuiBinary.ps1` and
  `.tools/bin/UAssetGUI.exe stress-open`;
- publish retoc, jmap, UAssetAPI, and CUE4Parse through their dedicated
  `Publish-*` wrappers, then consume only `.tools/bin/` on the normal path;
- build or prepare fork source only while deliberately changing a dependency
  checkpoint or diagnosing an unexpected publisher/tool result;
- install and remove unchanged runtime canaries only through the hash- and
  fingerprint-gated probe scripts.

The script is the reusable method; game-derived JSON, packages, reports, raw
mapping candidates, and test containers stay under ignored `artifacts/`.

## Validation ladder

Passing a lower level never implies a higher one:

1. fingerprint and dependency provenance match;
2. the mapping passes header, payload, required-schema, hash, and fingerprint
   gates;
3. the asset opens with no unexpected `RawExport` fallback;
4. unchanged save is binary-equal and reopens;
5. independent CUE4Parse inspection agrees with the intended structure;
6. retoc converts, verifies, and reports the exact semantic inventory;
7. a separately named unchanged or deliberately changed canary loads in the
   game;
8. the requested runtime behavior is tested in the real game.

The accepted GUI/API checkpoint passed all `27/27` upstream binary-roundtrip
tests. A stress run over `Voyage/Content/Blueprints` attempted `1067` assets:
`1057` were clean, `10` were reviewed opaque/numeric notices, `0` failed, and
all were binary-equal. The changed-save canary for
`BP_ToolAbility_Maintenance_Dismantle` loaded in the game and applied
`Duration = 1`. These results validate the tested layouts and workflow, not
arbitrary structural edits to every asset.

## Known boundaries

- A successful `retoc verify`, parse, cook, or container load is not gameplay
  validation.
- `VerifyBinaryEquality()` and fully parsed exports are independent gates:
  `RawExport` can preserve unknown bytes while hiding a parse failure.
- UAssetGUI `fromjson` does not carry the explicit engine-version contract used
  by normal GUI Save and previously produced a mixed-layout Voyage package.
  Use the canonical GUI save path for changed assets until that CLI contract is
  corrected and regression-tested.
- UE 5.8 filtered imports serialize `FObjectImport.PackageName`; older filtered
  fixtures do not. The API fix is deliberately gated on `VER_UE5_8`.
- The game is UE `5.8.1`, while the available editor is UE `5.8.2`; loose
  `.uasset/.uexp` cooking requires the documented `-SkipZenStore` path.
- The compatible retoc path may internally select its UE 5.7 profile for the
  matching IoStore format. Use the wrapper and manifest rather than choosing a
  profile from the game's marketing version.
- The canonical CUE4Parse bundle is managed-only. Its publisher uses a
  temporary host to replace upstream `Microsoft.Bcl.Memory 9.0.0` with
  `10.0.11` without modifying the upstream checkout. NuGet can still print the
  dependency project's upstream audit warning during compilation; the
  published manifest, DLL metadata, and `VoyageAssetInspector` output were
  verified to resolve `10.0.11`.
- A `.NET` dialog with exit code `0xE0434352` means an unhandled managed
  exception; inspect the application error and logs before blaming the CLR.
- Two valid retoc builds can differ in physical chunk order. Compare sorted
  semantic inventory between independent builds; reserve whole-container hashes
  for a prepared package versus its installed copy.
