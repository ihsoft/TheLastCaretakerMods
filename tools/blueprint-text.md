# BlueprintText format and tools

For new and existing mods, use the [workspace workflow](blueprint-workspace.md).
The interfaces below describe the format, lower-level tools and validation.

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
CDO defaults, Blueprint/graph properties, component object trees and templates,
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
from `/Script/CoreUObject.Object`. Other UObject families and UObject Blueprint
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

Create instances with `Construct Object from Class`, provide an `Outer`, and
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
edit the new node's defaults and links in the resulting complete source.
`template` produces a standalone insertion fragment. `compact`, `native` and
`compare` perform offline conversion and comparison; use `--help` for arguments.
New operations belong in shared discovery/serialization, not a per-mod builder.

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

For inexpensive source inspection, run the offline helper once:

```powershell
python -B tools/BlueprintText/graph_hierarchy.py $source
python -B tools/BlueprintText/graph_hierarchy.py $source --graph $graphGuid
```

Set `$graphGuid` to the desired ID from the summary. The first call returns
graph summaries with owners and local counts. Selection
accepts a GraphGuid or an unambiguous name and returns the graph, ancestor
operations, child summaries and required class/pin definitions. These are
read-only views; edit the complete source JSON for Apply. No Unreal query or
recovery of missing mod data is involved.

`Test-NestedGraphs.ps1 -OutputRoot <new directory below Tmp/BlueprintText>`
runs the public workspace workflow in an isolated project through the shared
Unreal queue. It covers two levels, repeated names, a BPT body edit and rename,
strict re-export, invalid-owner rejection without changing the package, a no-op
Apply, and one-call Preview. Fresh-process Blueprint execution checks branch
results and side-effect order in UObject functions and Actor event/construction
graphs. Vector, array and object boundaries/defaults are checked structurally.
These are editor checks, not a cooked-game acceptance claim.

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
extensions, unsupported nested object families and inherited component overrides.

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
when it is not selected for Apply. Catalog dependency/referencer lists are not
displayed as editable functionality.

Asset references in editable source are displayed at their node/pin or named
Blueprint property, with exact paths in tooltips and links to preview entries
when available. Stub markers come from the supplied project context. Viewer
role/link metadata stays separate from BPT source. The summary distinguishes
`editableAssets`, `listedAssets` and displayed `graphs` from total `assets`.

With no changes, the index is a browser: no status badges, review legend or diff
explanations. Any `added`, `changed` or `removed` record enables review mode with
badges and colored borders. Use these record shapes:

- Added: `{"source":"after/New.json","status":"added"}`.
- Changed: `{"source":"after/Actor.json","before":"before/Actor.json","status":"changed"}`.
- Removed: `{"before":"before/Old.json","status":"removed"}`.
- Unchanged: a source path, or a record with `source` and optional
  `"status":"unchanged"`; do not supply `before`.

Changed snapshots must have the same asset identity. A changed record may also
provide `reviews`, keyed by GraphGuid or unambiguous graph name, using the existing [node review lists](#review-mode):
`"reviews":{"EventGraph":{"changed":["SetLimit"]}}`. Use actual exported node
IDs. Without a list, the card still has its status and graph Before links; node
changes are not inferred. Added and removed graphs show all their nodes marked
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
a nested-change label; this does not infer individual node review lists.

Each graph row shows local source node/link counts and a separate descendant
node count, independent of viewer folding or hidden disconnected nodes. Deleted
graphs use Before counts. Widget previews
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
full/simplified views and top-to-bottom/left-to-right directions. Defaults are
`-View simplified -Direction TB`; `-Graph` selects a graph when needed. No model,
network, engine, or catalogue call is involved. Input is never rewritten.

In vertical HTML views, **Nodes per row** selects 1–12 nodes (default 3).
Changing it rearranges the current graph locally and fits its width; the value
is retained between Full and Simplified views. Horizontal views disable this
control. Standalone SVGs retain the default row capacity.

Nodes follow their upstream parents, prioritizing execution dependencies for
action chains. Data computations sit near their earliest consumer rather than
at the top of the graph; shared values remain single nodes. Cycles move together.
Edges curve directly between ports; joins, cycles and wrapped
rows can still cross. Incoming and outgoing ports place data left of execution
in vertical views (above execution in horizontal views), with stable pin order.

In HTML, click a node to highlight its incoming/outgoing links and immediate
neighbors; unrelated elements dim and link colours retain their meaning.
Click again, click the background, or press Escape to clear. Tab and Enter/Space
also select nodes. Changing view or direction clears selection. Only visible
links participate; use Full to include hidden technical nodes.

**Show unconnected** controls unconnected output descriptions on every node,
including execution branches, data types and output defaults, also inside folded
steps. Connected outputs and input defaults remain visible. Toggling it resizes
cards and reroutes links in both directions. A selected node is centered with
the current zoom retained; without a selection the view resets and fits its width.

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

Nodes describe operations and input values; function calls and variable getters
also show known output types. The header legend uses colored node cards and arrow
samples: blue entries/events, amber execution steps, grey data/other nodes;
solid amber execution links and dashed blue data links.
`Branch:` identifies execution outputs; `Output:` identifies data outputs.
Nodes without data outputs do not receive an extra placeholder label.
Display titles split identifiers into readable words and wrap at word boundaries;
continued titles retain their heading style. Original identifiers remain in details.
Stored defaults on unconnected output pins are labelled `Output: name = value
(default; type)`, not presented as computed results. Connected or ignored output
defaults are omitted. The copy icon
copies the original node ID, or all IDs in a folded chain. The info icon opens
selectable source details below the node; the panel also offers copy controls.

Hover anywhere along an edge for its endpoints, pin names and known source
output type. Click for immediate selectable details. Every connected execution
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

Supply the current compact source, an original source and a JSON file of explicit
node IDs for the selected graph:

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

The author supplies the lists; the tool does not discover changes or verify that
the lists are complete. For connection-only edits, mark both endpoint nodes as
changed. IDs must be unique across lists and match their declared state: added
exists in current (and must be absent from original when supplied), removed only
in original, changed in both. Original and
current must identify the same asset and graph. `-BeforePath` is required for
changed/removed nodes; it is optional for an additions-only review. Invalid marks
stop rendering with a diagnostic; correct the lists or selected source/graph.

Green `+`, purple `~` and red dashed `−` borders mark added, changed and removed
nodes; existing fill colours retain their execution/data meaning. Marked nodes
stay visible and separate in Simplified view. Removed nodes and their original
connections to displayed endpoints appear as translucent ghosts.
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

Offline renderer checks: `python -B tools/BlueprintText/test_render_graph.py`.
Batch index checks: `python -B tools/BlueprintText/test_preview_index.py`.
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
