# BoatTotalDiesel active backlog

## Goal

Make the Boat's lower-right HUD fuel value show the total Diesel stored by all
modules in the same ship grid. Petrol is a different resource and must never be
included. The wall and console fuel widgets already provide the intended total;
only the compact possessed-Boat HUD needs replacement.

## Current game gate

- Steam build: `24990438`
- Unreal Engine: `5.8.1`
- `VoyageSteam-Win64-Shipping.exe` SHA-256:
  `D9BF4C9624C60615198E62C87DA7792A9888AB02F7905AAAF1C9B02C7A9E524F`
- `EModuleResourceType::Petrol = 1`
- `EModuleResourceType::Diesel = 16`

## Validated architecture

The stock Boat HUD owns lifecycle and layout. Its inherited `PetrolTB` property
is the historical name of the lower-right fuel text, not evidence that the
displayed resource is petrol.

A stock-path child of the freshly relocated
`BP_VoyageIngameBoatHud` uses a one-shot `PreConstruct` override to collapse
only `PetrolTB` and insert a separate text-only widget into the same parent
panel. The separate widget derives directly from engine `UUserWidget`, so its
Tick does not add an unsafe CDO delta to the unversioned Voyage HUD inheritance
chain.

Each Tick, the separate widget:

1. obtains the owning player's possessed Boat pawn;
2. resolves its `VoyageModuleComponent`;
3. obtains `VoyageModuleSubsystem`;
4. asks for every module in the same grid;
5. sums only `GetResourceAmount(EModuleResourceType::Diesel)`;
6. rounds the total and renders it as `#### L`.

The aggregate probe was game-validated on 2026-08-31 against the previous
Steam build `23962331`. The lower-right value
changed from the main-tank-only `1911 L` to the confirmed total `18966 L`, while
the stock pump icon, position, speed, range, electricity, throttle, and steering
display remained intact.

## Rejected and discriminating experiments

- Two early candidates produced no visible change because their Tick event was
  cooked as an unbound same-name graph function.
- Binding inherited Boat-HUD Tick correctly caused the same startup crash
  twice: `Bad export index 66559/5` while loading the generated child CDO.
  Static isolation showed an unversioned native-ancestor property-index shift.
  Do not add repeating logic to that inherited child.
- A bound `PreConstruct` marker changed `PetrolTB` opacity, but stock native
  logic restored the litre string. This proved the stock-path child and field
  identity were live, while a one-shot text write was not sufficient.
- A separate direct-`UUserWidget` leaf displayed `TOTAL SLOT` in exactly the
  intended position. This proved the narrow composition before resource logic
  was added.

## UE 5.8.1 refresh

The game update invalidated the old relocated Blueprint snapshot and all cooked
outputs. The user removed the old container after it crashed the new game; it
must not be reinstalled.

Fresh build `24990438` runtime reflection confirms:

- `/Script/Voyage.VoyageInGameBoatWidget` still derives from
  `VoyageInGameVehicleWidget` and still owns the same six properties in the
  same order, including `PetrolTB` at own index `4`;
- `EModuleResourceType::Petrol = 1` and `Diesel = 16`;
- `VoyageModuleComponent:GetResourceAmount(Type)` remains a const Blueprint-pure
  native function returning `double`;
- `VoyageModuleSubsystem:GetModulesInSameGrid(Module, OutModules)` remains a
  native Blueprint-callable function with the same owner and parameter types.

CUE4Parse with explicit `GAME_UE5_8` also loaded the fresh stock
`BP_VoyageIngameBoatHud` directly from the base IoStore container. Its generated
pseudocode is byte-for-byte identical to the preserved build `23962331` stock
HUD report: the same native parent and widget identities remain, with native
Tick required and no Blueprint Tick implementation.

`jmap_dumper 0.2.0` supports the UE 5.8 layouts, but its automatic
`GUObjectArray` signature resolved a changing counter in this executable. A
bounded writable-`.data` scan found the structurally valid array (`221551`
objects, `4/65` chunks at the observation point), after which a 2,141,952-byte
`.usmap` and full reflection dump were produced under ignored `artifacts/`.

Current base containers report the same IoStore versions as retoc's `UE5_7`
profile: TOC `ReplaceIoChunkHashWithIoHash` and header
`SoftPackageReferencesOffset`. Retoc does not yet expose a `UE5_8` profile, so
the compatibility use of `UE5_7` remains explicit. The fresh stock HUD
extracted successfully (`1` asset, `0` failures, `0` shader libraries); its
relocation path still occurs exactly twice in `.uasset` and zero times in
`.uexp`. A non-installed `to-zen` round trip also passed `retoc verify`, kept
the same TOC/header versions, and converted back to a byte-identical `.uexp`
with an equal-length reserialized `.uasset`. The later final installed
candidate passed the required real-game loading gate.

