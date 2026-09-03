# Voyage research tools

This directory contains reproducible methods for inspecting **The Last
Caretaker** (`Voyage`). Game-derived outputs belong under ignored `artifacts/`
directories; the tools and the conclusions derived from them belong in Git.

Agents should start here instead of reading every implementation file.

## Black-box first

The table and documented commands are the public interface of this toolset.
On the normal path, choose a tool by intent, run it unchanged, and judge the
result from its exit code, manifest, summary, and produced files. Do not first
survey the script, re-derive its dependencies, or manually reproduce its
steps.

Open the implementation or a third-party fork only after a non-zero exit, a
crash/dialog, a hang, an unexpected output, or a case the documented interface
does not cover. Before changing code, inspect the tool's own diagnostics and
verify its fingerprinted inputs, paths, mapping gate, and dependency commit.
The accepted compatibility checkpoints and validation boundaries are recorded
in [`../docs/voyage-cooked-asset-toolchain.md`](../docs/voyage-cooked-asset-toolchain.md).

## Choose a tool

| Goal | Start with | Result |
| --- | --- | --- |
| Identify the installed game build | `Get-VoyageBuildFingerprint.ps1` | Steam build ID, executable hash, and container metadata/hashes |
| Get one cooked asset or Blueprint as reusable JSON | `Get-VoyageAssetJson.ps1` | Fingerprinted cache entry at the asset's mirrored virtual path |
| Find, list, or structurally inspect cooked assets | `Inspect-VoyageAsset.ps1` | Paths, JSON exports, Blueprint pseudocode, or mapping reports |
| Extract an exact cooked package for packaging or byte-level work | `Extract-VoyagePackage.ps1` | Legacy `.uasset/.uexp`, `scriptobjects.bin`, and provenance manifest |
| Build the reviewed retoc compatibility binary for UE 5.8 | `Build-RetocUe58Compatibility.ps1` | Patched ignored retoc source/binary plus compatibility manifest |
| Generate a fresh `.usmap` | `New-VoyageMappings.ps1` | One-shot running-game readiness wait, jmap dump, manifest, and validation |
| Build the reviewed standalone mapping dumper | `Build-JmapVoyageMappingsDumper.ps1` | Exact jmap fork commit with the UE 5.8 layout and nullable-metadata fixes |
| Locate the live `GUObjectArray` when signatures fail | `Find-VoyageUObjectArray.ps1` | Read-only structural scan of the shipping executable's `.data` section |
| Reject an empty, stale, or misrouted `.usmap` | `Test-VoyageMappings.ps1` | Header, payload, manifest, fingerprint, hash, and required-schema checks |
| Prepare the reviewed UAssetAPI source | `Prepare-UAssetApiVoyageUe58.ps1` | Exact UAssetAPI fork commit with the reviewed Voyage UE 5.8 compatibility fixes |
| Publish the validated compact UAssetGUI executable | `Publish-UAssetGuiBinary.ps1` | Stable ignored `.tools/bin/UAssetGUI.exe` |
| Stress-test hierarchy asset opens in patched UAssetGUI | `.tools/bin/UAssetGUI.exe stress-open` | Incremental per-asset JSONL plus parse/binary-equality summary |
| Install/remove one unchanged package canary | `Install-VoyageUnchangedProbe.ps1`, `Remove-VoyageUnchangedProbe.ps1` | Current-fingerprint and exact-hash guarded runtime roundtrip test |
| Locate native names, references, or correlated member offsets | `VoyageExecutableInspector` | Read-only executable report with version-specific offsets |
| Reproduce one of the existing surgical cooked-asset probes | `VoyageAssetPatcher` | Assertion-checked diagnostic asset written to a new path |
| Discover which Blueprint editor APIs Unreal Python exposes | `Inspect-UnrealBlueprintApi.py` | `Saved/BlueprintApi.txt` in an Unreal project |
| Reuse semantic C++ names while generating Blueprint graphs | `UnrealEditorGeneratorCommon` | Header-only build-time helpers; not a command-line tool |

`VoyageAssetInspector` is the CUE4Parse backend used by
`Inspect-VoyageAsset.ps1`. Prefer the PowerShell wrapper because it adds the
game fingerprint, a versioned output directory, and an inspection manifest.

## Common setup and safety

