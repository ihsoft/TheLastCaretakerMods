# BlueprintText workspace workflow

Use [Invoke-BlueprintWorkspace.ps1](BlueprintText/Invoke-BlueprintWorkspace.ps1)
for the recommended Blueprint authoring workflow for new and existing mods:
export, edit JSON, apply and preview. The Unreal project is the source of truth,
under the [project ownership rule](../docs/unreal-project-rule.md).
This workflow does not generate stubs, cook, install or run the game. Supported
Blueprint state is defined by the [format guide](blueprint-text.md#schema-2-blueprints).

Follow the [routine agent rules](BlueprintText/AGENTS.md#routine-agent-work).
Successful command results are the acceptance signal for mechanical work.
Generated HTML, images, native intermediates and full logs are not agent inputs.

## Source of truth and Git

BlueprintText (BPT) is a bidirectional editing interface, not a replacement for
the Unreal project. Export snapshots supported Blueprint state into compact
JSON; Apply writes proposed changes back to project assets. Unapplied JSON edits
are pending changes. Changes may also originate in Unreal Editor; export the
current saved project state before starting a new BPT editing cycle.

Commit BPT JSON alongside its corresponding authored Blueprint assets. This is
an intentional, meaningful duplicate: GitHub cannot expose binary asset changes
as useful text diffs, while BPT makes their represented structure reviewable.
At commit time, BPT must match the saved project asset. After editor changes,
refresh the export. After JSON changes, successfully Apply and use the tool's
fresh-process comparison as the synchronization check. Subsequent asset edits
invalidate that check. Do not commit stale exports or unapplied proposals as a
completed asset change. Preserve and reconcile both sides of any conflict.

Keep the committed BPT representations in the owning mod's tracked source tree,
with an unambiguous mapping to project asset paths. Working copies and manifests
under `Tmp/BlueprintText/` remain temporary. Copy only synchronized Blueprint
JSON into the tracked representation; baselines, backups, native intermediates,
diagnostics, preview resources and rendered previews remain tool-owned output.
This retention rule does not change the tool's workspace path requirements or
authorize committing extracted stock game content.

Adopting BPT replaces procedural graph authoring with export-edit-apply. Routine
release builds use project assets; they must not recreate them from BPT exports
or clear authored project content. Focused project-update scripts remain allowed
under the project ownership rule.

## Start a workspace

For an existing mod, export its uncooked Blueprints, generating the initial assets
with its current C++ producer when needed. For a new mod, first prepare the Unreal
project with the required parents, native modules and resources. Create complete
schema-2 Blueprint JSON, then add it to a workspace's source list as described
below; new assets use the same shared Apply path. The current Export entry point
starts from at least one existing Blueprint, so an empty project's first
Blueprint must be created in the editor before that first export. A reusable
UObject source can instead initialize a workspace with `AddSource`, described
below. Subsequent Blueprints can also be added directly through JSON.

Use an existing Unreal project with its required resources. Declare native
dependencies in [EditorSupport.json](unreal-editor-support.md#prepare-declared-project-dependencies)
when it needs owner-supplied native classes. Warn before engine actions; the
shared queue waits for Unreal to be free. Export, changed Apply and Preview
prepare those dependencies and the BPT tooling themselves. No separate preparation,
DLL selection or project build is needed before a normal workspace call.

New consumers receive a ready `BlueprintTextWorkspace` plugin without C++ or
build sources. Unchanged provider sources and editor identity reuse its binaries;
changed inputs rebuild through the common support tool. This does not generate
game assets or refresh a mod's DataAssets. `Prepare` remains available as an explicit
dependency/tooling-only operation.

Existing explicit source providers keep their supported path: preparation
refreshes the `BlueprintText` module from the shared template and builds their
project. Before replacing different module files it preserves their complete
previous contents in `moduleBackup`; other project modules are not replaced.
Moving an existing project's native definitions to a binary provider is an explicit
owner migration, not automatic source deletion.
Keep workspaces under `Tmp/BlueprintText/`.
Existing workspaces and installed shared modules using the previous names remain
supported by the current entry points. Run Prepare to refresh their module; do
not rename generated projects or move existing preview output just to match the
new tool directory. New projects and workspaces use the current names.
The current implementation selects ordinary uncooked Actor/Widget and direct UObject Blueprint
packages under `/Game/`; split packages are rejected. Use `-EngineRoot` when the
project uses an engine outside the wrapper's default installation.

```powershell
$tool = './tools/BlueprintText/Invoke-BlueprintWorkspace.ps1'
$project = 'R:/path/to/Project.uproject'
$work = './Tmp/BlueprintText/edit-example'
& $tool -Action Export -ProjectFile $project -WorkspaceRoot $work `
  -Asset @('/Game/Mods/Example/ModActor', '/Game/UI/ExampleHUD')
```

Export returns the workspace manifest, which lists editable source paths.
Widget preview resources are handled by the tool. It does not export every asset
type or turn the workspace into a standalone copy of the project.

| Output | Role |
| --- | --- |
| `workspace.json` | Selection of project assets and working JSON paths, plus optional review annotations. |
| `blueprints/**/*.json` | Editable snapshots of supported Blueprint state. |
| Adjacent `*.resources/` bundles | Tool-owned widget resource snapshots from Export; Apply does not refresh them. |
| `.workspace/` | Tool-owned baselines, backups, diagnostics and generated previews. |

The original export is the review baseline; subsequent Apply operations update
the accepted project baseline used for conflict detection. To start a new review
baseline, export into a new workspace.

## Edit and apply

### Add a reusable UObject source

`AddSource` stages a copy of a schema-2 UObject BPT in a new or existing workspace.
It is offline: it does not build, launch Unreal or write project assets. The
destination is an explicit `/Game` package path, normally under the consuming
mod. The source file is preserved. A new workspace requires `ProjectFile`; an
existing workspace keeps its project and baseline. Existing destination assets
or selected sources are rejected; export existing assets to edit them.

```powershell
& $tool -Action AddSource -ProjectFile $project -WorkspaceRoot $work `
  -SourcePath './tools/BlueprintText/Library/IniFileParser/BP_IniFileParser.json' `
  -Asset '/Game/Mods/Example/Settings/BP_IniFileParser'
# Review/edit the staged source, then apply through the normal transaction:
& $tool -Action Apply -WorkspaceRoot $work
```

The reusable [INI parser library](BlueprintText/Library/IniFileParser/AGENTS.md)
supplies that source at `tools/BlueprintText/Library/IniFileParser/BP_IniFileParser.json`.
Each consumer gets its own package; there is no shared runtime
package or automatic library-update relationship. After Apply, retain the
synchronized source with the mod as usual and cook its project normally.

Relocation rewrites supported own-package references, including class pin types,
function member owners and object defaults. String values and comments remain
literal. It preserves the Blueprint object name; if the new package basename
differs, `objectName` records the original name. It does not rename graphs,
variables or functions, move external dependencies, or edit other Blueprints.
An unrecognized field mentioning the old package is rejected with its location.
Currently this operation accepts direct `/Script/CoreUObject.Object` sources
without component/widget sections. Complex object-bearing struct/container
defaults may need additional typed relocation support; they fail explicitly.

### Change selected assets

Read `workspace.json` for the selected Blueprint/source mapping, then edit the
relevant files under `blueprints/`. They describe complete proposed Blueprint
state to apply to the project. Leave technical files to the tool. To add a Blueprint, add its compact
JSON and a corresponding source entry to the manifest. Deletion of whole assets
is not supported; removing a baseline entry or losing a source file must not be
used as a delete operation.

Use a separate workspace for each independent task. All commands hold an OS lock
on their workspace. Prepare, Export, Apply, Preview, MarkStubs and Recover also lock the project and
join the [shared Unreal queue](unreal-coordination.md) for their entire operation,
including rollback. Engine work waits its turn even for different projects.
Project and older engine gates also wait for their owners to finish; never delete
their lock files. A conflicting command in the same workspace still fails to
protect task ownership. The OS releases locks when their owning process exits.
Direct JSON editing in separate workspaces can proceed independently of engine work.
Direct file edits are outside these locks; follow the task ownership rules above.
Other Unreal producers must use the shared queue wrapper. Asset conflict checks still apply when sequential
tasks touch the same exported asset.

For a large source, use `python -B tools/BlueprintText/graph_hierarchy.py <source>`
to get graph summaries, or add `--graph <GraphGuid>` to read one graph with its
boundary context and required definitions. This is an offline read-only view;
edit the complete workspace source. Collapsed graphs use the
[explicit hierarchy](blueprint-text.md#collapsed-graphs), with no additional
asset queries or per-mod extraction scripts.

```powershell
& $tool -Action Apply -WorkspaceRoot $work
& $tool -Action Preview -WorkspaceRoot $work
```

Apply checks for conflicting project edits, backs up selected packages, rebuilds
and compiles them, then batch-exports all selected packages in one fresh process
after all import levels finish. Every selected asset, including unchanged ones,
is compared against its desired JSON. Missing, unreadable or mismatched exports
fail the operation.
A conflict detected before the selected-package write phase leaves those packages
untouched. Failures in the write phase restore selected original packages;
unrelated project resources stay outside the selected write set. The command
reports a no-op if nothing changed.
Inheritance ordering and compatible batches are tool-owned. Unsupported
dependency arrangements produce a diagnostic rather than requiring an agent to
invent a per-mod build procedure.

Apply verifies Blueprint state without generating or copying preview images.
Run Preview to collect current widget resources after Apply. A Preview failure
does not roll back applied packages or change the accepted project baseline.

On a conflict, preserve both the JSON edits and the editor changes. Export the
current project state into a separate workspace and reconcile the intended
change; do not remove conflict checks or overwrite the newer asset manually.
After an interrupted operation, use the tool's `Recover` action and its recovery
diagnostic before continuing. If Apply's journal records preparation only,
Recover clears it without changing packages or the accepted baseline and reports
`no-recovery-needed`. Once Apply enters the write phase (before deleting selected
packages), recovery restores the state before that operation; it first retains
current selected files in `recoveredCandidates`, including any possible manual
edits made after interruption. Older journals without a phase flag retain this
restoration behavior. Reconcile retained candidates separately if needed. Do not
move backup files by hand.

```powershell
& $tool -Action Recover -WorkspaceRoot $work
```

## Hand off the preview

Run `Preview` once and return its `previewUrl`. The command checks selected
assets for changes since export/apply, prepares the shared tooling and gathers
current project assets and references in Unreal before rendering offline.
For saved selected Widget Blueprints, it collects fresh resource bundles for
that preview instead of reusing the adjacent Export snapshots. The original
Before resource snapshots remain unchanged for review.
Warn before this engine action. Do not manually enumerate dependencies, inspect
the catalog, refresh the module separately, or assemble a preview manifest.
The compact result reports editable cards, listed assets, graphs, stubs, missing
local dependencies and unsupported Blueprints. A collection failure is an error,
not permission to present an incomplete preview as successful.

The preview shows what can be changed through BPT. Supported Blueprint sources
have graph/widget cards. References appear at their actual graph node, pin or
named Blueprint property, with stub or missing-local-asset labels when known.
Other assets, including DataAssets and unselected editor stubs, appear only in
a compact reference list at the bottom. Their presence is not a claim of BPT
editing support. In particular, a stub's empty graphs are not the original game
implementation. An explicitly selected stub can still be reviewed as an editor
contract. Discovered assets do not expand the workspace's Apply selection.

The same Preview call includes collapsed graphs automatically: an expandable
tree, operation-to-child links, ancestor navigation, local/descendant counts and
labels on operations whose nested implementation changed. Before links stay in
their own snapshot. The agent does not need to enumerate child graphs or render
them individually.

Blueprint status is compared with the original export, so successful Apply
operations do not erase the review. Optional `reviews` in a manifest asset entry
supplies known node IDs by GraphGuid or unambiguous graph name, using `added`,
`changed` and `removed` lists.
Optional `note` adds explanatory text. These annotations do not alter the Blueprint.

Read diagnostics only on failure: start with the returned short reason and
diagnostic path, then inspect the relevant evidence. A successful render does not
require screenshot inspection, link crawling or a second comparison by the agent.

## Editor stub identity

Stub creators must set asset metadata `BlueprintText.Role` to `stub`. Supported
BPT exports preserve it as optional `editorRole: "stub"`, and Apply restores it
on the project asset. The preview does not guess this role from asset names,
empty graphs or absence from a release package. An unmarked asset is unclassified.

For a one-time migration of verified existing stubs outside the workspace's
editing selection, use the common action below. It backs up the selected asset
packages, saves the marker, verifies it in a fresh process and restores originals
on failure. This is not a prerequisite to repeat before every preview.

```powershell
& $tool -Action MarkStubs -WorkspaceRoot $work -Asset @('/Game/Dependencies/ParentStub')
```

For a supported Blueprint already selected for editing, set `editorRole` in its
BPT source and use Apply instead. A manual workspace `placeholder` hint cannot
override contradictory project/source metadata. Marking an asset does not create
it, reconstruct its game implementation or determine whether it ships in a mod.

## Tool maintenance checks

The offline transaction and concurrency tests do not launch Unreal:

```powershell
python -B tools/BlueprintText/test_blueprint_workspace.py
```

Engine acceptance requires a disposable project with the relevant dependencies.
Compile/save and fresh-export comparison verify represented editor state, not
cooked output or gameplay behavior.
