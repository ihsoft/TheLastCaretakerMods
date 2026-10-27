# Blueprint text prototype

An experimental workflow for editing a Blueprint as declarative text and
mechanically creating its uncooked `.uasset`. The shared builder is independent
of any mod. Compact JSON and native Unreal graph text are both available; the
final production source format is still undecided. This is not a release or
installation tool, and it does not convert existing C++ generators.

## Supported boundary

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

This is **not a general lossless exporter for arbitrary existing Blueprints**.
Known unsupported state (including user functions, macros, timelines, custom
components, nonzero variable defaults and modified construction logic) is
rejected. Compact mode additionally rejects unsupported node classes and
disabled nodes. Other Unreal metadata is not comprehensively represented or
audited. Work only on disposable prototype assets until that boundary expands.

The text is the full desired supported state: omitting a node deletes it.
Export editor changes before editing/importing text again. Import does not merge
concurrent edits. IDs are retained through an internal node-comment prefix;
removing that prefix in the editor can change the exported identity. Optional
nonzero positions are preserved; the offline viewer computes its own layout.

## Basic use

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

## Visual review without Unreal

```powershell
./tools/BlueprintTextPrototype/Render-BlueprintGraph.ps1 -InputPath './tools/BlueprintTextPrototype/Examples/division.json' -OutputPath './Tmp/BlueprintTextPrototype/division.svg'
```

Requires Python 3.9+ (bundled runtime or `python`; override with `-PythonPath`).
Writes SVG, adjacent `.full.svg`, and a self-contained HTML viewer with zoom,
full/simplified views and top-to-bottom/left-to-right directions. Defaults are
`-View simplified -Direction TB`; `-Graph` selects a graph when needed. No model,
network, engine, or catalogue call is involved. Input is never rewritten.

In HTML, click a node to highlight its incoming/outgoing links and immediate
neighbors; unrelated elements dim and link colours retain their meaning.
Click again, click the background, or press Escape to clear. Tab and Enter/Space
also select nodes. Changing view or direction clears selection. Only visible
links participate; use Full to include hidden technical nodes.

Simplified view hides disabled and technical Self nodes, labels incoming hidden
sources, and does not invent bypass connections. Full view retains all nodes.
Hover reveals source fields. Execution colouring is based on known pin names;
the diagram shows explicit links, not inferred timer callbacks or proof of
runtime behaviour. This small viewer accepts compact exports (up to 500 nodes),
including the earlier standalone `nodes`/`links` shape; it does not render `.t3d`.

## Checks

Offline renderer checks: `python -B tools/BlueprintTextPrototype/test_render_graph.py`.
With Node.js available, run `node tools/BlueprintTextPrototype/test_graph_interactions.js <division.html>`
on HTML rendered from the division example to check selection and viewer controls
with a DOM stand-in (not a browser UI test).
The engine acceptance runner is
`tools/BlueprintTextPrototype/Test-BlueprintTextPrototype.ps1 -OutputRoot <new directory below Tmp/BlueprintTextPrototype>`
with optional `-EngineRoot`. It builds an isolated project, checks compact and
native round trips, adds and removes nodes, preserves a headless simulated
editor change, exercises another asset and rejects selected unsupported state.
It writes `result.json` and logs in that directory. This is compilation and
serialization evidence, not GUI interaction, cooking, game behaviour, or a
token-cost measurement.
