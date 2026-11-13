# ContainerLidLabel index

Start with the [repository index](../../AGENTS.md) and its shared rules.

- [Architecture](ARCHITECTURE.md): DML lifecycle, exact supported classes,
  display ownership, and editor-only C++ boundaries.
- [Maintained project](Project/Voyage.uproject) and
  [synchronized BPT](Project/BlueprintText/): source assets and graph review.
  Use the shared [BPT workflow](../../tools/blueprint-workspace.md) for edits.
- [Build and validation](BUILD.md): the public candidate builder, editor tests,
  package boundary, and installation entry point.
- [Editor dependencies and provenance](ARCHITECTURE.md#editor-dependencies-and-provenance):
  follow the shared [game-derived data rule](../../docs/game-derived-data-rule.md).
- Settings: [schema](Settings/ContainerLidLabel.settings.json) and
  [distributed INI](Assets/ContainerLidLabel.ini); follow the shared
  [configuration pattern](../../docs/runtime-configuration-patterns-knowledge.md).
- [Changelog](CHANGELOG.md): cumulative player-facing changes for publication.

The maintained Blueprint project is the implementation. Builds cook copies of saved assets;
there is no per-mod graph generator or stock class replacement. Keep native code
limited to editor contracts and the explicit test helpers described in architecture.
Keep Project/Content and Project/Config source-only. Run tests via
Test-ContainerLidLabel.ps1 and engine work in a temporary project copy as described
in BUILD.md; do not prepare plugins or create test fixtures in the maintained project.
