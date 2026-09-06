# HC04: occupied Drone root-physics discriminator

This is an isolated preparation project, not the HarpoonCannon release.
It uses Engine-only Blueprint calls: no Voyage native mirrors, stock package
overrides, model edits, possession or screen widgets. See the active Harpoon
backlog for interpretation limits and the player-facing `README.txt` for tests.

## Preparation (Windows PowerShell 5.1)

From the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/HarpoonCannon/AutoloadProbe/Build-Probe.ps1
```

Requires the reviewed installed Steam build 25056839, reviewed mappings,
editor `K:\Epic Games\UE_5.8` version 5.8.2, C++ toolchain and manifest-validated
canonical retoc. `-OutputRoot` must be a fresh path beneath repository artifacts.
The script fingerprints/resolves mappings, builds the editor module, generates
one asset, cooks one exact package without references, extracts current
scriptobjects, packages/validates an exact one-package inventory, creates a ZIP
and schema-2 manifest, then calls the common installer with `-ValidateOnly`.
The current producer uses `tools/New-VoyageReleaseManifest.ps1` for the common
manifest and validation. Engine/mapping/source hashes remain separately in
`build-provenance.json`; it no longer assembles schema2 fields itself.
It never installs. `-SkipBuild` is for explicit local debugging only, not a
release preparation claim; normal preparation always builds incrementally.

Stage logs and source hashes are retained in the result directory. Previous
generated Content is moved recoverably into that directory. Source hashes and
HEAD must remain stable throughout the preparation. Uncommitted experiment
sources are explicitly declared; installation validation uses AllowDirtySource.
Final output is compact JSON with status, manifest/archive and verification.

The current class identity was re-inspected through stock-isolated public JSON
on Steam25056839, executable CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
Only a soft reference to BP_CameraDrone is generated; extraction is not copied
as a replacement Blueprint. Fingerprint change invalidates this game reference,
scriptobjects and loader contract. The old Harpoon UE5.7 project remains blocked.

Autoload descriptor uses `entryClass` and `activateIn: gameplay` from the existing
C8 loader protocol. The loader is a prerequisite, not bundled or mutated here.
The observer is a mod-authored Engine Actor using a Tick state machine and
TextRender components. In the historical HC02 test the user observed the Drone alive
at 21.406092 seconds with hidden NO, collision ON and stock Loot/Enter/Grab
hints. This validates observation/target acquisition, not pressing those actions.
Native spawned-Drone save/streaming and
early teardown behavior are not assumed safe; use a disposable session.

HC01 already game-validated initial validity and survival to 8.002636 seconds.
HC02 changes only the observation window to 60 seconds and its instrumentation:
read-only whole-Actor hidden/collision flags, and an observer-owned text label
45 cm above the Drone origin. It does not activate, reposition, attach, register,
recharge or possess the Drone. Observe natural hints only; do not press entry or
loot because timed cleanup remains active. The same virtual package and
container family replaced HC01 through an explicitly authorized install. The
HC01 predecessor is recoverable through the HC02 installation manifest in the
active backlog.

Current HC03 source removes both timed and observer-destruction cleanup paths:
it never destroys the Drone. It samples continuously and records whether the
player pawn becomes the owned Drone and later returns to the stored original
pawn. These are observations only; stock actions still own entry and exit.
No active charge/item/attachment/possession/input/camera/HUD mutation is added.
Use a disposable session without manual saving, because retention/native
persistence is unvalidated. HC03 native entry/exit was confirmed in game and
preserved in commit a78baca1b529157d25b43d8eae380579e7258269.
Historical HC02's no-entry warning applies
to HC02 only, not to a verified installed HC03. Do not conflate the packages.

HC04 keeps that stock creation/entry/exit path and changes only the possessed
Drone's root PrimitiveComponent physics simulation. It captures the component,
its pre-entry simulation flag and entry world position, requests simulation OFF
once per occupancy, and restores the captured flag when the player pawn is no
longer that Drone. A failed root cast makes no mutation and is not retried until
another entry. No Tick-disable, inactive/snap state, attachment, input override,
stock asset override or native mirror is added. Continued observation exposes
native simulation re-enable instead of fighting it repeatedly. The board has
15 TextRender components, including original/current simulation and world drift.
This deliberately tests world stationarity on a stopped ship, not ship-relative
attachment. Mouse look and native exit must remain usable; otherwise restore
HC03. No claimed safety for observer teardown, save/load or travel while frozen.

Explicit installation, when authorized, must use `tools/Install-VoyageRelease.ps1`
with the returned manifest; restoration uses its returned installation manifest
and `tools/Restore-VoyageReleaseInstallation.ps1`. Never copy files by hand.
