# Runtime INI configuration knowledge (shared build tools; runtime ownership varies)

This is a reusable implementation recipe, not a claim that every game class has
the same events or native API. Railgun and StableGyro implement the canonical
INI/schema and reload pattern; ContainerLidLabel demonstrates a narrower cache.
A strictly session-lived single-read owner and one shared snapshot for multiple
helpers are proposed standards, not behavior established in all three mods.

For an existing mod, open its index before its settings files. For a Gyro
request, start with the [StableGyro index](../mods/StableGyro/AGENTS.md), then
read its [architecture](../mods/StableGyro/ARCHITECTURE.md) to locate the
settings owner and lifecycle, its [rules](../mods/StableGyro/MOD-RULES.md)
before changing anything, and its [README](../mods/StableGyro/README.md) for
the public producer. The [Railgun index](../mods/Railgun/AGENTS.md) and
[ContainerLidLabel index](../mods/ContainerLidLabel/AGENTS.md) provide the same
routing for those mods. Use the owning public producer before investigating
internals. For a new mod, identify the settings owner and lifecycle in its
architecture before applying this recipe. Investigate game-specific calls
only if the known path fails or the required event is absent.

## Choose the read policy before writing a loader

Keep **reading the file**, **storing the effective values**, and **applying them
to an instance** separate. One read can serve many instances; an instance
creation or save-load callback is often an application point, not a file-read
point. Record the policy, owner, and reload trigger in the mod's architecture.

| Policy | Use when | Read and apply contract |
| --- | --- | --- |
| Session snapshot | Many passive instances share settings and live edits are not promised, such as container capacity. | A session-lived owner reads the INI once on first need. Every instance, including one restored from a save, applies that stored snapshot through its normal lifecycle. Document that editing the file requires restarting the game. |
| Entry reload | A player explicitly enters or activates the configured object infrequently, such as Railgun or Gyro. | One owner reads and validates a fresh snapshot at each entry and distributes it to all consumers before they use it. Read earlier only if behavior genuinely needs the setting before first entry. An edit takes effect on the next entry without a rebuild. |

Neither policy reads on Tick or makes each instance read the file. A start-of-game
callback alone does not prove that later-created or save-loaded objects have
received the value. An entry reload does not justify duplicate reads by several
helpers on the same entry; give them one effective snapshot. If no suitable
session-lived owner exists, a class default object (CDO) cache can avoid a read
per instance, but it has a weaker contract: class unload/reload may reread the
INI. The current [ContainerLidLabel cache](../mods/ContainerLidLabel/ARCHITECTURE.md)
is once per loaded class, **not guaranteed once per game process**. Do not
describe it as a strict session snapshot without changing that ownership.

## Author one settings contract

1. Put the distributable default `Assets/<Mod>.ini` under the mod. It owns
   default values, player comments, order, and formatting. Install the same
   file beside the consumed container when the mod reads it from
   `Voyage/Content/Paks`.
2. Keep a small `Settings/<Mod>.settings.json` for what the INI cannot express:
   exact key and generated member names, type (`number` or `boolean`), numeric
   range, and runtime binding. The INI is the only source of default values;
   the schema must not repeat them. Railgun's schema also groups keys and can
   bind a value to an external field; StableGyro's schema covers booleans. For
   a new mod, the flat StableGyro shape is the smaller starting point; add
   groups or external fields only when the consumers require them.
3. At build time, require an exact case-sensitive key match between the default
   INI and schema. Reject duplicate or malformed keys, unsupported values,
   missing defaults, nonfinite numbers, bad ranges, and inline comments. Copy
   the validated INI without rewriting its comments or layout. Generate runtime
   names/defaults from these inputs; never hand-edit generated headers or
   cooked output.
