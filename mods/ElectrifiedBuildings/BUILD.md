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
edit, Apply and Preview `/Game/Mods/ElectrifiedBuildings/ModActor`,
`/Game/Mods/ElectrifiedBuildings/BP_BuildingSocket` and the mod-owned
`/Game/Mods/ElectrifiedBuildings/Settings/BP_IniFileParser` in that copy. Accept the saved
assets and synchronized JSON with the shared
[source-update operation](../../tools/mod-setup.md#explicit-source-update-acceptance).

## Build and install

```powershell
python -B tools/run_powershell.py mods/ElectrifiedBuildings/Build-ElectrifiedBuildings.ps1 -AllowDirtySource
```

The producer cooks and validates the shared code once, then uses the standard
archive and manifest tools for both variants. It returns `variants` (each with
`variant`, `archivePath`, `releaseManifestPath`) and `validationSummaryPath`.

| Download title | Canonical INI | Capacity (Wh) | Bidirectional |
| --- | --- | --- | --- |
| ElectrifiedBuildings — Standard | `Assets/ElectrifiedBuildings.ini` | 0 | false |
| ElectrifiedBuildings — Expanded | `Assets/Expanded/ElectrifiedBuildings.ini` | 200 | true |

Outputs live in separate `Standard/` and `Expanded/` run directories. Each ZIP
keeps the canonical `ElectrifiedBuildings-V<N>.zip` leaf name and has its own
manifest. Publish **both** as separate, simultaneously available downloads on
the same mod page, with the titles above and their respective player descriptions
from [README.md](README.md). Both belong to the same mod version; neither replaces
the other. They install to identical paths; players install one variant, never both.
The installed INI always has the name `ElectrifiedBuildings.ini`.

Validate either canonical INI after changes with `Validate-VoyageModSettings.ps1`
and `Settings/ElectrifiedBuildings.settings.json`. Runtime fallbacks remain the
Standard values. An ordinary build copies both maintained INIs unchanged.

Optional parameters are `-OutputRoot` (a fresh directory below
`Tmp/ElectrifiedBuildings/`), `-EngineRoot`, `-GameRoot` and
`-IgnorePinnedGameVersion`. The last flag permits a compatibility candidate when
the installed Steam build differs from [game-build.json](game-build.json).

Install the selected manifest with the shared installer:

```powershell
python -B tools/run_powershell.py tools/Install-VoyageRelease.ps1 -ReleaseManifest <release-manifest.json> -ExpectedMod ElectrifiedBuildings -SettingsArchivePath Voyage/Content/Paks/LogicMods/ElectrifiedBuildings.ini -AllowDirtySource
```

DML must be installed. The payload is the container triplet and the complete
`ElectrifiedBuildings.ini` in `Voyage/Content/Paks/LogicMods`. The shared installer
replaces the INI with the archived copy. Versioning, installation and publication follow
the [shared release workflow](../../docs/release-and-installation-rule.md).

## Validation boundary

BPT Apply establishes compilation and fresh-process equivalence; the producer
validates cook and package scope. Gameplay changes require checks of supported
controls, input/output power, inlet mode changes, reserve charging and discharge
after removing external power, rejection of ambiguous inlet matches, and cable
restoration after reload and location streaming. Check reserve persistence
across save/load and streaming separately from cable restoration. Placement changes require
visual review in game. These checks are separate from successful packaging.