- Run commands from the repository root in PowerShell.
- Patched third-party checkouts live under ignored `.tools/`: `ihsoft/retoc`
  at `234f4e5`, `ihsoft/jmap` at `4f88d8a`, `ihsoft/UAssetAPI` at `6b5ead3`,
  and `ihsoft/UAssetGUI` at `e362030`. The GUI checkout contains the same
  UAssetAPI commit as its submodule. Clone/fetch those exact commits before
  using a builder; the scripts reject a different or dirty source tree.
- The unmodified CUE4Parse dependency also lives under `.tools/CUE4Parse`, at
  upstream `FabianFG/CUE4Parse` commit
  `ec6595e46448a817ac21ea9bde01caa48f80a420`. It has no Voyage patch and does
  not require a fork.
- Most scripts default to the original developer's game path on drive `P:`.
  Pass `-GameRoot '<your Steam Voyage directory>'` on another machine.
- `Extract-VoyagePackage.ps1` requires a compatible `retoc` executable. Its
  local default is the reviewed fork build at
  `.tools/retoc/target/release/retoc.exe`; pass `-Retoc '<path to retoc.exe>'`
  when using another provenance-checked build.
- Current UE 5.8 cooked legacy packages require the reviewed retoc
  `FObjectImport.PackageName` compatibility build. See
  [`RetocUe58Compatibility/README.md`](RetocUe58Compatibility/README.md).
- The reviewed mapping for Steam build `25056839` / game UE `5.8.1` is tracked
  at `mappings/Voyage/steam-25056839-ue5.8.1/`. Generate candidates under
  ignored `artifacts/mappings/`; promote only a fully validated mapping as a
  new immutable versioned registry entry.
- C# tools currently target .NET 10. `Inspect-VoyageAsset.ps1` additionally
  expects a local CUE4Parse checkout at `.tools/CUE4Parse`; that dependency is
  intentionally ignored by Git.
- `VoyageAssetInspector` pins `Microsoft.Bcl.Memory` `10.0.11` to override the
  vulnerable `9.0.0` transitive dependency in the current CUE4Parse checkout.
  The dependency project can still emit its own audit warning while building;
  verify the final Inspector `.deps.json` resolves only `10.0.11`.
- Without `cmake`, CUE4Parse reports that its optional native backend was not
  built and continues with the managed implementation. Record that limitation
  rather than treating a managed-only inspection as native-backend coverage.
- Start every version-bound investigation with a fresh fingerprint. Do not use
  an old artifact merely because its path or asset name still looks correct.
- Installed override containers can shadow base-game assets during extraction.
  The extractor refuses additional containers by default. Use
  `-AllowAdditionalContainers` only when the combined, modded view is the
  explicit subject of the investigation.
- Never overwrite an extracted source asset. Write transformed output to a new
  ignored directory, preserve a known-good installed package, and do not
  replace installed files while the game is running.

## Typical workflow

### 1. Fingerprint the game

```powershell
.\tools\Get-VoyageBuildFingerprint.ps1 `
  -GameRoot 'D:\SteamLibrary\steamapps\common\Voyage' `
  -OutputPath '.\artifacts\fingerprints\current.json'
```

Compare the Steam build ID and executable SHA-256 with the owning mod's
`GAME_DERIVED_SOURCES.md`. A mismatch is a hard stop for old extracted assets,
reconstructed native contracts, relocation assumptions, and cooked packages.

### 2. Find or inspect an asset

List matching packages without loading their exports:

```powershell
.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'list:BP_Forklift' `
  -GameRoot 'D:\SteamLibrary\steamapps\common\Voyage'
```

Load matching packages and write JSON plus Blueprint pseudocode when
available:

```powershell
.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'BP_Forklift_Possesable' `
  -MappingsPath '.\artifacts\mappings\current\Mappings.usmap' `
  -EngineVersion UE5_8 `
  -GameRoot 'D:\SteamLibrary\steamapps\common\Voyage'
```

For a non-installed container that must be resolved together with stock game
dependencies, call the underlying inspector with the game Paks directory as
the primary input and the test package directory as the optional sixth
argument. Both directories remain read-only:

