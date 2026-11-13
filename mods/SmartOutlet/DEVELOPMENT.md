# Smart Outlet development

`Project/Voyage.uproject` owns release assets and editor dependencies.
`Project/BlueprintText/` mirrors `/Game/Mods/SmartOutlet/` for review; these are
synchronized source files, not build inputs. `Assets/SmartOutlet.ini` is the
distributable settings file. Mod-level `Settings/` and `Tests/` own offline
installation/migration code; Unreal fixtures live in `Project/Tests/`.

## Build and validation

[Build-SmartOutlet.ps1](Build-SmartOutlet.ps1) and
[Validate-SmartOutlet.ps1](Validate-SmartOutlet.ps1) create fresh working projects
from current source, prepare their native/BPT dependencies and run Unreal only
in those copies. Both accept `-EngineRoot` and a fresh `-OutputRoot` below
`Tmp/SmartOutlet/`. They delete the working project after success, retain useful
results/logs, and leave failed runs available for diagnosis. Build also accepts
`-GameRoot`, `-IgnorePinnedGameVersion` and `-AllowDirtySource` under the
[shared release workflow](../../docs/release-and-installation-rule.md).

Wrap either command in the [Unreal queue](../../tools/unreal-coordination.md).
For editor checks, run from the repository root:

```powershell
$shell = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
./tools/Invoke-UnrealQueued.ps1 -Operation 'Validate Smart Outlet' `
  -FilePath (Join-Path $PSHOME $shell) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/SmartOutlet/Validate-SmartOutlet.ps1')
```

The validator returns `reportPath` for six Blueprint suites. These exercise
editor mirrors, not native resource transport or the game save pipeline.
Missing/failed reports stop the command. The two offline scripts can also run
directly: [settings migration](Tests/ValidateSettingsMigration.ps1) and
[installation recovery](Tests/ValidateInstallation.ps1). The latter uses a
synthetic container installer and never writes to the installed game.

Use [Install-SmartOutlet.ps1](Install-SmartOutlet.ps1) with
`-ReleaseManifest <candidate manifest>` for the coupled container/INI operation;
pass `-AllowDirtySource` for a dirty candidate. Its mod-specific settings policy
is in [Architecture](ARCHITECTURE.md#source-and-package-boundaries).

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
