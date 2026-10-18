# Railgun index

Start with [repository index](../../AGENTS.md) for shared rules and knowledge.

- [Architecture](ARCHITECTURE.md):
  the current station, weapon, HUD, item, skill, registry, and their data flow.
  Read before changing behavior or deciding what to change in the source.
- **Required before any change, build, or installation:** [Mod rules](MOD-RULES.md)
  contain binding limits on the owned model, settings, producer, payload, and
  runtime validation. Follow them even when a source change looks small.
- [Shared INI recipe](../../docs/runtime-configuration-patterns-knowledge.md):
  canonical defaults, entry reload, and preservation of player settings.
- [Projectile modeling rule](modeling-four-plane-cut-nose-rule.md): the shape
  required when changing the projectile nose.
- [Input integration](../../docs/voyage-input-integration-knowledge.md): stock
  actions, contexts, and remapping when changing controls.
- [Item fabrication](../../docs/voyage-item-fabrication-and-pickup-knowledge.md)
  and [recipe unlocks](../../docs/voyage-scanning-and-recipe-unlocks-knowledge.md):
  stock consumers of Railgun's primary assets.
