# Railgun index

Start with [repository index](../../AGENTS.md) for shared rules and knowledge.

The user owns model geometry. The agent may move or wire model files, but must
not alter GLB geometry unless explicitly requested.

- [Architecture](ARCHITECTURE.md):
  the current station, weapon, HUD, item, skill, registry, and their data flow.
  Read when changing behavior, asset ownership or game contracts.
- [Project layout, build and tests](BUILD.md): maintained `Project/`, disposable
  working copies, public commands and accepting authoring changes.
  Use the shared [BPT workflow](../../tools/blueprint-workspace.md) for graph edits.
- [Model authoring and preview](BUILD.md#model-authoring-and-preview): project-owned
  model, GLB review copy and accepting an explicitly requested model replacement.
- [Images and audio](BUILD.md#images-and-audio): locate the saved resources and
  export or replace them through the shared authoring tool.
- [Primary DataAsset authoring](BUILD.md#primary-dataasset-authoring):
  BPT property editing, temporary previews and the native-read/cook boundary.
- [Settings parser](ARCHITECTURE.md#settings-parser): the mod-owned shared INI
  component, reload policy and schema bindings.
- [Candidate producer](Build-Railgun.ps1): use `-Install` for a routine game
  candidate so the owned INI is installed with the container.
- [Installer](Install-Railgun.ps1): install an existing candidate without rebuilding;
  [installation details](BUILD.md#install-an-existing-candidate) cover complete INI replacement
  and dirty-source policy.
- [Settings integration test](Test-Railgun.ps1): execute the saved Blueprint
  loader in a disposable project without cooking or installing a candidate.
- [Changelog](CHANGELOG.md): player-facing notes for the next Nexus release.
- [Shared INI recipe](../../docs/runtime-configuration-patterns-knowledge.md):
  canonical defaults, entry reload, and complete INI distribution.
- [Input integration](../../docs/voyage-input-integration-knowledge.md): stock
  actions, contexts, and remapping when changing controls.
- [Item fabrication](../../docs/voyage-item-fabrication-and-pickup-knowledge.md)
  and [recipe unlocks](../../docs/voyage-scanning-and-recipe-unlocks-knowledge.md):
  stock consumers of Railgun's primary assets.
