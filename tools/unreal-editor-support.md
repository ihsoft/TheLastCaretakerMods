# Ready native support for Unreal projects

`Prepare-UnrealEditorSupport.ps1` builds or reuses a native plugin, attaches its
editor binaries to an existing project, and verifies module loading in a fresh
editor process. It owns the shared Unreal queue, copying, conflict checks and
rollback. Call the public script once; do not manually build modules, select DLLs,
inspect manifests, or run a separate engine preflight first.

This mechanism is independent of BlueprintText and game-specific patching.
The owner of a mod or tool supplies the plugin's source, reflected types and
stub implementations. This tool supplies no game classes and does not infer
schemas, edit assets, generate Blueprint graphs, cook a release or install a mod
into the game. BPT workspaces and the VoyageItem updater prepare declared project
dependencies through this mechanism; other producers must call it explicitly.

## Prepare declared project dependencies

Place `EditorSupport.json` beside the `.uproject`. Each entry names an
owner-maintained source plugin and optional additional source trees:

```json
{
  "schema": 1,
  "plugins": [{
    "source": "EditorSupport/ExampleSupport.uplugin",
    "overlays": [{
      "source": "../../../tools/SharedSupport/Profile",
      "destination": "Source/Game"
    }]
  }]
}
```

The plugin source path is relative to the project. Overlay source paths are
relative to that plugin descriptor; destinations must be below its `Source/`.
They add files to a private build input, never overwrite an existing destination
or write copies back into the owner sources. Duplicate plugin/module identities
and file ownership collisions fail. This permits shared and mod-owned definitions
to compile into one required `/Script/<Module>` without moving their ownership.

```powershell
./tools/Prepare-UnrealEditorSupport.ps1 -ProjectFile './mods/Example/Example.uproject'
```

This mode reads the declaration, builds missing/changed providers, installs them
and checks loading. Identical source bytes and editor `BuildId` reuse the installed
package without compilation, with a fresh load check. Exact input snapshots live
under the project's ignored `Saved/UnrealEditorSupport/inputs/`; removing that
cache causes a rebuild. An absent declaration means no declared dependencies.
The result contains a `providers` list with each provider's status and `built` flag.

Transactions apply to each plugin. A successfully prepared dependency can remain
installed if a later provider fails; rerunning verifies and reuses it. No asset
refresh is performed by dependency preparation. Project declarations, plugin
descriptors and owned sources belong in Git; installed DLL packages and input
caches do not.

## Build and attach

Provide an existing project and a buildable, owner-maintained source plugin:

```powershell
./tools/Prepare-UnrealEditorSupport.ps1 `
  -ProjectFile './mods/Example/Example.uproject' `
  -PluginSource './mods/Example/EditorSupport/ExampleSupport.uplugin'
```

The tool uses Unreal's `BuildPlugin -NoTargetPlatforms` in a shallow private build
directory below `Tmp/UnrealEditorSupport/`, independent of the evidence path's
depth. This keeps compiler paths short enough for the covered providers under
UBT's action-path length limit. The returned `package` contains the plugin
descriptor, Win64 editor DLLs, `UnrealEditor.modules`, and `editor-support.json`. It contains no C++ source,
build rules, targets, PDBs, import libraries or game assets. The private UAT build
tree is removed after successful packaging; build and load diagnostics remain.

Project preparation installs the package under `Plugins/<PluginName>/` and adds
its enabled reference to the `.uproject` when absent. It does not add project
modules or a `Source` directory. The owning project should ignore this installed
plugin directory in its own `.gitignore`; retain and review the project descriptor
and the owner-maintained plugin sources. Tool preparation does not change Git rules.

Source mode deliberately builds the current supplied sources. After an intentional
source change, use source mode again and use its new returned package. Reuse the
package for routine project preparation:

```powershell
./tools/Prepare-UnrealEditorSupport.ps1 `
  -ProjectFile './mods/Example/Example.uproject' `
  -PluginPackage '<package path returned by a successful source-mode call>'
```

Package mode does not invoke a compiler. An identical installation returns
`unchanged` after loading the modules in a fresh editor process. Replacing a
managed installation uses complete replacement at the same plugin identity.

