# Mod setup and Unreal project ownership

Use this setup when creating or unifying a mod that maintains Unreal assets.
The [new-mod workflow](new-mod-rule.md) owns the earlier, implementation-free
skeleton. The layout and command conventions below are the target for new
implementations and explicit migrations; they do not assert that every existing
mod already implements the same interface. Until migrated, use its documented
public commands.

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
Prepare required resources, parents and native dependencies before graph work.
Do not introduce a per-mod C++ graph builder for routine Blueprint authoring.
A procedural step is appropriate only when the supported declarative format
cannot express the required state or would be disproportionately complex;
keep that exception bounded and explicit.

## Standard mod layout

Use the mod directory name as the stable technical identifier in script names
and `Tmp/<Mod>/`. A display name may contain spaces without changing that identity.
Create optional paths only for actual inputs or supported operations; do not
create empty folders or placeholder scripts to complete the table.

| Path below `mods/<Mod>/` | Owner and applicability |
| --- | --- |
| `AGENTS.md`, `ARCHITECTURE.md`, `BUILD.md`, `README.md` | The four [document roles](repository-documentation-rule.md); document local contracts and commands without copying shared rules. |
| `CHANGELOG.md`, `version.txt` | Publication input and published version under the [release workflow](release-and-installation-rule.md). |
| `Project/Voyage.uproject` | Maintained project descriptor. |
| `Project/Content/` | Saved release assets and required editor dependencies; distinguish their package ownership explicitly. |
| `Project/BlueprintText/` | Synchronized BPT, mirroring virtual asset paths. |
| `Project/Config/` | Deliberately maintained project configuration, when needed. |
| `Project/EditorSupport.json`, `Project/EditorSupport/` | Native editor dependencies, when needed; use the [shared dependency declaration](../tools/unreal-editor-support.md#prepare-declared-project-dependencies). |
| `Assets/` | External authoring inputs and distributable files such as `<Mod>.ini`; saved Unreal assets belong in `Project/Content/`. |
| `Settings/` | Settings schemas, bindings or focused template checks when the configuration requires them. |
| `Build/` | Mod-specific packaging inputs, serialization adapters, package checks and explicit source-update commands, only where shared tools do not cover the contract; never build output. |
| `slideshow/` | Publication media, when available. |
| `game-build.json` | Accepted source-build pin when a game-bound contract requires it; provenance and update handling remain with the [game-derived data rule](game-derived-data-rule.md). |

The standard build/test/install and project-copy scripts below live at the mod
root. Internal helpers stay with their owning responsibility instead of creating
a second set of root-level build or
test commands. Ignore rules belong to the owning mod or project under the
[repository hygiene rule](repository-hygiene-rule.md).

Virtual package paths are a separate contract from this filesystem layout.
Use `/Game/Mods/<Mod>/` for ordinary owned content, but preserve stock-reference
identities and any native discovery roots required by an asset consumer. An
on-disk reorganization must not silently rename runtime packages, classes or
saved-state identities. The release inventory must exclude editor support,
stock stand-ins and test-only assets.

## Public command conventions

Use one public entry point per operation. These names and common fields define
the migration target, not an additional tool API already supplied by the
skeleton generator. `BUILD.md` records the interface actually available.

| Entry point | Contract |
| --- | --- |
| `New-<Mod>ProjectCopy.ps1 -Destination <path>` | Copy maintained inputs to a fresh working project below `Tmp/<Mod>/`, including intended uncommitted source. Return the copied `.uproject` path. Do not launch Unreal. |
| `Build-<Mod>.ps1` | Produce and validate one candidate from a disposable copy. Return `status: built`, `archivePath`, `releaseManifestPath` and `validationSummaryPath`. Do not install by default. |
| `Test-<Mod>.ps1` | Run the mod's supported automated checks, creating a disposable project only for checks that need Unreal. Return `status: passed` and `reportPath`, with coverage and limitations in the report. Do not cook a release or install it as an implicit test step. |
| `Install-<Mod>.ps1 -ReleaseManifest <path>` | Provide the mod-owned installation policy when settings or other coupled payloads require it. Install the supplied candidate without rebuilding. For a standard container-only payload, use the shared installer directly instead of creating a pass-through wrapper. |

Build and test accept `-OutputRoot` for a fresh run directory anywhere below
`Tmp/<Mod>/`; omission allocates one automatically. Engine-consuming entry points
accept `-EngineRoot`; operations that inspect or install the game accept
`-GameRoot`. Build exposes `-AllowDirtySource` and, when a source-build pin is
used, `-IgnorePinnedGameVersion`; installation forwards the applicable dirty
candidate policy. These flags retain the meanings in the shared release workflow.
Local installation defaults must be overridable; do not bake a particular
workstation path into the shared setup.

Return one compact structured success result, with detailed diagnostics in the
run directory. Missing or failed required reports fail the command rather than
yielding a success result. Keep the common path field names even when adding
mod-specific fields. Test implementations may use different languages or engine
entry points without changing the public test interface.

A convenience `Build -Install` may call the same installation implementation
after building; it must not be the only way to install an existing candidate.
Source-update commands, when needed, live in `Build/Update-<Mod><Subject>.ps1`
and remain explicit authoring operations, separate from build and test. Invoke
them directly; do not add root-level forwarding wrappers. Each command owns its
parameters and enters the shared Unreal queue when it uses the engine.

## Shared mechanics and mod-owned contracts

Mod entry points select inputs and checks; reusable execution belongs in shared
tools. Do not copy another mod's producer or project-copy implementation as a
template. Reuse the existing public tools for BPT and dependency preparation,
the Unreal queue, game/mapping resolution, container verification, archive
creation and release installation. Find the supported entry point through the
[tool index](../tools/tool-index.md).

Use the [shared mod pipeline](../tools/mod-setup.md#shared-mod-pipeline) for
working copies, run paths, editor-test orchestration, cook, container packaging,
explicit source-update acceptance and cleanup. Keep path containment, linked-path
rejection, source selection and failure evidence there rather than in per-mod
copies. Extend that shared owner
when a recurring mechanism is missing and has an established consumer; keep
mod entry points thin. One mod-owned release inventory drives cook, staging and
container verification. Select tagged versus unversioned cook as data for the
shared cooker; a different serialization mode does not justify a local cooker.
A required cooked-asset conversion may sit between the shared cook and container
stages. Game script-object metadata and large-inventory transport also belong to
the shared tools, rather than per-mod extraction or native cook adapters.
Document reusable options in the tool guide and each mod's reasons in its
architecture. The skeleton tool still creates only the minimal mod skeleton,
not an implemented project or complete producer.

The mod still owns its package inventory, runtime entry identity, settings
policy, native mirrors, behavioral assertions and any required serialization
adapter. Keep those as data or focused code consumed by the shared mechanism.
Use `EditorSupport.json` for supported native dependency declarations rather
than inventing another loader. Do not force every mod to use DML, a flat settings
schema, a native commandlet or a particular cook profile. A real consumer
contract justifies such differences; copied orchestration does not.

## Project layout and temporary data

Keep project-owned inputs under `Project/`, separate from mod-level documents,
release scripts and packaging inputs. Existing flat layouts require an explicit
migration that updates their consumers.

The maintained project contains repository source. Deliberately authored
reusable test assets also belong in source,
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

## Unifying an existing mod

First identify its actual source inputs, public callers, release inventory and
runtime identities. Map them to the layout and operations above. Migrate one
coherent operation at a time: replace duplicated mechanics with a shared owner,
update callers and `BUILD.md`, and remove the superseded path once consumers
have moved. Keep only necessary temporary compatibility adapters, with their
remaining callers identified in the migration work; do not leave two canonical
producers. Do not copy implementation from a named mod as the normative design.

Validate at the changed boundary: copy and path rejection for a copy helper,
actual suites and report failures for a test runner, package inventory and
archive/manifest checks for a producer, and preservation/recovery for an
installer. Preserve editor-only exclusions and the source/working-copy boundary.
Changing behavior, runtime identity or serialization also requires the applicable
game acceptance. A filename-only change does not itself require a new game test.

Pre-migration implementations and their temporary constraints belong in the
owning mod's documentation, not in shared rules as alternative development
patterns. Migration to the recommended workflow is explicit, scoped work:
establish the maintained project, preserve required dependencies, and replace
destructive regeneration of authored assets. A documentation change alone
does not migrate a mod.
