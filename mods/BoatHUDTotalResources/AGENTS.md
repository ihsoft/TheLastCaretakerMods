# BoatHUDTotalResources index

Start with [repository index](../../AGENTS.md) for shared rules and knowledge.

- **Read first for every mod task:** [Architecture](ARCHITECTURE.md) explains
  stock HUD inheritance, the aggregation widget, and the two displayed values.
- **Required before changes, builds, or installation:** [Mod rules](MOD-RULES.md)
  bind resource identity, presentation, and validation.
- [Game-derived sources](GAME_DERIVED_SOURCES.md): pinned stock identities and
  revalidation boundaries.
- [Candidate producer](Build-BoatHUDTotalResources.ps1): prepares fresh stock
  HUD input, builds the container, and validates the ZIP and release manifest.
- [Shared Boat socket knowledge](../../docs/boat-resource-socket-architecture-knowledge.md):
  resource-module and HUD data paths when investigating stock behavior.
