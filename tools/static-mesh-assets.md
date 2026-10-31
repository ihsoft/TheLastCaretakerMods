# StaticMesh preparation and inspection

Use these tools when authored GLB geometry must become editor or cooked Voyage
`StaticMesh` assets, or when an exact cooked mesh must be inspected. They are
independent: inspection works on stock assets and containers produced by any
builder.

| Task | Public entry point | Result |
| --- | --- | --- |
| Import, build, and cook authored GLB meshes | `New-VoyageStaticMeshAssets.ps1` | Loose cooked packages and a build manifest |
| Build complete editor mesh packages from authored GLB meshes | `New-VoyageStaticMeshAssets.ps1 -OutputKind Uncooked` | Selected `.uasset` packages and a build manifest |
| Inspect cooked mesh data and evaluate explicit criteria | `Test-VoyageStaticMesh.ps1` | `inspection.json` and `validation.json` |

Use [model export](model-export.md) instead when the desired result is an
editable GLB representation of a stock asset. Neither StaticMesh tool assembles
a mod container, installs files, or proves runtime rendering.

## Build StaticMesh assets

Create a JSON manifest. Relative source paths resolve from the manifest's
directory. This minimal request uses the builder's documented defaults:

```json
{
  "schemaVersion": 1,
  "assets": [
    {
      "id": "sample",
      "source": "sample.glb",
      "destination": {
        "packagePath": "/Game/Mods/Example/SM_Sample",
        "objectName": "SM_Sample"
      }
    }
  ]
}
```

Run the producer from the repository root. Unreal Editor work must start
outside the restricted sandbox. Put a mod's selected output below its own
`Tmp/<Mod>/` directory:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File tools\New-VoyageStaticMeshAssets.ps1 `
  -Manifest 'Tmp\Example\static-mesh-request.json' `
  -OutputRoot 'Tmp\Example\static-mesh-assets'
```

Use `-ShowExample` for a minimal request and `-ShowSchema` for the complete
field reference. Important per-asset controls include:

- `sourceMesh` for selecting one mesh from a multi-mesh GLB;
- import settings for normals, tangents, UV precision, lightmap UV generation,
  and degenerate removal;
- independent Nanite enablement, precision, trimming, and fallback policy;
- independent ray-tracing support;
- `DefaultEngine` materials or exact `ExternalReferences` bindings;
- optional triangle-count assertions owned by the caller.

The default `-OutputKind Cooked` output contains `Content/...` loose cooked companions,
`build-result.json`, and `output-manifest.json`. Preserve every emitted
companion when packaging the asset. The compact command result gives these
manifest paths and the output root. Internal Unreal workspace, DDC, and logs
are removed after success. A failed run preserves its diagnostic directory
below `Tmp/tools/VoyageStaticMeshBuilder/` and reports `failure.json`.

Schema version 1 accepts GLB input and `/Game` destinations. An ambiguous GLB
fails with its discovered mesh names; set `sourceMesh` to one exact name and
retry.

Neither output mode includes materials or textures imported from the GLB.
It preserves the imported material-slot names, then replaces every slot with
the engine default or an explicit reference to an existing Unreal material.
That reference may identify a stock game or engine material, or a custom
material supplied by another package. The builder serializes its identity but
does not include the referenced material; the eventual container must provide
every non-stock dependency separately.

### Editor packages

Add `-OutputKind Uncooked` to the same public call to skip cooking and return
complete editor mesh packages under `Content/...`, plus `build-result.json`
and `output-manifest.json`. The result identifies `outputKind: Uncooked`.
The same import, Nanite, fallback, ray-tracing and material-binding controls
apply. Only the requested mesh packages are emitted; temporary material stubs
and other imported dependencies are excluded.

The target project must supply referenced materials or suitable editor stubs.
Bindings preserve material identity, not shader appearance. The producer does
not install or merge assets into that project. A consumer that refreshes
maintained assets owns complete package replacement at the declared paths,
backup/recovery, and reopening to check existing references. It must keep
unrelated project assets intact. Each tool call requires a fresh `OutputRoot`;
an existing output directory is rejected rather than updated.

Successful generation establishes the standalone editor packages, not their
behavior after installation or cooking in another project. The target project's
render settings affect cooked Nanite, fallback and ray-tracing data. Validate
the final cooked representations using the consumer's explicit criteria; see
[StaticMesh cooking and validation](../docs/static-mesh-cooking-knowledge.md).

## Inspect or validate a cooked StaticMesh

The validator reads either stock game containers or one exact candidate UTOC.
For a candidate, the requested asset must resolve from that UTOC; a stock
fallback with the same virtual path is reported as a source error.

```json
{
  "schemaVersion": 1,
  "source": {
    "kind": "utoc",
    "utoc": "container/Example_P.utoc"
  },
  "assets": [
    {
      "id": "sample",
      "package": "/Game/Mods/Example/SM_Sample",
      "decodeNanite": true,
      "expectations": [
        { "fact": "/nanite/present", "equals": true },
        { "fact": "/rayTracing/hasProxyFlag", "equals": true }
      ]
    }
  ]
}
```

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File tools\Test-VoyageStaticMesh.ps1 `
  -RequestPath 'Tmp\Example\static-mesh-validation.json' `
  -OutputDirectory 'Tmp\Example\static-mesh-validation'
```

Use `-ShowExample` and `-ShowSchema` before reading implementation. Relative
UTOC and GLB paths resolve from the request directory. `inspection.json`
contains observed facts. `validation.json` contains only evaluation requested
by the caller; without criteria its status is `not-requested`.

Exit codes are part of the public result:

| Code | Meaning | Next action |
| --- | --- | --- |
| `0` | Inspection completed; requested policy passed or no policy was requested | Read the two compact reports |
| `2` | Request or validator setup error | Correct the request or repair the published validator named by the error |
| `3` | Asset missing, unsupported, unreadable, or owned by the wrong source | Inspect the affected asset error and container identity |
| `4` | Facts were read but a caller-selected criterion failed | Compare the failed criterion with the consumer's actual requirement |

Complete Nanite decoding and source comparison are opt-in because they are more
expensive than structural inspection. Current GLB comparison covers local,
untransformed geometry and explicitly selected `POSITION`, `NORMAL`, `COLOR_0`,
and `TEXCOORD_n` attributes. It does not establish transformed instances,
sparse or Draco input support, shader fidelity, collision, game rendering, or
measured performance. Schema version 1 supports one `StaticMesh` export per
cooked package.

## Tool repair boundary

Normal calls consume published bundles and never compile tool source. If a
wrapper reports a missing, stale, or invalid bundle, use the publisher named in
[toolchain maintenance](toolchain-maintenance.md), then retry the same public
call. Inspect builder, validator, parser, or commandlet source only when that
repair path fails or the public result is unexpected.
