# HarpoonCannon

Game-validated operator/optics/model baseline; exact artifacts are in the backlog.
This is not a finished harpoon weapon or a production release: firing, cable
attachment and save-safe station persistence are not implemented/validated.
Current installed package and recovery receipts are recorded in the backlog.

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
own mouse inputs, first-person 1x/5x cameras, own HUD and standard action hints.
Character stats remain visible deliberately. Any first blocking optical hit
shows its game item name or technical actor name, plus optical-camera-to-hit
distance; a miss clears both. The accepted v5 model uses its authored palette.

The accepted HUD has no diagnostic overlays. 1x uses a hollow center circle and
tracks FirstPersonCamera position; its gaze ray converges gun aim within limits.
5x uses the barrel sight, crosshair and optical mask. RMB is Toggle scope; E exits.
F8 fallback and 20-second safety exit remain. This is not a save-safe weapon release.

## Current shell-only preparation

Run from the repository root, Windows PowerShell 5.1, OUTSIDE the sandbox:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/HarpoonCannon/Build-Shell.ps1 -OutputRoot R:/Codex/TheLastCaretakerMods/artifacts/harpoon-cannon/shell-next
```

Use a fresh output identity. Build-Shell owns fingerprint/mapping/editor gates,
GenerateHarpoonCannon -ShellOnly, exact five-package cook, original extraction,
retoc verification, independent semantic checks, ZIP and schema-2 manifest.
It never installs. The Cyclone leaf and four authored meshes are the inventory:
base, yaw, pitch and shared ammunition geometry. This existing producer uses the
frozen pre-GLB `models/HarpoonCannon/runtime-model.json` OBJ/palette bridge;
named ammo instances remain separate components without ammunition-state logic.
New model authoring and handoff use GLB V1 and
[`model-source.json`](../../models/HarpoonCannon/model-source.json), as described
in the [model guide](../../models/HarpoonCannon/README.md). This producer has NOT
yet been migrated to that GLB and does not build V1. Its changed pitch/ammo/sight
hierarchy requires a coordinated derived-import adapter, not a source-path swap.
The descriptor and source hashes are checked before Unreal runs. Role identities
and collision stay mod-owned; changing source names/offsets needs no C++ change.
Validate-Shell.ps1 is the independent cooked shell inspector. Preserve base
simple collision, native module/item identities and exact default subobjects.
Geometry changes belong to the model task.

## Current station preparation

[Station producer contract](AutoloadProbe/README.md).
AutoloadProbe/ contains the active station generator. Source/ contains the shell
generator, used through its ShellOnly entry. Both are required source inputs.

### Runtime tuning interface (awaiting game validation)

The prepared implementation reads `Config/HarpoonCannon.ini` beneath the game's
ProjectSavedDir on each entry, not every frame. The standard installed location
is `%LOCALAPPDATA%/Voyage/Saved/Config/HarpoonCannon.ini`; a template is provided
in this directory. Exit the cannon before editing, then enter again. No binary
rebuild is needed. Defaults:

```ini
OpticsMousePercent=35
YawLimitDegrees=80
MinimumPitchDegrees=-50
MaximumPitchDegrees=10
```

Normal1x mouse scalar is1.28 (100%); optics35% gives0.448. Legacy MousePercent
is ignored. RMB is the station's own toggle action; entry defaults to1x.
Only5x displays the circular mask, crosshair and hit data; 1x shows a center circle.
Modes and HUD are game-validated; settings reload remains a separate gate.
Yaw is symmetric about installation
forward. Keys are case-sensitive; whitespace around key/value is trimmed. Use
decimal points and no inline comments. Unknown/non-numeric lines are ignored;
last valid duplicate wins. Bounds are mouse1..100, yaw1..170, minimum pitch-89..0,
maximum pitch0..89. Missing/empty/unreadable file retains defaults; the native
reader does not distinguish these cases. Keep this small local file after test
cleanup to retain preferences. The current pending test is in the backlog.

## Install / remove

Route through tools/README.md. Install the unchanged shell FIRST, station SECOND;
restore station FIRST, shell SECOND with their exact paired installation manifests.
Require closed game, matching fingerprint, validated release/ZIP, backups and
independent installed hash readback. Never ship editor DLLs or install while playing.

## Recovery

See [Recovery and observations](PIPELINE_OBSERVATIONS.md).
Exact retained package identities are listed in the active backlog; archive
locations and hash-checked recovery instructions live in the linked document.
