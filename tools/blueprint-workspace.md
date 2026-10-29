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
Blueprint must be created in the editor before that first export. Subsequent
Blueprints can be added directly through JSON.

Use an existing Unreal project with its required resources and native modules.
Warn the user before engine actions; the shared queue waits for Unreal to be free. `Prepare`
provides/builds the shared editor tooling; it does not run a mod generator.
It installs the shared editor plugin when absent, or refreshes an existing
`BlueprintText` module from the current shared template. Before replacing
different module files it preserves their complete previous contents in the
returned `moduleBackup`; other project modules are not replaced.
Keep workspaces under `Tmp/BlueprintText/`.
Existing workspaces and installed shared modules using the previous names remain
supported by the current entry points. Run Prepare to refresh their module; do
not rename generated projects or move existing preview output just to match the
new tool directory. New projects and workspaces use the current names.
The current implementation selects ordinary uncooked Actor/Widget Blueprint
packages under `/Game/`; split packages are rejected. Use `-EngineRoot` when the
project uses an engine outside the wrapper's default installation.

```powershell
$tool = './tools/BlueprintText/Invoke-BlueprintWorkspace.ps1'
$project = 'R:/path/to/Project.uproject'
$work = './Tmp/BlueprintText/edit-example'
& $tool -Action Prepare -ProjectFile $project -WorkspaceRoot $work
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
| Adjacent `*.resources/` bundles | Tool-owned widget preview manifests and images, when exported. |
| `.workspace/` | Tool-owned baselines, backups, diagnostics and generated previews. |

The original export is the review baseline; subsequent Apply operations update
the accepted project baseline used for conflict detection. To start a new review
baseline, export into a new workspace.

## Edit and apply

Read `workspace.json` for the selected Blueprint/source mapping, then edit the
relevant files under `blueprints/`. They describe complete proposed Blueprint
state to apply to the project. Leave technical files to the tool. To add a Blueprint, add its compact
JSON and a corresponding source entry to the manifest. Deletion of whole assets
is not supported; removing a baseline entry or losing a source file must not be
used as a delete operation.

Use a separate workspace for each independent task. All commands hold an OS lock
on their workspace. Prepare, Export, Apply and Recover also lock the project and
join the [shared Unreal queue](unreal-coordination.md) for their entire operation,
including rollback. Engine work waits its turn even for different projects.
Project and older engine gates also wait for their owners to finish; never delete
their lock files. A conflicting command in the same workspace still fails to
protect task ownership. The OS releases locks when their owning process exits.
Offline preview and direct JSON editing in separate workspaces can proceed independently of engine work.
Direct file edits are outside these locks; follow the task ownership rules above.
Other Unreal producers must use the shared queue wrapper. Asset conflict checks still apply when sequential
tasks touch the same exported asset.

```powershell
& $tool -Action Apply -WorkspaceRoot $work
& $tool -Action Preview -WorkspaceRoot $work
```

Apply checks for conflicting project edits, backs up selected packages, rebuilds
and compiles them, then verifies a fresh-process export against the desired JSON.
Failure restores selected original packages; unrelated project resources stay
outside the selected write set. The command reports a no-op if nothing changed.
Inheritance ordering and compatible batches are tool-owned. Unsupported
dependency arrangements produce a diagnostic rather than requiring an agent to
invent a per-mod build procedure.

On a conflict, preserve both the JSON edits and the editor changes. Export the
current project state into a separate workspace and reconcile the intended
change; do not remove conflict checks or overwrite the newer asset manually.
After an interrupted operation, use the tool's `Recover` action and its recovery
diagnostic before continuing. Recovery restores the state before that operation;
it first retains current selected files in `recoveredCandidates`, including any
possible manual edits made after interruption. Reconcile those separately if
needed. Do not move backup files by hand.

```powershell
& $tool -Action Recover -WorkspaceRoot $work
```

## Hand off the preview

Preview is offline. It generates the [common browse/review index](blueprint-text.md#browse-a-set-of-blueprints)
and returns `previewUrl`. Send that link to the user without opening or parsing
the generated page. Blueprint status is compared with the original export, so
successful Apply operations do not erase the review. Conflict detection uses the
most recently accepted project state separately. Optional
`reviews` in a manifest asset entry supplies known node IDs by graph, using
`added`, `changed` and `removed` lists. `placeholder` and `note` describe editor
dependencies and their role. These annotations do not alter the Blueprint.

Read diagnostics only on failure: start with the returned short reason and
diagnostic path, then inspect the relevant evidence. A successful render does not
require screenshot inspection, link crawling or a second comparison by the agent.

## Tool maintenance checks

The offline transaction and concurrency tests do not launch Unreal:

```powershell
python -B tools/BlueprintText/test_blueprint_workspace.py
```

Engine acceptance requires a disposable project with the relevant dependencies.
Compile/save and fresh-export comparison verify represented editor state, not
cooked output or gameplay behavior.
