# Model and material tools

Choose the output needed, then use its public tool. The linked local guides
contain command arguments and supported representation:

| Need | Tool and guide | Output |
| --- | --- | --- |
| Convert or preview an editable GLB | [GLB tools](glb/README.md) | GLB or preview |
| Export one stock mesh or Blueprint default assembly | [Export-VoyageModelGlb.ps1](VoyageModelExporter/README.md) | Static geometry GLB |
| Export stock materials for visual reference | [Export-VoyageMaterialsGlb.ps1](VoyageMaterialLibrary/README.md) | Material sample GLB |
| Render material samples | [Export-VoyageMaterialPreview.ps1](VoyageMaterialLibrary/README.md) | PNG contact sheet |
| Exchange one material | [Export-VoyageMaterialPack.ps1](VoyageMaterialLibrary/README.md) | `.materialpack.zip` |
| Analyze a material batch | [Export-VoyageMaterialBundle.ps1](VoyageMaterialLibrary/README.md) | `.materialbundle.zip` |

Model and material exporters require an explicit `-MaterialMode`:
`PbrApproximation` or `BakeReconstructed`. Ask which representation is wanted
when the request does not establish it. Exported geometry and PBR materials do
not imply runtime Blueprint behavior, animation, widgets, or complete Unreal
shader fidelity; inspect the tool's omission report for the selected output.
Ordinary output belongs below `Tmp/`.
