# MaterialPack support files

Normal callers use `../Export-VoyageMaterialPack.ps1`; they do not invoke files in
this directory directly.

`render_material_pack_preview.py` runs inside Blender 5.0+ and renders the one
material from the exporter's transient evidence GLB with the fixed
`voyage.material-sphere/1` recipe. The manifest-gated `VoyageMaterialLibrary`
binary owns staging, deterministic ZIP assembly and readback validation. The final
archive never contains the transient GLB or any geometry.
