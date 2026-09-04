# Voyage research tools

Current [JSON save and engine-selection checkpoint](../docs/voyage-json-save-checkpoint.md)
records canonical publishers, manifests, API/GUI version semantics and the
public Test-UAssetGuiJsonRoundtrip / Invoke-VoyageBoundedTool interfaces.
Normal Voyage extraction uses UE5_8; UE5_7 is an explicit legacy profile.

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
| Get one cooked asset as JSON or list every package | `Get-VoyageAssetJson.ps1` | Validated JSON or package-list path; game storage and reuse are automatic |
| Find, list, or structurally inspect cooked assets | `Inspect-VoyageAsset.ps1` | Paths, JSON exports, Blueprint pseudocode, or mapping reports |
| Extract an exact cooked package for packaging or byte-level work | `Extract-VoyagePackage.ps1` | Legacy `.uasset/.uexp`, `scriptobjects.bin`, and provenance manifest |
| Publish or reuse canonical retoc | `Publish-RetocBinary.ps1` | Stable `.tools/bin/retoc.exe` plus hash/provenance manifest |
| Get mappings for the installed game | `Get-VoyageMappings.ps1` | Path to the matching reviewed and validated tracked `.usmap` |
| Generate mappings after a confirmed game update | `New-VoyageMappings.ps1` | One-shot running-game readiness wait, jmap dump, manifest, and validation |
| Publish or reuse canonical jmap | `Publish-JmapBinary.ps1` | Stable `.tools/bin/jmap_dumper.exe` plus hash/provenance manifest |
| Build the reviewed standalone mapping dumper for development | `Build-JmapVoyageMappingsDumper.ps1` | Lower-level source build used by the jmap publisher |
| Locate the live `GUObjectArray` when signatures fail | `Find-VoyageUObjectArray.ps1` | Read-only structural scan of the shipping executable's `.data` section |
| Reject an empty, stale, or misrouted `.usmap` | `Test-VoyageMappings.ps1` | Header, payload, manifest, fingerprint, hash, and required-schema checks |
| Publish or reuse canonical UAssetAPI | `Publish-UAssetApiBinary.ps1` | Stable `.tools/bin/UAssetAPI/` managed library bundle |
| Publish or reuse canonical CUE4Parse | `Publish-Cue4ParseBinary.ps1` | Stable managed `.tools/bin/CUE4Parse/` library bundle |
| Prepare reviewed UAssetAPI source for development | `Prepare-UAssetApiVoyageUe58.ps1` | Exact source snapshot for deliberate fork/API investigation |
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
- Public build, release, extraction, and packaging entry points are Windows
  PowerShell 5.1-compatible unless the command is explicitly documented as
  PowerShell 7-only. Verify them through `powershell.exe -NoProfile
  -ExecutionPolicy Bypass -File ...`; a successful `pwsh` run is insufficient.
  In those scripts, leave repository-relative parameters empty in `param(...)`
  and resolve them from `$PSScriptRoot` after parameter binding. Build paths
  with nested two-argument `Join-Path` calls because the additional-child-path
  positional form is unavailable in Windows PowerShell 5.1.
- Prefer a tracked `-File` entry point over a large inline `-Command`. If a
  diagnostic command must cross one PowerShell process boundary, quote the
  script block so the caller cannot expand its `$variables`; keep it short and
  shell-aware. For Windows PowerShell 5.1, materialize statement results first
  (`$rows = @(foreach (...) { ... })`) and pipe `$rows` separately instead of
  piping directly from a `foreach` statement.
- UnrealBuildTool-based mod builds must be launched with permission to write
  and rotate `%LOCALAPPDATA%\UnrealBuildTool\Trace*.uba`, even when `-Log`
  targets an ignored repository artifact. In a restricted sandbox, denial at
  that path produces a `dotnet.exe` dialog and managed exit `-532462766`
  (`0xE0434352`) before UBT loads the project. Read the redirected UBT output;
  treat an `UnauthorizedAccessException` for `Trace-backup-*.uba` as a launch
  permission failure, then rerun once with that narrow access. It is not a
  reason to change the project, SDK, Blueprint generator, or .NET installation.
- Patched third-party checkouts live under ignored `.tools/`: `ihsoft/retoc`
  at `49b7721`, `ihsoft/jmap` at `4f88d8a`, `ihsoft/UAssetAPI` at `21c982f`,
  and `ihsoft/UAssetGUI` at `df18b5f`. The GUI checkout contains the same
  UAssetAPI commit as its submodule. Clone/fetch those exact commits before
  using a builder; the scripts reject a different or dirty source tree.
