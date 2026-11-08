# Maintaining BlueprintText

Use this guide for BPT tool development and specialist review, under the
[ownership and decision rule](../docs/blueprint-text-maintenance-rule.md).
Ordinary mod work starts with the [workspace workflow](blueprint-workspace.md).
This guide selects existing contracts and checks; it does not add preflights
to successful authoring cycles.

## Find the owning boundary

| Question | Owning contract |
| --- | --- |
| Which project state is authoritative, and what belongs in Git? | [Workspace source and synchronization](blueprint-workspace.md#source-of-truth-and-git), [project ownership](../docs/unreal-project-rule.md). |
| Which Blueprint state can cross the boundary? | [Schema 2](blueprint-text.md#schema-2-blueprints), including typed defaults, components, object identity and explicit unsupported state. Schema 1 is a narrower example format. |
| Who owns conflict checks, batching, backups and recovery? | [Workspace coordination and Apply](blueprint-workspace.md#workspace-coordination). Lower-level ImportBatch is not a filesystem transaction. |
| Where do native classes and other dependencies come from? | [Editor support](unreal-editor-support.md) and the consuming project. Preparation does not reconstruct stock gameplay or arbitrary assets. |
| How should a graph be decomposed? | [Graph design](../docs/blueprint-graph-design-rule.md); [Collapse](blueprint-workspace.md#extract-an-operation-into-a-collapsed-graph) performs the supported mechanical extraction. |
| What can an agent inspect without Unreal? | [Hierarchy and selected-graph views](blueprint-text.md#collapsed-graphs), the workspace manifest and relevant editable source. |
| What does the user see, and what does it establish? | [Workspace Preview](blueprint-workspace.md#hand-off-the-preview), [graph review](blueprint-text.md#review-mode), [widget preview](blueprint-text.md#hud-widget-preview). Presentation is not execution. |
| How are reusable UObject sources distributed? | [AddSource](blueprint-workspace.md#add-a-reusable-uobject-source) and the [INI library](BlueprintText/Library/IniFileParser/AGENTS.md). Each consumer owns its imported copy. |

Keep three comparisons distinct: the current saved package against the accepted
workspace baseline detects conflicts; desired JSON against a fresh-process export
checks represented state; the original review snapshot against the current proposal
supplies the preview diff. Successful Apply advances the accepted baseline without
erasing the review baseline. None of these comparisons establishes gameplay
equivalence or preservation outside the supported envelope.

## Choose validation by the change

Run focused existing checks first. Broaden to the affected consumers when shared
comparison, representation or transaction behavior changes. Offline checks use
synthetic fixtures and mocked engine work; they do not replace engine acceptance.
Run Python checks with `python -B <test path>` and select an installed interpreter
explicitly if `python` resolves to a Windows Store alias. Temporary output belongs
under `Tmp/BlueprintText/`; a temporary-directory access failure is an environment
failure to resolve before interpreting the suite's result.

| Changed boundary | Relevant checks |
| --- | --- |
| Compact/native encoding, defaults, identity | [Codec](BlueprintText/test_asset_text_codec.py), [object relocation](BlueprintText/test_object_relocation.py), then [schema-2 round-trip acceptance](blueprint-text.md#checks) for engine-facing representation changes. |
| Workspace transactions, conflict detection, recovery, preparation | [Offline workspace checks](BlueprintText/test_blueprint_workspace.py), then an isolated public Export/edit/Apply/reopen cycle for changes affecting actual engine work. Include the affected failure/recovery path. |
| Hierarchy or extraction | [Nested-graph checks](BlueprintText/test_nested_graphs.py), [Collapse checks](BlueprintText/test_collapse_graphs.py); [engine runners](blueprint-text.md#collapsed-graphs) cover reconstruction and bounded executable fixtures. |
| Component-template inherited values and overrides | [Test-ComponentDefaults.ps1](BlueprintText/Test-ComponentDefaults.ps1), described below. A comparison using the same exporter alone can miss an incorrectly reconstructed inherited value. |
| Structural review or field display | [Review comparison](BlueprintText/test_graph_review.py), [Details](BlueprintText/test_review_details.py), [review rendering](BlueprintText/test_review_graph.py) and [batch preview](BlueprintText/test_preview_index.py). Check both snapshots and source isolation. |
| Graph presentation, navigation or interactions | [Renderer and JavaScript checks](blueprint-text.md#checks), [expressions](BlueprintText/test_graph_expressions.py), [function navigation](BlueprintText/test_preview_graph_links.py), [node search](BlueprintText/test_node_search.js). DOM stand-ins are not browser visual acceptance. |
| Widget input, resources or interaction | [Widget checks](blueprint-text.md#checks); resource collection changes also need an isolated Widget Blueprint export/Preview with its real dependencies. |
| Reusable Object behavior or INI parsing | [Object acceptance](blueprint-text.md#schema-2-blueprints) or the [INI integration suite](BlueprintText/Library/IniFileParser/AGENTS.md#validation-and-maintenance), followed by consumer-specific game checks when runtime behavior changes. |

For inherited child-actor defaults, run:

```powershell
./tools/BlueprintText/Test-ComponentDefaults.ps1 `
  -OutputRoot './Tmp/BlueprintText/test-component-defaults' `
  -EngineRoot '<selected engine root>'
```

The output directory must be new. The runner prepares an isolated project,
repeats changed Apply operations, checks inherited and overridden values in a
fresh engine process, and invokes a final Apply without further source edits. It returns a compact
result and retains evidence in that directory. On failure, inspect the reported
step and diagnostic; do not use a mod project as its fixture. Like all engine
checks, warn before launch and use its built-in shared queue. Specify
`-EngineRoot` consistently when wrappers' defaults differ from the selected engine.

For a presentation-only change, offline fixtures and applicable viewer checks
are sufficient unless the claim depends on real browser layout or interaction;
then inspect that concrete behavior. Do not build Unreal or request game tests
for a change confined to offline review. Conversely, successful rendering cannot
accept a serializer change. Report exactly which boundary was checked and keep
per-run measurements and results in temporary evidence, not this guide.

## Finish the shared change

Update the existing owning guide and [catalog](tool-catalog.md) if the public
interface changes. Preserve old inputs that remain supported, or describe an
explicit migration and its affected consumers. Keep source, previews and
runtime state distinct, including when a test expectation becomes obsolete.
Use [documentation ownership](../docs/repository-documentation-rule.md) for
durable conclusions and the normal [Git workflow](repository-git-coordination.md)
only after explicit commit authorization.
