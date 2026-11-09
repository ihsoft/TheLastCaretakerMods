# BlueprintText workspace workflow

BPT is an **AI-to-Human interface**: the agent authors Blueprint behavior and
the human understands and reviews it through the preview. Human-readable graphs
with minimal nonessential noise are a core requirement; follow the
[human readability contract](../docs/blueprint-graph-design-rule.md#bpt-is-an-ai-to-human-interface).

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

### Create reflected native call nodes

Use `CallTemplates` when an ordinary native function is not in the initial
math catalogue. Supply exact `/Script/Module.DeclaringClass::Function` identities
in one batch. The existing workspace supplies the project and selected source's
parent class; select `-Asset` when the workspace contains several Blueprints.
The action prepares editor support and launches headless Unreal through the
shared queue. Warn before launch as for other engine actions.

```powershell
$calls = (& $tool -Action CallTemplates -WorkspaceRoot $work `
  -Operation @('/Script/Engine.SceneComponent::GetChildrenComponents',
               '/Script/Engine.BoxComponent::SetBoxExtent')) | ConvertFrom-Json
python -B tools/BlueprintText/asset_text_codec.py add-call $source `
  --catalog $calls.catalog --operation /Script/Engine.BoxComponent::SetBoxExtent `
  --id SetExtent --graph $graphGuid --output "$work/edited.json"
```

`$source` is the editable source from the workspace manifest; `$graphGuid`
selects its target graph. `add-call` writes a complete edited document to a
different path, allocates node/pin identities and merges pin metadata. Set its
arguments and links, replace the selected workspace source with the complete
proposal, then use ordinary Apply and Preview. This is not an automatic graph
builder or a request to add runtime guards.

`CallTemplates` returns a run-local `catalog`, engine version, parent context,
and one compact row per operation: native identity, `pure` and
`requiredConnections` (native input names whose defaults Unreal marks ignored).
The generated catalogue retains the engine's full native insertion metadata,
including reference/const types and `AutogeneratedDefaultValue`. Keep that
metadata when changing an authored default; it can determine whether an explicit
`false` survives reconstruction. A required reference input such as
`AddComponentByClass.RelativeTransform` needs a connection, not a textual default.

Reflection uses a transient Blueprint with the selected parent class. It does
not edit source JSON or save project Blueprint assets. The shared math catalogue
is not overwritten. `add-call` rejects a different parent context; standalone
`template` fragments retain `contextParent` for the same restriction. Reuse a
catalogue within its project/engine context and regenerate it when that context
changes. Existing catalogues without a parent context remain supported.

The supported boundary is ordinary native `UK2Node_CallFunction` nodes with
concrete pins. The request must name the function's declaring class. Blueprint
functions, events, private/protected functions, latent calls, wildcard/container
specialization, dynamic output typing and enum/bool execution expansion are
outside this factory. Internal functions are rejected except the tested
`Actor.AddComponentByClass` native call; this does not synthesize its editor
special node or expose-on-spawn assignments. Unsupported requests fail without
publishing a partial catalogue.

Templates reflect declarations loaded by the selected editor project. For game
APIs, correctness still depends on its native mirrors; this is not discovery from
the Shipping executable. Actual target connections, graph-context legality and
behavior remain the author's responsibility. Strict Apply/reopen comparison is
unchanged: do not accept a changed re-export automatically because it compiles.

### Extract an operation into a collapsed graph

Choose a logically complete operation and its name under the
[graph design rule](../docs/blueprint-graph-design-rule.md). `Collapse` performs
the mechanical extraction in one offline call; it does not choose the operation
or launch Unreal. Select the parent by GraphGuid or an unambiguous graph name,
and provide the existing node IDs. `Asset` is required when the workspace selects
more than one Blueprint. The new name must be a unique ASCII identifier in that
Blueprint.

```powershell
& $tool -Action Collapse -WorkspaceRoot $work -Asset '/Game/Mods/Example/ModActor' `
  -Graph 'CalculateMotion' -Node @('K2Node_CallFunction_12', 'K2Node_CallFunction_13') `
  -Name 'CalculateBraking' `
  -BoundaryNames @{ 'K2Node_VariableGet_4.out:Speed' = 'Speed' }
& $tool -Action Apply -WorkspaceRoot $work
& $tool -Action Preview -WorkspaceRoot $work
```

`BoundaryNames` is optional. Its keys are crossing **source** endpoints, for
both inputs and outputs; values name the new boundary parameters. Otherwise
the tool uses the source pin name, with unique `Value` names for generic results.
Execution boundary names remain `execute` and `then`.

The supported selection is one connected region: a pure calculation, or a
region with one execution entry and at most one continuation. Several callers
may share that entry; terminal branches retain their original termination.
Common call, get/set, branch, sequence, cast, struct, select, reroute and existing
collapsed nodes are supported. Entry/result/event nodes and other special
classes are rejected. Boundary pins require concrete scalar or array types;
wildcards, references, weak pointers, delegates, maps, sets and split pins are
outside this operation. Existing derived-to-base object/class connections keep
the source type. Other type conversions and dependency paths that leave the
selection and re-enter it are rejected. A rejection changes neither source nor
project assets; use its reason to choose a supported boundary or discuss a tool
extension, rather than bypassing the check.

The compact result reports parent counts, moved nodes, the new graph identity
and `inverseVerified`. That check expands the new boundary and compares original
node payloads, ownership and links. It is structural evidence, not a substitute
for strict Apply or task-appropriate runtime validation. Existing node/pin GUIDs
are preserved, including nested children. The tool supplies cumulative preview
annotations against the workspace's original review baseline. Source and
manifest writes recover together after interruption; a subsequent conflicting
edit stops recovery for reconciliation. `applied: false` means project assets
are still unchanged. Apply and Preview remain the ordinary next steps; no
manual tunnel construction, annotation pass or generated-output inspection is
needed.

### Workspace coordination

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
to get graph summaries, or add `--graph <GraphGuid>` for a compact graph with
its boundary context. Reveal exact details with `--node <id>` and optionally
`--pin <alias>`; `--view full` restores the complete selected-graph view.
Use `--context locals` or `--context components` for declarations and templates
needed by that selection. To find callers or variable accesses across explicitly
selected sources, use [member-use search](blueprint-text.md#find-member-uses).
See [source reading](blueprint-text.md#read-graphs-without-unreal) for the
selection and value semantics. These are offline read-only views; edit the
complete workspace source. No additional asset queries or per-mod extraction
scripts are needed.

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

If a failed Apply reports broken references between a selected Blueprint and a
known mutually referencing Blueprint outside the selection, export both into a
new workspace, reapply the intended edit and run Apply with both selected. The
unchanged peer must also pass strict comparison. This is a focused recovery path,
not a requirement to enumerate all dependencies before ordinary edits or a
guarantee that every reference cycle is supported. Selection remains explicit;
Apply does not automatically expand its write set.

On engine failure, the compact reason includes the first Blueprint compiler error
with its logged asset identity and the final tool error when available. Repeated
summary entries are omitted and each message is bounded; use the diagnostic log
for complete text. An earlier error in an unselected dependency remains visible.

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
operations do not erase the review. Preview automatically compares node identities,
represented fields and connections to mark added, changed and removed nodes.
Graph badges and node overlays use the same resolved class and pin definitions;
node/link ordering and equivalent type-table aliases do not count as edits.
This is structural comparison, not behavioral equivalence.
Optional `reviews` in a manifest asset entry supplements that comparison with
node IDs by GraphGuid or unambiguous graph name, using `added`, `changed` and
`removed` lists. Missing or incomplete lists do not hide detected changes.
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
