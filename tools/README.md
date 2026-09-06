# Voyage research tools

Current [JSON save and engine-selection checkpoint](../docs/voyage-json-save-checkpoint.md)
records canonical publishers, manifests, API/GUI version semantics and the
public Test-UAssetGuiJsonRoundtrip / Invoke-VoyageBoundedTool interfaces.
Normal Voyage extraction uses UE5_8; UE5_7 is an explicit legacy profile.

This directory contains reproducible methods for inspecting **The Last
Caretaker** (`Voyage`). Game-derived outputs belong under ignored `artifacts/`
directories; the tools and the conclusions derived from them belong in Git.

Agents should start here instead of reading every implementation file.

## Low-cost normal path

The table and documented commands are the public interface of this toolset.
On the normal path, choose a tool by intent, run it unchanged, and judge the
result from its exit code, manifest, summary, and produced files. Do not first
survey the script, re-derive its dependencies, or manually reproduce its
steps.

The goal is not to maximize wrappers. It is to minimize agent work and context
consumption for predictable operations. A good normal path usually means one
intent-level call, a small structured result, and links/paths to complete local
evidence. A tool that prints a large asset or log into the conversation, forces
manual searching of its output, or requires callers to understand its internals
has not met that goal merely because it is called a black box. Detailed logs,
full JSON and inventories should be written under ignored artifacts and opened
only as narrowly as the task or a failure requires. Never trade away validation,
provenance, fail-closed behavior, or recoverability merely to reduce tokens.

Open the implementation or a third-party fork only after a non-zero exit, a
crash/dialog, a hang, an unexpected output, or a case the documented interface
does not cover. Before changing code, inspect the tool's own diagnostics and
verify its fingerprinted inputs, paths, mapping gate, and dependency commit.
The accepted compatibility checkpoints and validation boundaries are recorded
in [`../docs/voyage-cooked-asset-toolchain.md`](../docs/voyage-cooked-asset-toolchain.md).

## Coding-agent report contract

Every coding-agent handoff must include this compact evidence block:

```text
Pipeline operations: <intent -> public tool -> result/evidence>
Worked: <successful paths and outputs>
Failed/unexpected: <symptom, diagnostic path, and classification>
Fallback/internal inspection: <what was opened and which allowed trigger fired>
Validation: <tests, package checks, or real-game status>
Coverage: <public-tool operations>/<eligible recurring operations> = <percent>
Routine cost: <public calls; implementation/dependency files opened; repeated or oversized output>
Reusable gap: <missing or insufficient interface, or none>
```

An eligible operation is a recurring mechanical action in **game-asset work
or release preparation**: game fingerprinting, mapping selection/generation,
asset retrieval/inspection/modification/validation, or release build, cook,
package, verification, install and restore. Source-only modeling, image/video
production and unrelated development are excluded by the user's narrowed scope.
Import/cook/package steps that put content into an actual game release still
count; creating or rendering the source model does not.
Reasoning, code editing,
documentation, and novel diagnosis after a black-box failure are not counted.
The target is at least 80% public-tool coverage. A fallback is evidence of a
coverage gap, not a new normal workflow: add it to
`../docs/voyage-toolchain-backlog.md` when it can recur.

Reports inform prioritization; they do not automatically authorize tool
implementation. Fix current workflow failures first, then remove demonstrated
recurring friction. The target is 80%, not exhaustive automation of every
historical activity. Keep deferred gaps visible without starting unrelated
work. Preserve old measurements as superseded history when the user changes
scope; recompute the scoped sample explicitly, not as an apparent improvement.
Read the routing table and the selected tool's contract; unrelated recipes and
implementation files are not mandatory reading on a successful supported path.

Use `Routine cost` to expose avoidable effort, not to reward under-validation.
On a supported path, the expected implementation/dependency-file count is zero.
Count intent-level public calls rather than hidden subprocesses. Note repeated
calls caused by an unclear interface and any large output that had to be read
or filtered manually. Exact model-token accounting is neither required nor
reliably reconstructable; these observable proxies guide improvements.

Count each requested operation once, not every command, retry, internal phase,
or test assertion. A documented tool retry that completes the operation remains
tool-covered; an operation completed by an ad-hoc replacement is uncovered.
Report an empty denominator as `N/A`, not 100%, and mark unreconstructable
historical operations `unknown` rather than silently dropping them. Synthetic
tool-development tests prove the tool's behavior, not adoption by feature
agents: keep their coverage separate from real-task reports. Attach the task
identity and iteration boundary so the same activity is not counted twice.
Public entry points include documented canonical CLI commands, not only
PowerShell wrappers. Keep real-task samples and their limits in
`../docs/voyage-toolchain-coverage.md`; do not equate tool existence with adoption.

## Choose a tool

