# StableGyro index

Start with [repository index](../../AGENTS.md) for shared rules and knowledge.

- [Architecture](ARCHITECTURE.md):
  stock Gyro child, flight helpers, four-port layout, cargo action, saved
  connections, settings ownership, and game-validation boundaries.
- [Mod rules](MOD-RULES.md): flight-control, settings, source, and validation
  contracts; read when changing behavior or planning a targeted game test.
- [Altitude control](ALTITUDE-CONTROL.md): native hover and vertical-control
  behavior that bounds the mod.
- [Game-derived sources](GAME_DERIVED_SOURCES.md): version-bound inputs and
  reconstructed identities.
- [Candidate producer](Build-StableGyro.ps1): complete build; use `-Install`
  when preparing a game candidate so the owned INI is merged and installed.
- [Changelog](CHANGELOG.md): player-facing notes for the next Nexus release.
- [Shared INI recipe](../../docs/runtime-configuration-patterns-knowledge.md):
  schema-driven settings, entry reload, and user-value preservation.
- [Vehicle and HUD patterns](../../docs/vehicle-and-hud-modding-patterns-knowledge.md):
  shared vehicle lifecycle and action presentation.
