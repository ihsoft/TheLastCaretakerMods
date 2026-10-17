# Game-derived source contracts

Current gate:

- Steam app: `1783560`
- Steam build ID: `25056839`
- engine: Unreal Engine `5.8.1`

Reconstructed editor-only identities:

- `/Script/Voyage.VoyageBaseUserWidget : /Script/UMG.UserWidget`
- `/Script/Voyage.VoyageInGameWidget : VoyageBaseUserWidget`
- `/Script/Voyage.VoyageInGameVehicleWidget : VoyageInGameWidget`
- `/Script/Voyage.VoyageInGameBoatWidget : VoyageInGameVehicleWidget`
- `VoyageInGameBoatWidget.PetrolTB : UTextBlock`
- `VoyageInGameBoatWidget.BatteryTB : UTextBlock`
- `/Script/Voyage.EModuleResourceType::Electricity = 0`
- `/Script/Voyage.EModuleResourceType::Diesel = 16`
- `/Script/Voyage.VoyageModuleComponent.GetResourceAmount(EModuleResourceType) : double`
- `/Script/Voyage.VoyageModuleSubsystem : /Script/Engine.TickableWorldSubsystem`
- `VoyageModuleSubsystem.GetModulesInSameGrid(VoyageModuleComponent, out Array<VoyageModuleComponent>)`
- stock package `/Game/UI/Game/HUD/BP_VoyageIngameBoatHud`

The mapping records `Petrol = 1` and `Diesel = 16`. `BatteryTB` and
`PetrolTB` are separate `UTextBlock` leaves in the stock HUD. Both use size
`10`, `VoyageRoboto` Regular, specified color `(0.5, 0.5, 0.5, 1.0)`, centered
justification, and uppercase transformation.

The supported game build uses IoStore TOC `ReplaceIoChunkHashWithIoHash` and
container header `SoftPackageReferencesOffset`, matching retoc's `UE5_7`
profile. That compatibility profile remains explicit in the scripts because
the installed retoc does not yet name `UE5_8`. The stock HUD's relocation
contract requires exactly two source-path occurrences in `.uasset` and none
in `.uexp`. The UE 5.8.2 editor cooks for the UE 5.8.1 game. The release
contains four packages and displays grid-wide Diesel and Electricity totals
after a save load; Petrol is excluded.

UE 5.8.2 enables ZenStore for a direct cook commandlet through its effective
packaging defaults. The mod build passes `-SkipZenStore` explicitly because
the checked relocation and retoc pipeline consumes loose `.uasset`/`.uexp`
pairs. The build manifest records `LooseCookedPackageWriter`, and installation
rejects another cook-storage contract.

The unpatched retoc reader omits `FObjectImport.PackageName` while reading and
writing filtered legacy packages, although UE 5.8 serializes the field with an
`ObjectName` placeholder. Mixed conversion shifted every later import and
panicked on invalid outer indices. `tools/Build-RetocUe58Compatibility.ps1`
reproduces the narrow correction from a reviewed upstream source. Extraction
and build use the patched tool through its documented interface.

The freshly extracted stock package is relocated to the equal-length package path
`/Game/Mods/Boat/BP_VoyageIngameBoatHud_O`. The current stock `.uasset`
contains exactly two ASCII occurrences of its source package path and none in
its `.uexp`; the build must stop unless its input satisfies those assertions.
Revalidate all identities, path counts, and reconstructed headers after any
fingerprint change.

Extracted originals, mappings, cooked assets, `scriptobjects.bin`, staging
trees, and containers are generated game data and must stay under ignored
`artifacts/` or ignored project output directories.
