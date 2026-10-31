# Model and material tools

Choose the output needed, then use its public tool. Open the focused guide only
for that operation's syntax, result, and representation boundary:

| Need | Tool and guide | Output |
| --- | --- | --- |
| Convert or preview an editable GLB | [GLB tools](glb-tools.md) | GLB or preview |
| Export one stock mesh or Blueprint default assembly | [Model export](model-export.md) | Static geometry GLB |
| Build authored GLB geometry as Voyage StaticMesh assets | [StaticMesh tools](static-mesh-assets.md) | Editor packages or loose cooked packages |
| Inspect or validate an exact cooked StaticMesh | [StaticMesh tools](static-mesh-assets.md) | Inspection and policy reports |
| Export stock materials for visual reference | [Material export](material-export.md) | Material sample GLB |
| Render material samples | [Material export](material-export.md) | PNG contact sheet |
| Exchange one material | [Material export](material-export.md) | `.materialpack.zip` |
| Analyze a material batch | [Material export](material-export.md) | `.materialbundle.zip` |

Model and material exporters require an explicit `-MaterialMode`:
`PbrApproximation` or `BakeReconstructed`. Ask which representation is wanted
when the request does not establish it. Exported geometry and PBR materials do
not imply runtime Blueprint behavior, animation, widgets, or complete Unreal
shader fidelity; inspect the tool's omission report for the selected output.
Ordinary model output belongs below `Tmp/`. Material exporters require fresh
paths below ignored `artifacts/` because their exchange files and evidence are
designed for reuse beyond one tool invocation.

StaticMesh preparation and inspection use caller-owned fresh paths below
`Tmp/`. Keep tool-internal diagnostics below `Tmp/tools/`; a mod selects output
below its own `Tmp/<Mod>/` directory.