ScopeFix's fingerprint-bound two-field candidate builder is documented in
[`../mods/ScopeFix/README.md`](../mods/ScopeFix/README.md). Its public entry
point is `mods/ScopeFix/Build-ScopeFix.ps1`; it creates a fresh verified
container and evidence, without installation.

| Goal | Start with | Result |
| --- | --- | --- |
| Run a potentially failing native diagnostic with resource limits | `Invoke-VoyageBoundedTool.ps1` | Time/memory-bounded process tree, separate logs, exit and peak-memory report |
| Identify the installed game build | `Get-VoyageBuildFingerprint.ps1` | Steam build ID, executable hash, and container metadata/hashes |
| Inspect installed containers and running game processes without mutation | `Get-VoyageInstallationStatus.ps1` | JSON inventory, process snapshot, optional installed-manifest hash comparison |
| Get one cooked asset as JSON or list every package | `Get-VoyageAssetJson.ps1` | Validated JSON or package-list path; game storage and reuse are automatic |
| Summarize one Blueprint without reading its full JSON | `Get-VoyageAssetSummary.ps1` | Compact focused structure plus a complete summary path |
| Find, list, or structurally inspect cooked assets | `Inspect-VoyageAsset.ps1` | Paths, JSON exports, Blueprint pseudocode, or mapping reports |
| Publish or validate the Inspector executable | `Publish-VoyageAssetInspectorBinary.ps1`, `Get-VoyageAssetInspectorBinary.ps1` | Stable single-file EXE; validated source/dependency/binary identity |
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
| Publish or resolve the surgical Voyage asset patcher | `Publish-VoyageAssetPatcherBinary.ps1`, `Get-VoyageAssetPatcherBinary.ps1` | Manifest-validated single-file `.tools/bin/VoyageAssetPatcher.exe` |
| Publish or resolve the native executable inspector | `Publish-VoyageExecutableInspectorBinary.ps1`, `Get-VoyageExecutableInspectorBinary.ps1` | Manifest-validated single-file `.tools/bin/VoyageExecutableInspector.exe` |
| Stress-test hierarchy asset opens in patched UAssetGUI | `.tools/bin/UAssetGUI.exe stress-open` | Incremental per-asset JSONL plus parse/binary-equality summary |
| Build, cook and package an existing mod release | [Release producers](#release-producers) | Route to the owning mod's documented orchestrator; no generic rebuild recipe |
| Create the common manifest for an already-built triplet and ZIP | `New-VoyageReleaseManifest.ps1` | Immutable schema-2 manifest published only after installer validation |
| Validate or install an already-built standalone IoStore release | `Install-VoyageRelease.ps1` | Manifest-gated install plan or recoverable installation evidence |
| Verify one IoStore container and its expected package set | `Test-VoyageContainer.ps1` | Bounded integrity check, package inventory, exact-set differences and file hashes |
| Restore/remove a common release installation | `Restore-VoyageReleaseInstallation.ps1` | Hash-guarded predecessor restoration and recovery evidence |
| Install/remove one unchanged package canary | `Install-VoyageUnchangedProbe.ps1`, `Remove-VoyageUnchangedProbe.ps1` | Current-fingerprint and exact-hash guarded runtime roundtrip test |
| Locate native names, references, or correlated member offsets | `Invoke-VoyageExecutableInspector.ps1` | Compact fingerprinted result plus retained read-only executable report |
| Reproduce one of the existing surgical cooked-asset probes | `Invoke-VoyageAssetPatcher.ps1` | Manifest-validated patcher, reviewed current mapping, compact output hashes, and full log path |
| Discover which Blueprint editor APIs Unreal Python exposes | `Inspect-UnrealBlueprintApi.py` | `Saved/BlueprintApi.txt` in an Unreal project |
| Reuse semantic C++ names while generating Blueprint graphs | `UnrealEditorGeneratorCommon` | Header-only build-time helpers; not a command-line tool |

`VoyageAssetInspector` is the CUE4Parse backend used by
`Inspect-VoyageAsset.ps1`. Prefer the PowerShell wrapper because it adds the
game fingerprint, a versioned output directory, and an inspection manifest.


## Release producers

Build/cook/package contracts remain with their owning mods. Use these routes
before searching for scripts or assembling Unreal/retoc commands manually:

| Producer | Public workflow and owning rules | Output / boundary |
| --- | --- | --- |
| DonkLiftKeyboardControl | [One-command release](../mods/DonkLiftKeyboardControl/README.md#one-command-release), [rules](../mods/DonkLiftKeyboardControl/AGENTS.md) | `Build-DonkLiftRelease.ps1` owns build, generation, cook, extraction, package verification, ZIP and schema-2 release manifest |
| BoatHUDTotalResources | [Build and install contracts](../mods/BoatHUDTotalResources/README.md#build), [rules](../mods/BoatHUDTotalResources/AGENTS.md) | Documented prepare/build stages produce a verified container; installation/removal uses the mod-owned evidence contract |

Read only the selected producer's rules and workflow. These links are routing,
not permission to build/install, evidence of current-game compatibility, or a
request to revisit the mod's gameplay design. Run its fingerprint/provenance
gates before reusing inputs. A development probe or another mod's builder is
not a substitute release producer.

Do not assume that omitting `-Install` makes every preparation phase safe while
the game runs: the documented original-preparation workflows can temporarily
disable installed containers and require a closed game. Respect the selected
producer's preconditions. Do not manually bypass its source or manifest gates.

For an already-built release, skip build/cook and use the common verification
and installation contracts below. The common installer accepts schema-2 release
manifests, not every producer's build manifest. A producer without that schema
keeps its documented installer until an explicit migration is implemented and
validated. Keep exact commands and stage-specific details in the owning README,
not duplicated here.

## Release container verification

The common `Install-VoyageRelease.ps1` / `Restore-VoyageReleaseInstallation.ps1`
pair also supports one optional `.autoload` payload alongside the triplet.
Its case-sensitive name must equal the container basename plus `.autoload`.
Declare it in `payload` and include identical bytes in the release ZIP; empty
files are valid. It participates in the same backup, hash, rollback and restore
transaction and is reported as kind `autoload`. The installer does not parse
its contents or claim runtime loading. Test both the legacy contract and this
extension with Windows PowerShell 5.1 using `Test-Install-VoyageRelease.ps1`
and the same command with `-WithAutoLoadSidecar`.

```powershell
$check = & .\tools\Test-VoyageContainer.ps1 -Container '<exact .utoc>'
$check = & .\tools\Test-VoyageContainer.ps1 -Container '<exact .utoc>' `
    -ExpectedPackageList '<release-owned expected-paths.txt>'
```

Uses the manifest-validated canonical `.tools/bin/retoc.exe`; never rebuilds a
fork, mounts the game directory, extracts assets or installs files. Explicit
`-Retoc <candidate.exe>` is for deliberate tool development and is identified
as noncanonical in the report. `verify` and `list --path` each run through
`Invoke-VoyageBoundedTool.ps1` (default 1024 MiB, 60 seconds per call; override
with `-MemoryLimitMB`/`-TimeoutSeconds`).

The returned PowerShell object contains `status`, `packageCount`, nullable
`packageSetMatches`, `reportPath`, `packageListPath` and `error`. Read the full
JSON report only for details: native-run evidence paths, exact file and retoc
hashes, chunk types/counts, missing/unexpected packages and failure reason.
Ignored output lives under `artifacts/container-checks/<run>/`; the package
list contains sorted relative asset paths, one per line. Default failures
throw after returning/storing evidence. `-AllowFailure` returns failed status
without throwing; preflight input/manifest errors still throw. It does not
turn a failed result into a passing check.

Expected paths are exact, case-sensitive container paths such as
`Voyage/Content/Blueprints/Example.uasset`, with `.uasset` or `.umap` extension.
An optional `../../../` prefix and backslash separators are normalized. Blank
lines are ignored; duplicates, parent traversal, `/Game` aliases, and pipe-
separated inventory rows are rejected. Supply an independently maintained
release/build contract: using the check's own returned list as its expectation
is circular and cannot establish that the release contains the intended assets.
Without expectations `packageSetMatches` is null, not a claim of correct scope.

The integrity gate covers retoc's IoStore verification and exact
`ExportBundleData` package paths; all chunk types are reported. Matching `.utoc`,
`.ucas`, `_sN.ucas` partitions and optional `.pak` are hashed before/after.
The PAK itself is fingerprinted only, NOT parsed/verified by this gate. A pass
does not establish UObject parsing, dependency resolution, mount precedence,
game-fingerprint compatibility or gameplay correctness. Source/retoc changes
during a successful check invalidate it; no atomic snapshot is claimed.

Regression: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File
tools/Test-VoyageContainerTool.ps1 -KnownGoodContainer <exact.utoc>`.
It copies that container family to ignored test output, then exercises expected
sets and corrupts only its copy. The test's derived expectation proves the
comparator, not independent release acceptance. Original files are hash-checked
afterward. Use a provenance-validated input appropriate for the test.

## Read-only installation status

```powershell
& .\tools\Get-VoyageInstallationStatus.ps1
& .\tools\Get-VoyageInstallationStatus.ps1 -Summary
& .\tools\Get-VoyageInstallationStatus.ps1 -HashModFiles
& .\tools\Get-VoyageInstallationStatus.ps1 -InstallManifest '<returned installManifestPath>'
```

Returns JSON on stdout; does not create evidence, launch/stop processes, build,
install, restore, or write game files. `-GameRoot` defaults to the installed
Voyage location used by the other public tools. The result contains build/exe
identity, observed Voyage process IDs/paths, and top-level Paks `.pak`, `.utoc`,
`.ucas`, and `.zip` metadata. `stock-name` is only a filename classification;
`additional` does not prove the file is mounted. Subdirectories are explicitly
returned as `unscannedSubdirectories`: this is not a recursive mod-loader or
runtime mount inventory. Use asset inventory/extraction tools for package ownership.

Use `-Summary` for the ordinary readiness/readback question. It emits one
compact JSON object containing build identity, process/file counts, additional
top-level files, and condensed installation match verdicts without the full
stock-file, process, or per-manifest-file arrays. Rerun without `-Summary` only
when those details are actually needed. This tool already returns JSON; unlike
`Get-VoyageAssetJson.ps1`, it does not need `-AsJson`.

`-HashModFiles` adds SHA-256 for additional top-level files. An optional completed
common installation manifest (schema 1, returned by `Install-VoyageRelease.ps1`)
always hashes its exact installed targets, including the provenance ZIP. It
reports `match`, `different`, `missing`, `unsupported-path`, or
`changed-during-read` per file, and separates `filesMatch` from
`gameFingerprintMatches`. No manifest means `installation: null`, not failure.
Legacy/mod-specific manifests are not silently interpreted as the common schema.

Exit zero means the snapshot was obtained, not that files matched; consumers
must inspect the returned states. Invalid inputs/read failures throw. The
snapshot is not atomic and process/path visibility can race or be unavailable;
it never replaces an installer's immediate closed-game/hash gates, verifies
container contents, or establishes gameplay compatibility.

Regression: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File
tools/Test-VoyageInstallationStatus.ps1`. Synthetic fixtures/results remain
under ignored `artifacts/tests/installation-status-*/`.

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
  `UAssetGUI.exe`, `retoc.exe`, `jmap_dumper.exe`, `VoyageAssetInspector.exe`,
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
- C# tools currently target .NET 10. Normal `VoyageAssetPatcher`,
  `VoyageAssetInspector`, and `VoyageExecutableInspector` calls resolve their
  manifest-validated single-file executables under `.tools/bin/`; they never
  restore or build. Asset-tool source projects reference the canonical
  UAssetAPI and CUE4Parse bundles, and their override properties exist only for
  deliberate tool development.
- `Get-VoyageAssetJson.ps1` and `Inspect-VoyageAsset.ps1` resolve the canonical
  Inspector EXE and invoke it directly. Normal calls never build, restore, or
  read NuGet user configuration; missing/stale binaries stop with the explicit
  publisher command. Publication alone requires the .NET SDK and normal
  NuGet access. A publish-time `NuGet.Config` access denial is an environment
  failure, not asset or mapping evidence.
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
  `Extract-VoyagePackage.ps1` defaults to `-Source Game`; `-Source Mod` needs
  one exact `-ModContainer`. It never falls back to a mixed view silently.
  Container isolation requires retoc's explicit-set capability (rollout gate
  below). `Get-VoyageAssetJson.ps1` already supports isolated sources through
  the published Inspector. Do not remove installed mods for extraction.
- Never overwrite an extracted source asset. Write transformed output to a new
  ignored directory, preserve a known-good installed package, and do not
  replace installed files while the game is running.

### Release installation

For an already-built triplet and ZIP, create the common immutable schema-2
manifest through the producer rather than assembling JSON or inspecting the
installer implementation:

```powershell
.\tools\New-VoyageReleaseManifest.ps1 `
  -ReleaseRoot '.\artifacts\releases\Example-v1' `
  -Mod 'Example' `
  -Version 'v1' `
  -Container '.\artifacts\releases\Example-v1\package\Example_P.utoc' `
  -Archive '.\artifacts\releases\Example-v1\Example-v1.zip' `
  -SourcePath '.\mods\Example'
```

The producer fingerprints the installed game, records the exact source scope
and its Git state, hashes the triplet, optional matching `.autoload`, and ZIP,
then calls `Install-VoyageRelease.ps1 -ValidateOnly`. It publishes
`release-manifest.json` only after the installer accepts the archive/payload,
fingerprint, source commit, and dirty-source contract. Existing manifests are
immutable. A dirty test candidate needs explicit `-AllowDirtySource`; a normal
release should be committed first. `-InstalledArchiveName` can override the
default installed provenance ZIP name, and `-AsJson` returns a compact result.

Run its Windows PowerShell regression harness after changing this producer:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\Test-New-VoyageReleaseManifest.ps1
```

Use the common install-only wrapper for an already built standalone IoStore
release. It never builds, cooks, repackages, or edits the source release:

```powershell
.\tools\Install-VoyageRelease.ps1 `
  -ReleaseManifest '.\artifacts\releases\Example-v1\release-manifest.json' `
  -ValidateOnly
```

The accepted input is `release-manifest.json` schema 2. New release producers
should call `New-VoyageReleaseManifest.ps1` after their build and ZIP steps
rather than duplicating this contract or passing an unrelated build manifest.

`-ValidateOnly` verifies the clean source commit, exact manifest-owned
`.pak/.ucas/.utoc` triplet and ZIP, matching triplet contents inside the archive,
and current Steam
build/executable fingerprint, then returns the planned destinations without
creating evidence or touching the Paks directory. Remove `-ValidateOnly` only
for an explicitly authorized installation while the game is closed.

During installation the wrapper stages and hashes every source in the Paks
directory, backs up all existing targets before replacing any of them, checks
the process again immediately before replacement, reads every installed hash
back, and writes `install-transaction.json` plus `install-manifest.json` below
ignored `artifacts/installations/`. A handled failure restores all previous
files and removes newly installed targets; the transaction records whether the
rollback succeeded. A dirty-source manifest is rejected unless the caller
explicitly supplies `-AllowDirtySource`.

Restore the exact predecessor through the paired tool rather than copying the
backup manually:

```powershell
.\tools\Restore-VoyageReleaseInstallation.ps1 `
  -InstallManifest '<returned install-manifest.json>' `
  -ValidateOnly
```

Remove `-ValidateOnly` for an authorized restore. It refuses changed installed
files or damaged/out-of-evidence backups before mutation, restores previous
files, removes only manifest-owned files that did not previously exist, and
rolls back a handled restoration failure to the installed state. A changed game
build does not block removal: ownership and exact file hashes, not a runtime
compatibility allowlist, are the removal gate. The original release and
installation manifests remain immutable; restoration gets its own transaction
and result manifest beside the installation evidence.

If either transaction reports `recovery-failed`, stop normal install/restore
work: backups and remaining staging copies are intentionally retained for
recovery. Do not delete them or blindly retry. Abruptly interrupted transactions
likewise require diagnosis; the restore command accepts a completed installation
manifest, not an unfinished transaction.

For standalone IoStore mods whose tested installation contract allows a ZIP in
the Paks directory, retaining the exact release ZIP beside the installed
container is accepted provenance: Voyage ignores the archive while it remains
available to identify and recover the installed release. The ZIP name and
installation manifest must carry the artifact version, not merely the mod
version. This is a provenance copy of a validated release artifact, not a ZIP
that should be regenerated during installation.

Run the Windows PowerShell regression harness after changing this contract:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\Test-Install-VoyageRelease.ps1
```

It uses a unique synthetic Steam/Voyage tree under ignored `artifacts/tests/`
and covers validate-only, successful installation, previous-file backup,
installed hash readback, successful predecessor restoration, removal of new
files, injected mid-install and mid-restore failures, and complete rollback in
both directions. It also rejects a mismatched fingerprint, ZIP/payload mismatch,
directory target, changed installed file, and damaged backup, and verifies that
recovery copies survive a failed rollback. `-KeepArtifacts` retains the complete
synthetic evidence tree; otherwise it is removed after the run.

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

`-Source Game` is the default and mounts only stock `global` and `pakchunk*`
containers, regardless of installed mods. This is the normal research mode.
For exceptional debugging against one installed mod, use `-Source Mod
-ModContainer <exact.utoc>`; it mounts stock dependencies plus only that exact
additional container and records its path, size and hash in the inspection
manifest. The selected `.utoc` must be directly inside the game's Paks folder.
Use `Get-VoyageAssetJson.ps1 -Source Mod` when ownership of one exact returned
asset must first be proven; a broad inspection query does not itself establish
which duplicate provider owns an identity.

For a non-installed container that must be resolved together with stock game
dependencies, call the underlying inspector with the game Paks directory as
the primary input and the test package directory as the optional sixth
argument. Both directories remain read-only:

```powershell
.\.tools\bin\VoyageAssetInspector.exe `
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
`UE5_8`, matching the current Voyage parser workflow. Use `UE5_7` only for an
explicit older version-bound investigation with matching provenance.

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

A reference search with zero matches is a successful negative result. The
wrapper returns `status = no-match`, `matchCount = 0`, and paths to the empty
result plus its manifest; it does not disguise absence as an Inspector failure.
Use `-RequireMatch` when absence is itself a failed assertion. Per-package parse
errors remain failures in both modes, so `no-match` never means "nothing could
be parsed".

Every query writes to a new fingerprinted directory and refuses to overwrite a
previous result. Choose a new `-OutputRoot` for a repeated investigation.

### Compact Blueprint structure

Use `Get-VoyageAssetSummary.ps1` before opening or searching a full asset JSON
when the question is about a Blueprint's class, functions, calls, externally
owned members, soft-object constants, or SCS components:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File `
  tools\Get-VoyageAssetSummary.ps1 `
  -Query '/Game/Blueprints/Modules/Utility/Weapons/BP_Module_Turret' `
  -Focus Overview -AsJson

powershell.exe -NoProfile -ExecutionPolicy Bypass -File `
  tools\Get-VoyageAssetSummary.ps1 `
  -Query '/Game/Blueprints/Modules/Utility/Weapons/BP_Module_Turret' `
  -Focus Calls -FunctionName GetInteractiveProvidedActions -AsJson
```

`Overview` is the low-cost default. Other focuses are `Functions`, `Calls`,
`Members`, `SoftObjects`, `Components`, and explicit `All`. With `Functions`,
`Calls`, `Members`, or `SoftObjects`, optional `-FunctionName` selects one exact
serialized function and fails clearly when it does not exist. The compact
result carries the generated-class overview or selected records, total counts,
`resultCount`, and a hash-verified `summaryPath`; the complete summary contains
all focus families and their source-function correlations. Normal repeated
game calls reuse a content-addressed derived summary. Explicit `-Source Mod
-ModContainer <exact.utoc>` remains a one-off uncached diagnostic, matching the
underlying asset-retrieval contract.

This is a cooked-bytecode summary, not a native runtime call graph, ownership/
lifecycle proof, or provider-precedence model. It reports serialized function
and property flags only when present and uses `unknown` for missing member
access flags. Do not infer writability or runtime dispatch from absence. Use
`Get-VoyageAssetJson.ps1` only when the complete serialized export is necessary.

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
For a compact machine-readable result from a direct PowerShell `-File` call,
add `-AsJson`; this writes one compact JSON object with the complete paths and
hashes, avoiding console table truncation or a second `ConvertTo-Json` pipeline.
The option applies to both package-list and individual-asset results.

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

JSON retrieval and optional Blueprint pseudocode formatting are separate
outcomes. If the Inspector has already serialized one exact, structurally valid
JSON file and only `DecompileBlueprintToPseudo` fails, the wrapper returns the
JSON with `pseudocodeStatus = unavailable` and a short `pseudocodeError` instead
of failing the requested operation. Any load, export, JSON-validation, ambiguous
match, or differently classified Inspector error remains fatal. Call
`Inspect-VoyageAsset.ps1` when pseudocode itself is the required result.
Older otherwise-valid cache entries without that optional field return
`pseudocodeStatus = unknown`, never an ambiguous empty string.

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
path/hash/profile, selected container paths/hashes/priority, source mode, and
extracted asset hashes. The result object returns `outputPath`, `manifestPath`,
`source`, and `assetCount`. Failure logs remain in the output directory and do
not receive a success manifest. Shader conversion is disabled for this
package-extraction entry point.

Source modes:

- `Game` mounts only stock global/package containers, ignoring installed mods.
- `Mod -ModContainer <exact.utoc>` proves package ownership through the public
  mod inventory, then mounts global, the selected mod, and stock dependencies
  in that priority order. The selected mod wins even without `_P` in its name.
  Unrelated mods never participate. Output is diagnostic and never cached.

Canonical retoc `49b7721` provides `--include-container` and the repaired import reader. Published source-isolation and four-case import regressions passed; see the accepted JSON save checkpoint. Use the bounded runner for regression diagnostics. `-AllowAdditionalContainers` remains a
deprecated, explicit combined-view diagnostic control (manifest source
`InstalledLegacy`); never treat its output as stock-only evidence. It cannot
be combined with an explicit `-Source`.

Tool-development integration test (read-only against the game):

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\Test-VoyageExtractionSources.ps1 `
  -Retoc '<candidate retoc.exe>' `
  -ModContainer '<installed single-partition override mod.utoc>' `
  -OverrideFilter 'UI/Game/HUD/BP_VoyageIngameBoatHud'
```

Supply a real differing stock override fixture. The test checks stock/mod
separation, renamed-mod priority, stock-only provenance, unchanged candidate
serialization versus canonical retoc after excluding competing overrides,
Windows PowerShell default-path binding, and rejection of unowned queries.
Its copied mod fixture and reports stay below ignored `artifacts/tests/`;
it does not install anything or replace the canonical binary. Synthetic retoc
unit tests separately exercise broken unselected containers, duplicates,
missing paths, empty selection, version mismatch, and unchanged directory
priority. Passing these checks is not gameplay validation.

## Specialized tools

### retoc import compatibility regression

Canonical retoc `49b7721` accepts explicit `UE5_8` for Voyage. UE5_7 preserves the pre-fork filtered-import writer. Shared object/container version values do not identify that loose layout; use an explicit engine selector.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\Test-RetocImportCompatibility.ps1 `
  -CandidateRetoc '<candidate retoc.exe>' `
  -UpstreamRetoc '<recorded upstream 885a8da retoc.exe>' `
  -LegacyRetoc '<preserved 234f4e5 retoc.exe>'
```

This development test pins the two control executable hashes and the reviewed
Steam-25056839 Dismantle fixture. It rejects other installed mods owning that
asset before fresh legacy-control extraction. It checks candidate writer byte
identity at `UE5_8`, and byte identity with upstream at explicit `UE5_7`, then
packages all four producer/profile cases through the candidate using bounded
invocations, verifies each container, and asserts one exact asset plus the
container header. Raw Zen bytes must agree except CookedHeaderSize, whose
version-bound field and value are independently checked against the legacy
source. Nothing is installed or published. A changed fingerprint/control
requires deliberate revalidation, not silently accepting the new input.
The compact result returns the full `reportPath` under ignored `artifacts/`.
This is compatibility evidence for the candidate reader, not permission to
use upstream readers on the fork's expanded output or to claim runtime success.

### GUI full-JSON regression

`Test-UAssetGuiJsonRoundtrip.ps1 -CandidateGui <exe> -InputAsset <uasset>
-Mappings <usmap> -EngineVersion 5.8 -TestCanPlace` exercises a development
GUI executable's `tojson`/`fromjson` commands through the bounded runner.
Run its public `-File` entry point with Windows PowerShell 5.1. The input
must already have matching provenance and parse fully. `-TestCanPlace` is
specific to `BP_FabricationPlacementComponent.GenerateAndSetLocation`;
omit it for other inputs. All outputs and the returned `reportPath` stay in
a unique ignored `artifacts/tests/gui-json-*` directory.

Cases cover unchanged JSON, obsolete candidate JSON containing an ignored
engine hint, and optionally the `CanPlace = True` edit. Every write explicitly
supplies the engine version. The test
asserts no RawExport, preserved object/custom versions, unchanged source,
unchanged `.uexp` for no-op cases, and persistence of the changed expression.
Whole-file byte equality is reported separately: UAssetAPI intentionally
excludes `OverrideNameMapHashes` from JSON, so recomputed name hashes can
differ. This does not constitute GUI-click or gameplay validation.

Candidate `fromjson` syntax is `fromjson <json> <uasset> <mapping> [5.8]`;
the final engine hint selects binary serialization without resetting header
versions. Supply it for Voyage (5.8); JSON never carries SpecifiedEngineVersion.
GUI applies its dropdown selection immediately before binary Save. Candidate GUI builds can
set `-p:RetocResource=<absolute gzip path>` to test a specific retoc resource
without replacing canonical tools; normal builds use the bundled resource.

### Bounded diagnostic execution

Use the common runner before repeating a tool that hangs, crashes, or grows
memory unexpectedly:

```powershell
.\tools\Invoke-VoyageBoundedTool.ps1 `
  -Executable '.\.tools\bin\retoc.exe' `
  -Arguments @('--version') `
  -MemoryLimitMB 256 -TimeoutSeconds 10
```

`-Executable` is an existing `.exe`, not a shell command; `-Arguments` is an
array of literal arguments. Optional `-WorkingDirectory` defaults to the
repository root. Defaults are 1024 MiB and 60 seconds. Use a 64-bit Windows
PowerShell host. The small Win32 interop helper is loaded through PowerShell
`Add-Type`; no SDK project, restore, NuGet configuration, or fork rebuild is
involved. Changed helper source requires a fresh PowerShell host.

The tool creates a suspended process, assigns a Windows Job Object before
resuming it, and applies both per-process and aggregate-job committed-memory
limits. Its inherited handles are restricted to stdin/stdout/stderr; windows
are hidden. Timeout terminates the owned job, and background descendants are
also terminated when the root process returns or the runner host exits.
Failure to establish the job is a launch failure, never an unbounded fallback.
This follows the Windows [job-limit contract](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-jobobject_extended_limit_information)
and [restricted handle inheritance](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute).

Consume `status`, `exitCode` when present, `peakJobMemoryBytes`, `stdoutPath`,
`stderrPath`, and `reportPath`. Evidence is retained under ignored
`artifacts/tool-runs/`. Status is `passed`, `exit-failed`, `timeout`, or
`launch-failed`. A nonzero native exit is not automatically classified as a
memory failure: inspect stderr and the supplied limits. Failure throws by
default; use `-AllowFailure` only when inspecting an expected failure and then
check the returned status explicitly. The JSON records literal arguments, so
do not pass secrets there. Logs are files, not capped buffers; use an appropriate
time limit. This is resource containment, not a filesystem/network sandbox and
not authorization to install, delete, or change anything.

Verify with `powershell.exe -NoProfile -ExecutionPolicy Bypass -File
tools\Test-VoyageBoundedTool.ps1`. Its synthetic probe checks literal argument
encoding, both output streams, native failure/default throw, per-process and
aggregate-child memory denial, timeout cleanup, orphan prevention, and invalid
executables. It does not touch the game or canonical binaries.

### Canonical VoyageAssetInspector executable

```powershell
.\tools\Publish-VoyageAssetInspectorBinary.ps1
.\tools\Get-VoyageAssetInspectorBinary.ps1
```

The publisher produces `.tools/bin/VoyageAssetInspector.exe` plus its sibling
`VoyageAssetInspector.publish-manifest.json`. It requires committed, clean
Inspector source and the accepted CUE4Parse bundle. It publishes a
framework-dependent `win-x64` single file, smoke-tests it, records source,
dependency and output hashes, and preserves a current binary unchanged
(`Rebuilt = False`). It does not require unrelated mod work to be clean.
The executable needs the .NET 10 runtime; mappings remain external.

The read-only resolver checks current build-input fingerprints and binary
hash/size; it never builds or restores. Both public inspection wrappers call
it automatically. Clients do not need to locate the manifest or dependencies.
The internal `-BuildInputsOnly` publisher interface is not an asset-analysis
entry point. Explicit publication is needed only for changed inputs or a
missing/invalid binary, not for a new query or game update alone.

Verify this boundary with the installed game (read-only):

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\Test-VoyageAssetInspectorBinary.ps1
```

The test intercepts `dotnet`/`dotnet.exe` and fails if a supported runtime path
tries to invoke them. It checks publisher reuse, a narrow legacy query, both
reference-search absence contracts, public stock inventory and compact JSON
output, formatter-independent asset JSON, compact Blueprint overview/exact-
function focus, plus normal JSON retrieval and reuse.
Diagnostic output is retained under ignored `artifacts/tests/`; game files and
mapping registry are never changed.
This is tool validation, not new coding-agent adoption evidence.

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
.\tools\Invoke-VoyageExecutableInspector.ps1 `
  -Query VoyageInputControlsComponent,GetProvidedActions `
  -WindowBytes 8192
```

The wrapper resolves the manifest-validated canonical EXE, fingerprints the
installed game, refuses to overwrite a report, and returns only the match count,
hashes, game identity, report/log paths, and analysis boundary. Use
`-TargetVirtualAddress` or `-MemberOffset` for their respective correlation
modes. A zero-result string query returns structured `no-match` unless
`-RequireMatch` is selected.

This output is a correlation aid: names, bytes, pointers, and offsets do not
prove reflected ownership, call relations, or lifecycle. See
[`VoyageExecutableInspector/README.md`](VoyageExecutableInspector/README.md)
for details. Publish only after an intentional committed source change, then
run the Windows PowerShell regression:

```powershell
.\tools\Publish-VoyageExecutableInspectorBinary.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\Test-VoyageExecutableInspectorBinary.ps1
```

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
.\tools\Invoke-VoyageAssetPatcher.ps1 `
  -Operation break-bottom-action-filter `
  -InputAsset '<input.uasset>' `
  -OutputAsset '<new-output.uasset>'
```

The input and its companion files must come from the matching game build, and
the output must be a different path. See
[`VoyageAssetPatcher/README.md`](VoyageAssetPatcher/README.md) for the required
UAssetAPI checkpoint and the exact assertions of each operation. Normal builds
use canonical `.tools/bin/UAssetAPI/`; normal execution calls
`Invoke-VoyageAssetPatcher.ps1`, which resolves the published EXE and current
reviewed mapping, refuses an existing output, retains the detailed log, and
returns compact file hashes. An explicit mapping is intended for a reviewed
legacy/diagnostic case; UE5_7 requires one. Pass
`-p:UAssetApiProject=<path>` only while deliberately developing and validating
a replacement UAssetAPI checkpoint.

Publish after an intentional committed source or accepted dependency change,
then run the Windows PowerShell regression:

```powershell
.\tools\Publish-VoyageAssetPatcherBinary.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\tools\Test-VoyageAssetPatcherBinary.ps1
```

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

### Fork RC3 source tags

The pinned API preparation and retoc build scripts accept either the recorded
pre-tag git description or the corresponding RC3 tag for the same required
commit. Commit and critical-source-hash gates remain unchanged. Published
GitHub assets and verification are recorded in
[the RC3 checkpoint](../docs/voyage-json-save-checkpoint.md#github-rc3-publication).
