# Railgun index

Start with [repository index](../../AGENTS.md) for shared rules and knowledge.

The user owns model geometry. The agent may move or wire model files and update
the role manifest, but must not alter GLB geometry unless explicitly requested.

- [Architecture](ARCHITECTURE.md):
  the current station, weapon, HUD, item, skill, registry, and their data flow.
  Read before changing behavior or deciding what to change in the source.
- [Project layout, build and tests](BUILD.md): maintained `Project/`, disposable
  working copies, public commands and accepting authoring changes.
  Use the shared [BPT workflow](../../tools/blueprint-workspace.md) for graph edits.
- [Primary DataAsset authoring](BUILD.md#primary-dataasset-authoring):
  ordinary Unreal Details editing and the native-read/cook validation boundary.
- [Settings parser](ARCHITECTURE.md#settings-parser): the mod-owned shared INI
  component, reload policy and schema bindings.
- [Candidate producer](Build-Railgun.ps1): use `-Install` for a routine game
  candidate so the owned INI is installed with the container.
- [Settings integration test](Test-Railgun.ps1): execute the saved Blueprint
  loader in a disposable project without cooking or installing a candidate.
- [Changelog](CHANGELOG.md): player-facing notes for the next Nexus release.
- [Shared INI recipe](../../docs/runtime-configuration-patterns-knowledge.md):
  canonical defaults, entry reload, and preservation of player settings.
- [Input integration](../../docs/voyage-input-integration-knowledge.md): stock
  actions, contexts, and remapping when changing controls.
- [Item fabrication](../../docs/voyage-item-fabrication-and-pickup-knowledge.md)
  and [recipe unlocks](../../docs/voyage-scanning-and-recipe-unlocks-knowledge.md):
  stock consumers of Railgun's primary assets.