```powershell
dotnet run --project .\tools\VoyageAssetInspector -c Release -- `
  'P:\SteamLibrary\steamapps\common\Voyage\Voyage\Content\Paks' `
  'BP_VoyageIngameHud' `
  '.\artifacts\inspection\probe-hud' `
  '.\artifacts\mappings\current\Mappings.usmap' `
  'UE5_8' `
  '.\artifacts\builds\probe\package'
```

Query reflection mappings rather than package contents:

```powershell
.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'mappings:VoyageVehicleForkliftPawn' `
  -MappingsPath '.\artifacts\mappings\current\Mappings.usmap'

.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'mappings-property:ThrottleInput' `
  -MappingsPath '.\artifacts\mappings\current\Mappings.usmap'

.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'mappings-enum:EModuleResourceType' `
  -MappingsPath '.\artifacts\mappings\current\Mappings.usmap' `
  -EngineVersion UE5_8
```

`-EngineVersion` selects the CUE4Parse serialization rules. It defaults to
`UE5_7` for existing version-bound workflows; pass `UE5_8` for current Voyage
builds based on Unreal Engine 5.8.

Find Blueprint-generated classes by their exact direct parent while limiting
the package scan to a relevant content subtree:

```powershell
.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'parent:VoyageModuleResourceWidget|Voyage/Content/UI/' `
  -MappingsPath '.\artifacts\mappings\current\Mappings.usmap' `
  -EngineVersion UE5_8
```

The separator after the parent fragment is a literal `|`. Keep the path filter
narrow: this mode loads every matching package to inspect its exported classes.

Find packages whose serialized exports reference a class, function, property,
or asset identity while limiting the scan to a relevant content subtree:

```powershell
.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'references:AddModuleWidget|Voyage/Content/UI/' `
  -MappingsPath '.\artifacts\mappings\current\Mappings.usmap' `
  -EngineVersion UE5_8
```

The separator after the reference fragment is a literal `|`. This mode loads
and serializes every matching package in memory but writes only the matching
package paths and per-package errors. Keep the path filter narrow.

Every query writes to a new fingerprinted directory and refuses to overwrite a
previous result. Choose a new `-OutputRoot` for a repeated investigation.

### Stable JSON cache for individual assets

Use `Get-VoyageAssetJson.ps1` when later work needs the same complete asset
export more than once. Unlike the query-oriented inspector above, this wrapper
maintains one package index per game/container view and promotes one JSON file
to a stable, mirrored virtual path:

```powershell
.\tools\Get-VoyageAssetJson.ps1 `
  'Voyage/Content/Blueprints/BP_VoyageCableUpdater.uasset'
```

`/Game/Blueprints/BP_VoyageCableUpdater` is accepted as the equivalent Unreal
package identity. A short fragment is also accepted only when it identifies
exactly one package; an ambiguous fragment fails and lists exact candidates.

The cache layout is:

```text
artifacts/asset-cache/
  steam-<build-id>-<exe-sha-prefix>-base-<container-sha-prefix>/
    <container-view>/
      _catalog/packages.txt
      Voyage/Content/.../<Asset>.json
      Voyage/Content/.../<Asset>.asset-manifest.json
```

The first request for a game/container view builds `_catalog/packages.txt`.
The first exact asset request then parses and stores the JSON; later requests
return `cacheStatus = hit` without rewriting it. The sidecar binds the JSON to
the executable and base-container fingerprint, exact virtual path, mappings
hash, mounted container view, content hash, and inspector source hash. The
wrapper discovers the newest current-fingerprint mapping below
`artifacts/mappings/` and runs `Test-VoyageMappings.ps1` before using it.

Additional installed containers can shadow stock packages, so the default
call refuses them. Remove those containers to populate the `base` view, or use
the explicit opt-in below when the combined installed view is the intended
subject:

```powershell
.\tools\Get-VoyageAssetJson.ps1 `
  'Voyage/Content/Blueprints/BP_VoyageCableUpdater.uasset' `
  -AllowAdditionalContainers
```

The opt-in result goes into a separate `with-additional-<hash>` view; it never
masquerades as a base-game export. Cache contents and indexes remain ignored,
game-derived artifacts. Commit this wrapper and conclusions, not its output.

### 3. Extract an exact package

Use extraction when another tool needs the real legacy asset files, not merely
a structural report:

```powershell
.\tools\Extract-VoyagePackage.ps1 `
  -Filter 'Vehicles/BP_Forklift_Possesable' `
  -GameRoot 'D:\SteamLibrary\steamapps\common\Voyage' `
  -Retoc 'D:\Tools\retoc.exe' `
  -RetocEngineVersion UE5_7 `
  -OutputRoot '.\artifacts\extracted\forklift-refresh'
```

