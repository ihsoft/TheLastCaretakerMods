# Unreal project ownership

For mods adopting the Unreal-project authoring workflow, the Unreal project is
the source of truth and contains all asset content that will enter the mod
release: Blueprints, models, textures, materials, and every other shipped asset.
The release producer cooks and packages that project content. The project is a
maintained source asset store, not disposable output recreated on every build.

External authoring inputs and focused project-update scripts are allowed. For
example, a script may import a separately maintained GLB and update a particular
mesh in the project. Its resulting release asset must reside in the project;
the external input does not replace the project's authority over release asset
state. This does not prohibit packaging scripts or non-asset release files such
as user configuration and documentation.

Use [BlueprintText (BPT)](../tools/blueprint-workspace.md#source-of-truth-and-git)
as the bidirectional text editing and Git review interface for Blueprints.
Commit its synchronized text representation alongside the corresponding project
assets. BPT is an intentional duplicate, not a second source of truth.

Project ownership does not authorize committing extracted stock game content.
The [game-derived data rule](game-derived-data-rule.md) still governs provenance
and Git eligibility. Required stock dependencies may remain locally prepared;
do not confuse them with authored mod assets or ship editor-only placeholders.

Existing procedural producers remain descriptions of the current implementation
until their mod is explicitly migrated. Migration must establish the maintained
project, preserve required dependencies, and replace destructive regeneration
of authored assets. Do not infer that a documentation change has migrated a mod.
