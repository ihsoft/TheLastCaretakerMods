# Repository index

Start here for repository work. This file is a map, not a replacement for the
linked documents. For a mod task, open its `mods/<Mod>/AGENTS.md` first: it
identifies the architecture to read before working on that mod, any binding
mod rules, and the document owning build and installation details.
When no mod exists yet, start with the new-mod workflow below.

Shared `docs/` contains repository-wide rules and reusable knowledge. A
`-rule.md` file constrains work; follow it or raise a conflict with the user.
A `-knowledge.md` file records useful findings about the game, engine, or
established repository methods. Recheck it when evidence disagrees. Choose
the relevant links below by the task at hand;
do not load every document just because it is indexed here.

## Repository and process rules

- [Documentation ownership](docs/repository-documentation-rule.md): what belongs
  in shared docs, mod docs, an ignored backlog, or generated storage.
- [Starting a new mod](docs/new-mod-rule.md): find related game knowledge and
  create a minimal, honest mod directory before selecting an implementation.
- [Research workflow](docs/research-workflow-rule.md): investigate, record
  evidence, maintain a large-task backlog, and commit intermediate findings.
- [Generated-output lifecycle](docs/artifact-lifecycle-rule.md): choose `Tmp/`
  or retained `artifacts/`, including cleanup after a failed run.
- [Release and installation](docs/release-and-installation-rule.md): published
  versions, candidates, packages, installation, and rollback.
- [Game-derived data](docs/game-derived-data-rule.md): mappings, provenance,
  Steam build gates, public tools, and build compatibility.
- [Cooked-asset toolchain rules](docs/voyage-cooked-asset-toolchain-rule.md):
  tool use, DML installation, and validation levels.
- [Game experiments](docs/game-experiment-rule.md): live-game safety and
  evidence needed before accepting behavior.
- [Error handling](docs/error-handling-rule.md): defined outcomes and recovery
  from failures in mods and tools.
- [Repository hygiene](docs/repository-hygiene-rule.md): Git coordination,
  source conventions, and cleanup boundaries.
- [Subagent workflow](docs/subagent-workflow-rule.md): when and how to delegate,
  report, review, and hand off installation.
- [Agent work measurement](docs/agent-work-measurement-rule.md): optional
  diagnostics when the user asks for resource accounting.
- [Vehicle and HUD rules](docs/vehicle-and-hud-rule.md): shared implementation
  boundaries for lifecycle, input, and presentation.

## Reusable knowledge

- [Boat resource sockets](docs/boat-resource-socket-architecture-knowledge.md):
  power and Diesel connectors, registration, and hover-card data flow.
- [Gameplay observations](docs/gameplay-observations-knowledge.md): measured
  resource units and balancing values.
- [Surface normals](docs/modeling-surface-normals-knowledge.md): geometric
  orientation, shading exceptions, and useful checks.
- [Research pitfalls](docs/research-pitfalls-knowledge.md): observed failures
  and misleading approaches worth avoiding or rechecking.
- [Runtime INI configuration](docs/runtime-configuration-patterns-knowledge.md):
  a repeatable settings recipe, session-once versus entry reload, and
  installation of user-edited files.
- [Vortex release archives](docs/vortex-release-archive-knowledge.md): ZIP
  layouts for plain patches, extra files, content plugins, and DML actors;
  read when building or reviewing a release.
- [Vehicle and HUD patterns](docs/vehicle-and-hud-modding-patterns-knowledge.md):
  shared actor lifecycle, provided actions, and widget behavior.
- [Cooked-asset toolchain knowledge](docs/voyage-cooked-asset-toolchain-knowledge.md):
  serialization, registries, DML discovery, and known format boundaries.
- [Input integration](docs/voyage-input-integration-knowledge.md): stock action
  identity, mappings, device settings, and hint ownership.
- [Item fabrication and pickup](docs/voyage-item-fabrication-and-pickup-knowledge.md):
  item serialization, inventories, crafting, and transfer behavior.
- [Scanning and recipe unlocks](docs/voyage-scanning-and-recipe-unlocks-knowledge.md):
  scanner completion, item flags, and vault grants.

## Entry points outside shared docs

- [Tool index](tools/tool-index.md): choose a public script by intent and read its
  supported interface before inspecting implementations.
- [Git coordination](tools/repository-git-coordination.md): required semaphore
  and exact-path commit wrapper for Git mutations.
- [Artifact families](artifacts/ARTIFACT-FAMILIES.md): owners and consumers of
  retained ignored output; read local `AGENTS.md` before changing it.
- [Mapping registry](mappings/Voyage/): reviewed `.usmap` entries and their
  versioned manifests; resolution is owned by the tool index.
