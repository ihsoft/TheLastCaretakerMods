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
steering for the DonkLift-320 forklift and adds standard `X` (brake/reset
throttle) and `C` (center steering) actions to its native HUD. The hints use
English or Russian labels selected from Voyage's own language setting.

The current implementation is the self-contained IoStore project at
`mods/DonkLiftKeyboardControl`. It requires neither UE4SS nor DML. The older,
separately maintained Lua implementation remains available for reference and
manual UE4SS installation at `mods/DonkLiftKeyboardControlUE4SS`.
