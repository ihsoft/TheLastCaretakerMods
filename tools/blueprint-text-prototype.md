# Blueprint text prototype

An experimental workflow for editing a Blueprint as declarative text and
mechanically creating its uncooked `.uasset`. The shared builder is independent
of any mod. Compact JSON and native Unreal graph text are both available; the
final production source format is still undecided. This is not a release or
installation tool. An existing C++ generator can supply the initial uncooked
asset in an isolated project; the prototype exports that asset, not the C++.

## Schema 1: small examples

The envelope declares `schema`, `asset`, `parent`, `variables`, and `graphs`.
Currently it supports one `/Game` Blueprint, one `EventGraph`, and baseline
scalar declarations (`float`, `double`, `int`, `bool`, with zero defaults).
Compact factories are `event`, `call`, `sequence`, `getVariable`, and
`setVariable`. Links use `nodeId.pinAlias`; ordinary reflected calls use their
owner/function identity. See the complete [division example](BlueprintTextPrototype/Examples/division.json).

Native mode uses the same envelope with an adjacent editable `.t3d` graph body
referenced by `bodyFile`. It preserves native node text more broadly, but does
not remove the envelope's Blueprint-level limitations. Conversion between the
two modes goes through a prepared Unreal project.

Neither schema is a general lossless exporter for arbitrary Blueprints.
Schema 1 rejects known unsupported state (including user functions, macros, timelines, custom
components, nonzero variable defaults and modified construction logic).
Compact mode additionally rejects unsupported node classes and
disabled nodes. Other Unreal metadata is not comprehensively represented or
audited. Work only on disposable prototype assets until that boundary expands.

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
$tool = './tools/BlueprintTextPrototype/Invoke-BlueprintTextPrototype.ps1'
$run = './Tmp/BlueprintTextPrototype/demo' # must be a new directory
$project = (& $tool -Action Prepare -OutputRoot $run).projectRoot
& $tool -Action RefreshCatalog -ProjectRoot $project -Force
& $tool -Action SearchCatalog -Query divide -Limit 10 | ConvertTo-Json -Depth 12
& $tool -Action Import -ProjectRoot $project -InputPath './tools/BlueprintTextPrototype/Examples/division.json'
& $tool -Action Export -ProjectRoot $project -Asset /Game/BP_DivisionExample -Format compact -OutputPath "$run/export.json"
& $tool -Action Export -ProjectRoot $project -Asset /Game/BP_DivisionExample -Format native -OutputPath "$run/native.json"
```

Search is offline after refresh. It returns pin types, semantic aliases and a
copyable compact node template; `-Format native` returns engine-generated native
insertion text instead. The initial catalogue covers callable/pure functions in
`KismetMathLibrary` and `flow.sequence`, not all Unreal editor actions. It records
the engine version; refresh after changing engines. Discovery does not guarantee
that every catalogued function works in every graph context.

Prepared projects, exports and engine logs stay below `Tmp/BlueprintTextPrototype/`.
Assets are saved under the prepared project's `Content`. Import compiles before
saving and reports errors with log paths. On failure inspect that diagnostic;
do not bypass an unsupported-state rejection. `-Action Help` shows parameters
and the node skeletons. The `Simulate*` actions are test fixtures only.

## Schema 2: existing Actor Blueprint experiments

[Invoke-BlueprintAssetText.ps1](BlueprintTextPrototype/Invoke-BlueprintAssetText.ps1)
exports a complete supported Blueprint as compact JSON. It carries event and
function graphs (including construction logic), native variable descriptions,
CDO defaults, Blueprint/graph properties, the automatic scene root, native node
properties and typed pins. Original object names and GUIDs are retained. Class
aliases and shared pin templates reduce repetition without replacing the native
property values. Some fields deliberately remain Unreal property text.

The current boundary is a Blueprint whose direct parent is `/Script/Engine.Actor`,
with an automatic scene root only. Blueprint inheritance, custom component trees,
interfaces, macro definitions, timelines, extensions, nested graphs and nested node objects are
rejected. Required native classes and external asset dependencies must already
exist in the prepared project. A blank prototype project does not supply game
classes or assets. This is an uncooked editor-asset workflow.

Use a disposable prepared project containing the prototype editor module and
the asset's dependencies. All actions below launch headless Unreal; warn first
and close the editor. Project and output paths must be below
`Tmp/BlueprintTextPrototype/`.

```powershell
$assetTool = './tools/BlueprintTextPrototype/Invoke-BlueprintAssetText.ps1'
$projectFile = Join-Path $project 'BlueprintTextPrototype.uproject'
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
Numeric property spellings are compared strictly. This check does not establish
runtime equivalence or preservation of state outside the supported envelope.
Export editor changes before the next text edit; there is no concurrent-edit
merge. Keep experiments separate from production assets.

The offline [codec](BlueprintTextPrototype/asset_text_codec.py) converts schema-2
compact/native envelopes and creates call nodes from the engine catalogue:

```powershell
python -B tools/BlueprintTextPrototype/asset_text_codec.py add-call $source --catalog tools/BlueprintTextPrototype/.cache/catalog.json --operation /Script/Engine.KismetMathLibrary::Divide_DoubleDouble --id Division --graph Calculate --output "$run/edited.json"
```

