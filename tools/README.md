# Voyage research tools

This directory contains reproducible methods for inspecting **The Last
Caretaker** (`Voyage`). Game-derived outputs belong under ignored `artifacts/`
directories; the tools and the conclusions derived from them belong in Git.

Agents should start here instead of reading every implementation file. Read a
tool's own README or source only when its row below points there or when the
documented interface is insufficient.

## Choose a tool

| Goal | Start with | Result |
| --- | --- | --- |
| Identify the installed game build | `Get-VoyageBuildFingerprint.ps1` | Steam build ID, executable hash, and container metadata/hashes |
| Find, list, or structurally inspect cooked assets | `Inspect-VoyageAsset.ps1` | Paths, JSON exports, Blueprint pseudocode, or mapping reports |
| Extract an exact cooked package for packaging or byte-level work | `Extract-VoyagePackage.ps1` | Legacy `.uasset/.uexp`, `scriptobjects.bin`, and provenance manifest |
| Build the reviewed retoc compatibility binary for UE 5.8 | `Build-RetocUe58Compatibility.ps1` | Patched ignored retoc source/binary plus compatibility manifest |
| Generate a fresh `.usmap` | `VoyageMappingsDumper` | Version-bound `Mappings.usmap` through a temporary UE4SS session |
| Build the reviewed standalone mapping dumper | `Build-JmapVoyageMappingsDumper.ps1` | Exact current jmap commit with the post-0.2.0 UE 5.6+ alignment fix |
| Locate the live `GUObjectArray` when signatures fail | `Find-VoyageUObjectArray.ps1` | Read-only structural scan of the shipping executable's `.data` section |
| Reject an empty, stale, or misrouted `.usmap` | `Test-VoyageMappings.ps1` | Header, payload, manifest, fingerprint, hash, and required-schema checks |
| Prepare the reviewed UAssetAPI source | `Prepare-UAssetApiVoyageUe58.ps1` | Current upstream plus the Voyage filtered-import layout correction |
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
- Most scripts default to the original developer's game path on drive `P:`.
  Pass `-GameRoot '<your Steam Voyage directory>'` on another machine.
- `Extract-VoyagePackage.ps1` requires a compatible `retoc` executable. Its
  local default is the reviewed fork build at
  `.tools/retoc/target/release/retoc.exe`; pass `-Retoc '<path to retoc.exe>'`
  when using another provenance-checked build.
- Current UE 5.8 cooked legacy packages require the reviewed retoc
  `FObjectImport.PackageName` compatibility build. See
  [`RetocUe58Compatibility/README.md`](RetocUe58Compatibility/README.md).
- C# tools currently target .NET 10. `Inspect-VoyageAsset.ps1` additionally
  expects a local CUE4Parse checkout at `tools/CUE4Parse`; that dependency is
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

### `VoyageMappingsDumper`

Use only when a current `.usmap` is required. It is a temporary UE4SS mod, not
a runtime dependency of the shipped autonomous mods.

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
For Voyage UE 5.8, build reviewed commit
`3f189715f08a646a8c341bf80c2fe06e44177ac3` with
`Build-JmapVoyageMappingsDumper.ps1`, pass the explicitly validated
`GUObjectArray` when automatic resolution fails, and regenerate the mapping.
The CLI still prints version `0.2.0`, so the build manifest and executable hash
are the version authority.

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
UE 5.8 additionally requires the documented filtered-package
`FObjectImport.PackageName` read/write compatibility change.

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
