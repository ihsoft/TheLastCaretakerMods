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

## Shared Unreal execution

Agents share one Unreal execution resource across projects. Reserve the
[Unreal queue](../tools/unreal-coordination.md) before an engine build, commandlet,
cook, or editor session. BPT engine entry points reserve it automatically;
wrap other producers with `tools/Invoke-UnrealQueued.ps1`. Keep the reservation
through the whole operation, including subprocess gaps, validation and rollback.
Do not launch background engine children that outlive the reserved operation.

Contention is normal: let the script wait in its queue rather than treating a
busy engine as task failure, retrying commands, or asking another agent to stop.
Continue independent work while the queued process waits. Do not delete locks,
bypass the queue, or cancel somebody else's editor. An editor opened outside
the queue is observed as busy until its owner closes it; the queue cannot
prevent a human or an old tool from launching another uncoordinated process.
Warn the user before queuing an engine launch. Never close their editor without
authorization. Process-detection errors remain errors, not permission to run.

This reservation does not grant ownership of another task's source files.
Keep independent working directories and coordinate shared edits separately.
Same-workspace conflicts and changed-asset checks remain binding.

Project ownership does not authorize committing extracted stock game content.
The [game-derived data rule](game-derived-data-rule.md) still governs provenance
and Git eligibility. Required stock dependencies may remain locally prepared;
do not confuse them with authored mod assets or ship editor-only placeholders.

Existing procedural producers remain descriptions of the current implementation
until their mod is explicitly migrated. Migration must establish the maintained
project, preserve required dependencies, and replace destructive regeneration
of authored assets. Do not infer that a documentation change has migrated a mod.
