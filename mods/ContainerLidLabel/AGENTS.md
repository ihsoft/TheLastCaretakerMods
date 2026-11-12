# ContainerLidLabel index

Start with the [repository index](../../AGENTS.md) and its shared rules.

- [Architecture](ARCHITECTURE.md): DML lifecycle, exact supported classes,
  display ownership, and editor-only C++ boundaries.
- [Maintained project](Project/Voyage.uproject) and
  [synchronized BPT](Project/BlueprintText/): source assets and graph review.
  Use the shared [BPT workflow](../../tools/blueprint-workspace.md) for edits.
- [Build and validation](BUILD.md): the public candidate builder, editor tests,
  package boundary, and installation entry point.
- [Game-derived contracts](GAME_DERIVED_SOURCES.md): source identities,
  editor stand-ins, evidence boundaries, and revalidation.
- [Configuration](CONFIGURATION.md): LogicMods INI, defaults, snapshot ownership,
  and preservation of player settings.
- [Changelog](CHANGELOG.md): cumulative player-facing changes for publication.

The maintained Blueprint project is the implementation. Builds cook copies of saved assets;
there is no per-mod graph generator or stock class replacement. Keep native code
limited to editor contracts and the explicit test helpers described in architecture.
Keep Project/Content and Project/Config source-only. Run tests via
Test-ContainerLidLabel.ps1 and engine work in a temporary project copy as described
in BUILD.md; do not prepare plugins or create test fixtures in the maintained project.