The Epic Launcher installed UE `5.8.2` at `K:\Epic Games\UE_5.8`, while the
game executable reports UE `5.8.1`. The build records both versions and accepts
only this exact editor/game pair. The final installed candidate loaded and
worked in the real game; a successful cook alone would not have been
sufficient evidence.

UE 5.8.2 also enables ZenStore by default for direct commandlet cooks. The
first attempt completed with zero cook errors but produced only
`ue.projectstore`/metadata instead of the loose package pair required by the
relocation pipeline. The build now passes `-SkipZenStore`, cleans only its
generated platform cook directory, and records `LooseCookedPackageWriter` in
the install-gated manifest.

Upstream retoc `0.1.5` then exposed a second UE 5.8 incompatibility. It skipped
the serialized `FObjectImport.PackageName` field for filtered cooked packages,
so the generated leaf/child imports shifted and produced invalid outer indices
(`52/6` and `22/5`). Correcting the read/write layout removed those failures;
the stock HUD also had to be freshly re-extracted with the corrected converter
because the earlier `to-legacy` output had encoded the same shortened layout.
The reproducible, upstream-hash-gated builder lives at
`tools/Build-RetocUe58Compatibility.ps1`.

Blocked/pending:

- explicit leave/re-enter duplication validation.

## Current UE 5.8 candidate

The final default-path build used the fresh hash-linked extraction, UE 5.8.2,
and the reproducible retoc compatibility binary. It passed generation, both
loose cooks, relocation assertions, `retoc verify`, and the exact three-asset
inventory. It was installed while Voyage was closed with a hash-guarded
removal manifest.

Independent CUE4Parse inspection of the installed container together with the
base game confirmed:

- the stock-path child CDO contains only `UberGraphFrame` and templates the
  relocated current stock HUD;
- the child has bound `PreConstruct`, collapses `PetrolTB`, creates the separate
  leaf, and adds it to the same parent; it has no Tick;
- the leaf directly derives from `UUserWidget`, has bound Tick, enumerates
  `GetModulesInSameGrid`, calls `GetResourceAmount(0x10)`, and updates its text.

Container hashes:

- `BoatTotalDiesel_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `BoatTotalDiesel_P.ucas`:
  `3F29A5021D8BBAD8099423F421E90E091DCB7EECF0D1A0684EA5298501AEF14D`
- `BoatTotalDiesel_P.utoc`:
  `B2F824EF90B7ED1539DEC1680DEFEBA5C29983597A0462FDB93847C7E13509AB`

On 2026-08-31 the user confirmed that this installed UE 5.8 candidate works in
the real game. This closes the startup/loading, editor/game compatibility, and
primary total-Diesel display gates. Petrol stored in tanks on the ship was
confirmed not to enter the displayed sum. Boat leave/re-enter duplication
behavior remains unconfirmed unless separately reported.

## Previous UE 5.7 candidate

The source was renamed from the disposable probe to `BoatTotalDiesel`.
Production candidate `15` intentionally keeps the validated graph and changes
only internal/product naming plus the initial fallback text from diagnostic
`DIESEL ...` to neutral `-- L`.

The candidate passed the previous fingerprint gate, editor build, generation,
narrow cook, `retoc verify`, exact three-asset inventory, and independent
cooked inspection. The inherited stock-path child CDO contains only
`UberGraphFrame`; the separate leaf is a direct `UUserWidget` with a bound Tick,
and its cooked pseudocode calls `GetResourceAmount(0x10)`.

Container hashes:

- `BoatTotalDiesel_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `BoatTotalDiesel_P.ucas`:
  `FA8EFF615420D9AEC9FAA1499A8C423A8841203B7982DA287CA08AF7F9CFA787`
- `BoatTotalDiesel_P.utoc`:
  `94CD6F2672ADF9DDAAC6AFF1BEB2DD83C81FE4EB514203BA125DE99902C9C8A0`

The old `BoatDieselHudProbe_P` files were hash-guarded and removed. Candidate
`15` was installed with matching hashes; the user later removed the obsolete
container after the UE 5.8.1 update so the base game could start.

The following narrower lifecycle check remains pending for the UE 5.8 build:

- leave and re-enter the Boat and confirm there is one correctly positioned
  value with no duplicated leaf;
