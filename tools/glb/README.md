# GLB tools

GLB is the editable model source and model-to-build handoff. These tools inspect,
preview or convert files; they do not procedurally recreate the model. The owning
registry pins the authoritative GLB. Conversions and renders are disposable derived
outputs. Never replace a GLB with a roundtripped copy merely to inspect it.

## Available and tested

| Intent | Entry point | Requirements / result |
| --- | --- | --- |
| Inspect a GLB or owning registry | inspect_glb.py | Python 3.10+, NumPy; compact result, optional detailed JSON |
| Direct render of actual GLB | preview_glb.py | Blender bundled glTF importer; PNG, no source rewrite |
| Convert editable Blender hierarchy to GLB | export_blender_glb.py | Standard bundled exporter, source hash/geometry/material readback |
| Extract original game socket geometry | ../VoyageMeshReference | Fingerprinted stock-only LOD0; approximate palette; original UVs/pivot |

Python examples, repository root:

```powershell
python tools/glb/inspect_glb.py --registry models/HarpoonCannon/model-source.json
python tools/glb/inspect_glb.py path/to/model.glb --output artifacts/model-audit-next.json
python tools/glb/test_inspect_glb.py
```

Reader validates embedded GLB header/chunks, ranges and geometry accessors, triangle
indices, unique named scene nodes, parent graph, matrix/TRS transforms and bounds.
Registry mode checks source hash/size and required named nodes. Detailed node local
and world matrices are column-major, meters, glTF Y-up. It never executes extras.
Rejects external resources, sparse accessors, skins, morph targets and required
extensions; this bounded rigid-mesh checker is not a complete Khronos validator.
Animation data is reported, not simulated; geometry bounds use stored default pose.
Bounds/index rejection tests and matrix/hierarchy tests cover the supported
path. Images/material JSON can be inventoried; textures are NOT decoded or baked.

Blender examples (Windows PowerShell 5.1-compatible):

```powershell
$blenderExe = 'K:\Program Files\Blender Foundation\Blender-80\blender.exe'
& $blenderExe --background --factory-startup --python-exit-code 1 `
  --python tools/glb/preview_glb.py -- --input path/to/model.glb --output artifacts/model-preview-next.png
& $blenderExe --background --factory-startup --python-exit-code 1 `
  --python tools/glb/export_blender_glb.py -- --input path/to/edit.blend `
  --output artifacts/converted/model-next.glb --root HarpoonCannon
```

Use a separate background process. Both refuse overwriting outputs. Converter's
`--verify-only` verifies existing GLB without rewriting. Keep its output folder
separate from a same-basename blend audit. The converter was moved unchanged from
models/HarpoonCannon/export_glb.py; it uses the standard exporter, not a new GLB writer.
It handles selected mesh/empty hierarchy, Principled colors/metallic/roughness and
the simple PaletteEmission node, not arbitrary shader baking. Camera/lights are
excluded. Source-only conversion/preview are outside game-tool coverage.

Current local runtime: Blender 2.80 sub39. Harpoon GLB V1 and socket reference
import/preview checked; existing .blend -> GLB roundtrip checked. Other Blender
versions and arbitrary textured materials have not been validated. Do not edit
model material factors as if they were old MTL colors: glTF factors are already
linear PBR inputs. Missing textures must stay explicit, not silently synthesized.

## Collection policy / pending capabilities

Keep reusable tools for mesh extraction, UV/normal/material inspection, textures,
PBR conversion and validation. Reuse standard libraries/exporters; extend small
methods against a concrete requested asset rather than inventing a universal
converter. Desired does not mean available: texture decoding, inherited Unreal
material reconstruction/baking, high-resolution Nanite and general GLB-to-Unreal
integration are not provided by this toolkit yet.

Game-derived inputs retain fingerprint/provenance and normally stay ignored. GLB source
files containing only authored content may be tracked with their owning registry;
mixed GLBs follow the root rule and only its explicit, exact-file exceptions
(currently the user-approved HarpoonCannon V1 socket reference).
Do not remove embedded reference content without the user's model revision choice.