4. In the runtime loader, reset the owner to defined fallbacks before each
   read. Trim keys and values, parse numbers with an invariant decimal point,
   validate in the final storage type, and clamp only where the schema says to.
   Missing or invalid input uses the documented fallback. Most options use the
   canonical default; a safety-sensitive option may explicitly choose another
   value, as Railgun does for missing/invalid recoil settings. Unsupported keys
   must not silently alter another setting. Keep parser work free of latent,
   world, and instance side effects.
5. Apply effective values through the actual owning setter or consumer path.
   A serialized-field write alone may miss derived state, modifiers, delegates,
   or presentation refresh. Verify exact lifecycle and parent-call ownership
   for a game Blueprint override. Avoid polling for an owner or settings file
   when a lifecycle event or direct reference suffices.

The shared [settings generator](../tools/New-VoyageModSettings.ps1) validates
either schema shape, copies the default INI byte for byte, and emits the
runtime declarations. Mod-local scripts select the C++ collection form and
retain the existing public build interfaces. The shared
[INI merger](../tools/Merge-VoyageModIni.ps1) prepares a new file without
changing the installed one: it keeps player values and comments, removes only
explicitly retired keys/comments, and adds missing defaults. The owning
installer performs the game-process check, replacement, readback, and rollback.
The current concrete inputs and thin adapters are
[Railgun INI](../mods/Railgun/Assets/Railgun.ini),
[Railgun schema](../mods/Railgun/Settings/Railgun.settings.json),
[Railgun generator](../mods/Railgun/Build/New-RailgunSettings.ps1),
[StableGyro INI](../mods/StableGyro/Assets/StableGyro.ini),
[StableGyro schema](../mods/StableGyro/Settings/StableGyro.settings.json), and
[StableGyro generator](../mods/StableGyro/Build/New-StableGyroSettings.ps1).
The normal build entry points remain `Build-Railgun.ps1` and
`Build-StableGyro.ps1`; a coder adding a setting edits the INI, schema and
runtime consumer, then runs that entry point. Their exact
mod-owned additions are indexed in the [Railgun rules](../mods/Railgun/MOD-RULES.md)
and [StableGyro rules](../mods/StableGyro/MOD-RULES.md).

## Install and migrate a user-edited INI

The ZIP carries the validated default INI. When installing, create the file
from that template if absent. If it already exists, preserve values, comments,
and unrelated lines for active keys; append only missing current keys with
their canonical defaults. Remove or rename a retired key only under an explicit
mod-owned migration that preserves the user's effective value when required.
Do not replace the user's INI with a fresh template merely because a package
version changed. The installer checks the game process immediately before
mutation, preserves a temporary exact recovery copy, and reads back the final
file. The shared [release policy](release-and-installation-rule.md) owns the
transaction and rollback lifetime.

Railgun and StableGyro already merge active settings on installation. The
ContainerLidLabel installer does **not** manage its external INI; its README
requires a separate backup and migration. That is not the pattern to copy for
a new configurable mod.

## Short path for a coding agent

When asked to add an INI or a setting, use the mod's documented public build
path, not a fresh investigation of INI loading:

1. Identify the value's owner and choose **session snapshot** or **entry
   reload**. State exactly when a player edit takes effect. For a new mod,
   record the choice in its architecture; for an existing mod, keep its policy
   unless the request changes it.
2. Add the value and player-facing units/comment to the canonical INI; add its
   type, range, and binding to the schema. Wire only the consuming behavior by
   hand. Extend the installer merge only for an actual new or retired key.
3. Run the owning public producer first. Use its schema and package diagnostics
   if it fails. A successful build verifies structure, not the gameplay effect.
4. Test the chosen read policy in game: for session snapshot, many instances
   share one read and new/save-loaded instances apply the same value; for entry
   reload, an edit made while outside takes effect on re-entry. Also check
   missing/invalid input, the fallback, and existing user-value preservation.

The shared tools prepare files; the mod still owns the exact game event,
read policy, migration of values between different units, and behavior that
consumes each value. Do not change a read policy merely by choosing a tool.
