# Voyage toolchain active backlog

## Scope

Build a reproducible, versioned tool pipeline for Voyage asset work. This
backlog owns common tooling only; mod-specific feature research belongs in the
owning mod backlog.

## Operating model

- Normal work consumes published binaries from `.tools/bin/` as black boxes.
- Fork source trees under `.tools/` are inspected or rebuilt only after a tool
  fails, returns an unexpected result, or an intentional fork change has been
  committed and its accepted checkpoint updated.
- A publisher must verify the exact clean source commit, validate its output,
  write provenance and hashes, and leave an already current valid binary
  untouched.
- Tracked wrappers and manifests are the method. Published binaries remain
  ignored local artifacts.

## Stable binary registry

| Tool | Canonical path | Source checkpoint | State |
| --- | --- | --- | --- |
| UAssetGUI | `.tools/bin/UAssetGUI.exe` | UAssetGUI `e362030`, UAssetAPI `6b5ead3` | accepted |
| retoc | `.tools/bin/retoc.exe` | retoc `234f4e5` | accepted |
| UAssetAPI | `.tools/bin/UAssetAPI/UAssetAPI.dll` | UAssetAPI `6b5ead3` | accepted |
| CUE4Parse | `.tools/bin/CUE4Parse/CUE4Parse.dll` | CUE4Parse `ec6595e` | accepted, managed-only |

Exact binary hashes and bundle contents are recorded after successful local
publication in `docs/voyage-cooked-asset-toolchain.md` and in ignored sibling
publish manifests.

## Current iteration

- [x] Add idempotent, checkpoint-gated publishers for retoc, UAssetAPI, and
  CUE4Parse.
- [x] Publish all three accepted binaries to their canonical paths.
- [x] Make extraction use canonical retoc by default.
- [x] Make `VoyageAssetPatcher` and `VoyageAssetInspector` consume the stable
  UAssetAPI and CUE4Parse bundles by default, retaining explicit source-project
  overrides only for tool development.
- [x] Smoke-test retoc, both managed bundles, and the two consuming projects.
- [x] Record hashes and update tool routing documentation.

Acceptance evidence:

- a second invocation of every publisher returned `Rebuilt = False`;
- canonical retoc reported `retoc_cli 0.1.5`;
- both managed consumers rebuilt successfully from the canonical bundles;
- the canonical CUE4Parse bundle itself contains `Microsoft.Bcl.Memory 10.0.11`
  despite the unchanged upstream project declaring vulnerable `9.0.0`;
- an explicit-`UE5_8` UAssetAPI unchanged roundtrip of
  `BP_ToolAbility_Maintenance_Dismantle` was byte-identical for both
  `.uasset` and `.uexp`;
- a narrow CUE4Parse `list:BP_VoyageCableUpdater` query mounted the current
  containers and resolved exactly one package;
- the smoke tests exposed and fixed two wrapper contracts: Patcher now carries
  one explicit engine version through every reopen, and Inspector wrappers use
  `-` rather than a Windows-PowerShell-elided empty argument when mappings are
  absent.

## Later pipeline work

- Decide whether the tracked inspector and patcher themselves should also have
  canonical published executables instead of being built through `dotnet run`.
- Add a single read-only health check for every canonical binary and publish
  manifest if repeated manual verification becomes error-prone.
