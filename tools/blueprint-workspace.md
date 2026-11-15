# BlueprintText workspace workflow

BPT is an **AI-to-Human interface**: the agent authors Blueprint behavior and
the human understands and reviews it through the preview. Human-readable graphs
with minimal nonessential noise are a core requirement; follow the
[human readability contract](../docs/blueprint-graph-design-rule.md#bpt-is-an-ai-to-human-interface).

Use [Invoke-BlueprintWorkspace.ps1](BlueprintText/Invoke-BlueprintWorkspace.ps1)
for the recommended Blueprint authoring workflow for new and existing mods:
export, edit JSON, apply and preview. The Unreal project is the source of truth,
under the [project ownership rule](../docs/unreal-project-rule.md).
For maintained mods, run Unreal/BPT against a working project copy under
`Tmp/<Mod>/`, created from current source inputs by the owning mod's authoring
entry point. The BPT workspace is separate temporary text/review state; Export
does not create that project copy. Generated plugins, caches and logs stay in
the working copy, not in the maintained `mods/<Mod>/Project/`.
This workflow does not generate stubs, cook, install or run the game. Supported
Blueprint state is defined by the [format guide](blueprint-text.md#schema-2-blueprints).

Successful command results are the acceptance signal for mechanical work.
Generated HTML, images, native intermediates and full logs are not agent inputs.
Start from the workspace manifest and relevant editable source. On failure,
read only the diagnostic evidence needed for that failure; do not manually
repeat a successful export comparison. Keep the documented workflow in shared
tools instead of adding per-mod orchestration scripts. If a required operation
is missing, report the gap or address it through shared tool maintenance.

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

Before accepting working-copy changes, check for concurrent edits in the
maintained project and review the intended changes. Transfer only intentionally
changed saved assets, synchronized BPT and deliberately authored configuration
or other source files. Do not copy generated directories or the entire working
project back. Within the maintained Project, only `Intermediate/` may be a
temporary ignored directory; no `Plugins/` or `Saved/` exclusions are needed.

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

For an existing maintained project, export its saved uncooked Blueprints.
A pre-migration mod follows its own documented migration boundary; its old
producer is not the shared authoring recipe. For a new mod, first prepare the
Unreal project with the required parents, native modules and resources. Create complete
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

New consumers receive a ready `BlueprintTextWorkspace` plugin without C++
implementation sources. Its generated precompiled module rules support ordinary
Win64 Development Editor C++ builds without disabling the plugin; they are not
an editable source provider. Unchanged provider sources and editor identity reuse
its binaries, including across fresh working copies through the
[shared provider cache](unreal-editor-support.md#shared-provider-cache).
Changed inputs select a matching cached variant or rebuild through the common
support tool; each working copy still gets a fresh load check. This does not generate
game assets or refresh a mod's DataAssets. `Prepare` remains available as an explicit
dependency/tooling-only operation.
Projects with saved BPT call nodes keep `BlueprintTextWorkspace` as a required,
enabled plugin (`Enabled: true`, without `Optional: true`). On a clean checkout,
the same workspace preparation installs declared native support and BPT even
when its plugin directory is absent. Keep the required reference in the project;
no manual plugin disabling or DLL copying is needed. Preparation finishes by
loading BPT with the project dependencies enabled.
Existing binary installations update their package format on preparation. If a
previous build disabled the plugin, follow [receipt recovery](unreal-editor-support.md#recovery)
before resuming asset operations.

Existing explicit source providers keep their supported path: preparation
refreshes the `BlueprintText` module from the shared template and builds their
project. Before replacing different module files it preserves their complete
previous contents in `moduleBackup`; other project modules are not replaced.
Preparation also updates the owned BPT module type to `UncookedOnly`, preserving
the previous project descriptor in the run evidence when it changes.
Moving an existing project's native definitions to a binary provider is an explicit
owner migration, not automatic source deletion.
Keep workspaces under `Tmp/BlueprintText/`.
Existing workspaces and installed shared modules using the previous names remain
supported by the current entry points. Run Prepare to refresh their module; do
not rename generated projects or move existing preview output just to match the
new tool directory. New projects and workspaces use the current names.
The current implementation selects ordinary uncooked Actor/Widget, direct UObject and pure static function-library Blueprint
packages under `/Game/`; split packages are rejected. Use `-EngineRoot` when the
project uses an engine outside the wrapper's default installation.

```powershell
$tool = './tools/BlueprintText/Invoke-BlueprintWorkspace.ps1'
$project = './Tmp/Example/authoring/Project/Example.uproject'
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

`AddSource` stages a copy of a schema-2 UObject or supported function-library BPT in a new or existing workspace.
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

The reusable [INI parser library](BlueprintText/Library/IniFileParser/integration.md)
supplies the flat `BP_IniFileParser.json` and separate `BP_SectionedIniFileParser.json`
sources; select the documented format and interface before importing a copy.
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
and [pure static Blueprint Function Libraries](blueprint-text.md#blueprint-function-libraries)
without component/widget sections. The latter have an empty starter source at
`BlueprintText/Library/BFL_FunctionLibrary.json`. Complex object-bearing struct/container
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
math catalogue, or to call a saved pure static Blueprint Function Library.
Supply exact `/Script/Module.DeclaringClass::Function` or
`/Game/Path/Package.Object_C::Function` identities
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
metadata; new calls use [optional arguments](blueprint-text.md#optional-call-arguments)
and can inherit declared defaults. Set `arguments` from the caller's requirements;
there is no need to enumerate callee defaults first. Missing required arguments
are errors. Arithmetic/logic operands stay explicit, including in KismetMathLibrary
templates. A required reference input such as
`AddComponentByClass.RelativeTransform` needs a connection, not a textual default.

Reflection uses a transient Blueprint with the selected parent class. It does
not edit source JSON or save project Blueprint assets. The shared math catalogue
is not overwritten. `add-call` rejects a different parent context; standalone
`template` fragments retain `contextParent` for the same restriction. Reuse a
catalogue within its project/engine context and regenerate it when that context
changes. Existing catalogues without a parent context remain supported.

The supported boundary is ordinary native `UK2Node_CallFunction` nodes and
pure static functions of a supported [Blueprint Function Library](blueprint-text.md#blueprint-function-libraries),
with concrete pins. The request must name the function's declaring class. For
a saved project library, use `/Game/Path/Package.Object_C::Function`; the object
name can differ from the package after AddSource. Other Blueprint functions,
events, private/protected functions, latent calls, wildcard/container
specialization, dynamic output typing and enum/bool execution expansion are
outside this factory. Internal functions are rejected except the tested
`Actor.AddComponentByClass` and `KismetSystemLibrary.SetClassPropertyByName`
native calls; this does not synthesize editor special nodes or expose-on-spawn
assignments. Unsupported requests fail without
publishing a partial catalogue.

Request `/Script/Engine.KismetSystemLibrary::SetClassPropertyByName` for a named
class-property write. Its reflected inputs are `Object`, `PropertyName` and
`Value` (a class reference constrained to `UObject`), plus execution pins.
The engine writes only an existing class property whose class constraint accepts
the value; null clears it. A missing property or incompatible class is a silent
no-op. Strict Apply verifies the represented call, not the presence or effect of
the named property in the game; validate that consumer contract separately.

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
on their workspace. One task owns direct JSON and review-annotation edits;
coordinate ownership before sharing sources or selected assets. Commands cannot
protect direct edits by another task. Prepare, Export, Apply, Preview, MarkStubs
and Recover also lock the project and join the
[shared Unreal queue](unreal-coordination.md) for their entire operation,
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

Older custom-parent sources without `nativeDefaults` are checked before the
selected-package write phase. If the asset has native default subobjects, Apply
requires a fresh export of the intact asset into a new workspace; reapply the
intended edits there. A pointer-only export cannot reconstruct missing subobject
properties. Current exports carry that state, so ordinary Apply needs no manual
native-component inspection or extra preparation step.

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

The preview reviews the workspace selection and its relevant context. It includes
selected sources, their forward dependencies, and assets referenced by the current
proposal or original Before snapshot, with those assets' forward dependencies.
Unrelated project assets, including test copies, are omitted. Reverse references
alone do not bring another asset into the review. Selection is explicit; asset
names are not used to guess whether something is a test.

Supported Blueprint sources in that scope have graph/widget cards. References appear at their actual graph node, pin or
named Blueprint property, with stub or missing-local-asset labels when known.
Other assets, including DataAssets and unselected editor stubs, appear only in
a compact reference list at the bottom. Their presence is not a claim of BPT
editing support. In particular, a stub's empty graphs are not the original game
implementation. An explicitly selected stub can still be reviewed as an editor
contract. Discovered context does not expand the workspace's Apply selection.

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
Do not open generated HTML, SVG or images to infer edits already represented
in source. Inspect the rendered output when the user requests a preview
investigation or a concrete rendering failure requires it.

## Preview a project model

Use `ModelPreview` for a lightweight interactive view of one saved Static Mesh
or an Actor Blueprint's visible rigid Static Mesh assembly. The Unreal project
is the source: no external GLB or graph export is required. This is a separate
model view; `Preview` continues to review graphs and widgets.

Create the mod's working project copy first, then call the shared entry point
through the [PowerShell launcher](powershell-execution.md):

```powershell
python -B tools/run_powershell.py tools/BlueprintText/Invoke-BlueprintWorkspace.ps1 `
  -Action ModelPreview -ProjectFile './Tmp/Example/authoring/Project/Example.uproject' `
  -WorkspaceRoot './Tmp/BlueprintText/example-model' `
  -Asset '/Game/Mods/Example/BP_Model' -EngineRoot '<selected engine root>'
```

Supply exactly one `/Game` package identity. With an existing workspace,
`ProjectFile` may be omitted; a supplied project must agree with its manifest.
The asset need not be selected for graph editing. A standalone model call does
not create `workspace.json` or change an existing workspace's sources/baselines.
Pending JSON edits are not applied: to preview their resulting assembly, Apply
first. Unsaved changes in an interactive editor are not included.

Warn before this engine action. The command owns the shared queue, dependency
preparation, native export and HTML generation. The offline renderer uses NumPy
(included in the repository's bundled Python runtime). On success, return the
`previewUrl`; the result also identifies `source: saved-project`, the selected
asset, visible mesh-instance/triangle counts and bounds in metres. The standalone
HTML embeds its geometry and works locally without a server or network access.
Its English interface provides drag/keyboard orbit, wheel/keyboard zoom and
reset, side and top views. All generated GLB, evidence and HTML remain in the
workspace's `.workspace/runs/`; none is an authoring or release source.

Actor preview instantiates the saved Blueprint in an editor world, including its
editor construction script, without BeginPlay or gameplay simulation. It uses
visible, non-hidden Static Mesh components and their evaluated Unreal world
transforms. Nested non-uniform scale and reflections preserve placement and
outward shading. The tool does not save packages. A neutral material deliberately
shows shape, not game shaders, textures, collision, effects, animation or runtime
visibility. A direct Static Mesh is shown at its local origin; assemblies retain
their component placement. Camera framing does not change source geometry.

This version rejects visible skinned, instanced, spline-deformed or other
unsupported mesh components, and populated Child Actor components. Hidden
components are omitted; collision-only primitives are not model geometry.
An empty selection of visible meshes or an unsupported asset type is an error,
not an empty successful preview. Start from the returned reason and diagnostic
path on failure. Do not substitute an external review GLB or silently omit an
unsupported model part to obtain success.

## Editor stub identity

When creating or changing an external Blueprint compilation stub, establish its
exact generated-class parent from the stock asset's `SuperStruct`, rather than
inferring it from the asset name or the API the caller needs. Record that source
contract in the mod's [game-derived input provenance](../docs/game-derived-data-rule.md).
Revalidate it under that provenance's refresh conditions, not before each Apply.
Compilation and strict round-trip comparison establish consistency with the
editor dependencies supplied to BPT; they do not establish that those dependencies
match the game. The stub role marker below labels an asset, not its fidelity.

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
