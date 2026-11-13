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
`Build/` contains internal cook/package checks, source-state records and INI
migration policy. Its scripts are stages used by the public commands below.
Runtime and editor-only package ownership is defined by
[architecture](ARCHITECTURE.md#release-serialization) and
[provenance](ARCHITECTURE.md#editor-input-provenance).

## Working copies and Blueprint authoring

Run commands from the repository root in Windows PowerShell 5.1. The package
validators and item updater also require PowerShell 7 on PATH. Engine operations
require Unreal Engine 5.8.2 and join the shared Unreal queue automatically.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/StableGyro/New-StableGyroProjectCopy.ps1 -Destination R:/Codex/TheLastCaretakerMods/Tmp/StableGyro/authoring/Project
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

## Automated checks

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/StableGyro/Test-StableGyro.ps1
```

Optional `-OutputRoot` selects a fresh directory below `Tmp/StableGyro/`.
The offline suites cover source-state decisions, exact asset copies and BPT path
mapping, relocated overlays, unsafe copy destinations, concurrent-source rejection,
paired update recovery, forced-process interruption and public recovery,
rejection of newly created item dependencies, INI/schema validation, candidate-based INI installation,
user-value preservation and coupled rollback on a synthetic installation.
Success returns `status: passed` and `reportPath`. Failure retains its report and
diagnostics. These checks do not launch Unreal, cook or install into the game.

## Candidate build

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/StableGyro/Build-StableGyro.ps1 -AllowDirtySource
```

Omit `-AllowDirtySource` for clean source. Optional `-OutputRoot` selects a fresh
run below `Tmp/StableGyro/`; `-GameRoot`, `-EngineRoot` and `-Retoc` override local
installations. `-IgnorePinnedGameVersion` permits the shared pin override without
bypassing mapping, source-readiness or serialization checks. Candidate numbering
comes from `version.txt`; the optional legacy `-Version` accepts only that derived
number. `-OriginalsRoot` may reuse a clean current-build extraction with its manifest.

The producer validates settings, source readiness and current game inputs, then
prepares native support and cooks a disposable project copy. It does not refresh
or write maintained assets. The local cook adapter preserves the stock override
inventory and the separate tagged compass-display cook required by
[release serialization](ARCHITECTURE.md#release-serialization); the ordinary
shared additive cook profile does not cover those contracts.

The result contains `status: built`, `archivePath`, `releaseManifestPath` and
`validationSummaryPath`. Pass the returned release manifest to the installer;
the detailed validation summary is not an installer input. Successful builds
remove working-project, cook, extraction and staging scratch while keeping the
archive, container and validation evidence, including a separate copy of the
original extraction manifest. Failed runs retain diagnostics.
Build-only work can run while Voyage is running; installation requires it closed.

## Source updates

Source updates are explicit and separate from builds:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/StableGyro/Update-StableGyroItemAsset.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/StableGyro/Update-StableGyroMooringPlate.ps1
```

Both accept `-GameRoot`, `-EngineRoot`, `-OutputRoot` and
`-IgnorePinnedGameVersion`. The plate updater additionally accepts `-BlenderPath`
and `-Retoc`. Each works on a disposable copy, validates the updated inputs,
rejects concurrent edits to its acceptance paths, and transfers only the owned
asset/state pair after validation. Before the first source write, the operation
saves both previous and replacement bytes plus a recovery manifest in its run.
Each file replacement is atomic; ordinary exceptions restore the previous bytes.
Neither installs or commits. Review accepted source changes before building.

An interrupted acceptance leaves
`Project/Intermediate/StableGyroSourceUpdate.json` pointing to its recovery manifest.
Working-copy preparation and further source acceptance reject this pending state.
After the interrupted process has ended, recover through the public command:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/StableGyro/Restore-StableGyroSourceUpdate.ps1 -RecoveryManifest '<recovery.json from the marker or update output>'
```

Recovery checks backup integrity and all current target bytes before writing.
Unexpected bytes from a later edit block recovery; preserve the run and resolve
that conflict rather than deleting the marker. Repeating recovery is supported.
If acceptance had already completed, recovery only finishes cleanup. Successful
acceptance/recovery removes temporary backup bytes and integrity digests; the
small completion record remains with the run. This covers process interruption,
not loss of the storage volume containing both source and recovery data.

The item updater replaces the item and its owned crafting-generator stub using
`Build/Item.patch.json`, compares the cooked result to current stock, and updates
`Build/Item.source.json`. Run it after a game-build or recipe change, or to repair
missing/interrupted state. Unsupported stock data fails in the shared item tool.
The wrapper accepts only the item/generator pair and its source state. If the
shared updater creates a missing dependency or string table, removes a dependency,
or changes another content file, acceptance fails before any maintained-source
write. The working copy remains for a separately reviewed dependency repair.

The plate updater fully replaces the positive-Y mesh at its existing identity,
compares cooked geometry, normals, UVs and colors against fresh stock-derived
geometry, and updates `Build/MooringMeshes.source.json`. Run it after a game-build
change or import repair. Ordinary builds require ready current inputs and retain
final-container representation/material checks without extracting plate geometry.
Geometry checks do not establish game appearance.

## Installation

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/StableGyro/Install-StableGyro.ps1 -ReleaseManifest '<returned release manifest>' -AllowDirtySource
```

`-GameRoot` overrides the game location; `-ValidateOnly` returns the shared
read-only plan. `Build-StableGyro.ps1 -Install` invokes this same implementation.
The installer takes defaults from the exact candidate ZIP, preserves current
values/comments, removes only entries declared in `Build/SettingsMigration.psd1`,
and adds missing keys. It checks installed INI bytes and restores settings plus
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
The producer validates the exact key set and defaults; it does not create members
or rewrite graphs. Synchronize BPT and build after accepting the asset changes;
changed runtime loaders require their graph and game checks.

To remove an option, remove its INI entry, schema entry and every consumer.
For installed-file migration, add only its exact old key and exact canonical
comment lines to `Build/SettingsMigration.psd1`. Do not broaden deletion to user
comments or unrelated keys. The existing installer owns preservation and recovery.

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
| Mesh/HUD/fuel/settings | Plate placement, detail/material appearance; compass heading from camera and visibility with the vehicle HUD; fuel capacity/consumption without refilling or losing stored resources; missing/invalid settings, next-entry reload and preserved user INI values. |
