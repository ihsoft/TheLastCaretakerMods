# Harpoon station generator

This is the active editor-only producer for the stationary Harpoon operator.
Its native binaries are authoring inputs and must not enter the runtime package.
The user accepted the current operator/optics baseline on 2026-09-13.
See [architecture](../RESEARCH.md) and the [active backlog](../../../docs/harpoon-cannon-backlog.md)
for exact retained manifests.

## Prepare, never install implicitly

Windows PowerShell 5.1; FIRST invocation outside sandbox:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/HarpoonCannon/AutoloadProbe/Build-Probe.ps1 -StationPrototype -OutputRoot R:/Codex/TheLastCaretakerMods/artifacts/harpoon-cannon/station-next
```

OutputRoot must be fresh beneath repository artifacts. Producer gates exact game
fingerprint, reviewed mapping and editor 5.8.2; builds, generates, cooks, checks
tagged headers, extracts current scriptobjects, packages/verifies, writes ZIP and
schema-2 release manifest. Source hashes/status and HEAD must stay unchanged.
No native producer in sandbox; no dotnet run. SkipBuild only for an unchanged binary.
`-CacheRoot` defaults to `P:\UnrealCache\TheLastCaretakerMods\UE5.8`, shared with
the shell producer. Filesystem DDC and explicit Zen data path use this directory;
old caches and global Unreal settings are left unchanged.

StationInputsOnly is a mutually exclusive authoring-only preflight: seven input
assets, no runtime actor/HUD/autoload. It is not a playable station.
The common station contains exactly thirteen tagged packages:

- /Game/Mods/HarpoonCannon/Station/BP_HarpoonOperator
- /Game/Mods/HarpoonCannon/Station/WBP_HarpoonHUD
- /Game/Mods/HarpoonCannon/Inputs/IA_HarpoonLookYaw
- /Game/Mods/HarpoonCannon/Inputs/IA_HarpoonLookPitch
- /Game/Mods/HarpoonCannon/Inputs/IA_HarpoonExit
- /Game/Mods/HarpoonCannon/Inputs/IA_HarpoonZoom
- /Game/Mods/HarpoonCannon/Inputs/IA_HarpoonFire
- /Game/Mods/HarpoonCannon/Station/BP_HarpoonTestShot
- /Game/Mods/HarpoonCannon/Station/T_HarpoonOpticalMask
- /Game/Mods/HarpoonCannon/Inputs/IMC_HarpoonKeyboard
- /Game/Mods/HarpoonCannon/Inputs/DA_HarpoonInputContext
- /Game/Mods/HarpoonCannonLifecycleProbe/ModActor
- /Game/Mods/HarpoonCannonLifecycleProbe/ProbeHUD

The unchanged shell is a separate five-package manifest. Autoload support is an
external prerequisite, not bundled. Package names are stable runtime identities:
do not rename them to cosmetically remove the word Probe.

## Hard semantic gates

All station headers reject PKG_UnversionedProperties. Partial native mirrors
must not serialize guessed inherited properties, native struct defaults or
unintended native component/CDO deltas. Preserve native VehicleMesh identity.
Public exact-Mod JSON checks own K2 InteractiveInterface membership AND exact
GetInteractiveProvidedActions signature, plus explicit cooked Interact=Block.
An explicit interface implementation need not have an inherited SuperStruct.
Source graph creation must use the exact declaring interface.

Checkpoint-specific audit paths and results are recorded in the active backlog.
They consume public returned JSON paths plus hashes and are not generic future
gates. Preflight changed assertions against known JSON before another cook.
Never confuse static pass with runtime pass.

## Runtime protocol / cleanup

Load a save containing a built cannon. Aim at the base, use labeled Enter Harpoon,
check 1x eye view and center circle, RMB Toggle scope to5x, aim at two solid
targets then empty sky, and exit with E. Technical names are allowed when game
Name is absent. Wide mouse scalar1.28; optics defaults35% (0.448).
Stats remain. E is native exit; F8 is an emergency exit, with no timed auto-exit.
LMB fires one native projectile per click at2000m/s, up to1000m. No cooldown or
hold-to-repeat. ReceiveHit submits one native Directional attack with200 base
damage and the stock PhysicalForce damage class. Repeated hits on a shark and
its death were user-validated; effective damage and balance are not inferred.
No diagnostic trajectory, contact marker or shot-status overlay is emitted.
The cannon, operator and station are ignored; the ship remains an obstacle.
Actor tick precedes movement and bounds displacement by remaining range with a
1cm collision-radius allowance. No inherited velocity, gravity, ricochet or
penetration. A4s actor lifetime preserves the attack causer, not a firing delay.
Ammunition/energy conditions and physical/audio effects remain future work.
Muzzle fallback is pitch-local(208.4,0,4)cm and gated to the audited V1 GLB hash;
changed models require adapter review, not arbitrary reuse of the offset.
No saving, dismantling, looting or entering another vehicle while occupied.
Absent/empty hint: capture one useful observation and stop blind iterations.

Retain the accepted installation while preparing changes. Install or restore only
through the public manifest-based workflow after the game-closed gate; preparation
does not install or roll back implicitly.
