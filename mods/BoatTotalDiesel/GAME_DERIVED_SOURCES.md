# Game-derived source contracts

Current gate:

- Steam app: `1783560`
- Steam build ID: `24990438`
- engine: Unreal Engine `5.8.1`
- `VoyageSteam-Win64-Shipping.exe` SHA-256:
  `D9BF4C9624C60615198E62C87DA7792A9888AB02F7905AAAF1C9B02C7A9E524F`

Reconstructed editor-only identities:

- `/Script/Voyage.VoyageBaseUserWidget : /Script/UMG.UserWidget`
- `/Script/Voyage.VoyageInGameWidget : VoyageBaseUserWidget`
- `/Script/Voyage.VoyageInGameVehicleWidget : VoyageInGameWidget`
- `/Script/Voyage.VoyageInGameBoatWidget : VoyageInGameVehicleWidget`
- `VoyageInGameBoatWidget.PetrolTB : UTextBlock`
- `/Script/Voyage.EModuleResourceType::Diesel = 16`
- `/Script/Voyage.VoyageModuleComponent.GetResourceAmount(EModuleResourceType) : double`
- `/Script/Voyage.VoyageModuleSubsystem : /Script/Engine.TickableWorldSubsystem`
- `VoyageModuleSubsystem.GetModulesInSameGrid(VoyageModuleComponent, out Array<VoyageModuleComponent>)`
- stock package `/Game/UI/Game/HUD/BP_VoyageIngameBoatHud`

The reflected identities above were revalidated from a live UE 5.8.1 process
with `jmap_dumper 0.2.0`. Its automatic `GUObjectArray` resolver returned a
false address for this executable; a bounded scan of the writable image data
located the structurally valid array, after which both `.usmap` and full
reflection dumps succeeded. The current mapping keeps `Petrol = 1` and
`Diesel = 16`; the two native function owners, parameters, and return types are
unchanged.

CUE4Parse with the explicit UE 5.8 serializer loaded the current stock Boat HUD
from the base container. Its generated pseudocode is byte-for-byte identical to
the preserved build `23962331` stock report, including the native parent,
widget identities, native-Tick requirement, and absence of Blueprint Tick.

Current Voyage still uses IoStore TOC `ReplaceIoChunkHashWithIoHash` and
container header `SoftPackageReferencesOffset`, matching retoc's `UE5_7`
profile. That compatibility profile remains explicit in the scripts because
the installed retoc does not yet name `UE5_8`. The fresh stock HUD extracted
successfully with the corrected converter and no shaders, and preserves the
exact `2`/`0` relocation path counts. The final three-package container passed
`retoc verify` and independent CUE4Parse UE5_8 inspection while mounted with
the base game.
The installed UE 5.8.2 editor is used to cook for the UE 5.8.1 game. Static
container checks passed, and on 2026-08-31 the user confirmed that the
installed container loads and the total-Diesel HUD works in the real game.
Petrol present in tanks on the same ship was confirmed not to enter the sum.
This validates the 5.8.2-editor/5.8.1-game combination for the primary path.

UE 5.8.2 enables ZenStore for a direct cook commandlet through its effective
packaging defaults. The mod build passes `-SkipZenStore` explicitly because
the checked relocation and retoc pipeline consumes loose `.uasset`/`.uexp`
pairs. The build manifest records `LooseCookedPackageWriter`, and installation
rejects another cook-storage contract.

Upstream retoc `0.1.5` also omits `FObjectImport.PackageName` while reading and
writing filtered legacy packages, although UE 5.8 serializes the field with an
`ObjectName` placeholder. Mixed conversion shifted every later import and
panicked on invalid outer indices. `tools/Build-RetocUe58Compatibility.ps1`
reproduces the narrow correction from a hash-gated upstream source. Extraction
and build manifests must record the same patched executable SHA-256.

The freshly extracted stock package is relocated to the equal-length package path
`/Game/Mods/Boat/BP_VoyageIngameBoatHud_O`. The current stock `.uasset`
contains exactly two ASCII occurrences of its source package path and none in
its `.uexp`; the build must stop unless its input satisfies those assertions.
Revalidate all identities, path counts, and reconstructed headers after any
fingerprint change.

Extracted originals, mappings, cooked assets, `scriptobjects.bin`, staging
trees, and containers are generated game data and must stay under ignored
`artifacts/` or ignored project output directories.
