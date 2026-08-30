# The Last Caretaker Mods

## New here? Start with AI

This is an **AI-first modding workspace** for researching and developing mods
for **The Last Caretaker** (`Voyage`). It is designed to be opened in Codex or
another coding agent that can read the repository, run its tools, and work with
your local game installation. It is a development workspace, not a collection
of ready-to-install binary mods. You do not need to know Unreal Engine
internals before you begin: describe the outcome you want, and let the agent
first map the relevant part of the game.

After cloning the repository, open its root in your coding agent and start with
a natural-language request such as:

> My game is installed at `<game path>`. Read `AGENTS.md` and
> `docs/research-workflow.md`. I want to improve `<game feature>`. First inspect
> the existing knowledge and current game resources, then propose the cheapest
> useful experiment. Do not change my installed game yet.

To learn how something works without starting an implementation:

> Read the repository rules and relevant documents under `docs/`. Explain how
> `<vehicle, HUD, input, or other system>` works, clearly separating verified
> facts from hypotheses, and point me to the supporting files and tools. Do not
> modify anything.

To continue an existing mod safely:

> Read the root rules plus the `AGENTS.md` and `README.md` inside
> `mods/<mod-name>`. Check my installed game fingerprint against the documented
> version, then tell me what can be rebuilt or tested. Do not reuse stale
> extracted assets.

Tell the agent where your game is installed if it is not at the path currently
used by the scripts. Also say explicitly when you want **research only**, when
the game is running, and when you are ready for files to be installed.

A clean clone intentionally contains no extracted game assets, generated
Blueprints, cooked files, or ready-made packages. The agent recreates those
version-bound inputs under ignored `artifacts/` directories using the checked-in
tools. The normal workflow is: understand the current evidence, fingerprint
the game, run a narrow experiment, build a candidate, validate it in the real
game, and only then commit the durable source and knowledge.

## About this repository

The current DonkLift source is validated against Unreal Engine 5.7.4 and Steam
build `23962331`; its game-derived contracts must be revalidated after a game
update.

## Layout

- `mods/` — source code and mod-local build rules for installable mods.
- [`tools/`](tools/README.md) — catalog and usage guide for reproducible
  extraction, inspection, and patching utilities.
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
