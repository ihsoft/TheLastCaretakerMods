# Cooked mesh planning reference

Read-only geometry extraction, tested on Steam25191271 / UE5.8, executable SHA256
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
Validated target: `/Game/AssetSets/Sockets/SM_Socket_Electrical_Output`.
Not a general material/texture/Nanite converter. Uses .NET10, the existing
manifest-validated `.tools/bin/CUE4Parse` bundle and local Oodle DLL. No new package
dependency. Game-derived outputs must remain under ignored `artifacts/`.

Explicit tool-development publish (not a normal extraction step):

```powershell
dotnet publish tools/VoyageMeshReference/VoyageMeshReference.csproj -c Release -o .tools/bin/VoyageMeshReference --nologo -v quiet
```

If the SDK is denied access to the user's NuGet.Config, run that build outside
the sandbox. Normal extraction consumes the published binary, never dotnet run.
PowerShell 5.1-compatible commands from the repository root, with fresh outputs:

```powershell
$referenceDir = 'artifacts/modeling/ElectricalSocket-next'
$fingerprintPath = Join-Path $referenceDir 'fingerprint.json'
& tools/Get-VoyageBuildFingerprint.ps1 -OutputPath $fingerprintPath
$mapping = & tools/Get-VoyageMappings.ps1
& .tools/bin/VoyageMeshReference/VoyageMeshReference.exe $fingerprintPath `
  $mapping.manifestPath $mapping.mappingsPath `
  /Game/AssetSets/Sockets/SM_Socket_Electrical_Output `
  (Join-Path $referenceDir 'socket-source.json')
if ($LASTEXITCODE -ne 0) { throw 'Extraction failed' }
$blenderExe = 'K:\Program Files\Blender Foundation\Blender-80\blender.exe'
& $blenderExe --background --factory-startup --python-exit-code 1 `
  --python tools/VoyageMeshReference/make_socket_reference.py -- `
  --input (Join-Path $referenceDir 'socket-source.json') `
  --output (Join-Path $referenceDir 'ElectricalSocket_Reference.blend') --render
if ($LASTEXITCODE -ne 0) { throw 'Blender assembly failed' }
& $blenderExe --background --factory-startup --python-exit-code 1 `
  --python tools/glb/export_blender_glb.py -- `
  --input (Join-Path $referenceDir 'ElectricalSocket_Reference.blend') `
  --output (Join-Path $referenceDir 'glb/ElectricalSocket_Reference.glb') `
  --root ElectricalSocket_Reference
if ($LASTEXITCODE -ne 0) { throw 'GLB export/readback failed' }
```

Keep GLB in a separate folder because both converters write a sibling audit.json.
The exporter also supports `--verify-only`. Blender 2.80 sub39 tested; newer APIs
are not validated. No extracted data is committed. No game build/install occurs.

Extractor validates current EXE versus fingerprint, mapping hash/EXE binding,
and canonical parser bundle hashes, then mounts ONLY stock global/pakchunk Windows
containers, never installed mods. One exact package's ordinary render LOD0 is
exported with normals, UV channels, sections and indices. This is NOT reconstructed
high-resolution Nanite geometry. Renew fingerprint/mappings after a game update.

Socket specifics:

- Original 3504 vertices / 2932 triangles / 3 material slots. One source face
  reuses another face's vertex triplet. Blender rejects it: split 3 vertices at
  identical coordinates to preserve both triangles, UVs, normals and sections.
  No triangles or positions removed/changed. Result: 3507 builder vertices.
- Bounds cm: min `(-12.727964,-9.942081,-12.737566)`, max
  `(12.730652,5.981359,12.711356)`. ~25.46 cm diameter, 15.92 cm depth.
- Original pivot retained; Blender coordinates `(X,-Y,Z)/100`, meters. Reflection
  converts UE clockwise indices to outward Blender winding. GLB is standard Y-up.
- Socket-data Roll90 is metadata only, NOT applied. This is mesh-local orientation,
  not the full Typhoon component pose.
- Approximate metal and blue PBR slots ONLY: no textures, procedural dirt, electric
  glyph, runtime glow or behavior. Existing label surface is uniformly blue.
- Studio camera/lights excluded from GLB. Final cannon should retain an anchor for
  the ORIGINAL game socket, not ship this extracted reference as authored geometry.
- Blend reopen error=0; GLB readback 2932 triangles/3 materials, max world vertex
  error `2.1073424e-8` meters. Direct front render inspected.
