# HarpoonCannon

Stable **operator/optics checkpoint: HC33**, accepted by the user on 2026-09-13.
This is not a finished harpoon weapon or a production release: firing, cable
attachment and save-safe station persistence are not implemented/validated.
The completed test installation has been removed for normal play.

## Start here

- [Current state and exact recoverable packages](../../docs/harpoon-cannon-backlog.md)
- [Architecture and validation boundaries](RESEARCH.md)
- [Native entry / interaction contracts](VEHICLE_ENTRY_RESEARCH.md)
- [Current game-derived provenance](GAME_DERIVED_SOURCES.md)
- [Consolidated failures and tooling observations](PIPELINE_OBSERVATIONS.md)
- [Rules](AGENTS.md)
- Model work remains separately owned by
  [the model backlog](../../docs/harpoon-cannon-model-backlog.md).

## Accepted behavior

A buildable Cyclone-leaf module shell follows the deck. A separate stationary
common VoyageVehiclePawn supplies contextual Enter Harpoon, native entry/exit,
own mouse inputs, first-person x5 camera, own HUD and standard exit-action hint.
Character stats remain visible deliberately. Any first blocking optical hit
shows its game item name or technical actor name, plus character-to-hit distance;
a miss clears both. Mouse scale is 1.024 (20% below HC32).

The accepted package still has a diagnostic panel, F8 exit fallback and 20-second
safety exit. They are experiment scaffolding, not the intended final UX. Do not
silently rebuild or relabel it as a diagnostics-free release.

## Current shell-only preparation

Run from the repository root, Windows PowerShell 5.1, OUTSIDE the sandbox:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/HarpoonCannon/Build-Shell.ps1 -OutputRoot R:/Codex/TheLastCaretakerMods/artifacts/harpoon-cannon/shell-next
```

Use a fresh output identity. Build-Shell owns fingerprint/mapping/editor gates,
GenerateHarpoonCannon -ShellOnly, exact four-package cook, original extraction,
retoc verification, independent semantic checks, ZIP and schema-2 manifest.
It never installs. The Cyclone leaf and three authored meshes are the inventory.
Validate-Shell.ps1 is the independent cooked shell inspector. Preserve base
simple collision, native module/item identities and exact default subobjects.
Geometry changes belong to the model task.

## Current station preparation

[Station producer contract](AutoloadProbe/README.md).
AutoloadProbe is the historical directory name of the **active HC33 generator**,
not a disposable installed probe folder. Do not delete it during artifact cleanup.
Root Source/ also contains historical helpers compiled with the shell generator;
only its ShellOnly entry is authorized. Removing/renaming live code is a separate
refactor requiring output comparison; housekeeping does not justify a gameplay test.

Build-HarpoonCannonPlacementProbe.ps1 and Cook-HarpoonCannonPlacementProbe.ps1
were obsolete UE5.7 recipes and have been retired. Use the producers above.

## Install / remove

Route through tools/README.md. Install the unchanged shell FIRST, station SECOND;
restore station FIRST, shell SECOND with their exact paired installation manifests.
Require closed game, matching fingerprint, validated release/ZIP, backups and
independent installed hash readback. Never ship editor DLLs or install while playing.

## Cleanup and recovery

See [Recovery and observations](PIPELINE_OBSERVATIONS.md).
Old research/build outputs are archived under ignored
artifacts/archive/harpoon-hc33-20260913, with per-entry hashes. Regenerable
Intermediate trees were deleted, not archived. Current sources, SourceAssets,
reviewed mappings, shared tools, model assets and other mods remain intact.
