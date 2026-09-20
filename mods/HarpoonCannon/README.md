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
distance; a miss clears both. The GLB model retains its authored material factors.

The accepted HUD has no diagnostic overlays. 1x uses a hollow center circle and
tracks FirstPersonCamera position; its gaze ray converges gun aim within limits.
5x uses the barrel sight, crosshair and optical mask. RMB is Toggle scope; E exits.
F8 fallback and 20-second safety exit remain. This is not a save-safe weapon release.

## Current shell-only preparation

The model registry must include `nodes.powerSocketAnchor`, naming an empty GLB
node below the stationary base (possibly nested, but not under yaw/pitch).
For example: `"powerSocketAnchor": "PowerSocket"`. Its imported position and
rotation drive the native electrical socket through identity child transforms;
world scale must be one. The anchor defines the native component frame, including
the stock socket DataAsset's own mesh rotation. Use an empty node, not a copy of
the stock mesh. Update the source path and role when revising the model.
Missing anchors fail generation instead of placing a socket at the origin.

Run from the repository root, Windows PowerShell 5.1, OUTSIDE the sandbox:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/HarpoonCannon/Build-Shell.ps1 -OutputRoot R:/Codex/TheLastCaretakerMods/artifacts/harpoon-cannon/shell-next
```

Use a fresh output identity.
Both Harpoon producers accept `-CacheRoot` (default
`P:\UnrealCache\TheLastCaretakerMods\UE5.8`). They select process-local filesystem
DDC there and explicitly select its `Zen` subdirectory for editor invocations.
No global settings are changed and no previous cache is moved or deleted.

Build-Shell owns fingerprint/mapping/editor gates,
GenerateHarpoonCannon -ShellOnly, inventory-driven tagged cook, header verification, original extraction,
retoc verification, independent semantic checks, ZIP and schema-2 manifest.
By default it only prepares files. Add `-Install` to install the successful release
through the common manifest-gated installer, including dirty development sources.
It refuses a running game, preserves backups and verifies installed hashes.
The final JSON reports `installed` and the installation receipt; an installation
failure fails the command without discarding the prepared release.
Native Interchange imports GLB meshes/materials and hierarchy;
the adapter attaches visual components to the existing module root and assigns
registry-driven yaw/pitch/sight tags. Named ammo instances remain separate
components without ammunition-state logic.
New model authoring and handoff use GLB V1 and
[`model-source.json`](../../models/HarpoonCannon/model-source.json), as described
in the [model guide](../../models/HarpoonCannon/README.md). GLB integration is
game-validated for placement, construction, entry, rotation/elevation, optics
switching and exit. Save persistence and multiplayer are not validated.
The shell no longer consumes the old OBJ/palette bridge.
Generated `Saved/HarpoonGlbInventory.json` is copied to the artifact and drives
package selection and cooked component checks.
The registry selects the current GLB through `source.path`; its recorded SHA and
byte length are not revision gates. The producer records actual source hashes
and rejects source changes during the build. Role identities
and collision stay mod-owned; changing source names/offsets needs no C++ change.
Validate-Shell.ps1 requires `-ModelInventory` from the same build and is the
independent cooked shell inspector. Preserve base
simple collision, native module/item identities and exact default subobjects.
Geometry changes belong to the model task.

For independent source comparison, run `tools/glb/verify_harpoon_import.py`
with `--source-audit <inspect_glb JSON> --inventory <model-inventory.json>`
and `--semantic <semantic/validation.json>` from the same model/build.
Requires Python3.10+ and NumPy. This read-only check covers rigid local transforms,
hierarchy and untextured material factors, not runtime rendering or gameplay.

## Current station preparation

[Station producer contract](AutoloadProbe/README.md).
AutoloadProbe/ contains the active station generator. Source/ contains the shell
generator, used through its ShellOnly entry. Both are required source inputs.

### Runtime tuning interface

The implementation reads `HarpoonCannon.ini` beside the installed mod containers
in `Voyage/Content/Paks` on each entry, not every frame. The tracked distribution
template is [`Assets/HarpoonCannon.ini`](Assets/HarpoonCannon.ini). A successful
`Build-Probe.ps1 -StationPrototype -Install` creates the installed file when it is
missing and preserves an existing user-edited copy. Exit the cannon before editing,
then enter again. No binary rebuild is needed. Defaults:

```ini
OpticsMousePercent=35
YawLimitDegrees=80
MinimumPitchDegrees=-50
MaximumPitchDegrees=10
```

Normal1x mouse scalar is1.28 (100%); optics35% gives0.448. Legacy MousePercent
is ignored. RMB is the station's own toggle action; entry defaults to1x.
Only5x displays the circular mask, crosshair and hit data; 1x shows a center circle.
Modes, HUD and settings loading from the Paks-adjacent file are game-validated.
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
