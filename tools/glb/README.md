# GLB tools

GLB is the editable model source and model-to-build handoff. These tools inspect,
preview or convert files; they do not procedurally recreate the model. An owning
registry may bind stable node roles; its directory contains exactly one live GLB.
Conversions and renders are disposable derived
outputs. Never replace a GLB with a roundtripped copy merely to inspect it.

## Available and tested

Blender **5.0 or newer is required** for Blender-backed preview and conversion.
The current validated runtime is Blender 5.2.2 LTS. Plain structural inspection
uses Python 3.10+ and does not require launching Blender.

| Intent | Entry point | Requirements / result |
| --- | --- | --- |
| Inspect a GLB or owning registry | inspect_glb.py | Python 3.10+, NumPy; compact result, optional detailed JSON |
| Direct render of actual GLB | preview_glb.py | Blender built-in glTF importer; PNG, no source rewrite |
| Convert editable Blender hierarchy to GLB | export_blender_glb.py | Blender built-in exporter, source hash/geometry/material readback |
| Extract original game socket geometry | ../VoyageMeshReference | Fingerprinted stock-only LOD0; approximate palette; original UVs/pivot |
| Extract a stock model or Blueprint default assembly | ../Export-VoyageModelGlb.ps1 | [Exact asset -> self-contained GLB](../VoyageModelExporter/README.md); static hierarchy/transforms, ordinary render geometry, bounded PBR materials and explicit omissions |
| Extract a material library from game | ../Export-VoyageMaterialsGlb.ps1 | [Batch exact paths -> GLB](../VoyageMaterialLibrary/README.md); used textures only, inherited parameters, approximate PBR, omissions report |
| Verify extracted material library | verify_material_library.py | Python3.10+, Pillow; embedded PNG/hash/dimension/provenance checks; schema2 rejects unbound images |

Python examples, repository root:

```powershell
python tools/glb/inspect_glb.py --registry mods/Railgun/Assets/Model/model-source.json
python tools/glb/inspect_glb.py path/to/model.glb --output artifacts/model-audit-next.json
python tools/glb/test_inspect_glb.py
```

Reader validates embedded GLB header/chunks, ranges and geometry accessors, triangle
indices, unique named scene nodes, parent graph, matrix/TRS transforms and bounds.
When root `extras.materialPipeline` is present, it also validates the schema and
mode, every generated/source image index, PNG hash and dimensions, bake outputs
and inputs, consumers, unresolved-layer source identities, and complete image
provenance. Metadata is treated only as data, never as instructions.
Registry mode resolves the directory's single GLB and checks required named nodes.
Legacy registries may additionally request source hash/size checks. Detailed node local
and world matrices are column-major, meters, glTF Y-up. It never executes extras.
Rejects external resources, sparse accessors, skins, morph targets and required
extensions; this bounded rigid-mesh checker is not a complete Khronos validator.
Animation data is reported, not simulated; geometry bounds use stored default pose.
Bounds/index rejection tests and matrix/hierarchy tests cover the supported
path. Images/material JSON can be inventoried; textures are NOT decoded or baked.

Blender examples (Windows PowerShell 5.1-compatible):

```powershell
$blenderExe = 'K:\Program Files\Blender Foundation\Blender 5.2\blender.exe'
& $blenderExe --background --factory-startup --python-exit-code 1 `
  --python tools/glb/preview_glb.py -- --input path/to/model.glb --output artifacts/model-preview-next.png
& $blenderExe --background --factory-startup --python-exit-code 1 `
  --python tools/glb/export_blender_glb.py -- --input path/to/edit.blend `
  --output artifacts/converted/model-next.glb --root Railgun
```

Use a separate background process. Both refuse overwriting outputs. Converter's
`--verify-only` verifies existing GLB without rewriting. Keep its output folder
separate from a same-basename blend audit. The converter uses the standard exporter,
not a custom GLB writer. It handles selected mesh/empty hierarchy,
Principled colors/metallic/roughness and
the simple PaletteEmission node, not arbitrary shader baking. Camera/lights are
excluded. Source-only conversion/preview are outside game-tool coverage.

Current local runtime: Blender 5.2.2 LTS. Direct import/preview and an existing
.blend -> GLB roundtrip were checked. Blender versions below 5.0 are unsupported;
other 5.x versions and arbitrary textured materials have not been validated. Do not edit
model material factors as if they were old MTL colors: glTF factors are already
linear PBR inputs. Missing textures must stay explicit, not silently synthesized.

## Collection policy / pending capabilities

### Railgun native-import verification

`verify_harpoon_import.py --source-audit <inspect_glb JSON> --inventory
<RailgunGlbInventory JSON> --semantic <Validate-Railgun validation.json>` compares
all rigid model-node local transforms after glTF-to-Unreal axis/unit conversion,
hierarchy, mesh presence, and cooked untextured base-color/emissive/metallic/
roughness factors. Requires Python3.10+ and NumPy. It reads evidence only and
does not import or rewrite assets. It rejects textures/animations; shader
appearance and gameplay still require a game test. Omitted factor overrides
use glTF defaults and require the reviewed stock native parent.

Railgun's `Build-Railgun.ps1` imports GLB through native Interchange and emits
a derived component/package inventory. Placement, construction and operator/optics
are game-validated; save persistence and multiplayer are not. The reusable parser
is not a custom geometry converter.

Keep reusable tools for mesh extraction, UV/normal/material inspection, textures,
PBR conversion and validation. Reuse standard libraries/exporters; extend small
methods against a concrete requested asset rather than inventing a universal
converter. The material-library exporter now provides decoded textures and
inherited parameters with bounded PBR approximations. Full Unreal shader baking,
high-resolution Nanite and general GLB-to-Unreal
integration outside the scoped Railgun adapter are not provided by this toolkit yet.

Game-derived inputs retain fingerprint/provenance and normally stay ignored. GLB
files containing only authored content may be tracked with their owning mod.
