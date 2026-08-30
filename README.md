# The Last Caretaker Mods

Workspace for researching and developing mods for **The Last Caretaker**
(`Voyage`). The current DonkLift source is validated against Unreal Engine
5.7.4 and Steam build `23962331`; its game-derived contracts must be
revalidated after a game update.

## Layout

- `mods/` — source code and mod-local build rules for installable mods.
- `tools/` — reproducible extraction, inspection, and patching utilities.
- `docs/` — durable architecture findings, workflow, and research pitfalls.
- `artifacts/` — ignored local scratch space for extracted assets, mappings,
  generated Blueprints, cooked files, packages, and backups; it is never part
  of the repository.
- `dist/` — optional generated release output; it may not exist in a clean
  checkout.

## Current focus

`DonkLiftKeyboardControl` implements persistent, smoothed keyboard throttle and
steering for the DonkLift-320 forklift and adds standard `X` (brake/reset
throttle) and `C` (center steering) actions to its native HUD. The hints use
English or Russian labels selected from Voyage's own language setting.

The current implementation is the self-contained IoStore project at
`mods/DonkLiftKeyboardControl`. It requires neither UE4SS nor DML. Its exact
game-version gate and rebuild procedure are documented inside that mod.

The older Lua/UE4SS implementation is preserved at
`mods/DonkLiftKeyboardControlUE4SS` as a historical alternative and reference;
it is not a runtime dependency or part of the autonomous mod package.
