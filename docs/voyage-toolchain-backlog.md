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
| jmap | `.tools/bin/jmap_dumper.exe` | jmap `4f88d8a` | accepted |
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
- [x] Add the same stable publisher boundary for jmap and make mappings
  generation consume only the canonical dumper.
- [x] Make the asset JSON store a private implementation detail: remove the
  public root override and require callers to consume only the returned path.
- [x] Split asset retrieval into default cached `Game` and explicit uncached
  `Mod` sources; mount only selected containers and key the cache solely from
  the installed game fingerprint and stock tool inputs.
- [x] Expose the already useful complete package inventory through
  `Get-VoyageAssetJson.ps1 -ListPackages` for both sources instead of requiring
  callers to discover or read the private `_catalog` layout.
- [x] Make reviewed mapping reuse the default black-box path through
  `Get-VoyageMappings.ps1`; consumers resolve it automatically, while jmap
  generation is reserved for a confirmed unmatched game fingerprint.

Acceptance evidence:

- a second invocation of every publisher returned `Rebuilt = False`;
- canonical retoc reported `retoc_cli 0.1.5`;
- canonical jmap reported `jmap_dumper 0.2.0` and a second publication returned
  `Rebuilt = False`;
- `New-VoyageMappings.ps1` resolves repository-relative defaults after parameter
  binding, so its documented Windows PowerShell `-File` invocation reaches the
  intended running-game precondition instead of failing on an empty
  `$PSScriptRoot`;
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

Store-integrity and source-isolation follow-up:

- the earlier `with-additional-*` design was removed because any mod change
  made that cache disposable and allowed unrelated mods to affect research;
- game mode now registers only stock `global` and `pakchunk*` containers even
  when mods remain installed, while mod mode requires one exact `.utoc`, proves
  package ownership from that container alone, and writes an uncached run;
- `-ListPackages` returns a validated list path, count, and hash without
  requiring mappings; game lists reuse the game cache and mod lists remain
  one-off diagnostics;
- the public wrapper now rejects `-CacheRoot`, returns only the asset identity,
  validated JSON path, and content hash, and produces the same game identity
  under Windows PowerShell 5.1 and PowerShell 7;
- a Windows PowerShell `-File` generate-then-repeat test reused one path after
  normalizing `Get-Content` results to plain strings; its diagnostic view was
  removed after validation;
- cache schema 2 invalidates and rebuilds pre-isolation indexes and entries,
  because the older inspector silently mounted every installed container even
  for paths labeled as base-game data.

Windows PowerShell and Inspector-launch follow-up:

- public build/release/extraction/package scripts now have an explicit Windows
  PowerShell 5.1 contract: repository-relative defaults are resolved after
  parameter binding, multi-segment paths use nested `Join-Path`, and the real
  `powershell.exe -File` entry point is part of verification;
- a restricted `dotnet run` can fail before Inspector startup when NuGet cannot
  read `%APPDATA%\NuGet\NuGet.Config`; this is a launch-environment failure and
  must not be interpreted as asset, mapping, or parser evidence;
- duplicate identical `matches.txt` rows are accepted for a selected mod only
  after `ModOnly` proves exact ownership and every combined-view row is the
  requested virtual path. That gate does not prove provider precedence.

## Later pipeline work

- Publish `VoyageAssetInspector` as a manifest-validated canonical executable,
  move both public Inspector wrappers off `dotnet run`, and verify that normal
  inspection performs no restore or NuGet user-configuration access.
- Make mod override resolution emit the winning provider/container and read
  order, then assert that an exact stock replacement resolves from the selected
  mod rather than merely accepting duplicate identical virtual paths.
- Decide whether the tracked patcher should also have a canonical published
  executable instead of being built through `dotnet run`.
- Add a single read-only health check for every canonical binary and publish
  manifest if repeated manual verification becomes error-prone.
