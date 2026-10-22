# Toolchain maintenance

## Repair a published binary

Canonical binaries live below ignored `.tools/bin/`. If a public wrapper reports
one missing or invalid, run the publisher it names. Publishers own source
identity, clean-tree, dependency, and output checks; use their compact result
and diagnostic path before inspecting a checkout under ignored `.tools/`.

| Binary or bundle | Publisher |
| --- | --- |
| `VoyageAssetInspector.exe` | `Publish-VoyageAssetInspectorBinary.ps1` |
| `VoyageExecutableInspector.exe` | `Publish-VoyageExecutableInspectorBinary.ps1` |
| `retoc.exe` | `Publish-RetocBinary.ps1` |
| `jmap_dumper.exe` | `Publish-JmapBinary.ps1` |
| `UAssetAPI/` | `Publish-UAssetApiBinary.ps1` |
| `CUE4Parse/` | `Publish-Cue4ParseBinary.ps1` |
| `UAssetGUI.exe` | `Publish-UAssetGuiBinary.ps1` |

Run commands from the repository root with Windows PowerShell 5.1 unless the
entry point documents another host. Unreal build, generation, and cook stages
must start outside the restricted sandbox; a UBT `Trace*.uba` access denial is
a launch-permission failure. Pass `-GameRoot` when the game is installed outside
the configured default location.

## Development-only source preparation

`Prepare-UAssetApiVoyageUe58.ps1` copies the publisher-approved UAssetAPI
source to a new ignored directory for deliberate tool development. It is not
needed for an ordinary asset query:

```powershell
.\tools\Prepare-UAssetApiVoyageUe58.ps1 `
  -OutputRoot '.\Tmp\tools\uassetapi-development'
```

`Build-RetocUe58Compatibility.ps1` and
`Build-JmapVoyageMappingsDumper.ps1` are lower-level development builders.
Use their public publishers for a canonical binary.

## UAssetGUI stress inspection

Use the published `UAssetGUI.exe stress-open` only when diagnosing or
validating GUI asset parsing. Supply one exact container, a virtual prefix, a
fresh output directory, the engine profile, and a reviewed mapping:

```powershell
.\.tools\bin\UAssetGUI.exe --portable stress-open `
  '<stock-container.utoc>' '<virtual-prefix>' `
  '.\Tmp\tools\uassetgui-stress' UE5_8 '<reviewed.usmap>'
```

Inspect `summary.json` first and `results.jsonl` only for reported failures.
Use `--resume` to continue an interrupted run. Binary equality and successful
export parsing are separate results; neither proves gameplay behavior. If the
canonical executable is missing or invalid, call `Publish-UAssetGuiBinary.ps1`
and inspect its failure report before opening source.

## Mapping dumper fallback

If `New-VoyageMappings.ps1` reports a missing or invalid canonical jmap binary,
run `Publish-JmapBinary.ps1` and retry the mapping producer. The publisher owns
the source checkpoint and binary validation; the CLI version string alone does
not establish identity.

If automatic `GUObjectArray` discovery fails, use
`Find-VoyageUObjectArray.ps1`. Its address is valid only for the same game
process; retry `New-VoyageMappings.ps1` with the validated address.

When the standalone path itself is under investigation,
`tools/VoyageMappingsDumper` is an exceptional UE4SS fallback. Install it as a
temporary mod at
`<GameRoot>/Voyage/Binaries/Win64/ue4ss/Mods/VoyageMappingsDumper`, enable
`VoyageMappingsDumper : 1` in `ue4ss/Mods/mods.txt`, start the game, and
inspect the UE4SS log for `Mappings.usmap`. Keep the candidate and diagnostic
log below `Tmp/mappings/`, then disable the temporary mod. Run
`Test-VoyageMappings.ps1` before promoting any candidate to the tracked mapping
registry; a file's existence alone does not establish a valid map.

## Shared generator helpers

`UnrealEditorGeneratorCommon` contains header-only editor helpers, never
runtime payload. Keep only engine-level reflected identities and semantic graph
helpers there. Game paths, reconstructed Voyage contracts, and mod tuning stay
with their owning mod. Extend an existing semantic group before adding another;
consuming generators address semantic roles rather than raw reflected pins such
as `A`, `B`, or `bPickA`.

## Maintain tool documentation

When adding or changing a reusable public tool, update its
[catalog entry](tool-catalog.md) and the relevant task guide. Document the
supported call, useful result, output lifetime, and action on failure. Keep
source identities, implementation steps, and run histories in their owning
scripts, manifests, or ignored diagnostic output.
