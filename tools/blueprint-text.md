# BlueprintText format and tools

For new and existing mods, use the [workspace workflow](blueprint-workspace.md).
The interfaces below describe the format, lower-level tools and validation.
For shared tool changes and specialist reviews, use the
[maintenance guide](blueprint-text-maintenance.md).

BlueprintText (BPT) is the recommended bidirectional text interface for editing
Blueprint assets in an authoritative Unreal project. Its synchronized JSON is
committed alongside the assets for Git review, as defined by the
[source-of-truth and Git contract](blueprint-workspace.md#source-of-truth-and-git).
This document defines interchange capabilities, not a JSON-owned project model.
The shared builder is independent of any mod. Compact JSON and native Unreal
graph text are both available. Release packaging and installation remain separate.
An existing C++ generator can supply the initial uncooked asset; BlueprintText
exports that asset, not the C++ source. New mods use the same shared tools and
describe their Blueprints directly rather than adding a per-mod graph builder.

## Schema 1: small examples

The envelope declares `schema`, `asset`, `parent`, `variables`, and `graphs`.
Currently it supports one `/Game` Blueprint, one `EventGraph`, and baseline
scalar declarations (`float`, `double`, `int`, `bool`, with zero defaults).
Compact factories are `event`, `call`, `sequence`, `getVariable`, and
`setVariable`. Links use `nodeId.pinAlias`; ordinary reflected calls use their
owner/function identity. See the complete [division example](BlueprintText/Examples/division.json).

Native mode uses the same envelope with an adjacent editable `.t3d` graph body
referenced by `bodyFile`. It preserves native node text more broadly, but does
not remove the envelope's Blueprint-level limitations. Conversion between the
two modes goes through a prepared Unreal project.

Neither schema is a general lossless exporter for arbitrary Blueprints.
Schema 1 rejects known unsupported state (including user functions, macros, timelines, custom
components, nonzero variable defaults and modified construction logic).
Compact mode additionally rejects unsupported node classes and
disabled nodes. Compact export rejects pin `DefaultObject` and `DefaultTextValue`
values that its string-only arguments cannot preserve, including values on hidden
or connected pins. The exact automatic `self` object of a static call remains
supported because Unreal recreates it. Use native graph format or schema 2 for
explicit object/text defaults; a rejected compact export does not write a result.
Other Unreal metadata is not comprehensively represented or
audited. Use schema 2 for the ordinary workspace workflow; use schema 1 only
for Blueprints whose complete state fits its narrower supported envelope.

The text is the full desired supported state: omitting a node deletes it.
Export editor changes before editing/importing text again. Import does not merge
concurrent edits. IDs are retained through an internal node-comment prefix;
removing that prefix in the editor can change the exported identity. Optional
nonzero positions are preserved; the offline viewer computes its own layout.

## Preparing a project and discovering operations

Run from the repository root with Windows PowerShell and an Unreal C++ build
toolchain. The default engine is `K:\Epic Games\UE_5.8`; pass `-EngineRoot` to
override it. **Prepare builds the editor module; catalogue refresh, import and
export launch UnrealEditor-Cmd. Warn the user before engine work.** Close any
open Unreal editor before these actions; they do not open the graphical editor.

```powershell
$tool = './tools/BlueprintText/Invoke-BlueprintText.ps1'
$run = './Tmp/BlueprintText/demo' # must be a new directory
$project = (& $tool -Action Prepare -OutputRoot $run).projectRoot
& $tool -Action RefreshCatalog -ProjectRoot $project -Force
& $tool -Action SearchCatalog -Query divide -Limit 10 | ConvertTo-Json -Depth 12
& $tool -Action Import -ProjectRoot $project -InputPath './tools/BlueprintText/Examples/division.json'
& $tool -Action Export -ProjectRoot $project -Asset /Game/BP_DivisionExample -Format compact -OutputPath "$run/export.json"
& $tool -Action Export -ProjectRoot $project -Asset /Game/BP_DivisionExample -Format native -OutputPath "$run/native.json"
```

Search is offline after refresh. It returns pin types, semantic aliases and a
copyable compact node template; `-Format native` returns engine-generated native
insertion text instead. The initial catalogue covers callable/pure functions in
`KismetMathLibrary` and `flow.sequence`, not all Unreal editor actions. It records
the engine version; refresh after changing engines. Discovery does not guarantee
that every catalogued function works in every graph context.

Disposable projects, working exports and engine logs for these lower-level
examples stay below `Tmp/BlueprintText/`. This does not set the storage location
of maintained mod projects or their committed synchronized BPT representations.
Assets are saved under the prepared project's `Content`. Import compiles before
saving and reports errors with log paths. On failure inspect that diagnostic;
do not bypass an unsupported-state rejection. `-Action Help` shows parameters
and the node skeletons. The `Simulate*` actions are test fixtures only.

## Schema 2: Blueprints

[Invoke-BlueprintAssetText.ps1](BlueprintText/Invoke-BlueprintAssetText.ps1)
exports a complete supported Blueprint as compact JSON. It carries event and
function graphs (including construction logic), native variable descriptions,
CDO defaults and native default subobjects, Blueprint/graph properties, component object trees and templates,
native node properties and typed pins. Original object names and GUIDs are retained. Class
aliases and shared pin templates reduce repetition without replacing the native
property values. Some fields deliberately remain Unreal property text.

New exports address each `pinTemplates` entry by a SHA-256 key derived from its
complete pooled metadata. Adding or removing a different template leaves existing
`pin.type` references unchanged. This is a deduplication key, not an asset-integrity
check. Keys remain opaque document-local references: existing `p0`, `p1`, or
manually assigned keys still import and compare correctly, and editing metadata
does not require manually recalculating its key. Export/canonicalization assigns
the content keys, producing a one-time key change for older documents. Per-pin
identity, names, direction, links and defaults remain outside the pool.

`asset` identifies the package, not the object's full path. Export locates the
single Blueprint inside that package and rejects missing or ambiguous Blueprint
objects. When its name differs from the package basename, optional `objectName`
preserves that name; import recreates it without renaming the package. Omit
`objectName` when both names match. This also preserves generated-class identities
used by dependent Blueprints.

Optional `editorRole: "stub"` preserves the asset's `BlueprintText.Role` editor
metadata through export/import. No other role value is currently supported;
absence means unclassified. This is a declaration by the asset creator, not an
inference about the asset's completeness or release ownership. See the
[workspace stub workflow](blueprint-workspace.md#editor-stub-identity).

The import/export boundary is an ordinary Actor-derived or Widget Blueprint with
a native or available Blueprint parent, or a normal Blueprint directly derived
from `/Script/CoreUObject.Object`, or a pure static Blueprint Function Library
as described below. Other UObject families and UObject Blueprint
inheritance are not yet covered. A direct Object Blueprint omits `root`,
`components` and `widget`; these sections are rejected on import. It retains
member variables (including containers), CDO defaults and ordinary function
graphs through the same schema-2 path. Actor-only CDO exclusions do not apply
to it: a string variable named `RootComponent` remains an ordinary variable.
Component trees include SCS nodes, component templates and
their owned objects, including child-actor templates. Inherited component
overrides, macro definitions, timelines, extensions and arbitrary nested node
objects are rejected. Ordinary K2 collapsed graphs, widget trees and implemented
interfaces use the explicit representations described below. Required parent classes, native classes
and external asset dependencies must already
exist in the prepared project. A blank project does not supply game
classes or assets. This is an uncooked editor-asset workflow.

Editor dependency placeholders are not the full stock game assets. A successful
round-trip of a placeholder or function donor does not validate replacement of
the cooked stock package. Maps, fonts, textures, Input Actions and Input Mappings
are dependencies or separate asset types, not Blueprint conversion targets.

For direct Object Blueprints, create instances with `Construct Object from Class`, provide an `Outer`, and
retain the result in an object-reference variable while it is needed. Each
instance owns its own variables; `Outer` alone is not a lifetime guarantee.
File loading remains a normal callable runtime dependency, not a feature
provided by UObject support itself.

For source distribution use the workspace's [AddSource operation](blueprint-workspace.md#add-a-reusable-uobject-source).
The lower-level offline equivalent is
`asset_text_codec.py relocate source.json --asset /Game/Mods/Example/BP_Utility --output copy.json`.
It requires a new output file and has the same bounded reference handling.
`Test-BlueprintObjects.ps1` exercises relocation into two packages, source-package
absence, round-trip import/export and Blueprint execution in a disposable project
prepared with `Invoke-BlueprintText.ps1 -Action Prepare`. It covers Object
functions/defaults, a string map, an Actor consumer constructing two instances,
independent member values and a retained reference across garbage collection.
These are editor tests, not cooked-game validation.

### Blueprint Function Libraries

Schema 2 supports stateless libraries whose exact parent is
`/Script/Engine.BlueprintFunctionLibrary` and whose `properties.BlueprintType`
is `BPTYPE_FunctionLibrary`. Their ordinary function entries retain both
`FUNC_Static` and `FUNC_BlueprintPure` in `ExtraFlags`; graph identity, parameter
types, local variables, results and external class references use the existing
representations. Member variables, instance defaults, component/widget state,
event graphs and implemented interfaces are rejected. Impure library functions,
library subclasses and other UObject families are outside this boundary.

Use a library for shared pure calculations or predicates, called by name with
their inputs. Consumers do not construct an instance, locate a manager or obtain
a CDO manually. Preserve Unreal's generated hidden target/world-context pins in
the complete source; these are engine implementation details, not extra authored
graph operations. Simplified preview keeps the hidden CDO reference in Details;
Full shows the exact reference. Connect the Actor input in predicates such as `IsLarge(Actor)`.

Start a new library with the empty shared source:

```powershell
& $tool -Action AddSource -ProjectFile $project -WorkspaceRoot $work `
  -SourcePath './tools/BlueprintText/Library/BFL_FunctionLibrary.json' `
  -Asset '/Game/Mods/Example/BFL_Types'
```

Author its function graphs in the staged source, then use ordinary Apply.
AddSource also relocates an exported supported library; as with Object sources,
it retains its object identity and rewrites owned references, not string data.
External Blueprint classes must already exist in the project. An existing
library and its Actor consumers use ordinary Export/Apply/Preview together.

Once the library declaration is saved in the project, request its call nodes
through [CallTemplates](blueprint-workspace.md#create-reflected-native-call-nodes),
using the generated class's exact object path, for example
`/Game/Mods/Example/BFL_Types.BFL_FunctionLibrary_C::IsLarge` for the relocated
empty source above. Use `add-call` to insert the reflected node. The factory
reads the saved declaration, not unapplied source edits. A changed name or
signature requires corresponding edits in callers; include the library and
affected callers in the same Apply selection. Strict fresh-process comparison
still rejects unrepresented reconstruction changes.

The [optional argument contract](#optional-call-arguments) also applies to
supported scalar inputs on library calls. Defaults belong to the declaration;
explicit equal values remain fixed requirements. This does not make Actor or
other object-reference inputs part of scalar argument inheritance.

`Test-FunctionLibrary.ps1` exercises two Actor consumers, external Medium/Large
class references and their descendants, null/unrelated inputs, reflected call
insertion, joint function/input rename, changed optional defaults, fresh reopen,
AddSource and Preview. It cooks the library and consumers and independently
checks that editor BPT state is absent. This is editor execution and cooked
serialization evidence, not a test in the game.

### Component trees

The `components` section contains `objects`, the `scs` root identity, and
`templates` / `generatedTemplates` reference lists. Each object declares `id`,
`name`, `class`, `outer`, `flags` and native-text `properties`. IDs are package-relative;
`$blueprint` and `$class` identify the owning Blueprint and generated class.
Edit properties directly; to add a component, provide its objects and update the
appropriate SCS/template references in the complete source. Objects are allocated
before their properties are imported, allowing cross-references. The codec rejects
duplicate IDs, unresolved/cyclic owners and invalid template references.
Simple direct-Actor assets can retain the legacy `root` section instead;
`root` and `components` are mutually exclusive. Component data is preserved by
the codec but is not a separate diagram in the graph viewer.

The separate `nativeDefaults.objects` tree preserves constructor-created native
default subobjects below the class default object (CDO), including nested objects.
Rows use the same identity/property fields as component objects; `$cdo` is the
root owner. Their property values are deltas against the actual inherited
subobject archetype, including native-constructor initialization and parent
Blueprint overrides, rather than against the component class CDO. Omitted
properties inherit that baseline; explicit zero/null overrides remain meaningful.

Import binds the existing constructor-created objects after compilation. Their
names, classes, owners, flags and complete inventory must match; this section
does not allocate or remove native components. Persistent CDO-owned objects
outside native-class default-subobject trees are rejected. Transient objects and
transient properties are outside this saved-state representation. An absent
section compares equal only to an empty tree; it never substitutes for omitted
nonempty state. Older pointer-only exports require a fresh export of the intact
asset through the [workspace workflow](blueprint-workspace.md#workspace-coordination).

Changed properties include their complete nested values, including zero-valued
fields. Saved component references marked DuplicateTransient are retained;
transient component properties are excluded. Export normalizes user-defined pin
default metadata using Unreal's save-time method. JSON pin names remain
case-sensitive, including in the PowerShell entry point.

Use a prepared project containing the BlueprintText editor module and
the asset's dependencies. All actions below launch headless Unreal; warn first
and close the editor. Project and output paths must be below
`Tmp/BlueprintText/`.

```powershell
$assetTool = './tools/BlueprintText/Invoke-BlueprintAssetText.ps1'
$projectFile = Join-Path $project 'BlueprintText.uproject'
$source = "$run/actor.json"
& $assetTool -Action Export -ProjectFile $projectFile -Asset /Game/BP_Example -OutputPath $source
# Edit the source. Use a rebuild project with dependencies but no target asset.
# Keep the source asset identity; $rebuildProjectFile is that project's .uproject.
& $assetTool -Action Import -ProjectFile $rebuildProjectFile -InputPath $source -OutputPath "$run/reopened.json"
```

Import refuses an existing target asset. It reconstructs, compiles and saves,
then exports in a fresh engine process and compares all represented fields.
Failure reports an adjacent evidence directory; inspect its logs and
`comparison.json`. A saved candidate can remain after a failed comparison.
Numeric property spellings are compared strictly. Entry order in each `node.pins`
object is also significant: native text preserves it, and nodes such as `Sequence`
use pin order for execution. Comparison reports a reordered pin list even when
all per-pin values match; do not sort these keys when formatting JSON. Key order
in other JSON objects is ignored. This check does not establish
runtime equivalence or preservation of state outside the supported envelope.
Export editor changes before the next text edit; there is no concurrent-edit
merge. Use the workspace workflow for transactional updates to existing assets.

The offline [codec](BlueprintText/asset_text_codec.py) converts schema-2
compact/native envelopes and creates call nodes from the engine catalogue:

```powershell
python -B tools/BlueprintText/asset_text_codec.py add-call $source --catalog tools/BlueprintText/.cache/catalog.json --operation /Script/Engine.KismetMathLibrary::Divide_DoubleDouble --id Division --graph Calculate --output "$run/edited.json"
```

Use an existing GraphGuid or unambiguous graph name. The command allocates IDs
and merges pin metadata;
edit the new node's `arguments` overrides and links in the resulting complete source.
`template` produces a standalone insertion fragment. `compact`, `native` and
`compare` perform offline conversion and comparison; use `--help` for arguments.
New operations belong in shared discovery/serialization, not a per-mod builder.
For ordinary native functions beyond the initial math catalogue and supported
pure static Blueprint library functions, request
[workspace CallTemplates](blueprint-workspace.md#create-reflected-native-call-nodes)
and pass its returned catalogue to the same `add-call` command. Reflected pin
metadata supplies pure/exec structure, declaration defaults and reference requirements;
do not reconstruct those fields manually from a function's name or C++ signature.

### Optional call arguments

For function calls, **state the caller's requirements**. Do not enumerate or
compare callee defaults to choose the values to pass. A caller that needs a
maximum of 1000 passes 1000 even if the callee currently defaults to 1000;
a caller with no maximum requirement omits it. The tool accepts that omission
only if the declaration supplies a default. Simple Unreal arithmetic and logic
must show all operands explicitly under the [graph-writing rule](../docs/blueprint-graph-design-rule.md).
The presence of a node's `arguments` object opts that ordinary `call` into the
new contract. For example:

```json
"arguments": {
  "in:Name": "LargeContainerMaxWeightKg",
  "in:Default": "200.0",
  "in:Min": "1.0"
}
```

Here `GetFloat.Max` is omitted and uses its declared `FLT_MAX`. There is no
repeated upper-bound literal. `arguments: {}` requests declaration defaults for
unconnected inputs; it fails when any required argument is missing.
For each supported input, a wire supplies the expression; without a wire an
absent key uses the declaration default and a present string fixes an override.
`"0"`, `"false"`, `""`, and an explicit value equal to today's default are real
overrides. Wiring an input takes precedence over its retained disconnected value.
Delete its override key to return to inheritance; do not set it to today's
default as a substitute. Keep the complete pins and their engine metadata.

The initial boundary is ordinary Blueprint/native function calls with unsplit,
non-reference, non-container `int`, `int64`, `real` (float/double), `bool`, or
`string` input pins whose defaults Unreal does not mark ignored. The function's
default is used only when declared. Without one, a value or connection is
required: BPT does not supply an implicit zero, false or empty string by type.
Execution, target, output, reference, wildcard, enum, object/class, FText,
struct and container pins keep their existing representation and requirements.
Their defaults, when needed, remain in `defaults` or the typed pin fields.
Do not duplicate a governed input in both `arguments` and `defaults`.
Collapsed graph tunnel boundaries, events and macro definitions are not covered
by this argument contract. Calls *inside* a supported graph can use it.

New `template` and `add-call` results use this contract automatically. Reflected
native declaration defaults may be omitted by the factory; KismetMathLibrary
templates retain every literal operand explicitly for readable arithmetic and
logic. A template's operand is a starting value for the author to set according
to the required calculation, not a reason to rely on a native default. For an
existing ordinary call, use the offline explicit transition:

```powershell
python -B tools/BlueprintText/asset_text_codec.py use-arguments $source `
  --graph InitializeSettings --id ReadLargeCapacity --output "$work/edited.json"
```

The transition preserves every existing governed literal as an override.
Remove only keys that should follow the declaration (for example `in:Max`),
replace the workspace source with the complete edited document, then Apply.
Legacy calls without `arguments` keep their old literal semantics; deleting
one of their `defaults` entries is not a supported inheritance operation.
Older tools reject the new field rather than silently losing its meaning.

Function entry nodes can declare defaults with `parameterDefaults`, keyed by
their output parameter aliases. Its presence makes absent parameters required:

```json
"parameterDefaults": {
  "out:Default": "0.0",
  "out:Min": "-3.4028234663852886e38",
  "out:Max": "3.4028234663852886e38"
}
```

In this `GetFloat` declaration `Name` is required. An explicit `""` in this map
declares an empty-string default; absence means no default. Do not repeat these
governed parameter values in the entry's `defaults` or native user-pin metadata;
the codec materializes that engine representation. `use-parameter-defaults`,
with the same `--graph`, `--id`, `--output` interface as `use-arguments`, moves
nonempty old entry values into this map. The declaration's author then chooses
which defaults belong in its API, including any empty-string defaults. This is
declaration authoring, not a preliminary step for every caller. Existing
declarations without the map retain their nonempty native defaults; their
ambiguous empty defaults require an explicit declaration. Native C++ defaults
come from reflected declaration metadata, including explicitly empty defaults.

Apply stores intent in an uncooked BPT call node, resolves the current function
declaration and materializes Unreal's pin values before compilation. Save,
fresh export and normal node reconstruction preserve explicit equal overrides.
Fresh export checks actual pin values against the declaration and overrides;
stale or invalid state is an error, not silently repaired by the exporter.
Strict comparison checks intent and overrides. Only the derived declaration
snapshot (`declarationDefaults` and `AutogeneratedDefaultValue` on governed inputs) is excluded from
that comparison, because the engine independently verifies it. Successful
Apply refreshes these callers' source snapshots from the checked export;
recovery retains subsequent author edits and rolls back its own refreshes.
These snapshots are tool-owned: edit `arguments` for caller intent or
`parameterDefaults` on the declaration, not the cached default values.

Changing a declaration updates inheriting callers when those callers are
updated and compiled. Include the affected callers in the workspace Apply
selection; the tool never expands its write set silently. This is not a runtime
lookup, an update of already shipped bytecode, or synchronization of independent
imported library copies. Editing a pin value in Unreal records an explicit
override; use BPT key deletion to request inheritance, even after an editor
reset to the current default.

Cards show explicit arguments; Details and `--node ... --pin ...` expose the
inherited default and declaration. Explicit equal overrides remain visible.
Review marks a caller when a default it inherits changes; an unused default
change does not mark a wired or explicitly overridden input. Selected proposed
declarations resolve before Apply as a view-only operation.
Unavailable declarations retain their last exported snapshot; only Apply and
fresh engine validation establish the compiled result.

The project must retain the prepared BPT tooling to edit these uncooked nodes.
The shared module uses `UncookedOnly`; cook strips the editor nodes and their
argument state, leaving ordinary Blueprint calls without a BPT runtime module.

### Collapsed graphs

Schema 2 supports ordinary K2 Collapsed Graph nodes (`K2Node_Composite`) with
entry/exit tunnels, including multiple nesting levels. Each child is a separate
entry in the asset's existing `graphs` array, with `kind: "collapsed"` and an
`owner` record:

```json
"owner": {
  "graph": "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
  "node": "K2Node_Composite_0",
  "entry": "K2Node_Tunnel_0",
  "exit": "K2Node_Tunnel_1"
}
```

`graph` is the parent's existing `properties.GraphGuid`; `node` names the
Composite in that parent, while `entry` and `exit` name tunnels in the child.
GraphGuid is the stable identity, including across renames. Preserve existing
GUIDs. Display names and node IDs are scoped: distinct owners may contain graphs
with the same short name. Root graphs omit `owner`. Flat documents retain their
existing shape, including after the last collapsed graph is removed.

The owner record is the sole representation of `BoundGraph`, `SubGraphs` and
reciprocal tunnel ownership pointers. Do not add those pointers to node or graph
properties. Node properties, user-pin metadata, pin identities, typed defaults
and links remain explicit. Links stay inside their graph; the boundary binds
matching native pin names/types with complementary directions. Export keeps the
parent before its descendants. Keep this order when editing the registry.

The codec rejects missing/multiple owners, cycles, duplicate graph identities,
missing tunnels, inconsistent pin boundaries and invalid local links before
Apply changes a package. Native reconstruction restores graph Outers and tunnel
references before compilation. Fresh-process comparison remains strict.
This is not macro-definition or arbitrary UObject-tree support.

Use [compact source reading](#read-graphs-without-unreal) to inspect this
hierarchy and reveal exact node or pin details as needed.

`Test-NestedGraphs.ps1 -OutputRoot <new directory below Tmp/BlueprintText>`
runs the public workspace workflow in an isolated project through the shared
Unreal queue. It covers two levels, repeated names, a BPT body edit and rename,
strict re-export, invalid-owner rejection without changing the package, a no-op
Apply, and one-call Preview. Fresh-process Blueprint execution checks branch
results and side-effect order in UObject functions and Actor event/construction
graphs. Vector, array and object boundaries/defaults are checked structurally.
These are editor checks, not a cooked-game acceptance claim.

For mechanical extraction, use the workspace
[Collapse action](blueprint-workspace.md#extract-an-operation-into-a-collapsed-graph).
`Test-CollapseGraphs.ps1 -OutputRoot <new directory below Tmp/BlueprintText>`
checks that public action in an isolated project, followed by strict Apply,
fresh-process result/side-effect assertions, no-op Apply and one-call Preview.
Its executable cases cover pure integer arithmetic, an assignment with a
continuation, terminal Actor paths and reparenting existing collapsed graphs.
Existing vector/array/object defaults remain part of the strict comparison;
this does not establish executable coverage for every new boundary type.
Offline transformation, rejection, identity and write-recovery checks run with
`python -B tools/BlueprintText/test_collapse_graphs.py`.

## Read graphs without Unreal

Read schema-2 source in layers: graph summaries, a compact selected graph,
then exact details only where the task needs them.

```powershell
python -B tools/BlueprintText/graph_hierarchy.py $source
python -B tools/BlueprintText/graph_hierarchy.py $source --graph $graphGuid
python -B tools/BlueprintText/graph_hierarchy.py $source --graph $graphGuid --node $nodeId
python -B tools/BlueprintText/graph_hierarchy.py $source --graph $graphGuid --node $nodeId --pin 'in:Value'
python -B tools/BlueprintText/graph_hierarchy.py $source --graph $graphGuid --view full
```

Take `$graphGuid` from the summary; an unambiguous graph name also works.
Node IDs are local to that graph. Use the exact pin alias shown in its `pins`
(including `in:` or `out:`); `in:Value` above is an example. Repeat `--node`
to inspect several nodes together. `--pin` requires exactly one node.
Unknown or ambiguous selections fail explicitly.

The default selected-graph view includes every node and link, original pin order,
operation identities, comments, relevant native properties and typed pins.
It preserves disabled nodes, terminal branches, reference/container semantics,
unknown properties and custom data. Ancestor operations and child summaries
retain the ownership context. It omits known editor layout, redundant GUIDs,
pin-template indirection and known inactive metadata. It does not fold flow,
infer reachability, resolve callbacks or simulate execution.

Explicit unconnected input values remain visible, including zero and empty strings;
inherited [optional arguments](#optional-call-arguments) are available in pin details.
connected, ignored and ordinary output defaults are not presented as runtime
values. Entry/tunnel `signatureDefaults` describe the declared signature.
Referenced own variables include their declaration and, when exported, their
`initialDefault`; that is an initial asset value, not a value at the current
execution point. External and local variables are not guessed from matching names.

`--node` returns exact source nodes, only their class/pin definitions, incident
links and typed neighboring endpoints. `--pin` narrows this to its exact source
record, full template, stored default and incident links. These details expose
fields omitted from the compact view. `--view full` returns the previous complete
selected-graph payload, including ancestor operations and required definitions.
Consumers of the old CLI detail shape must add `--view full`; the summary and
Python `inspect_graphs` API remain unchanged. Do not combine `--view full` with
node/pin selectors or `--context`.

All outputs are read-only views, not Apply input. Edit the complete workspace
source JSON. Reading does not change source files, launch Unreal, query missing
assets or establish runtime behavior.

### Reveal source context

Add `--context locals`, `--context components` or `--context all` to a selected
graph or node/pin view. This optional section does not expand the ordinary
compact output. For example, on the StableGyro child source:

```powershell
python -B tools/BlueprintText/graph_hierarchy.py $source --graph RefreshCargoAvailability --context locals
python -B tools/BlueprintText/graph_hierarchy.py $source --graph CreateMooringPlate --context components
python -B tools/BlueprintText/graph_hierarchy.py $source --component StableGyroMooringPlate
```

Local context follows collapsed ownership to the enclosing FunctionEntry and
matches referenced local variables by scope and GUID, or name when no GUID is
present. It returns the exact `LocalVariables(...)` source field, its parsed
declaration and the declaring graph/node. It never borrows a same-named variable
from another function. Missing or ambiguous declarations remain unresolved.
Declared defaults describe initialization, not values at the selected operation.

Component context recognizes named `AddComponent` uses of local templates.
`source-template` means the named template exists in this source; it does not
establish runtime target identity or creation success. Connected template names,
explicit template-context inputs and missing/ambiguous local templates remain
unresolved. It does not evaluate an inherited template or arbitrary component
lookup. Each resolved result contains the original object and its owned subtree;
property references are preserved without fetching referenced assets.

`--component` reads an exact object ID or an unambiguous object name from
`components.objects` or `nativeDefaults.objects`, including its owned subtree
and template-list membership or `native-default-subobject` role.
It is independent of graph selection and cannot be combined with graph/detail
selectors. Legacy `root` representations have no component-object registry.

### Find member uses

Search explicitly supplied schema-2 files. Use the workspace manifest to select
current editable sources; synchronized tracked sources also support read-only
analysis. Supply one version of each asset, rather than mixing snapshots.
For example:

```powershell
$sources = @('./mods/StableGyro/BlueprintText/ModActor.json', './mods/StableGyro/BlueprintText/PitchInputActor.json')
python -B tools/BlueprintText/graph_references.py @sources --variable EnableAltitudeControl --owner /Game/Mods/StableGyro/ModActor
python -B tools/BlueprintText/graph_references.py @sources --function HandleControllerChanged
python -B tools/BlueprintText/graph_references.py $source --variable StableGyroCargoAvailable --scope GetProvidedActionsBP
python -B tools/BlueprintText/graph_references.py $source --function RunDisabledMooringCleanup
```

Choose exactly one `--function` or `--variable` with an exact member name.
`--owner` optionally limits the declared owner to an asset package or native
`/Script/Module.Class`; `--scope` limits local variables to an enclosing function
name. Results retain the asset, stable graph identity, full nested path, node
and source field. Disabled sites remain included and labelled.

Uses distinguish `direct-call`, variable `read`/`write`, `delegate-reference`
and `timer`. Delegate results identify directly connected bind/unbind/assign
nodes; storing or forwarding delegates is not traced. Named timers cover
`KismetSystemLibrary.K2_SetTimer` and `K2_SetTimerForNextTick`. Scheduling or
binding is not immediate callback execution and is never added as an execution
wire. Variable access means an explicit Get/Set node, not all indirect mutation
through references or native calls.

Source definitions resolve by owner and GUID, or name when no GUID is present,
using the same function-identity logic as preview navigation. A valid GUID can
locate a renamed definition; the result retains both source spelling and resolved
name. Missing source, unknown targets and ambiguous definitions are reported as
unresolved. `unresolvedCandidates` separately lists sites whose dynamic name or
unknown owner could not be matched to the query; these are not counted as uses.
No runtime subtype dispatch, arbitrary string invocation, recursive asset
discovery or missing native declaration is inferred. An empty search therefore
does not prove that a member is unused outside the stated coverage.

## Widget trees, interfaces and resources

`Invoke-BlueprintAssetText.ps1 -Action Export` reads ordinary uncooked Widget
Blueprints, their widget/slot object trees and animation-owned objects. The
`widget` section declares the `tree` object ID, `objects`, and optional `animations`
IDs. Objects retain their names, classes, owners, flags and native-text properties,
using the same object representation as component trees. `widget`, `components`
and the legacy `root` are mutually exclusive.

Implemented interfaces are exported as `interfaces` entries containing a `class`
path and owned `graphs` names; corresponding graphs use kind `interface`.
The offline compact/native codec preserves these sections and validates their
references. `-Action Import` reconstructs these same widget and interface sections,
compiles/saves the Blueprint, and compares a fresh-process export with the source.
This verifies represented state; it does not establish runtime equivalence or general lossless
Blueprint support. Existing export exclusions still apply to macros, timelines,
extensions, unsupported nested object families and SCS inherited-component
overrides stored in `InheritableComponentHandler`.

To add a widget or animation, declare its objects and hierarchy in the complete
source, then allocate its missing editor identity without changing existing GUIDs:

```powershell
python -B ./tools/BlueprintText/asset_text_codec.py register-widget `
  ./Tmp/BlueprintText/HUD-edited.json --id NewLabel `
  --output ./Tmp/BlueprintText/HUD-ready.json
```

`--id` is the declared widget/animation's `name`, not its full object ID. Repeating
registration for an existing name preserves its GUID. The importer rejects missing,
invalid or duplicated active widget identities; repair existing conflicts in the
source GUID map. Object owners, hierarchy references, reciprocal panel/slot links
and cycles are checked before Unreal traverses the widget tree. Animation objects
retain native property text; the shared importer handles Unreal's frame-range text
without weakening full-source comparison.

### Mutually referencing Blueprints

Use `-Action ImportBatch` when Blueprints reference each other. Its input manifest
is `{"assets":["Actor.json","Operator.json","HUD.json"]}`; paths are resolved
relative to the manifest, and absolute source paths are also accepted.

```powershell
& ./tools/BlueprintText/Invoke-BlueprintAssetText.ps1 `
  -Action ImportBatch -ProjectFile <prepared-project.uproject> `
  -InputPath ./Tmp/BlueprintText/batch.json `
  -OutputPath ./Tmp/BlueprintText/batch-comparison.json
```

Every target asset must be absent and occur only once. The importer allocates all
Blueprint classes before populating their objects/graphs and compiling them as a
group. Parent classes must already exist outside the batch: rebuilding a parent
and its child together is explicitly rejected; import the parent first. Other
external dependencies, including textures and native class mirrors, must be
available in the project.

All members must compile and accept their defaults/templates before any package
is saved. The operation is not a filesystem transaction: save-time or I/O failures
can still leave partial output. Preserve the input and inspect the returned
evidence directory on failure. On success, the wrapper reopens all assets in a
fresh Unreal process and writes an overall/per-asset comparison report to
`-OutputPath`; reopened sources are retained beside it. Any represented-field
difference fails the operation. Import does not merge with existing assets.

### Preview resources

For HUD images, add `-PreviewResources` to the same export invocation:

```powershell
& ./tools/BlueprintText/Invoke-BlueprintAssetText.ps1 `
  -Action Export -ProjectFile <prepared-project.uproject> `
  -Asset /Game/UI/HUD -OutputPath ./Tmp/BlueprintText/HUD.json `
  -PreviewResources
```

This requires a Widget Blueprint. Warn before launching headless Unreal and use
a prepared project under `Tmp/BlueprintText/` with its dependencies.
The command reads asset data without saving the source assets. It writes compact
JSON plus `<output stem>.resources/manifest.json` and PNGs in one engine process;
no mod-specific source-image paths are used. Re-export after editor changes.
The PNG bundle is preview output; Blueprint import resolves the referenced texture
assets already present in the project and does not recreate textures from PNGs.

The resource manifest has `schema: 1`, `asset`, `engine`, a `textures` map keyed
by Unreal object path, and an `unsupported` list. Texture entries contain a PNG
`file` relative to the manifest and pixel `width`/`height`. Repeated brush references
share one image. Reflected brushes include nested styles and containers, but
external custom-widget/material object graphs are not expanded. Only ordinary
single-block, single-layer, single-slice Texture2D editor source images are exported.
Materials, render targets, named resources without a loaded asset, and textures
without readable source pixels are reported as unsupported with a reason. PNGs
represent source mip 0, not cooked compression or runtime material effects.

Unsupported resources do not fail the Blueprint export; inspect the manifest to
understand missing preview images. File-write and conversion failures do fail the
command and retain diagnostic evidence. Use only output from a successful run;
failed runs can leave partial files. Keep generated exports and images under `Tmp/`.

## Visual review without Unreal

### Browse a set of Blueprints

[Render-BlueprintPreviews.ps1](BlueprintText/Render-BlueprintPreviews.ps1)
generates all compact graphs, available widget previews, and a common HTML index
from one JSON manifest. It is offline and does not modify source exports:

Routine agent work uses the workspace's single `Preview` action, which owns
engine discovery and this rendering manifest. Use the lower-level renderer
directly only for an explicitly supplied set of exports or tool development.

```powershell
& ./tools/BlueprintText/Render-BlueprintPreviews.ps1 `
  -InputPath ./Tmp/BlueprintText/review/previews.json `
  -OutputPath ./Tmp/BlueprintText/review/site
```

Output must be a new or empty directory. Open its `index.html`. The directory
contains source snapshots, individual viewers and `summary.json`. On failure,
inspect the reported source/graph or resource diagnostic and use a new output
directory after correcting the input; a partial directory is not a finished preview.

The manifest accepts source paths or explicit records, in display order:

```json
{
  "title": "Blueprint previews",
  "description": "Exported editor assets",
  "assets": [
    "exports/Actor.json",
    {"source": "exports/Parent.json", "placeholder": true}
  ]
}
```

Paths are relative to the manifest. Package identity comes from each source;
duplicate identities are rejected. `placeholder: true` gives the card a yellow
background and a red italic warning. The tool does not infer placeholders from
names. Source `editorRole: "stub"` also supplies the marker. Optional `note` adds
plain text to a card.

Workspace-generated records also carry project `context`: asset identity,
class, role, selection, diagnostic reason, dependencies and referencers. A
record without source snapshots requires an explicit reason and unchanged
status. Such records and unselected stubs go into the compact inventory at the
bottom, not graph cards. A supported non-stub Blueprint can have a card even
when it is not selected for Apply but belongs to the selected work's forward
reference context. Workspace Preview excludes unrelated project assets; it is
not a project inventory. Both Before and proposed source references contribute
context, while reverse references alone do not expand it. Catalog dependency/referencer lists are not
displayed as editable functionality.

Asset references in editable source are displayed at their node/pin or named
Blueprint property, with exact paths in tooltips and links to preview entries
when available. Stub markers come from the supplied project context.
When an input literal contains one asset reference, its displayed value carries
the link, including inside a folded chain's own step; no duplicate reference
line is appended to the card. Typed object/class inputs (`DefaultObject`) appear
at the same input position with a readable object name; their exact reference
remains in the tooltip and Details. Review compares exact references even when
their displayed names match.
Simplified and Expressions cards omit technical pin-type and member-owner links;
these remain resolved in node Details (`_typeReferences`) and visible in Full.
Chosen asset values (including class defaults) retain their links in every view.
Viewer role/link metadata stays separate from BPT source. The summary distinguishes
`editableAssets`, `listedAssets` and displayed `graphs` from total `assets`.

With no changes, the index is a browser: no status badges, review legend or diff
explanations. Any `added`, `changed` or `removed` record enables review mode with
badges and colored borders. Use these record shapes:

- Added: `{"source":"after/New.json","status":"added"}`.
- Changed: `{"source":"after/Actor.json","before":"before/Actor.json","status":"changed"}`.
- Removed: `{"before":"before/Old.json","status":"removed"}`.
- Unchanged: a source path, or a record with `source` and optional
  `"status":"unchanged"`; do not supply `before`.

Changed snapshots must have the same asset identity. The batch renderer compares
stable node identities, represented fields (including resolved class and pin
definitions) and incident connections to derive node review marks automatically.
Both endpoints of a changed connection are marked. Node/link ordering and
equivalent type-table aliases do not count as edits. This compares represented
state, including technical fields, rather than proving behavioral equivalence.
Order inside each node's `pins` object is significant and marks the node and
graph as changed, even when pin values and connections are identical.
A changed record may also provide supplemental `reviews`, keyed by GraphGuid or
unambiguous graph name, using the existing [node review lists](#review-mode):
`"reviews":{"EventGraph":{"changed":["SetLimit"]}}`. Use actual exported node
IDs. Missing or incomplete lists do not suppress automatically detected changes.
Graph Before links appear only when the graph or its represented descendants
differ between snapshots, or the graph moves to another owner. A relocation
shows `Moved` with the previous full path in its tooltip; it does not by itself
mark the graph's contents as changed or enable **Show changes**. A graph moved
and edited shows both `Moved` and `Changed`. Changes to shared type-table aliases alone do not
make a graph different. Added and removed graphs show all their nodes marked
accordingly, including graphs removed from a changed Blueprint. Removed content
is rendered from the Before snapshot. `"demo":true` marks the index explicitly
as a fictional review. None of these annotations becomes Blueprint source state.

Collapsed graphs appear in an expandable tree with counts aligned at the right.
Their parent operation has a double border and shows the child name; a third
toolbar icon beside Details and Copy opens the child graph. The inner border
follows card resizing. The child page links back through ancestor
operations. Before navigation stays within the Before snapshot. Stable graph
identity keeps a rename from appearing as an unrelated deletion/addition.
When a represented child or descendant changes, the parent operation receives
a purple frame and a nested-change label; the child's page shows its own node
changes. Added and removed operations keep their own status without a
redundant nested-change label. Final-only view clears nested-change highlights.

Function calls also receive a double border and an open-graph icon when their
implementation is represented in the batch preview, including functions in other
Blueprints. Targets are resolved from BPT owner and member identity; absent or
ambiguous targets do not receive a guessed link. Removed calls use the Before
target; review batches also generate Before pages for unchanged callees so reverse
navigation stays in the same snapshot. Folded chains have no graph of their own,
so they have neither a header navigation button nor a double
border. Each represented function call inside a chain links from its numbered step.

The function or collapsed graph's entry node has a reverse-arrow control next to
Details and Copy for represented calling graphs. One caller opens directly;
multiple callers open a chooser beside the entry control, with full Blueprint
and nested graph paths. Repeated calls within one graph share one entry. A single
call site is highlighted on arrival, including when folded into an action chain.
Collapsed graphs use their owning graph as the caller. These links are derived
from the same BPT batch; no additional asset discovery or agent command is needed.
The chooser supports keyboard activation and Escape returns focus to its control.
Browser Back restores the saved view, filters, zoom and scroll position.

The preview root has **Find a node**, searching represented node IDs, display
names and graph paths without regard to case. Results show the Blueprint and
full nested path because IDs can repeat between graphs. Select a result to open
its graph and highlight the node, including a member of a folded card. A node
hidden in Simplified opens in Full instead. **Include Before** adds old-snapshot
results, including deleted nodes. The query and snapshot choice survive Back
and are retained in the URL. Enter opens a unique result; with multiple matches,
it focuses the first result for selection. Search uses the same preview batch
and needs no extra agent command or asset query.

Each graph row shows local source node/link counts and a separate descendant
node count, independent of viewer folding or hidden disconnected nodes. Deleted
graphs use Before counts. Review rows distinguish Added, Changed, Removed and
Nested changes; the last means the local graph is unchanged but a represented
descendant differs. Relocations additionally show Moved. Unchanged graphs have no badge,
and ordinary browse mode has no review badges. Widget previews
use the adjacent `.resources/manifest.json`; optional `resources` and
`beforeResources` override it for the corresponding snapshot. An empty widget
tree or missing resource manifest is explained in the index instead of creating
a misleading layout. Supplied widget resources use the same validation and
rendering limits as the [HUD preview](#hud-widget-preview). Stock layouts absent
from editor placeholders and runtime-created widgets are not reconstructed.

### View one graph

```powershell
./tools/BlueprintText/Render-BlueprintGraph.ps1 -InputPath './tools/BlueprintText/Examples/division.json' -OutputPath './Tmp/BlueprintText/division.svg'
```

Requires Python 3.9+ (bundled runtime or `python`; override with `-PythonPath`).
Writes SVG, adjacent `.full.svg`, and a self-contained HTML viewer with zoom,
Full, Simplified and Expressions views and top-to-bottom/left-to-right directions. Defaults are
`-View simplified -Direction TB`; `-Graph` selects a graph when needed. No model,
network, engine, or catalogue call is involved. Input is never rewritten.

In vertical HTML views, **Nodes per row** selects 1–12 nodes (default 3).
Changing it rearranges the current graph locally and fits its width; the value
is retained between all three views. Horizontal views disable this
control. Standalone SVGs retain the default row capacity.

Nodes follow their upstream parents, prioritizing execution dependencies for
action chains. Data computations sit near their earliest consumer rather than
at the top of the graph; shared values remain single nodes. Cycles move together.
Edges curve directly between ports; joins, cycles and wrapped
rows can still cross. Incoming and outgoing ports place data left of execution
in vertical views (above execution in horizontal views), with stable pin order.
Links from the same output pin share one source point; distinct output pins
keep separate points, including after browser layout changes.

In HTML, click a node to highlight its incoming/outgoing links and immediate
neighbors; unrelated elements dim and link colours retain their meaning.
Click again, click the background, or press Escape to clear. Tab and Enter/Space
also select nodes. Changing view or direction clears selection. Only visible
links participate; use Full to include hidden technical nodes.

**Show unconnected** controls unconnected output descriptions on every node,
including execution branches, data types and output defaults, also inside folded
steps. Connected outputs, input defaults and graph entry/exit signatures
remain visible. Unconnected advanced outputs are omitted when BPT declares
`AdvancedPinDisplay=Hidden`; connected advanced outputs remain visible, and
Details retains all source pins and defaults. Toggling **Show unconnected** resizes
cards and reroutes links in both directions. A selected node is centered with
the current zoom retained; without a selection the view resets and fits its width.

Browser Back/Forward restores each graph visit's view, direction, row capacity,
unconnected-output filter, review mode, zoom, selected node and scroll position.
The graph title shows the full nesting path: the Blueprint name links to the
preview index and ancestor graph names link to their respective pages.

**Show changes**, next to **Show unconnected**, requires both graph snapshots
and either review marks or nested implementation changes. Turn it off to render
the final graph with its own layout, without removed nodes, old links or change highlights.
The final-only choice follows graph links. Ordinary browse pages, empty review
mark lists and entirely added or removed graphs do not add this control. Entirely
added or removed graphs retain their status without a comparison toggle.

Simplified view hides disabled and technical Self nodes, labels incoming hidden
sources, and folds exclusive data chains and linear action chains into numbered
steps. It does not combine repeated computations or invent bypass connections.
An action with a connected data input stays separate, including when that input
comes from the preceding action. Unconnected defaults and technical Self target
inputs do not prevent folding; a target supplied by another object does.
Full view retains all nodes. Folded steps preserve defaults, output connections
and unconnected-output notes; data chains retain their internal dependencies.
A sole unused execution output is treated as a terminal path; unused alternatives
remain visible.

**Expressions** is a presentation mode, not another editable format. It folds
recognized pure scalar arithmetic, comparisons, boolean operations, select,
min/max and supported Vector calls into bounded formula cards. Native function identity and pin types
must match; unsupported operations and incomplete signatures stay as nodes.
Variable reads, shared results and graph boundaries remain separate. Operand
order, parentheses, select alternatives and numeric conversions are preserved;
the viewer does not evaluate or algebraically simplify formulas. Different review
statuses do not merge, and removed connections remain explicit. Use Full or
Details to inspect the original nodes, pins and links; edit the BPT graph normally.

Vector formulas support `Vector(x, y, z)`, `dot(a, b)`, vector select,
addition/subtraction and vector-times-number multiplication. `VInterpTo_Constant`
remains a named call with labelled current/target/time/speed arguments; its
interpolation algorithm is not expanded. Only the native `CoreUObject.Vector`
type is recognized, not arbitrary structs or containers. Vector defaults require
three explicit numeric components; unsupported representations remain as nodes.
A `MakeVector` whose components are numeric defaults and have no incoming links
can be repeated as a literal in each consuming expression. Its source node and
links remain in Details. A consumer outside the supported expressions keeps the
constructor card; computed shared vectors are never duplicated this way.

Expression cards list typed inputs before `Output (type): formula`, separated by
a small vertical gap. Known names are retained; anonymous inputs use local
`value1`, `value2`, etc. in first-use order, reusing the name for repeated uses
and avoiding collisions with real names. Formulas use monospace text, two-space
structural indentation and a target width of 60 characters. Long identifiers are
not split; cards widen as needed. Service labels use the ordinary bold label
font. Repeated uses of one source pin in one expression share a single arrow;
its tooltip lists all original destinations. Distinct source pins stay separate.
The mode works with review/final-only switching and browser history restoration.

Nodes describe operations and input values; function calls and variable getters
also show known output types. The header legend uses colored node cards and arrow
samples: blue entries/events, amber execution steps, grey data/other nodes;
solid amber execution links and dashed blue data links.
`Branch:` identifies execution outputs; `Output:` identifies data outputs.
Nodes without data outputs do not receive an extra placeholder label.
Unconnected text inputs show the stored source text from `DefaultTextValue`
(`NSLOCTEXT`, `LOCTEXT`, or `INVTEXT`), without resolving runtime translations.
Unknown text expressions remain visible verbatim; ignored defaults are omitted.
Details retains the complete original localization expression.
`Break Struct` names its declared structure and lists visible output fields as
`Field: name (type)`. Connected fields remain visible; unused fields follow
**Show unconnected**. These labels describe extracted fields, not stored values.
Collapsed graph tunnels are labelled `Graph Entry` and `Graph Exit` from their
declared direction. An exit Tunnel with no incoming execution link is shown as
an action labelled `Graph Never Continues` when it declares execution inputs or
the graph has an execution entry. This includes an exit with no pins. Review
uses the corresponding snapshot; removed links cannot imply a current continuation.
If the exit also declares data
results, the card warns that those results cannot be returned. This is a local
connection check, not whole-graph reachability analysis; pure data tunnels and
function returns keep their ordinary labels. Graph and function entries list visible data parameters as
`Parameter: name (type)`, including unused parameters. These are signatures,
not runtime values inferred from stored output defaults.
Graph exits and function results list their visible data results as
`Output: name (type)`, using their declared input pins. These signatures remain
visible with **Show unconnected** off. Input and output sections are separated
by a small gap. Folded chains instead separate successive steps, keeping each
step's inputs and outputs together without an internal gap. This grouping is
preserved when unconnected outputs are hidden. `Input`, `Output`, `Parameter`
and `Field` labels are bold.
Display titles split identifiers into readable words and wrap at word boundaries;
continued titles retain their heading style. Original identifiers remain in details.
Stored defaults on unconnected output pins are labelled `Output: name = value
(default; type)`, not presented as computed results. Connected or ignored output
defaults are omitted. The copy icon
copies the original node ID, or all IDs in a folded chain. The info icon opens
selectable source details below the node; the panel also offers copy controls.

Hover anywhere along an edge for `source node ID -> target node ID`, followed by
`From: pin (type)` and `To: pin (type)`. Each type comes from its own endpoint;
missing types are omitted, and folded steps resolve to their original endpoints.
Click for immediate selectable details. Every connected execution
output has a permanent label near its source; data labels stay in tooltips.
For Branch choices, `then` is left of `else` in vertical
view (above it in horizontal view). Unknown types are not guessed. These are
explicit graph links, not inferred timer callbacks or runtime behaviour.

The viewer accepts both compact schemas, including the earlier standalone
`nodes`/`links` shape; it does not render `.t3d`. There is no fixed node-count cap.
Traversal is iterative and selection uses a cached connection index. Large graphs
still require browser memory and rendering work; HTML contains all four
view/direction variants.

### Review mode

Workspace Preview and the batch renderer derive node marks automatically from
their snapshots. The lower-level single-graph renderer instead accepts the
current compact source, an original source and a JSON file of explicit node IDs
for the selected graph:

```json
{
  "added": ["NewDivision"],
  "changed": ["SetLimit"],
  "removed": ["OldConstant"]
}
```

```powershell
./tools/BlueprintText/Render-BlueprintGraph.ps1 -InputPath ./Tmp/BlueprintText/review/after.json -BeforePath ./Tmp/BlueprintText/review/before.json -ReviewPath ./Tmp/BlueprintText/review/review.json -Graph EventGraph -OutputPath ./Tmp/BlueprintText/review/EventGraph.svg
```

For this single-graph interface, the author supplies the lists; it does not
discover changes or verify that the lists are complete. For connection-only edits, mark both endpoint nodes as
changed. IDs must be unique across lists and match their declared state: added
exists in current (and must be absent from original when supplied), removed only
in original, changed in both. Original and
current must identify the same asset and graph. `-BeforePath` is required for
changed/removed nodes; it is optional for an additions-only review. Invalid marks
stop rendering with a diagnostic; correct the lists or selected source/graph.
Use fresh node identities when replacing a removed chain with a new call;
reusing a removed operation's ID represents it as Changed instead of Added.

Green `+`, purple `~` and red dashed `−` borders mark added, changed and removed
nodes; existing fill colours retain their execution/data meaning. Marked nodes
stay visible in Simplified view. Linear actions with the same review status may
fold under the ordinary dependency and branching rules; different statuses never
merge. Folded review details preserve every step's Before/After snapshots and
changed-field highlights. Marked data nodes remain separate. Removed nodes and
their original connections to displayed endpoints appear as translucent ghosts.
Changes remain a display overlay; neither
source is rewritten or imported into Unreal.

On changed nodes, modified description fields are purple; hover shows the
previous value. Comparison precedes text wrapping and matches named inputs and
outputs. Connection-only edits do not highlight unchanged description fields.
Only fields represented in the description can be highlighted there.

Details opens After by default (Before for a removed node), with a version
selector. Changed-node After marks individual fields by key, including nested
object headings; hover shows prior values. Added fields use `+`; removed fields
use `−` and strikethrough. GUID and position changes use neutral grey. Incident
links are compared by endpoint pair, ignoring order; other arrays are marked as
whole fields without an element diff. Before shows the original snapshot.
Reordered node pins mark the `pins` heading, whose tooltip lists the previous
order. After lists surviving pins in their current order, with removed pins
following them. Other object-key ordering does not count as a field change.
Copy contents returns clean JSON of the selected snapshot, without review
markers or deleted-field overlays. Keep review lists and generated previews with
temporary review output; they are not Blueprint source requirements.

### Function names

[Metadata/function-labels.json](BlueprintText/Metadata/function-labels.json)
is a versioned English snapshot of native Engine function display names and
compact titles. Lookup uses owner/function identity and native ancestry, with a
readable fallback for unknown functions. It covers loaded `/Script/Engine`
classes, not all plugin, game or Blueprint functions. This is UI metadata, not
the ignored insertion catalogue or a catalogue of every editor action.

To refresh after changing engines, run `Invoke-BlueprintAssetText.ps1
-Action ExportLabels -ProjectFile <prepared.uproject> -OutputPath <Tmp output>`,
then `python -B tools/BlueprintText/function_labels.py <export.json>
--output tools/BlueprintText/Metadata/function-labels.json`. The first
command launches Unreal; the second validates and deterministically publishes
the snapshot. Normal rendering reads the snapshot locally.

## HUD widget preview

[Render-WidgetPreview.ps1](BlueprintText/Render-WidgetPreview.ps1) builds
a standalone HTML preview from an existing widget-tree export and a PNG resource
bundle. It requires Python 3.9+ and does not launch Unreal, modify source assets,
or import review changes. First use the [widget export](#widget-trees-interfaces-and-resources)
command with `-PreviewResources` to produce the compact source and images in one
headless Unreal invocation; rendering the resulting HTML is entirely offline.

```powershell
& ./tools/BlueprintText/Render-WidgetPreview.ps1 `
  -InputPath ./Tmp/BlueprintText/HUD.json `
  -OutputPath ./Tmp/BlueprintText/HUD.html
```

The default resource manifest is `HUD.resources/manifest.json` next to `HUD.json`.
Use `-ResourcesPath <manifest.json>` to override it and `-PythonPath <python.exe>`
to select an interpreter. The result reports the HTML path, widget/texture counts
and warnings. The HTML embeds its data and images; it needs no server or network.
After editor changes, regenerate the export and resource bundle, rebuild the HTML,
and reload it. The viewer does not watch assets or merge concurrent changes.

The exported tree's `RootWidget`, panel `Slots` and slot `Content` references
define the hierarchy. Object IDs retain the Blueprint name, for example
`HUD:WidgetTree.Root`; references can also use their full Unreal object paths.
Graphs and animations are not needed for layout rendering.

Use the resource manifest produced by the exporter for the same `asset`.
An empty texture map is valid. Missing referenced images and unsupported widgets
produce placeholders and warnings. Invalid references, cycles, duplicate object
IDs, a mismatched asset, invalid PNG dimensions or paths outside the resource
bundle fail rendering. Correct the reported input or regenerate its export before
retrying; do not substitute an unrelated resource bundle.

In the viewer:

- Select a widget in the hierarchy or canvas. Hierarchy checkboxes show/hide it;
  hiding a parent preserves its children's individual choices. Selection does
  not reveal hidden widgets. Exported `Hidden` widgets retain layout space but
  are not drawn, including their descendants. `Collapsed` widgets are excluded
  from layout. Reset visibility restores the exported states.
- Adjust image/text tint, widget opacity and sample text in the inspector.
  Multiline samples update approximate automatic sizing. Drag an element to
  move it; the gold corner handle moves the selected container and its children.
  Escape cancels a drag. Reset controls discard one widget's or all adjustments.
- **Copy review notes** copies widget IDs, original/requested values and movement
  deltas for the agent. If clipboard access is unavailable, the notes are selected
  for Ctrl+C. Tint values are linear RGBA converted from the browser's sRGB picker.
  Movement is recorded in preview layout units with its viewport and source slot;
  the agent must adapt it to the actual Unreal layout. Text samples are explicitly
  for layout review, not requests to replace dynamic text or bindings. Visibility
  checkboxes are viewing aids and are excluded from review notes.

Adjustments live only in the open page and disappear on reload. No modified asset
or compact source is saved. The review notes are a handoff for the agent, not an
automatically applicable patch.

The preview approximates CanvasPanel anchors/offsets/alignment, horizontal and
vertical boxes, InvalidationBox, ScaleBox, TextBlock, Image and RadialSlider.
Images use exported UV regions and tint. Font metrics, automatic sizing, box fill,
ScaleBox and radial rendering are approximate. Custom widgets are not expanded;
materials, transforms, non-default clipping, DPI rules, Blueprint bindings and
animations are not executed. This is a layout review aid, not a runtime HUD render
or proof of arbitrary widget support.

## Checks

Source reading and scoped detail checks: `python -B tools/BlueprintText/test_graph_inspection.py`.
Member-use search and optional context checks: `python -B tools/BlueprintText/test_graph_references.py`.
Call-template request/factory checks: `python -B tools/BlueprintText/test_call_templates.py`.
Native reflection acceptance: `tools/BlueprintText/Test-CallTemplates.ps1 -OutputRoot <new directory below Tmp/BlueprintText>`
with `-EngineRoot` for the selected engine. The isolated queued runner checks
five component/math calls through strict Apply and fresh-process inspection,
explicit false overrides, pure pins, a connected const-reference input, rejected
missing-reference/latent cases, asset preservation after rejection and no-op Apply.
These are editor checks, not gameplay acceptance.
Offline renderer checks: `python -B tools/BlueprintText/test_render_graph.py`.
Expression presentation checks: `python -B tools/BlueprintText/test_graph_expressions.py`.
Batch index checks: `python -B tools/BlueprintText/test_preview_index.py`.
Automatic structural comparison checks: `python -B tools/BlueprintText/test_graph_review.py`.
Function navigation checks: `python -B tools/BlueprintText/test_preview_graph_links.py`.
Review overlay checks: `python -B tools/BlueprintText/test_review_graph.py`.
Details comparison checks: `python -B tools/BlueprintText/test_review_details.py`.
Codec checks: `python -B tools/BlueprintText/test_asset_text_codec.py`.
Label checks: `python -B tools/BlueprintText/test_function_labels.py`.
HUD input/resource checks: `python -B tools/BlueprintText/test_render_widget.py`.
HUD interaction checks: `node tools/BlueprintText/test_widget_viewer.js`
(synthetic DOM checks for visibility, tint/opacity/text, dragging, review copying
and source isolation; no Unreal or browser is launched).
With Node.js available, run `node tools/BlueprintText/test_graph_interactions.js <division.html>`
on HTML rendered from the division example to check selection, details, copying and viewer controls
with a DOM stand-in (not a browser UI test).
Run `node tools/BlueprintText/test_graph_layout.js <generated.html>`
to check Python/browser geometry agreement, row capacities, port ordering and
reversible hide/show layout updates in both directions on any generated preview.
The schema-1 engine acceptance runner is
`tools/BlueprintText/Test-BlueprintText.ps1 -OutputRoot <new directory below Tmp/BlueprintText>`
with optional `-EngineRoot`. It builds an isolated project, checks compact and
native round trips, adds and removes nodes, preserves a headless simulated
editor change, exercises another asset and rejects selected unsupported state.
It also checks object/text pin defaults through native import/export and verifies
that compact export refuses them without changing the saved package. The whole
runner reserves the shared Unreal queue, including its expected-failure checks.
It writes `result.json` and logs in that directory. This is compilation and
serialization evidence, not GUI interaction, cooking, game behaviour, or a
token-cost measurement. Schema-2 imports perform their own fresh-process
round-trip comparison; they are not covered by the schema-1 acceptance runner.

For schema-2 engine acceptance, use the generic fixture runner on a disposable
prepared project and existing compact sources:

```powershell
python -B ./tools/BlueprintText/test_asset_text_roundtrip.py `
  --project ./Tmp/BlueprintText/demo/Project/BlueprintText.uproject `
  --input ./Tmp/BlueprintText/batch.json `
  --output ./Tmp/BlueprintText/acceptance-new
```

Warn before running it and close Unreal Editor: it invokes the public importer
and a fresh-process re-export. Python 3.9+, Windows PowerShell and the configured
Unreal engine are required. The project and a new output directory must be below
`Tmp/BlueprintText/`. The project must already contain all external
dependencies and the current BlueprintText module. The runner temporarily moves only
the target `.uasset` files, retains rebuilt candidates and logs, then restores
originals and checks their bytes even after an import failure. It accepts ordinary
uncooked Blueprint packages without `.uexp`/`.ubulk` sidecars. Never use it on a
production project.

Use `--single` with one compact source instead of a batch manifest. For a negative
case, `--expect-error "diagnostic text"` requires a nonzero result, the named
diagnostic and no saved candidate. `--keep-existing` is restricted to negative
cases testing refusal to overwrite a target. A successful result is reported in
`result.json`; on failure inspect `worker.log` and the wrapper's engine/comparison
evidence. These checks validate serialization and compilation, not gameplay.
