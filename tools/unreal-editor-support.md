# Ready native support for Unreal projects

`Prepare-UnrealEditorSupport.ps1` builds or reuses a native plugin, attaches its
editor binaries to an existing project, and verifies module loading in a fresh
editor process. It owns the shared Unreal queue, copying, conflict checks and
rollback. Call the public script once; do not manually build modules, select DLLs,
inspect manifests, or run a separate engine preflight first.

For a maintained mod, pass a disposable working copy under `Tmp/<Mod>/` as
`-ProjectFile`. Prepare that copy from current source inputs using the owning
mod's copy/build/authoring entry point. The maintained `mods/<Mod>/Project/`
stays source-only; within it, only `Intermediate/` may be temporary and ignored.
Installed plugins, input caches, logs and generated settings belong to the
working copy, under the [project layout rule](../docs/unreal-project-rule.md#project-layout-and-temporary-data).
This tool prepares the supplied project; it does not create or synchronize a
working copy for the caller.

This mechanism is independent of BlueprintText and game-specific patching.
The owner of a mod or tool supplies the plugin's source, reflected types and
stub implementations. This tool supplies no game classes and does not infer
schemas, edit assets, generate Blueprint graphs, cook a release or install a mod
into the game. BPT workspaces and the VoyageItem updater prepare declared project
dependencies through this mechanism. The [shared mod cook](mod-setup.md#shared-mod-pipeline)
and [DataAsset reader](data-asset-conversion.md#native-readback) own their complete
preparation, including BPT; their callers do not repeat it. A standalone native
consumer owns preparation when its public operation does not already provide it.

Loading or cooking an asset through native mirrors does not establish that its
fields are editable in the standard Unreal Details panel. That requires editor
property exposure, including nested fields. The owner selects an authoring path
for those asset types; native preparation does not add arbitrary DataAsset
editing to BPT.

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
./tools/Prepare-UnrealEditorSupport.ps1 -ProjectFile './Tmp/Example/authoring/Project/Example.uproject'
```

This mode reads the declaration, builds missing/changed providers, installs them
and checks loading. It first reuses an installation matching current inputs and
package bytes, then looks in the shared provider cache, and builds only on a miss.
Every path checks module loading in a fresh editor process. Local input records
live under the working copy's `Saved/UnrealEditorSupport/inputs/`; a new copy
without those records can still reuse the shared cache. An absent declaration
means no declared dependencies. The result contains a `providers` list with each
provider's status, `built` flag and `reuse` origin.

Transactions apply to each plugin. A successfully prepared dependency can remain
installed if a later provider fails; rerunning verifies and reuses it. No asset
refresh is performed by dependency preparation. Project declarations, plugin
descriptors and owned sources belong in Git; installed DLL packages and input
caches do not.

## Build and attach

Provide an existing project and a buildable, owner-maintained source plugin:

```powershell
./tools/Prepare-UnrealEditorSupport.ps1 `
  -ProjectFile './Tmp/Example/authoring/Project/Example.uproject' `
  -PluginSource './Tmp/Example/authoring/Project/EditorSupport/ExampleSupport.uplugin'
```

The tool uses Unreal's `BuildPlugin -NoTargetPlatforms` in a shallow private build
directory below `Tmp/UnrealEditorSupport/`, independent of the evidence path's
depth. This keeps compiler paths short enough for the covered providers under
UBT's action-path length limit. The returned `package` contains the plugin
descriptor, Win64 editor DLLs, `UnrealEditor.modules`, `editor-support.json`, and
generated `Source/<Module>/<Module>.Build.cs` rules for precompiled modules.
These rules let an ordinary Win64 Development Editor C++ project build reuse the
DLLs without rebuilding them or disabling the plugin. The package contains no
C++ implementation, headers, project targets, PDBs, import libraries or game assets. The private UAT build
tree is removed after successful packaging; build and load diagnostics remain.

Project preparation installs the package under `Plugins/<PluginName>/` and adds
its enabled reference to the `.uproject` when absent. It does not add project
modules or a project-level `Source` directory. These generated files stay in the
working copy; do not add `Plugins/` or `Saved/` exclusions to the maintained
project. Retain and review its required plugin references and owner-maintained
plugin sources. For authoring, transfer only intended source changes after
checking for concurrent edits; never copy the whole working project back.

Source mode also checks current inputs and reuses a matching installed or shared
provider before compiling. Use the same call after source changes; the tool
selects the matching variant or builds it. For an explicit ready-package handoff,
use the returned `package` path rather than assuming a location below OutputRoot:

```powershell
./tools/Prepare-UnrealEditorSupport.ps1 `
  -ProjectFile './Tmp/Example/authoring/Project/Example.uproject' `
  -PluginPackage '<package path returned by a successful source-mode call>'
```

Package mode does not invoke a compiler. An identical installation returns
`unchanged` after loading the modules in a fresh editor process. Replacing a
managed installation uses complete replacement at the same plugin identity.
Current packages use manifest schema 2. Declared dependency preparation and BPT
preparation replace older packaging with a current cached or rebuilt package.
Legacy schema-1 packages
remain readable for loading and rollback, but do not support ordinary C++ builds;
rebuild them from owner sources rather than adding rules to an installed package.

## Shared provider cache

BPT preparation, declared project dependencies and `-PluginSource` automatically
share ready providers in the ignored repository-local
`artifacts/UnrealEditorSupport/providers/`. No cache path or key is required from
the caller. This store is outside maintained projects and disposable working
copies; it contains native packages and the input records needed for reuse,
not authored assets, logs or compiler scratch.

Reuse requires identical effective input paths and bytes after applying overlays,
including the plugin descriptor, the selected editor's `BuildId`, package format
and build recipe. Moving a project or its source tree does not itself invalidate
reuse. Changed inputs select another variant; an earlier variant can be reused
when those inputs return. Supply all non-engine compilation inputs in the source
tree or overlays. Undeclared external build inputs are outside this contract.
The supported build profile remains Win64 Development Editor.

Only a complete package that passed installation and loading is published.
Cache publication uses private staging under the run's `Tmp/` directory; failed
publication removes that staging. Each consumer gets its own managed installation
and load check. A damaged shared entry is discarded and rebuilt; a consumer's
load failure still fails the call and rolls back its installation. Cache reuse
does not suppress that failure or authorize replacing unmanaged project files.

After successful preparation, the tool retains at most 32 entries and 2 GiB of
cache files, evicting the least recently used entries first. A package exceeding
the byte limit is usable but is not retained. Eviction does not remove installed
plugins or run evidence. Cache entries are disposable: their absence causes a
build when no matching local installation remains. Use the returned `package`
path for a handoff; consumers do not depend on private cache paths surviving
later cleanup. Build intermediates and diagnostics stay under `Tmp/`.

## Inputs, results and boundaries

- `-EngineRoot` selects the engine, defaulting to `P:/UnrealEngine/UE_5.8`.
  The tested configuration is the local Win64 UE 5.8 editor. Binary packages
  must match the selected editor's own module `BuildId`. A mismatch requires
  rebuilding from source; version labels alone are not accepted as compatibility.
- Ready packages support modular Win64 Development Editor builds and editor
  loading/cooking. Their generated rules reject other target types, platforms
  and configurations. They provide reflected editor support, not a C++ SDK:
  consumer C++ code cannot include their omitted headers or link directly to
  their native API. Monolithic and game-binary builds are outside this package.
- `-OutputRoot` optionally selects a fresh directory below repository `Tmp/`.
  Otherwise the tool creates `Tmp/UnrealEditorSupport/prepare-<run>/`. Mod callers
  choosing output paths use `Tmp/<Mod>/`. Keep a returned package available while
  it has consumers; retained storage follows the normal artifact lifecycle rule.
- The supported input is a native-only plugin with `Runtime`, `Editor`, or `UncookedOnly`
  modules and `Default` loading. Content plugins, external project plugin
  directories and hot-reload module binaries are outside this interface.
  Required engine plugin dependencies belong in the supplied `.uplugin`.
- Keep game-facing reflected classes in a `Runtime` module even when the
  support binaries are used only in the editor. Classifying those definitions
  as editor-only can make their functions illegal in gameplay Blueprints.
  Commandlets and editing utilities can remain in `Editor` modules. Custom K2
  authoring nodes belong in `UncookedOnly`, so they can compile gameplay
  Blueprints and be stripped by cook. Ready rules still target Win64 Development
  Editor; this does not add game-target native binaries. Plugin
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
Source and declaration results additionally report `reuse`: `installed`,
`shared`, or `null` when a build was needed. `built: false` never skips loading.
Failure returns a nonzero exit and a diagnostic location. A failed compiler run
retains `build-scratch.json` pointing to its private scratch. Do not treat an
existing package or an old successful result as success for the failed call.

The load check temporarily enables PythonScriptPlugin and the requested support
plugin through the command line, after rejecting an explicitly disabled project
reference. This also permits preparation when an old build receipt disabled the
plugin. It loads each declared native module and checks that its DLL is
present in the editor process. It adds no Python dependency to the project
descriptor. This verifies binary loading, not asset semantics or gameplay;
consumer-specific export/cook/semantic and game tests still belong to consumers.

## Recovery

If an earlier C++ build used `-DisablePlugin=<PluginName>`, remove that workaround,
prepare the current package through the public tool, and run the ordinary project
build. UBT then replaces the stale disabled-plugin receipt. For BPT, use workspace
`Prepare` before that build, then resume the ordinary workspace actions. Do not
edit receipts or binary manifests by hand. The preparation load check alone does
not clear a stale receipt; normal editor/BPT launches need the subsequent build.

On installation or editor-load failure, the tool restores the previous project
descriptor and managed plugin. A recovery journal remains if the process was
interrupted or restoration could not finish. Later preparation refuses to run
over that pending transaction. Restore it through the same public entry point:

```powershell
./tools/Prepare-UnrealEditorSupport.ps1 `
  -ProjectFile './Tmp/Example/authoring/Project/Example.uproject' -Recover
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
without project C++ sources. The installed plugin carries only its generated
precompiled rules beside the binary package. A native DataAsset is saved with a changed value,
reopened in a fresh process under the same `/Script/SupportFixture` class identity,
and cooked. Run output stays in a fresh ignored directory; validated providers
may also populate the shared cache. No game installation or maintained mod
project is changed.

```powershell
./tools/UnrealEditorSupport/tests/Test-EditorSupport.ps1
```

The test accepts `-EngineRoot`, `-OutputRoot`, and an optional `-PluginPackage`
to reuse a previously built fixture package. It owns queue reservations for its
engine operations. Successful results do not establish support for arbitrary
third-party plugins, other engine builds or any particular game's class layout.

[Test-CppProjectSupport.ps1](BlueprintText/Test-CppProjectSupport.ps1) covers a
C++ consumer with both the native fixture and BPT ready providers: ordinary
project builds, a real native recompile, package reuse, recovery from a disabled
receipt, two changed strict Apply/reopen passes, independent component-value
checks and cook. Run it with a fresh `-OutputRoot` below `Tmp/BlueprintText/`
and the selected `-EngineRoot`; it owns the shared Unreal queue.

[Test-CleanProjectSupport.ps1](BlueprintText/Test-CleanProjectSupport.ps1) checks
two legacy project fixtures with mandatory BPT references, native support and
saved custom call nodes. The fresh copy must load both providers without compiling either;
changing a test overlay rebuilds only native support, and restoring it reuses
the retained variant. The runner also checks public source-mode reuse, asset
preservation, strict Apply/reopen and cook. Use a fresh `-OutputRoot` below
`Tmp/BlueprintText/` and the selected `-EngineRoot`; it owns the shared queue.
