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
> `docs/research-workflow-rule.md`. I want to improve `<game feature>`. First inspect
> the existing knowledge and current game resources, then propose the cheapest
> useful experiment. Do not change my installed game yet.

To learn how something works without starting an implementation:

> Read the repository rules and relevant documents under `docs/`. Explain how
> `<vehicle, HUD, input, or other system>` works, clearly separating verified
> facts from hypotheses, and point me to the supporting files and tools. Do not
> modify anything.

To continue an existing mod:

> Read the root index and the mod's `AGENTS.md`. Follow its architecture and
> rule links, then use its `README.md` for behavior and public build steps. For
> a requested candidate, run the documented producer first and report the next
> validation step. Do not reuse stale extracted assets.

Tell the agent where your game is installed if it is not at the path currently
used by the scripts. Also say explicitly when you want **research only**, when
the game is running, and when you are ready for files to be installed.

A clean clone intentionally contains no extracted game assets, generated
Blueprints, cooked files, or ready-made packages. The agent recreates needed
inputs with the checked-in tools under ignored `Tmp/`, retaining generated
output under `artifacts/` only when it has a use beyond the current work cycle.
Research may commit a useful, evidence-bound intermediate conclusion while the
investigation remains open. A gameplay compatibility claim still requires a
proportional test in the real game. See the [research workflow](docs/research-workflow-rule.md)
and [generated-output lifecycle](docs/artifact-lifecycle-rule.md).

## About this repository

DonkLift `v2` supports The Last Caretaker `5.5`, Steam build `25056839`,
with game Unreal Engine `5.8.1`. This compatibility record is not a hard
runtime allowlist; game-derived build inputs require review after an update.

## Layout

- `mods/` — source code, authored assets and mod-local build rules for
  installable mods. Railgun's live GLB and role manifest are owned by
  [`mods/Railgun`](mods/Railgun/README.md).
- [`tools/`](tools/README.md) — catalog and usage guide for reproducible
  extraction, inspection, and patching utilities.
- `docs/` — reusable cross-mod `-knowledge.md` findings and `-rule.md`
  constraints; active research and mod-specific knowledge belong with the mod.
- `Tmp/` — ignored output for the current investigation, feature, or release
  cycle.
- [`artifacts/`](artifacts/README.md) — ignored generated output retained for a
  concrete use beyond the current cycle, including selected caches and rollback
  evidence; its contents are not committed.
- `dist/` — optional generated release output; it may not exist in a clean
  checkout.

## Shared game research

- [Game input integration](docs/voyage-input-integration-knowledge.md) — stock action
  identity, remapping, context ownership, camera settings and action hints.
- [Runtime configuration](docs/runtime-configuration-patterns-knowledge.md) — setting
  ownership, shared cache lifetime, native setters and validation boundaries.
- [Item fabrication and pickup](docs/voyage-item-fabrication-and-pickup-knowledge.md) —
  separate registration, serialization and drop contracts; tested ammo
  dependencies, output grouping and numeric caliber defaults.
- [Scanning and recipe unlocks](docs/voyage-scanning-and-recipe-unlocks-knowledge.md) —
  the stock Replica Scanner completion path, item flags and vault grants;
  fingerprint-bound static findings and runtime validation limits.

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
