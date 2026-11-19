# Maintaining BlueprintText

Use this guide for BPT tool development and specialist review, under the
[ownership and decision rule](../docs/blueprint-text-maintenance-rule.md).
Ordinary mod work starts with the [workspace workflow](blueprint-workspace.md).
This guide selects existing contracts and checks; it does not add preflights
to successful authoring cycles.

## Find the owning boundary

| Question | Owning contract |
| --- | --- |
| Which project state is authoritative, and what belongs in Git? | [Workspace source and Git review](blueprint-workspace.md#source-of-truth-and-git), [project ownership](../docs/unreal-project-rule.md). |
| Which Blueprint state can cross the boundary? | [Schema 2](blueprint-text.md#schema-2-blueprints), including typed defaults, components, object identity and explicit unsupported state. Schema 1 is a narrower example format. |
| Who owns conflict checks, batching, backups and recovery? | [Workspace coordination and Apply](blueprint-workspace.md#workspace-coordination). Lower-level ImportBatch is not a filesystem transaction. |
| Where do native classes and other dependencies come from? | [Editor support](unreal-editor-support.md) and the consuming project. Preparation does not reconstruct stock gameplay or arbitrary assets. |
| How should a graph be decomposed? | [Graph design](../docs/blueprint-graph-design-rule.md); [Collapse](blueprint-workspace.md#extract-an-operation-into-a-collapsed-graph) performs the supported mechanical extraction. |
| What can an agent inspect without Unreal? | [Compact graph reading and exact details](blueprint-text.md#read-graphs-without-unreal), the workspace manifest and relevant editable source. |
| What does the user see, and what does it establish? | [Workspace Preview](blueprint-workspace.md#hand-off-the-preview), [graph review](blueprint-text.md#review-mode), [widget preview](blueprint-text.md#hud-widget-preview). Presentation is not execution. |
| How are reusable UObject sources distributed? | [CopyAsset](blueprint-workspace.md#copy-a-native-library-asset) and the [INI library](BlueprintText/Library/IniFileParser/AGENTS.md). Each consumer owns its imported copy. |

Keep three comparisons distinct: the current saved package against the accepted
workspace baseline detects conflicts; desired JSON against a fresh-process export
checks represented state; the original review snapshot against the current proposal
supplies the preview diff. Successful Apply advances the accepted baseline without
erasing the review baseline. None of these comparisons establishes gameplay
equivalence or preservation outside the supported envelope.

## Choose validation by the change

Use the [test scope rule](../docs/testing-rule.md) to choose retained coverage.
The table below routes affected changes to existing checks; it is not an
unconditional full-suite checklist. Run focused existing checks first.
Broaden to the affected consumers when shared
comparison, representation or transaction behavior changes. Offline checks use
synthetic fixtures and mocked engine work; they do not replace engine acceptance.
Run Python checks with `python -B <test path>` and select an installed interpreter
explicitly if `python` resolves to a Windows Store alias. Temporary output belongs
under `Tmp/BlueprintText/`; a temporary-directory access failure is an environment
failure to resolve before interpreting the suite's result.

| Changed boundary | Relevant checks |
| --- | --- |
| Compact/native encoding, defaults, identity | [Codec](BlueprintText/test_asset_text_codec.py), [object relocation](BlueprintText/test_object_relocation.py), then [schema-2 round-trip acceptance](blueprint-text.md#checks) for engine-facing representation changes. |
| Workspace transactions, conflict detection, recovery, preparation | [Offline workspace checks](BlueprintText/test_blueprint_workspace.py), then an isolated public Export/edit/Apply/reopen cycle for changes affecting actual engine work. Include the affected failure/recovery path. [Clean-checkout acceptance](BlueprintText/Test-CleanProjectSupport.ps1) covers legacy mandatory BPT references with absent plugin directories, declared native support, saved custom call nodes, shared reuse across copies, overlay invalidation, public source-mode reuse, changed Apply and cook. |
| Authoring dependencies across a fresh mod build | [Shared cook acceptance](mod-setup.md#shared-cook-maintenance-checks) checks compiled BPT calls and arguments after source-only copying and shared cook, plus missing-provider rejection. Run for changes to this integration, not for each mod release. |
| Ready-provider packaging and C++ project compatibility | [Support tests](UnrealEditorSupport/tests/test_editor_support.py), [provider reuse tests](UnrealEditorSupport/tests/test_project_support.py), then [C++ consumer acceptance](BlueprintText/Test-CppProjectSupport.ps1) and the [project without C++ test](UnrealEditorSupport/tests/Test-EditorSupport.ps1). |
| Hierarchy or extraction | [Nested-graph checks](BlueprintText/test_nested_graphs.py), [Collapse checks](BlueprintText/test_collapse_graphs.py); [engine runners](blueprint-text.md#collapsed-graphs) cover reconstruction and bounded executable fixtures. |
| Source reading, compact fields or scoped details | [Inspection checks](BlueprintText/test_graph_inspection.py): semantic fields, defaults, complete node/link coverage, pin order, exact drilldown, CLI compatibility and source isolation. |
| Member resolution, use search or declaration context | [Reference/context checks](BlueprintText/test_graph_references.py) and [preview navigation](BlueprintText/test_preview_graph_links.py): owner/scope/GUID identity, indirect-call classification, unresolved targets and source isolation. |
| Reflected native call templates | [Offline request/factory checks](BlueprintText/test_call_templates.py), then [Test-CallTemplates.ps1](BlueprintText/Test-CallTemplates.ps1): actual engine metadata, strict Apply/reopen, false overrides, pure/exec pins, required reference inputs and rejection recovery. The class setter fixture executes the imported graph on isolated objects, checking compatible writes, null clearing, missing/incompatible no-ops and instance isolation; unrelated internal setters remain rejected. |
| Transient call arguments, native default semantics and legacy migration | [Argument checks](BlueprintText/test_call_arguments.py), codec/workspace/review checks, then [Test-Arguments.ps1](BlueprintText/Test-Arguments.ps1): changed Apply, fresh export, independent execution, changed callee default, explicit equal/zero/false/empty, local/native/peer calls, direct legacy migration, physical BPT removal, ordinary editor compile/save/reopen, reconstruction and cook without editor node state. |
| Pure static Blueprint Function Libraries | [Offline library checks](BlueprintText/test_function_library.py), then [Test-FunctionLibrary.ps1](BlueprintText/Test-FunctionLibrary.ps1): external class hierarchy, two Actor consumers, reflected insertion, joint function/input rename, optional defaults, AddSource, fresh comparison, Preview and cooked editor-state stripping. |
| Component-template inherited values and overrides | [Test-ComponentDefaults.ps1](BlueprintText/Test-ComponentDefaults.ps1), described below. A comparison using the same exporter alone can miss an incorrectly reconstructed inherited value. |
| Native CDO subobjects, inherited archetypes and legacy-source rejection | [Offline native-default checks](BlueprintText/test_native_defaults.py), then [Test-NativeDefaults.ps1](BlueprintText/Test-NativeDefaults.ps1): changed Apply/reopen, external asset and nested-object values, constructor defaults, parent/child overrides, zero/null resets, rejection before package writes and independent cooked-package readback. |
| Structural review or field display | [Review comparison](BlueprintText/test_graph_review.py), [Details](BlueprintText/test_review_details.py), [review rendering](BlueprintText/test_review_graph.py) and [batch preview](BlueprintText/test_preview_index.py). Check both snapshots and source isolation. |
| Graph presentation, navigation or interactions | [Renderer and JavaScript checks](blueprint-text.md#checks), [expressions](BlueprintText/test_graph_expressions.py), [function navigation](BlueprintText/test_preview_graph_links.py), [node search](BlueprintText/test_node_search.js). DOM stand-ins are not browser visual acceptance. |
| Widget input, resources or interaction | [Widget checks](blueprint-text.md#checks); resource collection changes also need an isolated Widget Blueprint export/Preview with its real dependencies. |
| Project model export or presentation | [Model checks](BlueprintText/test_model_preview.py): reflections, non-uniform scale, normal direction, project/source isolation and explicit failures. Export changes also need public `ModelPreview` calls on an Actor assembly and an individual Static Mesh in a disposable project, plus rejection of an unsupported asset. Compare saved Content before/after; rendering is not gameplay or material-fidelity proof. |
| Existing DataAsset property edits | [DataAsset checks](BlueprintText/test_data_asset_workspace.py) and workspace checks: untouched field serialization, owned identity/custom state, fresh reopen, late conflicts, rollback and review baselines. Native changes also need public ExportDataAsset/Apply on a disposable consumer, relevant scalar/structure/collection and owned-object edits, rejected nested references, and independent cooked comparison of all affected exports including untouched text histories/flags. Use the consumer validator where its declared change contract applies; otherwise construct the expected semantic changes explicitly. Do not make this a routine build preflight. |
| DataAsset comparison and display | DataAsset checks cover reference selection independent of Apply, read-only differences, incompatible snapshots and absent baselines. Check both Changes and Full asset against a real exported pair; the baseline label, before/after values and full-state highlights must agree. A stock label requires independently obtained stock data, not a reversed patch. |
| Native library copying and revision review | [Copy isolation/rollback](BlueprintText/test_native_library.py), [Git snapshot checks](BlueprintText/test_git_review.py), then actual native copies and revision exports. Parser integration exercises independent consumer paths and behavior after duplication. |
| Reusable Object behavior or INI parsing | [Object acceptance](blueprint-text.md#schema-2-blueprints) or the [INI integration suite](BlueprintText/Library/IniFileParser/integration.md#validation-and-maintenance), followed by consumer-specific game checks when runtime behavior changes. |

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

For native CDO default-subobject preservation, run:

```powershell
./tools/BlueprintText/Test-NativeDefaults.ps1 `
  -OutputRoot './Tmp/BlueprintText/test-native-defaults' `
  -EngineRoot '<selected engine root>'
```

Use a fresh output directory. The runner owns the shared Unreal queue, builds its
isolated native fixture, checks reflected values in fresh editor processes and
reads the cooked packages through the canonical UAssetGUI binary. It requires
that binary at `.tools/bin/UAssetGUI.exe`; no game installation is involved.
The compact result covers this fixture's saved-state boundary, not arbitrary
native classes or runtime gameplay. Preserve the diagnostic path on failure.

## Finish the shared change

Update the existing owning guide and [catalog](tool-catalog.md) if the public
interface changes. Preserve old inputs that remain supported, or describe an
explicit migration and its affected consumers. Keep source, previews and
runtime state distinct, including when a test expectation becomes obsolete.
Use [documentation ownership](../docs/repository-documentation-rule.md) for
durable conclusions and the normal [Git workflow](repository-git-coordination.md)
only after explicit commit authorization.
