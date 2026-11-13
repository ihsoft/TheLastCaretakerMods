# Unreal project ownership

The recommended development workflow is a maintained Unreal project with BPT
for Blueprint authoring. In this workflow, the project is the source of truth
and contains all asset content that will enter the mod
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

## Project layout and temporary data

Keep each mod's maintained Unreal project in its own `mods/<Mod>/Project/`
directory. Place the `.uproject` and its project-owned inputs there. Do not mix
Unreal project directories with mod-level documentation, release scripts,
packaging inputs or other files at the mod root. Existing flat layouts require
an explicit, scoped migration that updates their consumers; they are not a
pattern for new projects or reorganizations.

The maintained Project directory contains repository source: authored assets,
synchronized BPT, required project configuration, editor-support sources and
test sources. Deliberately authored reusable test assets also belong in source,
with a test-only package boundary that excludes them from the release.
The fact that an asset or configuration file was written by an editor does not
make it temporary; its role in the maintained project determines ownership.

Keep `Content/` and `Config/` source-only. Do not hide per-run fixtures, temporary
INI files or incidental editor settings there with Git exclusions. Required
configuration belongs in version control. Within the maintained project, only
`Intermediate/` may serve as an explicitly temporary, Git-ignored directory;
do not accumulate other generated directories beside the sources.

Run tests and builds against disposable copies of the maintained project under
the owning mod's `Tmp/<Mod>/` directory. Copy current source inputs, including
intended uncommitted changes, rather than regenerating assets or copying stale
execution output. Prepare plugins and run Unreal in the copy, so caches, logs,
generated configuration, test fixtures and cook output stay outside the source
project. Test and build runs must not write results back into the source project.
Test code and reusable fixtures remain repository data; data created by a test
run belongs to that run.

Use a working copy for interactive Unreal/BPT authoring as well: directly
opening the maintained project in Unreal recreates standard generated folders.
Before accepting authoring changes, check for concurrent source edits and review
the exact changes. Transfer only intentionally changed saved assets, synchronized
BPT and deliberately authored configuration or other source files. Never copy
the whole working project back. A working copy is not a second source of truth.

Refreshing game-derived project inputs is a source-update stage, separate from
the disposable build. Review and accept the refreshed assets, owned patches,
provenance and synchronized BPT into the maintained project through the same
authoring boundary before taking the build copy. A build must not silently
substitute refreshed release assets or write them back as build output.
Accepting a source update does not itself authorize a commit or establish gameplay.

Follow the [generated-output lifecycle](artifact-lifecycle-rule.md) for cleanup
and failure evidence. Keep concrete copy, test, build and authoring commands in
the owning mod or tool guide; do not duplicate them in this rule.

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

The [game-derived data rule](game-derived-data-rule.md) governs provenance and
Git eligibility for maintained game-derived project assets and compilation
stubs. Retain required source assets in the project, with bounded refresh or
revalidation, rather than treating all project content as disposable extraction
output. Keep intermediate dumps ignored and exclude editor-only placeholders
from the release payload.

Pre-migration implementations and their temporary constraints belong in the
owning mod's documentation, not in shared rules as alternative development
patterns. Migration to the recommended workflow is explicit, scoped work:
establish the maintained project, preserve required dependencies, and replace
destructive regeneration of authored assets. A documentation change alone
does not migrate a mod.
