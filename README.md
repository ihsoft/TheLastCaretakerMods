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
steering for the DonkLift-320 forklift. Physical control works; the forklift HUD
still displays the original digital input. The next step is source-driven
analysis of `BP_VoyageIngameForklift` and its data sources.