`-RetocEngineVersion` records and passes the exact engine profile supported by
the selected retoc build. Do not silently substitute it across game updates;
first compare the live IoStore header and package versions and prove extraction
against a fresh fingerprint.

The retoc executable path and SHA-256 in `extraction-manifest.json` are part of
the test identity. Do not compare parser results from assets produced by
different retoc binaries merely because their `.uexp` payloads match: the
legacy `.uasset` import/header conversion can differ and invalidate every
downstream mapping result.

`-Filter` matches the IoStore directory-index path. A successful run must
produce at least one `.uasset`; zero matches are an error. The output includes
`extraction-manifest.json`, which records the filter, fingerprint, retoc
path/hash/profile, and whether additional containers were permitted.

## Specialized tools

### `New-VoyageMappings.ps1`

This is the normal happy path for refreshing Voyage mappings:

```powershell
.\tools\New-VoyageMappings.ps1
```

Start Voyage first; exactly one running shipping process is a precondition.
The script never starts or stops the game. It fingerprints the installation,
verifies or builds the reviewed jmap fork, waits for the process to be at least
60 seconds old and for three identical structural `GUObjectArray` samples,
then creates a complete `--all` UE 5.8 USMAP with concurrency `128`, writes
logs and provenance, and runs `Test-VoyageMappings.ps1`. It returns only after
the new mapping passes every gate. Each attempt uses a new fingerprinted
directory below `artifacts/mappings/`, so a previous known-good mapping is
never overwritten.

Use `-InstallForUAssetGUI` to copy the validated result to
`%LOCALAPPDATA%\UAssetGUI\Mappings\Voyage-<build>.usmap`; a different existing
file is backed up first. Reflection readiness and the dumper both have bounded
timeouts. A partial run and its logs remain under `artifacts/` for
diagnosis instead of being promoted.

### `VoyageMappingsDumper`

This temporary UE4SS mod is the fallback when the standalone jmap path itself
is under investigation. It is not a runtime dependency of the shipped
autonomous mods.

1. Install a Voyage-compatible UE4SS build.
2. Copy `tools/VoyageMappingsDumper` to
   `<GameRoot>/Voyage/Binaries/Win64/ue4ss/Mods/VoyageMappingsDumper`.
3. Enable `VoyageMappingsDumper : 1` in `ue4ss/Mods/mods.txt`.
4. Launch the game and wait at least 12 seconds. The mod calls `DumpUSMAP()`
   once after reflection data has loaded.
5. Use the UE4SS log to confirm success and locate the generated
   `Mappings.usmap`, then copy it under a fingerprinted `artifacts/mappings/`
   directory.
6. Disable or remove the dumper and UE4SS before testing a loader-free setup.

The mapping is a snapshot of one executable and is never committed.

Standalone `jmap_dumper` release `0.2.0` predates upstream commit `805cd7a`,
which fixes the serialized width of `UStruct::MinAlignment` for UE 5.6+.
For Voyage UE 5.8, build reviewed fork commit
`4f88d8af758712839529f9eeeb02b82c9469e271` with
`Build-JmapVoyageMappingsDumper.ps1`, pass the explicitly validated
`GUObjectArray` when automatic resolution fails, and regenerate the mapping.
This commit is based on upstream `3f189715f08a646a8c341bf80c2fe06e44177ac3`
and additionally handles nullable transient Blueprint enum, interface, and
object metadata. The CLI still prints version `0.2.0`, so the source commit,
build manifest, and executable hash are the version authority.

```powershell
.\tools\Build-JmapVoyageMappingsDumper.ps1 `
  -SourceRoot '.\.tools\jmap' `
  -OutputRoot '.\artifacts\tools\jmap-4f88d8a'
```

`Find-VoyageUObjectArray.ps1` performs the fallback address discovery without
injecting code or writing process memory. It reads only the shipping module's
PE `.data` section and requires exactly one candidate matching the chunk count,
capacity, pointer, and first-object invariants. Use its `guObjectArray` result
only for the same live process invocation; ASLR changes the absolute address on
the next launch.

