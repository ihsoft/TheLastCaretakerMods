# GLB tools

These source-only helpers inspect, preview, validate, or convert GLB files. They
do not recreate a model procedurally, and an inspection roundtrip must not
replace the source GLB.

| Intent | Entry point | Runtime |
| --- | --- | --- |
| Inspect a GLB or owning model registry | `glb/inspect_glb.py` | Python 3.10+ and NumPy |
| Preview the actual GLB | `glb/preview_glb.py` | Blender 5.0+ |
| Render imported materials as labeled swatches | `glb/preview_material_glb.py` | Blender 5.0+ |
| Convert an authored Blender hierarchy to GLB | `glb/export_blender_glb.py` | Blender 5.0+ |
| Verify a material-library GLB | `glb/verify_material_library.py` | Python 3.10+ and Pillow |

## Normal calls

```powershell
python tools\glb\inspect_glb.py --registry mods\Railgun\Assets\Model\model-source.json

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