- The unmodified CUE4Parse dependency also lives under `.tools/CUE4Parse`, at
  upstream `FabianFG/CUE4Parse` commit
  `ec6595e46448a817ac21ea9bde01caa48f80a420`. It has no Voyage patch and does
  not require a fork.
- Normal work uses only canonical published artifacts under `.tools/bin/`:
  `UAssetGUI.exe`, `retoc.exe`, `jmap_dumper.exe`,
  `UAssetAPI/UAssetAPI.dll`, and `CUE4Parse/CUE4Parse.dll`. The fork/source
  checkouts above are build inputs, not normal invocation paths. Run a
  publisher only after its accepted source checkpoint changes or when its
  canonical artifact fails manifest/hash checks; otherwise the publisher
  returns the existing binary with `Rebuilt = False`.
- Most scripts default to the original developer's game path on drive `P:`.
  Pass `-GameRoot '<your Steam Voyage directory>'` on another machine.
- `Extract-VoyagePackage.ps1` requires a compatible `retoc` executable. Its
  default is canonical `.tools/bin/retoc.exe`; pass `-Retoc '<path to
  retoc.exe>'` only for an explicit provenance-checked comparison.
- Current UE 5.8 cooked legacy packages require the reviewed retoc
  `FObjectImport.PackageName` compatibility build. See
  [`RetocUe58Compatibility/README.md`](RetocUe58Compatibility/README.md).
- The reviewed mapping for Steam build `25056839` / game UE `5.8.1` is tracked
  at `mappings/Voyage/steam-25056839-ue5.8.1/`. Generate candidates under
  ignored `artifacts/mappings/`; promote only a fully validated mapping as a
  new immutable versioned registry entry.
- Do not inspect the registry or run jmap during normal work. Call
  `Get-VoyageMappings.ps1`; regenerate only when it reports that the installed
  fingerprint has no matching reviewed entry.
- C# tools currently target .NET 10. `VoyageAssetPatcher` and
  `VoyageAssetInspector` reference the canonical UAssetAPI and CUE4Parse
  bundles by default. Their source-project override properties exist only for
  deliberate tool development.
- `Get-VoyageAssetJson.ps1` and `Inspect-VoyageAsset.ps1` still launch the
  tracked Inspector project through `dotnet run`. This is a known transitional
  exception to the stable-binary model: restore may read the user's NuGet
  configuration, and a restricted process can therefore report a false tool
  failure before asset inspection starts. Run these wrappers with access to
  the normal .NET/NuGet user configuration. Do not diagnose the asset or change
  mappings when the log reports access denial for `NuGet.Config`. Publishing a
  canonical Inspector executable and moving both wrappers to it is tracked in
  the toolchain backlog.
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
  `Extract-VoyagePackage.ps1` refuses additional containers by default; use
  its `-AllowAdditionalContainers` switch only when the combined installed
  view is explicitly required. `Get-VoyageAssetJson.ps1` instead selects its
  source and never mixes unrelated installed mods into a game-asset result.
- Never overwrite an extracted source asset. Write transformed output to a new
  ignored directory, preserve a known-good installed package, and do not
  replace installed files while the game is running.

### Release installation

There is not yet a common install-only wrapper for an already built Voyage mod
release. Do not emulate one with an unverified copy. Until the backlog item is
implemented, an owning mod's installer must prove the release manifest and
source identity, confirm the game is closed immediately before mutation,
preserve an exact recoverable backup, copy only the manifest-owned files, read
back their hashes, and update installation evidence only after success.

For standalone IoStore mods whose tested installation contract allows a ZIP in
the Paks directory, retaining the exact release ZIP beside the installed
container is accepted provenance: Voyage ignores the archive while it remains
available to identify and recover the installed release. The ZIP name and
installation manifest must carry the artifact version, not merely the mod
version. This is a provenance copy of a validated release artifact, not a ZIP
that should be regenerated during installation.

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
  '.\mappings\Voyage\steam-25056839-ue5.8.1\Voyage-25056839.usmap' `
  'UE5_8' `
  '.\artifacts\builds\probe\package'
```

Query reflection mappings rather than package contents:

```powershell
.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'mappings:VoyageVehicleForkliftPawn'

.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'mappings-property:ThrottleInput'

.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'mappings-enum:EModuleResourceType' `
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
  -EngineVersion UE5_8
```

The separator after the parent fragment is a literal `|`. Keep the path filter
narrow: this mode loads every matching package to inspect its exported classes.

Find packages whose serialized exports reference a class, function, property,
or asset identity while limiting the scan to a relevant content subtree:

