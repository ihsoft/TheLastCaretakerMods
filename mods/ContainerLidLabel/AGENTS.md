# ContainerLidLabel index

Start with the [repository index](../../AGENTS.md) and its shared rules.

- [Architecture](ARCHITECTURE.md): DML lifecycle, exact supported classes,
  display ownership, and editor-only C++ boundaries.
- [Maintained project](Project/Voyage.uproject) and
  [synchronized BPT](Project/BlueprintText/): source assets and graph review.
  Use the shared [BPT workflow](../../tools/blueprint-workspace.md) for edits.
- [Build and validation](BUILD.md): the public candidate builder, editor tests,
  package boundary, and installation entry point.
- [Candidate producer](Build-ContainerLidLabel.ps1): build the candidate and its
  validation summary. [Installer](Install-ContainerLidLabel.ps1) installs it
  with the owned INI policy.
- [Editor dependencies and provenance](ARCHITECTURE.md#editor-dependencies-and-provenance):
  follow the shared [game-derived data rule](../../docs/game-derived-data-rule.md).
- Settings: [schema](Settings/ContainerLidLabel.settings.json) and
  [distributed INI](Assets/ContainerLidLabel.ini); follow the shared
  [configuration pattern](../../docs/runtime-configuration-patterns-knowledge.md).
- [Changelog](CHANGELOG.md): cumulative player-facing changes for publication.
