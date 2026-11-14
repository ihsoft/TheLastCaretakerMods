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

Both commands enter the [Unreal queue](../../tools/unreal-coordination.md)
automatically. Run from the repository root through the
[shared PowerShell launcher](../../tools/powershell-execution.md). To build a candidate:

```powershell
python -B tools/run_powershell.py mods/SmartOutlet/Build-SmartOutlet.ps1
```

Add `-AllowDirtySource` to the producer's arguments for uncommitted source.
Building does not install the candidate. Its result has `status: built`,
`archivePath`, `releaseManifestPath` and `validationSummaryPath`.
For editor checks:

```powershell
python -B tools/run_powershell.py mods/SmartOutlet/Test-SmartOutlet.ps1
```

The test runner returns `reportPath` for its Blueprint suites. Missing/failed
reports stop the command. See the [validation boundary](#validation-boundary)
for coverage and required game checks.

## Installation and settings

Use [Install-SmartOutlet.ps1](Install-SmartOutlet.ps1) with
`-ReleaseManifest <candidate manifest>` for the coupled container/INI operation;
pass `-AllowDirtySource` for a dirty candidate.

The installer replaces the complete INI from the selected ZIP through the
[shared installation operation](../../tools/release-installation.md#install-a-candidate-with-settings).
`-ValidateOnly` checks the candidate and destinations without installing.

When changing the canonical INI or its section rules, run:

```powershell
python -B tools/run_powershell.py mods/SmartOutlet/Settings/Test-SettingsTemplate.ps1 -Path mods/SmartOutlet/Assets/SmartOutlet.ini
```

This focused check owns the mod's class/mode section names, threshold ordering
and material-slot constraints; the shared flat settings schema does not express
that contract. It runs only when those inputs change, not during ordinary builds.
Runtime parser changes also require the editor and game checks below.

## Authoring

Create a working project without launching Unreal:

```powershell
python -B tools/run_powershell.py mods/SmartOutlet/New-SmartOutletProjectCopy.ps1 `
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
Run them for changes to the covered behavior. Profile fixtures supply their own
values so changes to distributed defaults do not redefine the test. Visual
ownership and material restoration are tested; authored appearance is accepted
visually rather than frozen as scalar or color constants. Validate the shipped
INI separately when it or its section rules change, as described above.

Changes to native flow, attachment/restoration or persistence require their
corresponding game tests; editor mirrors cannot establish those contracts.
Recheck affected identities and notification behavior when the native contract
changes. Technical identity changes require a fresh DML discovery/load check.
Candidate acceptance belongs to release evidence under the
[shared release workflow](../../docs/release-and-installation-rule.md).
