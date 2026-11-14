# StableGyro build and authoring

## Source layout

`Project/Voyage.uproject` and `Project/Content/` own the maintained Unreal assets.
`Project/BlueprintText/` mirrors their `/Game/` paths, including the two HUD
dependency stubs under `UI/Game/`. Native declarations and their dependency
declaration live under `Project/EditorSupport/` and `Project/EditorSupport.json`.
`Project/Config/` contains deliberately maintained configuration. Only
`Project/Intermediate/` is ignored temporary project storage.

`Assets/StableGyro.ini` owns the distributed configuration and comments;
`Settings/StableGyro.settings.json` owns bindings, fallbacks and ranges.
`Build/` contains explicit source-update operations and source-state records.
Its scripts are stages used by the public commands below.
Runtime and editor-only package ownership is defined by
[architecture](ARCHITECTURE.md#release-serialization) and
[provenance](ARCHITECTURE.md#editor-input-provenance).

## Working copies and Blueprint authoring

Run commands from the repository root through the
[shared PowerShell launcher](../../tools/powershell-execution.md). The item
updater also requires PowerShell 7 on PATH. Engine operations require Unreal
Engine 5.8.2 and join the shared Unreal queue automatically.

```powershell
python -B tools/run_powershell.py mods/StableGyro/New-StableGyroProjectCopy.ps1 -Destination R:/Codex/TheLastCaretakerMods/Tmp/StableGyro/authoring/Project
```

Choose a fresh destination anywhere below `Tmp/StableGyro/`. The copy command
returns its `.uproject` path without launching Unreal. It copies current source,
including intended uncommitted edits, and resolves the two shared native overlays
for the new location. Use that project with the shared
[BPT workflow](../../tools/blueprint-workspace.md); BPT preparation installs its
own editor plugin into the copy. Export/apply and synchronize the corresponding
tracked JSON when accepting asset changes. Check concurrent source edits and
transfer only intentionally changed assets, text and configuration. Do not open
the maintained source project in Unreal or copy an entire working project back.

## Candidate build

```powershell
python -B tools/run_powershell.py mods/StableGyro/Build-StableGyro.ps1 -AllowDirtySource
```

Omit `-AllowDirtySource` for clean source. Optional `-OutputRoot` selects a fresh
run below `Tmp/StableGyro/`; `-GameRoot`, `-EngineRoot` and `-Retoc` override local
installations. `-IgnorePinnedGameVersion` permits the shared pin override without
bypassing mapping, source-readiness or serialization checks. Candidate numbering
comes from `version.txt`.

The producer checks source readiness and the installed game build, then
prepares native support and cooks a disposable project copy. It does not refresh
or write maintained assets. If an item's or mesh's recorded source build differs
from the installed build, stop and ask whether to refresh those inputs; do not
run an updater automatically. Item recipe changes also require an explicit update.
The producer supplies one package inventory to the
[shared cook and container stages](../../tools/mod-setup.md#cook-and-container-stages).
The compass display selects tagged serialization for the reason recorded in
[release serialization](ARCHITECTURE.md#release-serialization); the other packages
use unversioned properties. No local cook or container implementation is needed.

The result contains `status: built`, `archivePath`, `releaseManifestPath` and
`validationSummaryPath`. Pass the returned release manifest to the installer;
the detailed validation summary is not an installer input. Successful builds
remove working-project, cook, extraction and staging scratch while keeping the
archive, container and validation evidence, including the metadata extraction
report. Failed runs retain diagnostics.
Build-only work can run while Voyage is running; installation requires it closed.
The shared container stage obtains only `scriptobjects.bin` from the installed
game's global metadata. Routine builds neither extract the stock item nor repeat
the updater's comparison of item data against stock.

## Source updates

Source updates refresh maintained assets derived from the game. Ordinary builds
reuse those saved assets; they never run either updater automatically. Invoke
the public commands at the mod root; their implementations live in `Build/`.

| Command | Purpose and when to use it |
| --- | --- |
| [Update-StableGyroItemAsset.ps1](Update-StableGyroItemAsset.ps1) | Reapply the mod's `DroppedActor` replacement to the current stock gyrocopter item, so placing the item creates the StableGyro Blueprint while retaining current stock item data. Use when accepting a new game build, changing `Build/Item.patch.json`, or regenerating missing item inputs. Routine flight-logic or INI edits do not require it. |
| [Update-StableGyroMooringPlate.ps1](Update-StableGyroMooringPlate.ps1) | Recreate the maintained half-plate mesh from the stock mooring plate for the mod's port layout. Use when accepting a new game build or repairing the mesh import. Routine Blueprint or INI edits do not require it. |

These commands change maintained source files after validation. They do not
install a mod or commit the accepted changes:

```powershell
python -B tools/run_powershell.py mods/StableGyro/Update-StableGyroItemAsset.ps1
python -B tools/run_powershell.py mods/StableGyro/Update-StableGyroMooringPlate.ps1
```

Both accept `-GameRoot`, `-EngineRoot`, `-OutputRoot` and
`-IgnorePinnedGameVersion`. The plate updater additionally accepts `-BlenderPath`.
Each works on a disposable copy, validates the updated inputs,
rejects concurrent edits to its acceptance paths, and transfers only the owned
assets and source-state record after validation, using the
[shared source acceptance](../../tools/mod-setup.md#explicit-source-update-acceptance).
That guide describes watched inputs, backup lifetime and recovery if copying
fails. Assets are copied first, then the source-state record. Review accepted
source changes before building.

These are asset-generation workspaces: the item commandlet and mesh importer
produce binary editor packages there. Only their validated source outputs are
accepted into the maintained project. Ordinary build/test copies have no
copy-back stage.

### Item source update

The item updater replaces the item and its owned crafting-generator stub using
`Build/Item.patch.json`, compares the cooked result to current stock, and updates
`Build/Item.source.json`. A stale saved item override can otherwise mask changes
to stock item data. Unsupported stock data fails in the shared item tool.
The updater accepts only the item/generator pair and its source state. If the
shared updater creates a missing dependency or string table, removes a dependency,
or changes another content file, acceptance fails before any maintained-source
write. The working copy remains for a separately reviewed dependency repair.

### Mooring plate source update

The plate updater extracts the current stock mesh, splits its disconnected
halves and imports the positive-Y half into a disposable project through the
shared StaticMesh tool. Extraction and splitting check their results where they
run; no fixed source triangle counts are prescribed. The importer reopens the
saved mesh and checks its material bindings. After successful acceptance,
`Build/MooringMeshes.source.json` records the source game build.
Ordinary builds check source readiness and game-build identity, then cook the
maintained mesh. They do not reimport GLB or compare the cooked mesh to it.
Import success does not establish game appearance.

## Installation

```powershell
python -B tools/run_powershell.py mods/StableGyro/Install-StableGyro.ps1 -ReleaseManifest '<returned release manifest>' -AllowDirtySource
```

`-GameRoot` overrides the game location; `-ValidateOnly` returns the shared
read-only plan. `Build-StableGyro.ps1 -Install` invokes this same implementation.
The installer replaces the entire INI with the exact file from the selected
candidate ZIP, including on updates. It does not merge values, preserve old
comments or migrate keys. It checks installed INI bytes and restores settings plus
the container if the coupled change fails. Preserve reported recovery files if
recovery fails. Successful transaction scratch is removed.

Publication, retention and rollback selection follow the
[shared release workflow](../../docs/release-and-installation-rule.md).
Automated/build success does not replace the [gameplay checks](#gameplay-validation).

## Changing settings

Use the [shared settings procedure](../../docs/runtime-configuration-patterns-knowledge.md#adding-a-setting)
and [schema/INI preparation interface](../../tools/mod-setup.md#runtime-ini-preparation).
For StableGyro, update `Assets/StableGyro.ini` and
`Settings/StableGyro.settings.json`, then change the owning Blueprint through BPT:
member, getter arguments, fallback, bounds and consumers must agree. Keep the
[consumer ownership and read lifetimes](ARCHITECTURE.md#settings-parser).
After changing the schema or INI, run the shared validator once:

```powershell
python -B tools/run_powershell.py tools/Validate-VoyageModSettings.ps1 `
  -SchemaPath mods/StableGyro/Settings/StableGyro.settings.json `
  -DefaultIniPath mods/StableGyro/Assets/StableGyro.ini
```

The validator does not create members or rewrite graphs. Ordinary builds copy
the INI unchanged. Synchronize BPT and build after accepting the asset changes;
changed runtime loaders require their graph and game checks.

To remove an option, remove its INI entry, schema entry and every consumer.
Installation distributes the complete current INI; do not add key-migration or
merge logic. The installer owns exact replacement and recovery on failure.

## Gameplay validation

Choose the affected scenarios when changing behavior or consumed game contracts.
This matrix states required checks; it does not claim they have all passed on
every game build. Established applicability remains in
[architecture](ARCHITECTURE.md#validation-boundary) and
[native altitude compatibility](ALTITUDE-CONTROL.md#compatibility-boundary).

| Changed contract | Required scenarios |
| --- | --- |
| Pitch and actions | W/S in both directions, release hold, both limits, reset once while held, exit/re-entry and disabled pitch on lifecycle paths; remapped controls, preserved stock actions and conditional cargo hint. Cook must retain all three action-value bindings plus cargo event bindings. |
| Altitude braking/hold | Partial/full throttle, threshold crossing, positive and negative Vz acquisition, Ctrl release and delayed reacquisition; positive and zero ascent/descent deceleration, later exact hold and re-entry with climb inertia. |
| Ctrl cap and contact | Preserve existing upward inertia/natural decay while rejecting renewed upward acceleration; allow unrestricted descent, including below the Collective threshold. Exercise release, controller change, EndPlay, invalid references, contact cooldown, swept blocking hits and initial penetration. |
| Independent flight flags | Both flags independently and together; entry reload, native assistance restoration, no saved HoverPitchScale reset with altitude disabled, no XY physics-velocity access with horizontal control disabled, and continued operation of the other control paths. |
| Horizontal damping | Climb, descent, neutral, directional and reversing input; contact release, no Z modification, and no horizontal-state reset from the vertical Collective gate. |
| Four-port/cargo lifecycle | Four simultaneous connections, short/held cargo input and once-per-press behavior, shared live hold threshold in input/hint, saved extra connections loaded with four-port mode disabled, late restoration and native two-port behavior in disabled mode. |
| Mesh/HUD/fuel/settings | Plate placement, detail/material appearance; compass heading from camera and visibility with the vehicle HUD; fuel capacity/consumption without refilling or losing stored resources; missing/invalid settings and next-entry reload. Installation must copy the complete candidate INI. |
