# ElectrifiedBuildings build and authoring

## Source and authoring

`Project/Voyage.uproject` and `Project/Content/` own the maintained assets.
Synchronized BPT mirrors their paths under `Project/BlueprintText/`.
`Project/EditorSupport.json` declares native editor dependencies.
[Build-ElectrifiedBuildings.ps1](Build-ElectrifiedBuildings.ps1) defines the release
inventory passed to the shared cook and exact package validation.

Run commands from the repository root through the
[PowerShell launcher](../../tools/powershell-execution.md). Create a working copy:

```powershell
python -B tools/run_powershell.py mods/ElectrifiedBuildings/New-ElectrifiedBuildingsProjectCopy.ps1 -Destination Tmp/ElectrifiedBuildings/authoring/Project
```

Use the [BPT workspace workflow](../../tools/blueprint-workspace.md) to Export,
edit, Apply and Preview `/Game/Mods/ElectrifiedBuildings/ModActor` and
`/Game/Mods/ElectrifiedBuildings/BP_BuildingSocket` in that copy. Accept the saved
assets and synchronized JSON with the shared
[source-update operation](../../tools/mod-setup.md#explicit-source-update-acceptance).

## Build and install

```powershell
python -B tools/run_powershell.py mods/ElectrifiedBuildings/Build-ElectrifiedBuildings.ps1 -AllowDirtySource
```

The producer uses the shared pipeline for project preparation, unversioned cook,
exact package validation, LogicMods ZIP and release manifest. It returns
`archivePath`, `releaseManifestPath` and `validationSummaryPath`.

Optional parameters are `-OutputRoot` (a fresh directory below
`Tmp/ElectrifiedBuildings/`), `-EngineRoot`, `-GameRoot` and
`-IgnorePinnedGameVersion`. The last flag permits a compatibility candidate when
the installed Steam build differs from [game-build.json](game-build.json).

Install the selected manifest with the shared installer:

```powershell
python -B tools/run_powershell.py tools/Install-VoyageRelease.ps1 -ReleaseManifest <release-manifest.json> -ExpectedMod ElectrifiedBuildings -AllowDirtySource
```

DML must be installed. The payload is the container triplet in
`Voyage/Content/Paks/LogicMods`. Versioning, installation and publication follow
the [shared release workflow](../../docs/release-and-installation-rule.md).

## Validation boundary

BPT Apply establishes compilation and fresh-process equivalence; the producer
validates cook and package scope. Gameplay changes require checks of supported
controls, input/output power, rejection of ambiguous inlet matches, and cable
restoration after reload and location streaming. Placement changes require
visual review in game. These checks are separate from successful packaging.
