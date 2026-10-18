# PalletContainerSnap index

Start with [repository index](../../AGENTS.md) for shared rules and knowledge.
This investigation is paused; preserve its current work.

- [Architecture](ARCHITECTURE.md):
  four-slot placement, generator ownership, candidate routes, and validation limits.
- **Required before changes, builds, or installation:** [Stable mod rules](MOD-RULES.md)
  bind the public producer, authored slot data, and generator boundaries.
- Local ignored `BACKLOG.md`, when present: current checkpoint,
  candidate-specific constraints, pending gates, and next action; read its
  restart section before resuming.
- Local ignored `PLACEMENT-RESEARCH.md`, when present: mod-local inspection and
  hypotheses; open targeted sections only when the question needs them.
- [Candidate producer](Build-PalletContainerSnap.ps1) and
  [DML installation wrapper](Install-PalletContainerSnap.ps1): current public
  entry points; use the local backlog for active experimental routes.
- [Shared item knowledge](../../docs/voyage-item-fabrication-and-pickup-knowledge.md):
  placement, inventory, and pickup contracts when applicable.