```powershell
.\tools\Inspect-VoyageAsset.ps1 `
  -Query 'references:AddModuleWidget|Voyage/Content/UI/' `
  -EngineVersion UE5_8
```

The separator after the reference fragment is a literal `|`. This mode loads
and serializes every matching package in memory but writes only the matching
package paths and per-package errors. Keep the path filter narrow.

Every query writes to a new fingerprinted directory and refuses to overwrite a
previous result. Choose a new `-OutputRoot` for a repeated investigation.

### Stable JSON retrieval for individual assets

Use `Get-VoyageAssetJson.ps1` when work needs the complete JSON for one asset.
Unlike the query-oriented inspector above, this wrapper owns persistence,
fingerprinting, indexing, and reuse and returns the one validated path callers
should consume:

```powershell
.\tools\Get-VoyageAssetJson.ps1 `
  'Voyage/Content/Blueprints/BP_VoyageCableUpdater.uasset'
```

Return the complete stock-game package inventory from the same validated cache:

```powershell
.\tools\Get-VoyageAssetJson.ps1 -ListPackages
```

This returns `packageListPath`, `packageCount`, and `packageListSha256`. Package
listing does not parse exports and does not require reflection mappings.

`-Source Game` is the default. It mounts only the stock `global` and
`pakchunk*` containers even when mods are installed, and it is the only mode
that uses the reusable cache. The cache identity is derived solely from the
game fingerprint, stock containers, mappings, and parser—not from installed
mods. Pass the asset identity, plus `-GameRoot` only when the installation is
not at the configured default.

Treat the returned `jsonPath` as the entire storage interface. Do not locate
assets by walking the internal store, invent a task-specific output root, or
copy, merge, promote, rewrite, and delete its JSON or sidecars. If this command
fails, investigate the tool as a failed black box instead of manufacturing
another store.

`/Game/Blueprints/BP_VoyageCableUpdater` is accepted as the equivalent Unreal
package identity. A short fragment is also accepted only when it identifies
exactly one package; an ambiguous fragment fails and lists exact candidates.

The first game request may build its internal package index and parse the
asset. Later calls with the same identity reuse the result after validating
the game, mappings, parser, content hash, and provenance. These mechanics are
deliberately not part of the calling contract.

To inspect an asset that is physically supplied by one mod, opt in explicitly
and identify that exact mod container:

```powershell
.\tools\Get-VoyageAssetJson.ps1 `
  'Voyage/Content/Blueprints/BP_VoyageCableUpdater.uasset' `
  -Source Mod `
  -ModContainer 'P:\SteamLibrary\steamapps\common\Voyage\Voyage\Content\Paks\ExampleMod_P.utoc'
```

Mod mode first proves that the requested package belongs to that container,
then mounts the stock dependencies plus only that mod for parsing. Its JSON,
index, logs, and manifest go to a unique ignored diagnostic run under
`artifacts/asset-inspections/`; they are never read from or promoted into the
game cache. An intentional stock-package replacement can be reported once for
the stock container and once for the selected mod; after the ownership proof,
the wrapper accepts those duplicate identical virtual paths but still rejects
any different match. This gate proves exact identity and exclusive ownership,
but does not yet prove which duplicate provider won CUE4Parse resolution; do
not treat it as general override-priority evidence. Provider identity/read
order must be emitted and asserted before this becomes a reusable precedence
contract. Commit this wrapper and conclusions, not generated output.

Use `-ListPackages -Source Mod -ModContainer <container.utoc>` to return the
full inventory of that one mod container. This is also an uncached diagnostic
run and does not require mappings.

### 3. Extract an exact package

Use extraction when another tool needs the real legacy asset files, not merely
a structural report:

