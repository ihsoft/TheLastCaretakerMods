# GLB tools

These source-only helpers inspect, preview, validate, or convert GLB files. They
do not recreate a model procedurally, and an inspection roundtrip must not
replace the source GLB.

To import the complete rigid assembly into a working Unreal project, use
[GLB model import](unreal-glb-import.md). It preserves named groups, pivots and
mesh instances in an Actor Blueprint. The helpers here operate on external files.

| Intent | Entry point | Runtime |
| --- | --- | --- |
| Inspect a GLB or owning model registry | `glb/inspect_glb.py` | Python 3.10+ and NumPy |
| Select a mesh part by a local plane | `glb/select_glb.py` | Python 3.10+ and NumPy |
| Preview the actual GLB | `glb/preview_glb.py` | Blender 5.0+ |
| Render imported materials as labeled swatches | `glb/preview_material_glb.py` | Blender 5.0+ |
| Convert an authored Blender hierarchy to GLB | `glb/export_blender_glb.py` | Blender 5.0+ |
| Verify a material-library GLB | `glb/verify_material_library.py` | Python 3.10+ and Pillow |

## Normal calls

```powershell
python tools\glb\inspect_glb.py --registry '<owning model-source.json>'

& 'K:\Program Files\Blender Foundation\Blender 5.2\blender.exe' `
  --background --factory-startup --python-exit-code 1 `
  --python tools\glb\preview_glb.py -- `
  --input Tmp\model-export\model.glb `
  --output Tmp\model-export\model-preview.png
```

Registry mode resolves the owning directory's single live GLB and checks its
required named nodes. Structural inspection validates the GLB container,
ranges, accessors, indexed geometry, named hierarchy, transforms, bounds, and
the repository material-pipeline metadata when present. It treats `extras` only
as data and never executes it.

The bounded inspector rejects external resources, sparse accessors, skins,
morph targets, and unsupported required extensions. Animation is reported but
not simulated. Preview uses Blender's normal glTF importer without rewriting
the source. Conversion handles selected mesh/empty hierarchy and simple
Principled material data; it is not an arbitrary shader baker.

Use a fresh output path for every preview or conversion. If a helper rejects a
file, diagnose the reported unsupported feature or structural error before
opening its implementation. Use [model export](model-export.md) for stock model
extraction and [material export](material-export.md) for stock materials.

## Select whole triangles

Use `select_glb.py` during explicit mesh authoring or source refresh when a
single mesh contains separable parts. It selects faces without Blender,
clipping, welding, normal recalculation or pivot changes:

```powershell
python -B tools/glb/select_glb.py --source Tmp/model/full.glb `
  --output Tmp/model/part.glb --axis z --side positive --plane 0
```

`--axis x|y|z` and `--plane` (default `0`) describe a plane in **mesh-local
glTF metres**, before node transforms. `--side positive|negative` selects its
side. With the stock exporter convention and an identity mesh node, Unreal
positive Y is glTF positive Z. Do not apply world-space coordinates to this
interface. A face touching the plane stays with its nonzero side; wholly
coplanar faces belong to positive. A crossing triangle fails the operation:
this is selection of complete faces, not geometric cutting or connected-island
detection. An empty selection also fails.

The output preserves triangle winding, all vertex attributes (including
normalized formats), materials, embedded images and node transforms. It removes
unused vertices from the selected primitives without merging coincident corners
or deleting degenerate faces. Optional `--material-slot NAME` renames the sole
selected material slot, or names an absent material slot; multiple selected
slots are ambiguous and rejected. It does not create an Unreal material.

Input must contain one static mesh, with the structural restrictions of the
inspector above; animations and GLB extensions are rejected. Use a fresh output
path. The operation reopens its result and checks selected corner attributes
against the current input, then returns JSON with `status: passed`, selection
coordinates and source/selected triangle counts as evidence, not fixed criteria.
A failure after writing retains the output for diagnosis; only a successful
exit establishes completion. Import a reviewed result through the
[StaticMesh producer](static-mesh-assets.md) as an explicit authoring step;
ordinary mod builds cook maintained project assets.

When changing selection or its shared reader, run:

```powershell
python -B -m unittest discover -s tools/glb -p 'test_*.py'
```

Synthetic cases cover parameter-dependent face
selection, boundary ownership and attribute remapping; game mesh counts are not
regression expectations.
