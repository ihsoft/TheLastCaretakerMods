# DonkLift active backlog

## Current validated release

Steam build `25056839` / game UE `5.8.1` changed the executable fingerprint
from the last validated DonkLift checkpoint. Current mapping, stock forklift,
input-context, and executable evidence show that the mod's native identities,
input mappings, HUD enums, and used struct fields remain compatible. The build
pipeline has been moved to UE Editor `5.8.2`, loose cooking, and the canonical
retoc compatibility binary.

The closed-game pipeline has now completed a fresh extraction, exact relocation
checks, UE 5.8.2 generation and loose cook, six-asset packaging, `retoc verify`,
archive creation, installation, and installed-hash readback. Independent mod
inspection confirms that the replacement child inherits the relocated current
forklift, binds `GetProvidedActionsBP` to `VoyageVehiclePawn`, uses the current
160-byte action struct, and creates the expected helper actor and X/C actions.

The user confirmed the full real-game input, native-percentage, X/C hint, pause,
exit, and re-entry behavior on 2026-09-03. The installed candidate is the
game-validated `v2` checkpoint for The Last Caretaker `5.5`, Steam build
`25056839`, and game UE `5.8.1`. There is no pending DonkLift experiment.

`v1` is known to be incompatible with game `5.5`, primarily across the change
from UE `5.7.4` to UE `5.8.1`. This is a recorded result, not a policy that every
game update is incompatible: tested game versions are evidence, not an
allowlist, and mod versions change independently from game versions.

The previous one-command release pipeline and its installed package passed
real-game validation on 2026-08-29. Durable build, extraction, packaging,
installation, and safety contracts live in the mod-local source, `README.md`,
and `AGENTS.md`.

## Compaction rule

After compaction, reread this file and the mod-local `AGENTS.md` before acting.
Do not replace installed game files while Voyage is running.
