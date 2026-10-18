# Game asset inspection and extraction

Select the narrowest public query that answers the question. Run it before
opening implementation files or full asset JSON. `-Source Game` is the default
and uses only stock containers even when mods are installed. Pass `-GameRoot`
only when the installation is outside the configured default location.

## Find an asset or query mappings

`Inspect-VoyageAsset.ps1` supports these query shapes:

| Need | Example `-Query` |
| --- | --- |
| Find package names without parsing exports | `list:BP_Forklift` |
| Inspect one Blueprint and request pseudocode | `BP_Forklift_Possesable` |
| Find classes by direct parent in a narrow subtree | `parent:VoyageModuleResourceWidget\|Voyage/Content/UI/` |
| Find serialized references in a narrow subtree | `references:AddModuleWidget\|Voyage/Content/UI/` |
| Inspect a reflected class, property, or enum | `mappings:VoyageVehicleForkliftPawn`, `mappings-property:ThrottleInput`, `mappings-enum:EModuleResourceType` |

```powershell
.\tools\Inspect-VoyageAsset.ps1 -Query 'list:BP_Forklift'
```

Parent and reference searches load matching packages; keep their path filters
narrow. A zero-result reference search returns `no-match`, not a tool failure;
use `-RequireMatch` when absence should fail. The tool returns result and
diagnostic paths below `Tmp/inspection/` by default. Open those files only as
needed for the specific question.

For exceptional inspection of one mod's content, add `-Source Mod
-ModContainer <exact.utoc>`. This excludes unrelated installed mods and does
not populate the reusable stock cache. It does not by itself prove which
duplicate provider wins at runtime.

## Compact Blueprint structure

Use `Get-VoyageAssetSummary.ps1` for a Blueprint's class, functions, calls,
members, soft objects, or components:

```powershell
.\tools\Get-VoyageAssetSummary.ps1 `
  -Query '/Game/Blueprints/Modules/Utility/Weapons/BP_Module_Turret' `
  -Focus Calls -FunctionName GetInteractiveProvidedActions -AsJson
```

`Overview` is the default focus. Other focuses are `Functions`, `Calls`,
`Members`, `SoftObjects`, `Components`, and explicit `All`. The compact result
and `summaryPath` are the interface; use the full summary only when the focused
result is insufficient. A cooked Blueprint summary is not proof of native
runtime dispatch or ownership.

## Complete asset JSON or package inventory

Use `Get-VoyageAssetJson.ps1` only when the complete serialized export is
needed. Consume its returned `jsonPath` instead of locating or copying files
inside the tool's cache:

```powershell
.\tools\Get-VoyageAssetJson.ps1 `
  'Voyage/Content/Blueprints/BP_VoyageCableUpdater.uasset' -AsJson
```

`-ListPackages` returns `packageListPath` without parsing every export. For
one exact mod container, add `-Source Mod -ModContainer <exact.utoc>`; mod
queries are diagnostic and uncached. The tool checks package ownership, but
the JSON result alone does not establish runtime override priority. If asset
loading or validation fails, inspect the reported failure before touching the
cache or trying a manual extraction.

## Extract an exact cooked package

Use `Extract-VoyagePackage.ps1` when another tool requires actual legacy
`.uasset/.uexp` files rather than a structural report:

```powershell
.\tools\Extract-VoyagePackage.ps1 `
  -Filter 'Vehicles/BP_Forklift_Possesable' `
  -RetocEngineVersion UE5_8 `
  -OutputRoot '.\Tmp\Example\forklift-extraction'
```

The result supplies `outputPath`, `manifestPath`, source mode, and asset count.
`-Source Game` is stock-only; `-Source Mod -ModContainer <exact.utoc>` selects
one mod plus stock dependencies for diagnosis. A successful extraction must
contain at least one asset. Use the returned manifest to check game and source
identity before reusing saved output. Do not relabel an older extraction as
current or treat parsing as a gameplay test.
