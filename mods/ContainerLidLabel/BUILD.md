# ContainerLidLabel build entry points

Run commands from the repository root through the
[shared PowerShell launcher](../../tools/powershell-execution.md). Build and test
commands enter the Unreal queue automatically. Follow the shared
[project layout rule](../../docs/unreal-project-rule.md#project-layout-and-temporary-data),
[Unreal queue guide](../../tools/unreal-coordination.md) and
[release workflow](../../tools/release-build.md).

## Build

```powershell
python -B tools/run_powershell.py mods/ContainerLidLabel/Build-ContainerLidLabel.ps1 -OutputRoot Tmp/ContainerLidLabel/candidate -AllowDirtySource
```

[Build-ContainerLidLabel.ps1](Build-ContainerLidLabel.ps1) prepares and cooks a copy
of [Project](Project/Voyage.uproject), using the
[shared pipeline](../../tools/mod-setup.md#shared-mod-pipeline) for copies and cook.
It returns `status: built`, `archivePath`, `releaseManifestPath` and
`validationSummaryPath`.
OutputRoot must be fresh and below `Tmp/ContainerLidLabel`; omit it for an automatic
run directory. EngineRoot and GameRoot override local installations;
AllowDirtySource permits uncommitted inputs. The
[package boundary](ARCHITECTURE.md#supported-containers-and-package-ownership)
is defined in architecture. Building does not install the candidate.

## Editor checks

```powershell
python -B tools/run_powershell.py mods/ContainerLidLabel/Test-ContainerLidLabel.ps1
```

[Test-ContainerLidLabel.ps1](Test-ContainerLidLabel.ps1) prepares its own project
copy through the shared test runner and returns `reportPath` with
`status: passed`. EngineRoot is configurable; an explicit OutputRoot must be fresh below
`Tmp/ContainerLidLabel`. Use this runner rather than launching ValidateEditor.py directly.
The suite checks discovery/filtering, capacity snapshots, presentation events,
attachment and cleanup on synthetic actors. It does not validate stock event
ordering, save serialization, visual appearance or performance in the game.
Run it when changing those behaviors. Small fixtures cover the supported types
and independent instances; increasing their population is useful only for an
explicit scale-dependent claim. Authored placement and typography belong to
visual acceptance, while attachment, live updates and cleanup are behavioral
contracts covered by the suite.

## Authoring copy

```powershell
python -B tools/run_powershell.py mods/ContainerLidLabel/New-ContainerLidLabelProjectCopy.ps1 `
  -Destination 'Tmp/ContainerLidLabel/edit-label/Project'
```

Destination must be fresh and below `Tmp/ContainerLidLabel`. Use the returned
.uproject with the shared [BPT workflow](../../tools/blueprint-workspace.md).
The copy helper is also used automatically by the build and test entry points.

## Install and upgrade

DML must already be installed. Use the owning installer for both the LogicMods
payload and user settings:

```powershell
python -B tools/run_powershell.py mods/ContainerLidLabel/Install-ContainerLidLabel.ps1 `
  -ReleaseManifest 'Tmp/ContainerLidLabel/candidate/release-manifest.json' `
  -AllowDirtySource
```

Omit `-AllowDirtySource` for a candidate built from clean source.
The installer replaces the complete INI from the selected ZIP, together with
its containers, through the [shared installation operation](../../tools/release-installation.md#install-a-candidate-with-settings).
`-ValidateOnly` checks the candidate and destinations without installing.

## Changing settings

After changing the schema or canonical INI, run the shared validator once:

```powershell
python -B tools/run_powershell.py tools/Validate-VoyageModSettings.ps1 `
  -SchemaPath mods/ContainerLidLabel/Settings/ContainerLidLabel.settings.json `
  -DefaultIniPath mods/ContainerLidLabel/Assets/ContainerLidLabel.ini
```

Ordinary builds copy the INI unchanged. Runtime loader changes also need their
editor and game checks under the [settings contract](ARCHITECTURE.md#settings-ownership).