Do not promote a dumper output merely because a file named `Mappings.usmap`
exists. The UE 5.8 automatic `GUObjectArray` resolver can emit a structurally
valid but empty 28-byte map. Store the selected output with an ignored
`mapping-manifest.json`, then gate every parser use through
`Test-VoyageMappings.ps1`. The validator checks the USMAP header and payload,
the exact mapping hash and length, the owning game fingerprint, and a small set
of required Voyage schemas. Keep failed automatic outputs under diagnostic
names rather than at the canonical path.

### `Prepare-UAssetApiVoyageUe58.ps1`

The reviewed UAssetAPI fork commit
`6b5ead37f213adc79d814689040a519be4e04a74` contains the scoped filtered import,
filtered `FField`, dependency-schema engine-version/parent-chain fixes, native
custom-export preservation, empty `InstancedPropertyBag` support, and missing
native-enum fallback used by the GUI and command-line asset tools. The
preparation script accepts only its configured clean checkpoint and copies
tracked source files to a new ignored output directory. The filtered import
rule retains the legacy layout through UE 5.7 and enables the additional
`FObjectImport.PackageName` only for an explicitly selected UE 5.8 asset; this
restores UAssetAPI's complete upstream suite to `27/27` while retaining the
accepted `1,067`-asset Voyage stress result.

```powershell
.\tools\Prepare-UAssetApiVoyageUe58.ps1 `
  -OutputRoot '.\artifacts\tools\uassetapi-6b5ead3'
```

### Patched UAssetGUI `stress-open`

The reviewed UAssetGUI fork under `.tools/UAssetGUI` has a headless stress
command that exercises the same path as selecting an asset in the IoStore
hierarchy. For every `.uasset` and `.umap` below the requested virtual prefix
it extracts through `DirectoryTreeItem.SaveFileToTemp`, loads dependencies,
builds the GUI property table, and invokes UAssetGUI's unchanged-save
`VerifyBinaryEquality` check.

```powershell
.\.tools\bin\UAssetGUI.exe --portable stress-open `
  'P:\SteamLibrary\steamapps\common\Voyage\Voyage\Content\Paks\pakchunk0-Windows.utoc' `
  '\Voyage\Content\Blueprints' `
  '.\artifacts\uassetgui-stress\blueprints' `
  UE5_8 `
  '.\mappings\Voyage\steam-25056839-ue5.8.1\Voyage-25056839.usmap'
```

The report directory must not already contain output unless `--resume` is
passed. `results.jsonl` is flushed after every asset and `summary.json` is
atomically refreshed, so an interrupted run can continue without repeating
completed paths. An optional non-negative limit may precede or follow
`--resume` for a canary run.

The result is `failed` for an exception, a binary mismatch, any `RawExport`,
or unversioned properties without mappings. Missing dependencies, explicitly
opaque native exports, and numeric fallbacks for enums absent from mappings are
`notice`. Binary equality and complete parsing are deliberately separate
gates: preserving unknown bytes can make equality pass even when the GUI
displayed `Failed to parse N exports`. The summary records hashes for the
mapping, selected container, and every `.utoc` mounted from the same directory
because the embedded retoc reads that complete set.

Outputs are game-derived diagnostics and remain below ignored `artifacts/`.
The headless command was introduced by UAssetGUI fork commit `b95587b`. The
reviewed compatibility checkpoint is `e362030`, which pins UAssetAPI `6b5ead3`
and records the additional structured diagnostics in the stress report.

The canonical executable path for both interactive use and automated tests is
`.tools\bin\UAssetGUI.exe`. Rebuild it only through the checkpoint-gated
publisher:

```powershell
.\tools\Publish-UAssetGuiBinary.ps1
```

The script verifies the validated UAssetGUI and UAssetAPI commits, refuses
tracked source changes, publishes to a temporary ignored directory, verifies
the candidate hash, atomically replaces the canonical EXE, and cleans the
temporary output. Its compact framework-dependent publish is equivalent to:

```powershell
dotnet publish .\.tools\UAssetGUI\UAssetGUI\UAssetGUI.csproj `
  -c Release -r win-x64 --self-contained false `
  -p:PublishSingleFile=true
```

