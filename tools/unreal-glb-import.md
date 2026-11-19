# Import a complete rigid GLB model

Use `Import-UnrealGlb.ps1` when the user selects a complete GLB assembly for a
project edit. It imports meshes, materials, textures and the named node hierarchy
into a fresh content directory, then creates an Actor Blueprint `BP_Model`.
This is an explicit authoring operation, never a build prerequisite.

## Call

Create a working project with the owning mod's `New-<Mod>ProjectCopy.ps1`, then
run from the repository root through the [PowerShell launcher](powershell-execution.md):

```powershell
python tools/run_powershell.py tools/Import-UnrealGlb.ps1 `
  -ProjectFile './Tmp/Example/model-edit/Project/Voyage.uproject' `
  -Source './Tmp/Example/received-model.glb' `
  -Destination '/Game/Mods/Example/ModelImport' `
  -OutputRoot './Tmp/Example/model-edit/import'
```

`ProjectFile` must be an existing working project below repository `Tmp/`.
`Destination` is a new `/Game/` directory; existing content is never overwritten.
`OutputRoot` must be fresh, below `Tmp/`, and separate from the project.
`EngineRoot` selects the Unreal installation. The command owns the shared queue,
declared project dependencies, its editor support and BPT export. Start it with
the permissions required by the [tool execution rules](toolchain-maintenance.md).
No separate native build or dependency preflight is needed.

The success result returns `blueprint`, `reportPath`, `blueprintWorkspace` and
`outputRoot`. Detailed diagnostics and import/reopen reports remain in OutputRoot;
the BPT workspace is below `Tmp/BlueprintText/`. A failed operation retains
diagnostics and may leave partial content in the working copy. Correct the cause
and use a fresh destination/output or recreate the disposable copy. No source
rollback or automatic replacement workflow is involved.

## Imported model contract

- Each active named GLB node becomes a movable SceneComponent or
  StaticMeshComponent. Empty grouping/pivot nodes remain present. Attachment,
  local and world transforms are compared with the selected source scene,
  including metres-to-centimetres and Y-up-to-Z-up conversion.
- Repeated instances of one source mesh share a saved StaticMesh. Components
  retain their own placement and material slots. The source GLB is not rewritten.
- Components carry `GLB.Name:<source name>` tags; the report maps those names to
  component variable names. Use this identity when wiring roles rather than
  assuming component array order or an importer-generated variable name.
- Unreal Interchange owns GLTF material translation. Used Interchange material
  parents and their dependencies are copied under `MaterialSupport/` with native
  reference remapping. Runtime asset references must stay inside the selected
  destination or use `/Engine/` and `/Script/`; they cannot depend on the
  editor's `/InterchangeAssets/` library. Original plugin assets remain unchanged.
- `BP_Model` has no authored gameplay graphs. Mesh component collision is disabled;
  collision, sockets, animation drivers and gameplay bindings are consumer-owned.
  Import does not guess that a node named `YAW` should respond to player input.

The operation saves the result, reopens it in another editor process and checks
the hierarchy, transforms, shared mesh identity, materials and dependency
boundary again. It exports temporary BlueprintText for review. Checks are
against the current GLB; no production vertex/triangle count is frozen in a test.
This proves the saved assembly contract, not shader appearance or game behavior.

## Review and acceptance

Open `BP_Model` in the working Unreal project. Inspect materials, scale and
orientation; rotate intended joints and check the moving subtree and pivot.
The [GLB preview](glb-tools.md) can show the supplied file for comparison, but
does not preview the saved Unreal assets.

For a new mod, add behavior through the [BPT workflow](blueprint-workspace.md)
or Unreal Editor. For an existing mod, integrate the reviewed component hierarchy
and resources with its maintained actor and explicit role bindings. A new
graph-free `BP_Model` is not a replacement for an actor that already owns gameplay.
Do not flatten moving groups or bind behavior merely by decorative names.

Accept only the reviewed source assets through
the [project authoring workflow](../docs/unreal-project-rule.md#project-layout-and-temporary-data).
Keep the selected virtual asset paths or rename through Unreal and refresh BPT;
moving `.uasset` files alone does not rewrite references. Include referenced
`MaterialSupport/` assets in the release inventory. Interchange scene-import
metadata is editor-only and does not belong in the runtime inventory. The
temporary `UnrealGlbImportSupport` plugin contains no runtime code or content and
does not belong in maintained project dependencies or a release.
Subsequent builds consume the accepted project assets without importing GLB again.

## Supported scope

The input is a self-contained GLB 2 rigid assembly with a selected scene and
explicit active node names unique without regard to case. The shared
[GLB reader](glb-tools.md) defines the supported geometry/accessor subset.
Skins, morph targets, animation clips, cameras, punctual lights, zero scales and
sheared transforms (including shear accumulated through parents) are rejected.
Unsupported import results fail validation instead of silently flattening them.
Use a dedicated authoring operation for skeletal or animated assets; a full
rigid assembly does not imply support for every GLTF extension.

## Tool maintenance checks

```powershell
python -B tools/UnrealGlbImport/test_import.py
```

These offline cases cover destination protection, coordinate conversion and
unsupported/ambiguous scenes. The conversion has an independent expected result:
an incorrect basis could otherwise be shared by import and readback expectations.
Saved hierarchy, mesh instances, materials and reopening are validated by the
import operation itself. Use a focused editor review when accepting a change
that affects those contracts; do not keep a separate test of Unreal's standard
parent/child movement. Run the request tests when changing the importer, not on
each consuming mod build.
