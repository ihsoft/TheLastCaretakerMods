# The Last Caretaker Mods

Workspace for researching and developing mods for **The Last Caretaker**
(`Voyage`, Unreal Engine 5.7).

## Layout

- `mods/` — source code for installable mods.
- `tools/` — source and utilities used to inspect Unreal resources.
- `artifacts/` — extracted assets, reflection mappings, Blueprint JSON, and
  decompiled Kismet output.
- `docs/` — research notes and verified runtime observations.
- `dist/` — user-facing mod packages when a version is ready to distribute.

## Current focus

`DonkLiftKeyboardControl` implements persistent, smoothed keyboard throttle and
steering for the DonkLift-320 forklift. `DonkLiftHotkeyHints` adds native-style
`X` (brake/reset throttle) and `C` (center steering) hints to the forklift HUD,
with English and Russian labels selected from Voyage's own language setting.

The HUD mod requires the version-pinned UE4SS signature in
`ue4ss/UE4SS_Signatures/FText_Constructor.lua`. Revalidate that signature after
every game executable update before enabling the HUD mod.