## Inputs, results and boundaries

- `-EngineRoot` selects the engine, defaulting to `P:/UnrealEngine/UE_5.8`.
  The tested configuration is the local Win64 UE 5.8 editor. Binary packages
  must match the selected editor's own module `BuildId`. A mismatch requires
  rebuilding from source; version labels alone are not accepted as compatibility.
- `-OutputRoot` optionally selects a fresh directory below repository `Tmp/`.
  Otherwise the tool creates `Tmp/UnrealEditorSupport/prepare-<run>/`. Mod callers
  choosing output paths use `Tmp/<Mod>/`. Keep a returned package available while
  it has consumers; retained storage follows the normal artifact lifecycle rule.
- The supported input is a native-only plugin with `Runtime` and/or `Editor`
  modules and `Default` loading. Content plugins, external project plugin
  directories and hot-reload module binaries are outside this interface.
  Required engine plugin dependencies belong in the supplied `.uplugin`.
- Keep game-facing reflected classes in a `Runtime` module even when the
  support binaries are used only in the editor. Classifying those definitions
  as editor-only can make their functions illegal in gameplay Blueprints.
  Commandlets and editing utilities can remain in `Editor` modules. Plugin
  sources must satisfy Unreal's plugin compilation rules, including explicit
  categories for exposed properties/functions. Plugin modules use ordinary
  module registration, not primary-game registration.
  When moving project definitions into a plugin, preserve their former default
  categories explicitly; a new blanket category can change exported graph metadata.
- Existing project/plugin modules with colliding names are rejected. Moving
  classes out of a project module is an explicit owner migration, not something
  this tool silently performs. An explicitly disabled plugin remains disabled.
- The tool refuses unmanaged plugin directories, unexpected files in a managed
  package, linked paths, missing binaries and mismatched module inventories.
  Existing unrelated plugins and project settings are preserved.

The compact result reports `status` (`prepared`, `unchanged`, or `failed`),
`project`, `plugin`, `package`, `built`, `engineBuildId`, `loadedModules`, and the
`evidence` directory. Successful source/package calls also save `result.json`.
Failure returns a nonzero exit and a diagnostic location. A failed compiler run
retains `build-scratch.json` pointing to its private scratch. Do not treat an
existing package or an old successful result as success for the failed call.

The load check temporarily enables the engine's PythonScriptPlugin through the
command line. It loads each declared native module and checks that its DLL is
present in the editor process. It adds no Python dependency to the project
descriptor. This verifies binary loading, not asset semantics or gameplay;
consumer-specific export/cook/semantic and game tests still belong to consumers.

## Recovery

On installation or editor-load failure, the tool restores the previous project
descriptor and managed plugin. A recovery journal remains if the process was
interrupted or restoration could not finish. Later preparation refuses to run
over that pending transaction. Restore it through the same public entry point:

```powershell
./tools/Prepare-UnrealEditorSupport.ps1 `
  -ProjectFile './mods/Example/Example.uproject' -Recover
```

Preserve `Saved/UnrealEditorSupport/` until recovery succeeds. Do not manually
delete the journal or its transaction backup. Recovery does not rebuild or
reapply the requested replacement. Ordinary successful calls remove their
transaction staging and backups.

## Validation

Offline transaction tests use synthetic binary bytes, not game data:

```powershell
python -B -m unittest discover -s tools/UnrealEditorSupport/tests -p "test_*.py"
```

The engine test uses the checked-in, game-independent `SupportFixture` plugin.
It exercises the public build/install path, repeat package reuse, and a project
without C++ or build sources. A native DataAsset is saved with a changed value,
reopened in a fresh process under the same `/Script/SupportFixture` class identity,
and cooked. All output stays in a fresh ignored directory; no game installation
or maintained mod project is changed.

```powershell
./tools/UnrealEditorSupport/tests/Test-EditorSupport.ps1
```

The test accepts `-EngineRoot`, `-OutputRoot`, and an optional `-PluginPackage`
to reuse a previously built fixture package. It owns queue reservations for its
engine operations. Successful results do not establish support for arbitrary
third-party plugins, other engine builds or any particular game's class layout.