Only the resulting `UAssetGUI.exe` is needed for distribution. On first run it
still creates normal working data: `%LOCALAPPDATA%\UAssetGUI` by default or a
`Data` directory beside the executable with `--portable`. Voyage mappings stay
external and build-specific; do not embed or commit the generated `.usmap`.
The target machine must have the matching .NET Desktop Runtime. Use
`--self-contained true` only when that prerequisite cannot be imposed; it
packages the runtime too and makes the executable substantially larger.
Validate a supposedly standalone build from an otherwise empty directory with
`stress-open`, so an older extracted retoc or adjacent DLL cannot mask a
missing bundled resource.

### `VoyageExecutableInspector`

This is a read-only PE inspector, not a decompiler and not an injector. It can
search ASCII/UTF-16 strings, show nearby bytes and pointers, find references to
known virtual addresses, and correlate pages containing several member
offsets.

```powershell
dotnet run --project .\tools\VoyageExecutableInspector -c Release -- `
  'D:\SteamLibrary\steamapps\common\Voyage\Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe' `
  '.\artifacts\inspection\native-input-chain.txt' `
  --window=8192 `
  VoyageInputControlsComponent GetProvidedActions
```

See [`VoyageExecutableInspector/README.md`](VoyageExecutableInspector/README.md)
for `--target-va` and `--member-offsets`. Record the executable hash beside any
conclusion based on an offset.

### `VoyageAssetPatcher`

This is not a general-purpose asset editor. It preserves assertion-heavy,
version-bound diagnostic transformations, including:

- `break-bottom-action-filter` proves ownership of the native bottom HUD row;
- `swap-forklift-horn-to-exit` probes the standard-action producer path;
- `swap-hud-indicator-subclass` appends a distinct marker-class import and
  changes only the original HUD CDO's indicator class reference/dependency;
- `roundtrip-unchanged` separates writer normalization from a requested patch;
- `set-cable-updater-tick-interval` extends only the exact current
  `BP_VoyageCableUpdater` CDO with a one-second TickInterval marker;
- `break-cable-updater-super-index` creates an intentional bad-export-index
  crash marker for proving that exact stock package is loaded;
- `swap-hud-indicator-existing-control` is a non-installable field-identity
  control using an already imported widget class.

```powershell
dotnet run --project .\tools\VoyageAssetPatcher\VoyageAssetPatcher.csproj `
  -c Release -- `
  break-bottom-action-filter `
  '<input.uasset>' `
  '<Mappings.usmap>' `
  '<new-output.uasset>'
```

The input and its companion files must come from the matching game build, and
the output must be a different path. See
[`VoyageAssetPatcher/README.md`](VoyageAssetPatcher/README.md) for the required
UAssetAPI source checkout and the exact assertions of each operation. Voyage
UE 5.8 requires all three compatibility changes pinned by
`Prepare-UAssetApiVoyageUe58.ps1`.

### `Inspect-UnrealBlueprintApi.py`

Run this script inside an Unreal Editor project with Python Editor Scripting
enabled when generator work depends on an uncertain Python API surface:

```powershell
UnrealEditor-Cmd.exe '<project.uproject>' `
  -ExecutePythonScript='<absolute-path>\tools\Inspect-UnrealBlueprintApi.py' `
  -unattended -nop4 -nullrhi
```

It reports the public names exposed for selected Blueprint graph/editor types
to `<project>/Saved/BlueprintApi.txt`. It does not inspect Voyage game assets.

### `UnrealEditorGeneratorCommon`

This folder contains header-only, editor-build helpers shared by mod generator
modules. It is never shipped in a mod. Read
[`UnrealEditorGeneratorCommon/README.md`](UnrealEditorGeneratorCommon/README.md)
before adding names or helpers; call sites must use semantic pin roles rather
than opaque reflected names such as `A`, `B`, or `bPickA`.

## Maintaining this index

When adding, renaming, or materially changing a reusable tool:

1. update its entry and example here;
2. add a tool-local README when prerequisites or safety contracts no longer fit
   in one catalog entry;
3. keep developer-specific dependency paths overridable by parameters;
4. document generated outputs and ensure they remain ignored;
5. promote stable findings to `docs/` rather than expanding the tool README
   into an architecture snapshot.
