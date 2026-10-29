# BlueprintText workspace workflow

Use [Invoke-BlueprintWorkspace.ps1](BlueprintText/Invoke-BlueprintWorkspace.ps1)
for the recommended Blueprint authoring workflow for new and existing mods:
export, edit JSON, apply and preview. The Unreal project remains the
home of resources, native classes, dependency placeholders and rebuilt assets.
This workflow does not generate stubs, cook, install or run the game. Supported
Blueprint state is defined by the [format guide](blueprint-text.md#schema-2-blueprints).

Follow the [routine agent rules](BlueprintText/AGENTS.md#routine-agent-work).
Successful command results are the acceptance signal for mechanical work.
Generated HTML, images, native intermediates and full logs are not agent inputs.

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
Warn the user before engine actions and close Unreal Editor first. `Prepare`
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
| `workspace.json` | Agent-facing source selection and optional review annotations. |
| `blueprints/**/*.json` | Complete editable Blueprint descriptions. |
| Adjacent `*.resources/` bundles | Tool-owned widget preview manifests and images, when exported. |
| `.workspace/` | Tool-owned baselines, backups, diagnostics and generated previews. |

The original export is the review baseline; subsequent Apply operations update
the accepted project baseline used for conflict detection. To start a new review
baseline, export into a new workspace.

## Edit and apply

Read `workspace.json` for the selected Blueprint/source mapping, then edit the
relevant files under `blueprints/`. They describe complete desired Blueprint
state. Leave technical files to the tool. To add a Blueprint, add its compact
JSON and a corresponding source entry to the manifest. Deletion of whole assets
is not supported; removing a baseline entry or losing a source file must not be
used as a delete operation.

Use a separate workspace for each independent task. All commands hold an OS lock
on their workspace. Prepare, Export, Apply and Recover also lock the project and
share a repository-wide engine gate for their entire operation, including
rollback. This intentionally serializes engine work even for different projects:
the current running-editor check is machine-wide. A competing command fails
immediately before asset/module changes; retry after the owner finishes, without
deleting lock files. The OS releases locks when their owning process exits.
Preview and direct JSON editing in separate workspaces can proceed independently.
Direct file edits and other Unreal entry points are outside these locks; follow
the task ownership rules above. Asset conflict checks still apply when sequential
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
