# Smart Outlet build and authoring

`Project/Voyage.uproject` owns release assets and editor dependencies.
`Project/BlueprintText/` mirrors `/Game/Mods/SmartOutlet/` for review; these are
synchronized source files, not build inputs. `Assets/SmartOutlet.ini` is the
distributable settings file. Mod-level `Settings/` owns settings-template
validation; Unreal fixtures live in `Project/Tests/`.

## Build and validation

[Build-SmartOutlet.ps1](Build-SmartOutlet.ps1) and
[Test-SmartOutlet.ps1](Test-SmartOutlet.ps1) create fresh working projects
through the [shared pipeline](../../tools/mod-setup.md#shared-mod-pipeline)
from current source, prepare their native/BPT dependencies and run Unreal only
in those copies. Both accept `-EngineRoot` and a fresh `-OutputRoot` below
`Tmp/SmartOutlet/`. They delete the working project after success, retain useful
results/logs, and leave failed runs available for diagnosis. Build also accepts
`-GameRoot`, `-IgnorePinnedGameVersion` and `-AllowDirtySource` under the
[shared release workflow](../../docs/release-and-installation-rule.md).

Wrap either command in the [Unreal queue](../../tools/unreal-coordination.md).
Run from the repository root. To build a candidate:

```powershell
$shell = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
./tools/Invoke-UnrealQueued.ps1 -Operation 'Build Smart Outlet' `
  -FilePath (Join-Path $PSHOME $shell) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/SmartOutlet/Build-SmartOutlet.ps1')
```

Add `-AllowDirtySource` to the producer's arguments for uncommitted source.
Building does not install the candidate. Its result has `status: built`,
`archivePath`, `releaseManifestPath` and `validationSummaryPath`.
For editor checks:

```powershell
$shell = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
./tools/Invoke-UnrealQueued.ps1 -Operation 'Test Smart Outlet' `
  -FilePath (Join-Path $PSHOME $shell) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/SmartOutlet/Test-SmartOutlet.ps1')
```

The test runner returns `reportPath` for its Blueprint suites. Missing/failed
reports stop the command. See the [validation boundary](#validation-boundary)
for coverage and required game checks.

## Installation and settings

Use [Install-SmartOutlet.ps1](Install-SmartOutlet.ps1) with
`-ReleaseManifest <candidate manifest>` for the coupled container/INI operation;
pass `-AllowDirtySource` for a dirty candidate.

An existing INI is left untouched while the shared installer updates containers.
If the INI is absent, the mod installer creates it from the candidate ZIP.
`-ResetSettings` explicitly replaces it with that candidate's defaults.
Creation/reset keeps container recovery data and, when replacing an INI, its
backup until both writes succeed. Successful operations remove these temporary
files; failed operations report their recovery location.

## Authoring

Create a working project without launching Unreal:

```powershell
./mods/SmartOutlet/New-SmartOutletProjectCopy.ps1 `
  -Destination './Tmp/SmartOutlet/authoring-ports/Project'
```

The destination must be fresh. The copy includes current uncommitted source,
excludes `Intermediate/`, and rejects unexpected top-level entries and linked
inputs. Use the returned `.uproject` with the shared
[BPT workflow](../../tools/blueprint-workspace.md) or queued editor session.
Preparation supplies declared dependencies; builds do not reconstruct assets.

Before transferring authoring changes back, compare the maintained sources
with the starting snapshot and resolve concurrent edits. Review and transfer
only intended saved assets and their synchronized BPT (plus intentionally
authored source/config changes), following
[project ownership](../../docs/unreal-project-rule.md). Runtime package paths
remain `/Game/Mods/SmartOutlet/` regardless of the working directory.

## Validation boundary

Editor tests execute the compiled Blueprints against native mirrors: input and
output hysteresis, per-port independence, configuration and discovery, latch
snapshots, bandwidth preservation, notifications, reconciliation and visual
cleanup. They also check optional mode profiles, class lookup, mode selection,
hint text, HUD attachment/gating, input debouncing, research and restoration.
These tests do not simulate native network transfer or the game save pipeline.

Changes to native flow, attachment/restoration or persistence require their
corresponding game tests; editor mirrors cannot establish those contracts.
Recheck affected identities and notification behavior when the native contract
changes. Technical identity changes require a fresh DML discovery/load check.
Candidate acceptance belongs to release evidence under the
[shared release workflow](../../docs/release-and-installation-rule.md).
