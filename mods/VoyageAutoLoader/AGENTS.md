# VoyageAutoLoader index

Start with [repository index](../../AGENTS.md) for shared rules and knowledge.

- [Architecture](ARCHITECTURE.md):
  sidecar discovery, menu/gameplay phases, and entry-actor lifecycle.
- **Required before changes, builds, or installation:** [Mod rules](MOD-RULES.md)
  bind sidecar defaults, DML coexistence, and loader boundaries.
- [Candidate producer](Build-VoyageAutoLoader.ps1): builds, generates, cooks,
  packages, and validates the four-package asset-only loader.
- [Descriptor contract](DESCRIPTOR.md): accepted fields and runtime limits.
- [DML proposal](DML-PROPOSAL.md): external proposal context only; it is not
  an implementation or release contract.
- [Cooked-asset knowledge](../../docs/voyage-cooked-asset-toolchain-knowledge.md):
  DML discovery and package identity when comparing loaders.
