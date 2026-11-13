# ContainerLidLabel build entry points

Run commands from the repository root. Follow the shared
[project layout rule](../../docs/unreal-project-rule.md#project-layout-and-temporary-data),
[Unreal queue guide](../../tools/unreal-coordination.md) and
[release workflow](../../tools/release-build.md).

## Build

```powershell
$hostExe = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
& ./tools/Invoke-UnrealQueued.ps1 -Operation 'Build ContainerLidLabel' `
  -FilePath (Join-Path $PSHOME $hostExe) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/ContainerLidLabel/Build-ContainerLidLabel.ps1',
    '-OutputRoot','Tmp/ContainerLidLabel/candidate',
    '-AllowDirtySource')
```

[Build-ContainerLidLabel.ps1](Build-ContainerLidLabel.ps1) prepares and cooks a copy
of [Project](Project/Voyage.uproject), returning the ZIP and release manifest.
OutputRoot must be fresh and below `Tmp/ContainerLidLabel`; omit it for an automatic
run directory. EngineRoot and GameRoot override local installations;
AllowDirtySource permits uncommitted inputs. The
[package boundary](ARCHITECTURE.md#supported-containers-and-package-ownership)
is defined in architecture. Building does not install the candidate.

## Editor checks

```powershell
$hostExe = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
& ./tools/Invoke-UnrealQueued.ps1 -Operation 'Test ContainerLidLabel' `
  -FilePath (Join-Path $PSHOME $hostExe) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/ContainerLidLabel/Test-ContainerLidLabel.ps1')
```

[Test-ContainerLidLabel.ps1](Test-ContainerLidLabel.ps1) prepares its own project
copy and returns the report path, requiring `status: passed`. EngineRoot is
configurable; an explicit OutputRoot must be a fresh direct child of
`Tmp/ContainerLidLabel`. Use this runner rather than launching ValidateEditor.py directly.
The suite checks discovery/filtering, capacity snapshots, presentation events,
attachment and cleanup on synthetic actors. It does not validate stock event
ordering, save serialization, visual appearance or performance in the game.

## Authoring copy

```powershell
./mods/ContainerLidLabel/New-ContainerLidLabelProjectCopy.ps1 `
  -Destination 'Tmp/ContainerLidLabel/edit-label/Project'
```

Destination must be fresh and below `Tmp/ContainerLidLabel`. Use the returned
.uproject with the shared [BPT workflow](../../tools/blueprint-workspace.md).
The copy helper is also used automatically by the build and test entry points.

## Install and upgrade

DML must already be installed. Use the owning installer for both the LogicMods
payload and user settings:

```powershell
& ./mods/ContainerLidLabel/Install-ContainerLidLabel.ps1 `
  -ReleaseManifest 'Tmp/ContainerLidLabel/candidate/release-manifest.json'
```

Existing LogicMods INI values are preserved on updates. Recovery follows the shared
[INI installation contract](../../docs/runtime-configuration-patterns-knowledge.md#install-and-migrate-a-user-edited-ini).
