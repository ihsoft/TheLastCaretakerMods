# Harpoon station generator: accepted HC33

Historical directory name only. This is the active editor-only producer for the
station, not a runtime DLL and not a stock Drone/Forklift replacement.
The user accepted HC33 target labels and 20%-reduced mouse tuning on 2026-09-13.
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

StationInputsOnly is a mutually exclusive authoring-only preflight: five input
assets, no runtime actor/HUD/autoload. It is not a playable station.
The common station contains exactly nine tagged packages:

- /Game/Mods/HarpoonCannon/Station/BP_HarpoonOperator
- /Game/Mods/HarpoonCannon/Station/WBP_HarpoonHUD
- /Game/Mods/HarpoonCannon/Inputs/IA_HarpoonLookYaw
- /Game/Mods/HarpoonCannon/Inputs/IA_HarpoonLookPitch
- /Game/Mods/HarpoonCannon/Inputs/IA_HarpoonExit
- /Game/Mods/HarpoonCannon/Inputs/IMC_HarpoonKeyboard
- /Game/Mods/HarpoonCannon/Inputs/DA_HarpoonInputContext
- /Game/Mods/HarpoonCannonLifecycleProbe/ModActor
- /Game/Mods/HarpoonCannonLifecycleProbe/ProbeHUD

The unchanged shell is a separate four-package manifest. Autoload support is an
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

Independent audits are retained at artifacts/harpoon-cannon/hc32-audit.ps1
(66 baseline structural checks) and hc33-compare.ps1 (22 HC32/HC33 comparisons).
They consume public returned JSON paths plus hashes. The comparison defaults to
the retained HC33 candidate; it is an evidence helper, not a generic future gate.
Adapt a future experiment's assertions deliberately and preflight against known
JSON before paying for another cook. Never confuse static pass with runtime pass.

## Runtime protocol / cleanup

Load a save containing a built cannon. Aim at the base, use labeled Enter Harpoon,
check first-person x5, aim at two solid targets then empty sky, and exit with E.
Technical names are allowed when game Name is absent. Both mouse axes scale1.024.
Stats remain. F8 and20 seconds are EXIT safeguards, not an alternative entry route.
No firing, saving, dismantling, looting or entering another vehicle while occupied.
Absent/empty hint: capture one useful observation and stop blind iterations.

After result and closed-game gate, restore station then shell by exact installation
manifests. Prepared artifacts are retained; no automatic next-test installation.
Legacy HC01-HC32 protocols are historical archive material, not normal instructions.
