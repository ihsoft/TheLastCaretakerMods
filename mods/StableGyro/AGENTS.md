# StableGyro index

Start with [repository index](../../AGENTS.md) for shared rules and knowledge.

- [Architecture](ARCHITECTURE.md):
  stock Gyro child, input helper, stabilization helper, and settings ownership.
- **Required before changes, builds, or installation:** [Mod rules](MOD-RULES.md)
  bind flight-control, settings, source, and validation contracts.
- [Altitude control](ALTITUDE-CONTROL.md): native hover and vertical-control
  behavior that bounds the mod.
- [Game-derived sources](GAME_DERIVED_SOURCES.md): version-bound inputs and
  reconstructed identities.
- [Candidate producer](Build-StableGyro.ps1): complete build; shared release
  and installation rules apply.
- [Shared INI recipe](../../docs/runtime-configuration-patterns-knowledge.md):
  schema-driven settings, entry reload, and user-value preservation.
- [Vehicle and HUD patterns](../../docs/vehicle-and-hud-modding-patterns-knowledge.md):
  shared vehicle lifecycle and action presentation.