Use an existing graph name. The command allocates IDs and merges pin metadata;
edit the new node's defaults and links in the resulting complete source.
`template` produces a standalone insertion fragment. `compact`, `native` and
`compare` perform offline conversion and comparison; use `--help` for arguments.
New operations belong in shared discovery/serialization, not a per-mod builder.

## Visual review without Unreal

```powershell
./tools/BlueprintTextPrototype/Render-BlueprintGraph.ps1 -InputPath './tools/BlueprintTextPrototype/Examples/division.json' -OutputPath './Tmp/BlueprintTextPrototype/division.svg'
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
action chains. Edges curve directly between ports; joins, cycles and wrapped
rows can still cross. Incoming and outgoing ports place data left of execution
in vertical views (above execution in horizontal views), with stable pin order.

In HTML, click a node to highlight its incoming/outgoing links and immediate
neighbors; unrelated elements dim and link colours retain their meaning.
Click again, click the background, or press Escape to clear. Tab and Enter/Space
also select nodes. Changing view or direction clears selection. Only visible
links participate; use Full to include hidden technical nodes.

Simplified view hides disabled and technical Self nodes, labels incoming hidden
sources, and folds exclusive data chains and linear action chains into numbered
steps. It does not combine repeated computations or invent bypass connections.
Full view retains all nodes. Folded steps preserve their inputs, internal data
dependencies and unconnected-output notes. A sole unused execution output is
treated as a terminal path; unused alternatives remain visible.

Nodes describe operations, input values and known output types. Amber identifies
execution nodes using available pin metadata; blue identifies entries/events.
Display titles split identifiers into readable words and wrap at word boundaries;
continued titles retain their heading style. Original identifiers remain in details.
Output-pin default metadata is not shown as a computed result. The copy icon
copies the original node ID, or all IDs in a folded chain. The info icon opens
selectable source details below the node; the panel also offers copy controls.

Hover anywhere along an edge for its endpoints, pin names and known source
output type. Click for immediate selectable details. Only Branch choices keep
permanent edge labels near their source: `then` is left of `else` in vertical
view (above it in horizontal view). Unknown types are not guessed. These are
explicit graph links, not inferred timer callbacks or runtime behaviour.

The viewer accepts both compact schemas (up to 500 nodes per graph), including
the earlier standalone `nodes`/`links` shape; it does not render `.t3d`.

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
./tools/BlueprintTextPrototype/Render-BlueprintGraph.ps1 -InputPath ./Tmp/BlueprintTextPrototype/review/after.json -BeforePath ./Tmp/BlueprintTextPrototype/review/before.json -ReviewPath ./Tmp/BlueprintTextPrototype/review/review.json -Graph EventGraph -OutputPath ./Tmp/BlueprintTextPrototype/review/EventGraph.svg
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
connections to displayed endpoints appear as translucent ghosts. The combined
review must fit the 500-node limit. Changes remain a display overlay; neither
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

[Metadata/function-labels.json](BlueprintTextPrototype/Metadata/function-labels.json)
is a versioned English snapshot of native Engine function display names and
compact titles. Lookup uses owner/function identity and native ancestry, with a
readable fallback for unknown functions. It covers loaded `/Script/Engine`
classes, not all plugin, game or Blueprint functions. This is UI metadata, not
the ignored insertion catalogue or a catalogue of every editor action.

To refresh after changing engines, run `Invoke-BlueprintAssetText.ps1
-Action ExportLabels -ProjectFile <prepared.uproject> -OutputPath <Tmp output>`,
then `python -B tools/BlueprintTextPrototype/function_labels.py <export.json>
--output tools/BlueprintTextPrototype/Metadata/function-labels.json`. The first
command launches Unreal; the second validates and deterministically publishes
the snapshot. Normal rendering reads the snapshot locally.

## Checks

Offline renderer checks: `python -B tools/BlueprintTextPrototype/test_render_graph.py`.
Review overlay checks: `python -B tools/BlueprintTextPrototype/test_review_graph.py`.
Details comparison checks: `python -B tools/BlueprintTextPrototype/test_review_details.py`.
Codec checks: `python -B tools/BlueprintTextPrototype/test_asset_text_codec.py`.
Label checks: `python -B tools/BlueprintTextPrototype/test_function_labels.py`.
With Node.js available, run `node tools/BlueprintTextPrototype/test_graph_interactions.js <division.html>`
on HTML rendered from the division example to check selection, details, copying and viewer controls
with a DOM stand-in (not a browser UI test).
Run `node tools/BlueprintTextPrototype/test_graph_layout.js <generated.html>`
to check Python/browser geometry agreement, row capacities, port ordering and
reversible layout updates on any generated preview.
The schema-1 engine acceptance runner is
`tools/BlueprintTextPrototype/Test-BlueprintTextPrototype.ps1 -OutputRoot <new directory below Tmp/BlueprintTextPrototype>`
with optional `-EngineRoot`. It builds an isolated project, checks compact and
native round trips, adds and removes nodes, preserves a headless simulated
editor change, exercises another asset and rejects selected unsupported state.
It writes `result.json` and logs in that directory. This is compilation and
serialization evidence, not GUI interaction, cooking, game behaviour, or a
token-cost measurement. Schema-2 imports perform their own fresh-process
round-trip comparison; they are not covered by the schema-1 acceptance runner.