```powershell
.\tools\Extract-VoyagePackage.ps1 `
  -Filter 'Vehicles/BP_Forklift_Possesable' `
  -GameRoot 'D:\SteamLibrary\steamapps\common\Voyage' `
  -RetocEngineVersion UE5_8 `
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

### Canonical retoc, jmap, UAssetAPI, and CUE4Parse binaries

Publish the reviewed source checkpoints once:

```powershell
.\tools\Publish-RetocBinary.ps1
.\tools\Publish-JmapBinary.ps1
.\tools\Publish-UAssetApiBinary.ps1
.\tools\Publish-Cue4ParseBinary.ps1
```

Each publisher rejects a different commit or tracked source changes, writes a
hash/provenance manifest beside its ignored canonical output, and returns
without rebuilding when that output already matches the current checkpoint.
The canonical paths are:

```text
.tools/bin/retoc.exe
.tools/bin/jmap_dumper.exe
.tools/bin/UAssetAPI/UAssetAPI.dll
.tools/bin/CUE4Parse/CUE4Parse.dll
```

UAssetAPI and CUE4Parse are managed libraries, so their canonical directories
also contain the runtime dependencies emitted by `dotnet publish`. Consumers
must reference the bundle, not copy only the primary DLL. The CUE4Parse bundle
is intentionally managed-only. Its temporary publish host pins
`Microsoft.Bcl.Memory` `10.0.11` over the upstream `9.0.0` dependency; NuGet
may still print the upstream project warning while building, but the canonical
bundle manifest and DLL metadata must resolve `10.0.11`.

`Build-RetocUe58Compatibility.ps1` and
`Build-JmapVoyageMappingsDumper.ps1` remain the lower-level reproducible
builders used by their publishers. `Prepare-UAssetApiVoyageUe58.ps1` remains
a source-development tool; none is a normal runtime path.

### `Get-VoyageMappings.ps1`

This is the only normal entry point for obtaining mappings:

```powershell
.\tools\Get-VoyageMappings.ps1
```

It fingerprints the installed game, searches only the reviewed tracked
registry under `mappings/Voyage/`, runs `Test-VoyageMappings.ps1`, and returns
the exact mapping and manifest paths plus their identity. Consumers such as
`Get-VoyageAssetJson.ps1` and non-list `Inspect-VoyageAsset.ps1` call it
automatically when no explicit mapping is supplied. A new agent should neither
walk mapping directories nor inspect generation scripts on this successful
path.

If no reviewed entry matches, the resolver exits non-zero and explicitly says
not to regenerate automatically. Confirm that the fingerprint is genuinely
new, ask the user to start Voyage, and only then use the generator below.

### `New-VoyageMappings.ps1`

This is the exceptional refresh path after a confirmed game update:

```powershell
.\tools\New-VoyageMappings.ps1
```

Start Voyage first; exactly one running shipping process is a precondition.
The script never starts or stops the game. It fingerprints the installation,
verifies the canonical `.tools/bin/jmap_dumper.exe`, waits for the process to
be at least 60 seconds old and for three identical structural `GUObjectArray`
samples, then creates a complete `--all` UE 5.8 USMAP with concurrency `128`,
writes logs and provenance, and runs `Test-VoyageMappings.ps1`. It returns only
after the new mapping passes every gate. Each attempt uses a new fingerprinted
directory below `artifacts/mappings/`, so a previous known-good mapping is
never overwritten. A missing or invalid canonical dumper is a hard stop; run
`Publish-JmapBinary.ps1` separately rather than rebuilding source during a
mapping job.

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

The dumper output is a candidate and remains ignored until it passes the full
review and promotion gate. Only the resulting immutable registry entry under
`mappings/Voyage/` is committed.

Standalone `jmap_dumper` release `0.2.0` predates upstream commit `805cd7a`,
which fixes the serialized width of `UStruct::MinAlignment` for UE 5.6+.
For Voyage UE 5.8, build reviewed fork commit
`4f88d8af758712839529f9eeeb02b82c9469e271` once with
`Publish-JmapBinary.ps1`, pass the explicitly validated `GUObjectArray` when
automatic resolution fails, and regenerate the mapping through
`New-VoyageMappings.ps1`.
This commit is based on upstream `3f189715f08a646a8c341bf80c2fe06e44177ac3`
and additionally handles nullable transient Blueprint enum, interface, and
object metadata. The CLI still prints version `0.2.0`, so the source commit,
build manifest, and executable hash are the version authority.

```powershell
.\tools\Publish-JmapBinary.ps1
```

Use `Build-JmapVoyageMappingsDumper.ps1` directly only while developing or
diagnosing the fork itself. Its output is not the canonical runtime path.

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
`21c982fa8f04e12d5d216fdf330a2f206e81156f` contains the scoped filtered import,
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
  -OutputRoot '.\artifacts\tools\uassetapi-21c982f'
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
reviewed compatibility checkpoint is `df18b5f`, which pins UAssetAPI `21c982f`
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
  '<new-output.uasset>' `
  UE5_8
```

The input and its companion files must come from the matching game build, and
the output must be a different path. See
[`VoyageAssetPatcher/README.md`](VoyageAssetPatcher/README.md) for the required
UAssetAPI checkpoint and the exact assertions of each operation. Normal builds
use canonical `.tools/bin/UAssetAPI/`. Pass
`-p:UAssetApiProject=<path>` only while deliberately developing and validating
a replacement UAssetAPI checkpoint.

The optional final engine selector defaults to `UE5_7` only for preserved
legacy operations. Current Voyage assets must pass `UE5_8`; the patcher uses
that one explicit value for the initial read and every verification reopen.

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
