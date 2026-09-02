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

## Diesel socket hover-card research

The Boat's large Diesel-tank side connector is part of
`/Game/Blueprints/Modules/Storage/BP_Module_Diesel_Container`. Its two visible
socket components are native `VoyageModuleSocketViewComponent` instances with
`Port.ResourceType = Diesel`; the owning module has a local capacity of
`2000.0`. This explains the stock hover card's local `1897 L / 2000 L` value.

On Steam build `24990438`, the compact hover card is produced by
`VoyageInGameCharacterWidget::OnPointerHoverChanged`. The derived handler hides
one action container and delegates to the base implementation. The base
implementation reconciles the active-action set, creates the HUD's
`IndicatorSubClass`, configures it with the active action data, and adds it to
either `CentralInputActionContainer` or `ContextInputActionContainer`.

The current `BP_VoyageIngameHud` CDO sets `IndicatorSubClass` to
`/Game/UI/Game/Interact/WBP_InteractIndicator.WBP_InteractIndicator_C`. That
widget owns a `BP_ButtonInfo_Vertical_Container`; the module/socket producer has
already formatted the title, local resource amount/capacity, and Equalize action
before this display consumer receives them.

`/Script/Voyage.VoyageModuleResourceWidget` is not the producer of this card:
the current containers contain no Blueprint child or serialized reference to
it in the surveyed UI, Data/UI, or Blueprints packages, and the native class
registration/accessor has no runtime call site in the shipping executable. Its
similarly named text fields are therefore a deceptive lead for this UI.

The lightest loader-free discriminator is a current-version relocation of
`WBP_InteractIndicator` with a stock-path child that changes one unmistakable
visual marker only. A successful marker would prove class interception and
the live widget tree before adding aggregation. It is a global interaction
widget, however, and its reflected API exposes no stable hovered-module/source
identity. Production logic must not replace text based only on child index,
localized labels, or rendered key text. If the child cannot recover the exact
socket/module identity from its runtime children, prefer a narrow native hook
at `OnPointerHoverChanged` (which receives the interactive component) over a
global text-matching Blueprint patch.

Disposable marker probe `BoatDieselSocketProbe_P` was built from a fresh clean
extraction for Steam build `24990438` and installed while Voyage was closed on
2026-08-31. It relocated only `WBP_InteractIndicator`; the stock-path child had
a one-shot `PreConstruct` that called `SetRenderOpacity(0.2)`. Independent
cooked inspection confirmed that exact float argument, the relocated current
stock parent, and a child CDO containing only `UberGraphFrame`.

The real-game test failed during startup on `FAsyncLoadingThread` with a null
read access violation, before `PreConstruct` could run. The probe was removed
immediately by its hash-guarded manifest; the installed `BoatTotalDiesel_P`
hashes remained unchanged. This rejects this specific reconstructed-native-
parent plus relocated-`WBP_InteractIndicator` child as a safe probe
architecture. It does not disprove the independently established
`IndicatorSubClass` identity. Do not layer another graph change onto this
container; the next discriminator must change the interception architecture.

Probe container hashes:

- `BoatDieselSocketProbe_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `BoatDieselSocketProbe_P.ucas`:
  `E5B2DD02E89279DA70582FE2B23BD7BE25FE2F109588AC54F03D459678CB7510`
- `BoatDieselSocketProbe_P.utoc`:
  `2CB1C20D1EF542E3874DBC437C9571F534BDAF89C3325A291570BF8491CC8BE0`

A second, non-installed discriminator kept the stock interaction widget in
place and redirected the single serialized `WBP_InteractIndicator` package
import inside `BP_VoyageIngameHud` to a mod-path marker child. Static
inspection rejected this architecture before installation: the same import is
used both by the HUD CDO's `IndicatorSubClass` and by the embedded
`WBP_InteractIndicator` instance in `CentralInputActionContainer`. Redirecting
it therefore changes two consumers, not one. CUE4Parse also reported an
unknown unversioned property while decoding the changed embedded instance.
Do not install or revive this shared-import redirect.

Current native disassembly adds two useful constraints. The derived
`VoyageInGameCharacterWidget::OnPointerHoverChanged` hides its extra action
container and delegates to the base handler. The base handler creates an
`InteractIndicator`, then passes action data to its `ButtonInfoContainer`; it
does not call `VoyageBaseUserWidget::SetInteractiveSource` on that container.
`SetInteractiveSource` itself stores component/source at the current native
offsets and recursively propagates them to child `VoyageBaseUserWidget`s, but
that propagation is not part of this indicator creation path. Consequently a
replacement button container cannot identify the hovered Diesel socket via
`GetInteractiveSource()` alone.

A third, non-installed discriminator relocated the current
`BP_VoyageIngameHud` and generated a stock-path child intended to have only one
CDO delta: `IndicatorSubClass` pointing to the already cooked mod-path marker
child. Generation, cook, exact three-asset inventory, and `retoc verify` all
succeeded. Independent inspection with the base game mounted rejected the
result before installation: the marker class decoded under the derived widget
property `ButtonInfoContainer_Action_1`, not `IndicatorSubClass`. The native
prefix mirror therefore did not provide a safe unversioned-property index for
an inherited property serialized through this generated Blueprint parent.
Do not install or revive this HUD-child CDO-delta probe.

The next safe direction is a structural patch of the exact original HUD CDO:
add a distinct marker import and replace only the serialized object reference
owned by `IndicatorSubClass`, while proving that the embedded widget-tree
instance remains byte-for-byte untouched. If that cannot be expressed and
verified surgically, return to the hovered Diesel socket's action producer
instead of guessing another Blueprint property layout.

Probe 4 implemented that structural direction. An unchanged UE 5.8-compatible UAssetAPI roundtrip of the
fresh extracted HUD was byte-for-byte identical. The patch then appended a
distinct marker package/class import, changed the sole stock indicator class
index in the raw CDO at offset `0xA` from `-200` to `-210`, and changed the
matching `CreateBeforeSerialization` dependency. The complete `.uexp` differs
from the exact original at one byte only; the embedded widget export still
uses the stock class import. The package contains only the patched stock-path
HUD and the mod-path marker child and passes `retoc verify`.

The marker child's package is
`/Game/Mods/BoatProbe/WBP_InteractIndicator_M`, but its internal Blueprint
asset name remains `WBP_InteractIndicator`; the generated class import must
therefore be `WBP_InteractIndicator_C`, not `WBP_InteractIndicator_M_C`.
The real-game test loaded without a crash but produced no visible opacity
change on the Diesel socket card. This rejects `IndicatorSubClass` as the
producer of that visible card under the tested interaction path (or, less
likely, proves that a later owner replaces the marker's render state before
display). The probe was removed by its exact hash-guarded manifest immediately
after the closed-game result. Do not add content changes to this interception
path; return to the socket component's provided-action producer.

Installation/removal evidence is
`artifacts/tests/boat-diesel-socket-probe4-game-01/install-manifest.json`.

Probe 4 container hashes:

- `BoatDieselSocketProbe4_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `BoatDieselSocketProbe4_P.ucas`:
  `43D090819B125B0A9E3ED638EADF7C7EDCC3784D7B705F7A32E38F791F2B349B`
- `BoatDieselSocketProbe4_P.utoc`:
  `FEBB05A48FC5F9A2BE7FCCACA87C71473BEACA70B1F56886532DE88A6D434373`

Producer-side follow-up established that
`DA_Socket_LiquidData_Diesel` is not a text or action-data owner. It contains
only the socket meshes, Diesel materials, connection sounds, rotation, and
leak effect. `VoyageModuleSocketViewComponent` likewise adds only the view
mesh/data-asset fields over `ModuleSocketComponent`.

The containing `BP_Module_Diesel_Container` directly derives from native
`VoyageModuleActor`, which implements `InteractiveInterface` and exposes the
separate Blueprint-native event `GetDescriptionFooter() -> FText`. Current
native registration places this function beside the module resource getters,
making it the narrowest remaining candidate for the card's middle
`amount / capacity` line.

A producer-side marker probe now relocates the fresh current
`BP_Module_Diesel_Container`, installs a stock-path child, and overrides only
the exact declaring function `VoyageModuleActor::GetDescriptionFooter` to
return `TOTAL FOOTER`. Structural inspection with the base game mounted
confirmed the literal `EX_TextConst`, the exact native `SuperStruct`, a CDO
templated from the relocated original with no serialized property deltas, and
an exact two-asset container. Unreal initially generated an implicit parent
call in the function graph; the rejected build was caught before installation
and the final graph disconnects that call before setting the literal return.

Probe `BoatDieselSocketFooterProbe_P` was installed while Voyage was closed.
The game loaded the package but crashed on the GameThread after 22 seconds with
`EXCEPTION_ACCESS_VIOLATION writing address 0x00000000000000e0`, before a visible
footer result could be checked. This differs from the rejected Boat-HUD mirror
crash: it is not an async-loading serialization error, and proves that static
bytecode/CDO/container checks did not cover the later runtime contract.

The probe was removed by its exact hash-guarded manifest after the closed-game
result. Removal initially exposed a probe-tool allowlist omission; the exact
footer container name is now accepted without weakening the manifest kind or
file-hash checks. Evidence is
`artifacts/tests/boat-diesel-socket-footer-probe-game-01/install-manifest.json`.

This result does not yet reject inheritance by itself because the experiment
combined two variables: stock-path actor-child replacement and the
`GetDescriptionFooter` override compiled through a minimal native mirror. The
next discriminating control is the same relocated original plus stock-path
child with no added override. If that control loads, the unsafe variable is the
function/mirror contract; if it crashes with the same signature, the actor
replacement lifecycle is unsafe for this module class.

Final probe container hashes:

- `BoatDieselSocketFooterProbe_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `BoatDieselSocketFooterProbe_P.ucas`:
  `6720C2BDC04494E6393AA46C1D20E8E55173419155EC8C5E13DD985D4782695A`
- `BoatDieselSocketFooterProbe_P.utoc`:
  `5814B1C369B780F75DF79653044EDB490180D59B1ECE27955863185F9FB82E60`

The prepared follow-up control changes exactly one variable: it keeps the same
relocated original and stock-path child but generates no
`GetDescriptionFooter` function at all. Independent inspection confirms the
child has no functions and its CDO only templates the relocated original.

Inspection also exposed the leading actor-specific hypothesis. The original
Diesel container already owns `DefaultSceneRoot_GEN_VARIABLE`. The generated
child contains another same-named default-root template, which resolves to the
relocated parent's component after packaging. The validated DonkLift child also
has a generated default root, but its relocated parent did not supply a
same-named template, so it did not create this exact archetype collision.

`BoatDieselSocketChildControl_P` was installed while Voyage was closed. It
repeated the crash after 24 seconds with the exact same GameThread write to
`0xe0`, stack hash `450DCE5B07CD34E3C918D1411716F4D17550AB76`, and call stack as
the footer-override candidate. This rejects stock-path actor-child/SCS
replacement of this Diesel-container Blueprint independently of the function
override. The same-name default-root archetype collision remains the leading
mechanism. Do not build another inherited actor child for this package.

The control was removed by its exact hash-guarded manifest; the production
`BoatTotalDiesel_P` remained installed and unchanged. Removal evidence is
`artifacts/tests/boat-diesel-socket-child-control-game-01/install-manifest.json`.

Control container hashes:

- `BoatDieselSocketChildControl_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `BoatDieselSocketChildControl_P.ucas`:
  `596C5912CBB0C573D447A00508A7D4B84C1E5E2F59E37E7524B73D50489CEE32`
- `BoatDieselSocketChildControl_P.utoc`:
  `E00B100AA1B27998BC7338C87EA129E2F6703EE135C937E9B64C32954682A632`

The next candidate avoids actor inheritance and relocation entirely. Native
disassembly identified `ModuleSocketComponent::GetScanText` as the producer of
the scan card's middle line: it resolves `ModuleOwner`, reads the socket port's
resource type, calls the module-local amount/capacity getters, and writes the
formatted result into `VoyageScanText.Description`. The two Diesel ports in
the stock container are existing `VoyageModuleSocketViewComponent` templates.

The component marker is a direct Blueprint child of that native component and
overrides the exact declaring `InteractiveTextInterface::GetScanText` with
`DIESEL PROBE / TOTAL SOCKET`. A structural patch redirects the original
native class import only after asserting that exactly
`VoyageModuleSocketView_GEN_VARIABLE` and
`VoyageModuleSocketView1_GEN_VARIABLE` use it as their export class. The
stock actor, CDO, SCS owner, root template, and complete `.uexp` remain
byte-identical; there is no relocated actor package.

Generation, cook, an exact byte-identical unchanged UAssetAPI roundtrip, the
asserted class-import patch, native `retoc verify`, and the exact two-asset
inventory passed. Independent CUE4Parse inspection is temporarily pending
because repeated UnrealBuildTool/.NET invocations exited with CLR code
`0xE0434352` and raised desktop crash dialogs; final container assembly used
only the already validated outputs and native `retoc` afterward. The marker
container was installed while Voyage was closed. The game did not crash, but
the result was severely invalid: the player character became stuck to the Boat
and some cables detached visually from their connections. The probe was
removed by its exact hash-guarded manifest, after which the game returned to
the known-good state. Removal evidence is
`artifacts/tests/boat-diesel-socket-component-probe-game-01/install-manifest.json`.

This rejects redirecting the stock
`VoyageModuleSocketViewComponent` class import to a Blueprint child. Although
the patch touched only the two expected Diesel socket templates in the asset,
that native component identity participates in wider attachment, cable, and
interaction behavior. Treat it as a shared behavioral contract, not a narrow
scan-text producer. Do not install or extend this component-redirection
architecture.

Component-marker container hashes:

- `BoatDieselSocketComponentProbe_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `BoatDieselSocketComponentProbe_P.ucas`:
  `33B8EAF570BD5CD630AEBB37CCB84F8DED7C24625185D8A6F860620E0AE7C44F`
- `BoatDieselSocketComponentProbe_P.utoc`:
  `5A7D0876493ABE5F028A58BA7B82AF057B296788070CBB32BE57566D2E41F892`

## Toolchain audit before further socket experiments

Further functional probes are paused. Community reports and our own results
show that a successful parse, save, cook, `retoc verify`, or container load can
still produce a runtime-invalid UE 5.8 package.

The refreshed build-`24990438` mapping exists and is non-empty only in the
manual-`GUObjectArray` dump (`2,141,952` bytes, SHA-256
`7A3B9CA7E3868703A6302E6A1F4E28FD84E2B24A2FF70B44B4CEFB2F2F81C955`).
The automatic and elevated outputs are both structurally valid 28-byte empty
maps. Until corrected, the canonical `Mappings.usmap` path pointed at that
empty result. This made default parser checks incomplete even when operations
that only needed package/import tables happened to succeed.

Audit results:

1. The verified full dump is now the canonical ignored `Mappings.usmap` and
   has a fingerprint/hash/payload/schema manifest gate. The old automatic
   output was preserved as `Mappings-auto-failed.usmap`; the validator accepts
   the full map and deliberately rejects the empty one.
2. The retoc compatibility builder now asserts clean upstream commit
   `885a8dae740cb1ce1e41ff2e74f67f9f0c118237`
   (`v0.1.5-2-g885a8da`) and records that provenance. A fresh native rebuild
   completed. Its full executable hash differs from the older build despite
   equal size and equal critical source hashes, so whole-binary reproducibility
   is not assumed; semantic roundtrip and game canaries remain authoritative.
   The retoc-only unchanged roundtrip of the known-hard
   `/Game/Blueprints/BP_VoyageCableUpdater` subsequently passed both the
   byte-identical legacy control and a real-game test on build `24990438`:
   Voyage loaded and the user observed normal runtime behavior. This clears
   the audited retoc conversion/container layer for this package only; it does
   not clear an UAssetAPI/UAssetGUI save. After the game closed, the canary
   was removed through its exact install manifest. No
   `VoyageCableUpdaterRetocRoundtrip_P` files remain, and the installed
   production `BoatTotalDiesel_P` hashes are unchanged (`pak` `75E714...`,
   `ucas` `3F29A5...`, `utoc` `B2F824...`).
3. Official UAssetAPI master is
   `3228c1e86261aa08131f7ec0ff1a395f5d0b2a84`. Since the prior local base it
   adds `EX_EndMapConst` visitation and a required seek to `MetaDataOffset`.
   The official support statement still stops at UE 5.7. A reproducible
   preparation script now applies the Voyage filtered-import correction to
   that exact current source. Its unchanged-save pass on
   `BP_VoyageCableUpdater` parsed, wrote, and reopened successfully; both the
   2,115-byte `.uasset` and 244-byte `.uexp` are byte-identical to the extracted
   originals. A separately named container passed `retoc verify`, and reverse
   conversion with the base `global` container again produced both original
   hashes. Static UAssetAPI writer normalization is therefore ruled out for
   this package. The separately packaged canary then passed its real-game test:
   Voyage loaded without a crash and the user observed no visual or behavioral
   change. This clears the prepared UAssetAPI unchanged-save layer for this
   package on build `24990438`. After Voyage closed, the canary was removed
   through its exact hash-guarded manifest.
4. Current UAssetGUI master
   `6b9dcf3daa128cd84a29006ccb29aec72b191939` embeds that same UAssetAPI commit,
   but not the Voyage import-layout correction. Do not use stock UAssetGUI to
   save the canary or a production Voyage UE 5.8 asset.
   A local build with corrected UAssetAPI commit `48b6096` still failed when
   opening `BP_FootStep_Componet` directly from the game's IoStore browser.
   The package name is misspelled `Componet` in the stock container. The same
   package extracted by UAssetGUI's embedded retoc without `--version` produced
   a 15,894-byte legacy header that the corrected API could not export to JSON.
   `DirectoryTreeItem.SaveFileToTemp` omits the version argument although the
   project already supplies one for `to-zen`. Adding only `--version UE5_7` to
   that selected-file `to-legacy` call made the package header load, proving
   that the direct-IoStore path needs an explicit fallback version.

   This does not yet make the asset editable. Without mappings the GUI reported
   26 failed exports. After importing the fingerprint-matched 2,141,952-byte
   mapping, it reported 25 failed exports and then failed its binary-equality
   check. A diagnostic load of the same package converted by the corrected
   retoc reduced the failures to 17 raw exports, but binary equality still
   failed. Both UAssetAPI `VER_UE5_7` and `VER_UE5_8` produced the same result.
   UE 5.8.2 also retains package/object version `1018`, identical to UE 5.7, so
   merely adding a newer numeric engine profile is ruled out. The remaining
   mismatch was localized at the first serialized `FProperty`. Its type
   (`IntProperty`) and name (`EntryPoint`) decoded correctly, but UAssetAPI then
   unconditionally consumed four bytes as `FField.Flags`. Native UE 5.8.2
   `FField::Serialize` omits those flags when `Ar.IsFilterEditorOnly()`, as this
   package is. The four-byte shift changed the real `ArrayDim = 1` and
   `ElementSize = 4` into the impossible `ArrayDim = 4` and
   `ElementSize = 148`, then desynchronized every following field. A global
   read/write guard on `!IsFilterEditorOnly` fixed Voyage but regressed two
   existing UE 4.25 tests. The final compatibility rule therefore omits
   `FField.Flags` only for a filtered asset whose caller explicitly selected
   `VER_UE5_8`. UAssetAPI now preserves that exact selection internally because
   reducing it to object version `1018` otherwise makes it indistinguishable
   from UE 5.7. This scoped rule yields `26/26` parsed exports and passes binary
   equality on the corrected-retoc extraction.

   The two A/B-sensitive legacy tests pass with the scoped rule. The complete
   upstream suite returns its unchanged baseline (`17` passed, `10` failed),
   with no failures added relative to corrected-API commit `48b6096`.

   The old UAssetGUI embedded retoc remains independently invalid: even with
   the explicit fallback version and corrected FField layout, its extraction
   parses only `1/26` exports and fails equality. The GUI fork now embeds the
   already reviewed corrected retoc executable (decoded SHA-256
   `415074C00B1F70317B6DC5EF887742DD351E5A2346CA710B2BB3050FD8674399`) instead
   of that upstream resource. A Release build completes with zero warnings and
   zero errors. The user then opened
   `../../../Voyage/Plugins/Channel37/Content/Audio/FootSteps/BP_FootStep_Componet.uasset`
   directly from the game's IoStore browser with the full mapping; it loaded in
   the patched GUI without parse or binary-equality errors. This clears the
   complete direct-container GUI read path for that 26-export package. The user
   then opened both the known-hard
   `../../../Voyage/Content/Blueprints/BP_VoyageCableUpdater.uasset` and the
   production Diesel HUD source
   `../../../Voyage/Content/UI/Game/HUD/BP_VoyageIngameHud.uasset`; both loaded
   cleanly in the same GUI build. Together these controls clear the patched
   direct-IoStore read path across a component-rich reported-hard Blueprint and
   the actual feature asset. This does not yet validate saving a changed asset
   or a resulting runtime package. The scoped API fix is committed in the
   UAssetAPI fork as `09b4899`, and the GUI integration is committed in the
   UAssetGUI fork as `4833a49`; neither commit has been pushed.

   All active GUI work is consolidated in the ignored upstream fork at
   `.tools/UAssetGUI`; temporary duplicate GUI/build/probe source directories
   have been removed.

   A later direct-container read of
   `/Game/BP_OptionsDetailsWidget` exposed one remaining dependency-only gap.
   The top-level package retained binary equality but parsed `31/32` exports;
   its CDO could not find the generated schema
   `BP_ButtonInfo_Vertical_Container_C`. UAssetGUI had correctly extracted
   that dependency, and the dependency itself parses `6/6` with binary
   equality when opened directly as `VER_UE5_8`. The failure occurs only in
   `UAsset.PullSchemasFromAnotherAsset`: its internal dependency `UAsset` is
   constructed from UE object/custom versions, which loses the caller's exact
   `SpecifiedEngineVersion`. Because UE 5.7 and 5.8 share object version
   `1018`, the scoped UE 5.8 `FField.Flags` rule is then skipped while reading
   the dependency class export. Propagating that exact engine selection to the
   internal dependency asset changes the original package from `31/32` to
   `32/32` parsed exports while retaining binary equality. The dependency
   itself remains `6/6`; the earlier FootStep (`26/26`) and CableUpdater
   (`5/5`) controls also retain binary equality. The complete upstream suite
   remains at its exact pre-existing baseline (`17` passed, `10` failed), so
   this probe adds no test regression. After the pre-fix GUI closed, the
   default Release output rebuilt in place with zero warnings and zero errors;
   direct GUI validation is still pending and the new API change is not
   committed.

   Before that direct validation, the installed game advanced again from Steam
   build `24990438` to `25056839`; the executable SHA-256 is now
   `CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
   No mapping for the new fingerprint exists yet, so
   `Voyage-24990438.usmap` is stale and cannot certify arbitrary current-game
   assets. It remains useful only for a controlled before/after diagnosis. On
   freshly extracted build-`25056839` packages, committed API `09b4899`
   exactly reproduces the GUI reports: `BP_Boat_Barge` parses `24/28` and
   `BP_Boat_Base` parses `39/40`, with binary equality for both. With their
   exact current dependencies present, the uncommitted version-propagation fix
   raises them to `28/28` and `40/40`, again with binary equality. All four
   Barge raw exports and the one Base raw export are therefore consequences of
   the same dependency-schema load path. A fresh build-`25056839` mapping is
   still mandatory before broader compatibility or safe-save claims.

   The build-`25056839` mapping refresh exposed two additional deterministic
   jmap failures after upstream master `3f18971`: a null `UEnum*` in an
   `FEnumProperty` default value caused `expect("valid enum")` to panic, and
   two transient `FInterfaceProperty` instances named
   `CallFunc_GetAssetTools_ReturnValue` had a null `InterfaceClass`. The
   requested fork now exists at `https://github.com/ihsoft/jmap` and its only
   working checkout is `.tools/jmap`, with `origin` pointing to the fork and
   `upstream` to `trumank/jmap`. Both remotes' master branches are exactly
   `3f189715f08a646a8c341bf80c2fe06e44177ac3`; no upstream synchronization
   delta exists. The uncommitted narrow patch preserves a numeric enum default
   when runtime enum metadata is absent and treats `InterfaceClass` as nullable
   while retaining the existing string representation for non-null values.
   Release compilation succeeds.

   A later dump advanced beyond both deterministic failures and exposed a
   third real compatibility case rather than a generic live-process race:
   transient Blueprint `FObjectProperty` instances such as
   `CallFunc_GetAllActorsOfClass_OutActors` can have a null `PropertyClass`.
   The USMAP conversion records only `ObjectProperty` for this branch and
   already discards the class identity, so the narrow uncommitted fallback
   preserves an empty jmap class path while warning. No general object-skip or
   `ReadProcessMemory` error suppression was added. With concurrency limited
   to `128`, the resulting build-`25056839` all-object dump completed twice
   with `77,698` objects, `5,198` vtables, the same 13 explicit nullable
   metadata warnings, and byte-identical `2,374,096`-byte output. Its SHA-256
   is `5118549ACD3F34A03E790C307BCDB108632D5E1D87BCA72D7EE5B5EF081538BF`.

   `Test-VoyageMappings.ps1` passed the USMAP v4 header, uncompressed payload,
   build/executable provenance, and required-name gates. Patched UAssetAPI
   then parsed the fresh, exact-dependency canaries with binary equality:
   `BP_Boat_Barge` `28/28`, `BP_Boat_Base` `40/40`, and
   `BP_OptionsDetailsWidget` `32/32`. Codex's attempted write to the host
   `%LOCALAPPDATA%\UAssetGUI\Mappings` was visible only through its virtualized
   filesystem view and did not materialize in the user's Explorer. The
   validated file is therefore staged for an explicit host-side copy at
   `artifacts/mappings/Voyage-25056839.usmap`; the older installed mapping is
   unchanged. No jmap changes are committed.

   The Options canary also caught a deceptive tool-selection failure. The old
   default retoc under `R:\Codex\ToolCache\rust-retoc-master` has SHA-256
   `DF2B6F5D5087E5A49015E60EC88E41A76D4670A64F4BB31547A20BBCC0B5BD72`
   and produced different, invalid legacy `.uasset` headers while leaving the
   `.uexp` payloads byte-identical. Those files generated cascading mapping
   errors and false `0/32` or `1/32` results. UAssetGUI actually uses the
   reviewed fork build `415074C00B1F70317B6DC5EF887742DD351E5A2346CA710B2BB3050FD8674399`
   from its local `Libraries\retoc.exe`; re-extraction with that exact binary
   restored `32/32`. Treat the retoc path and hash in every extraction manifest
   as a test input, not incidental provenance.
5. UE4SS main gained explicit UE 5.8 support on 2026-08-28 in commit
   `5e755b6d55ee1cb06fc8105f165a7e29c3b9509d`, after the earlier public UE 5.8
   PatternSleuth failure report. Current main also adds
   `FUObjectHashTables`-backed lookup with a `GUObjectArray` fallback. A current
   experimental build is therefore the correct loader/dumper candidate; no
   UE4SS files are currently installed in Voyage.
6. Standalone `jmap_dumper 0.2.0` predates upstream commit `805cd7a`, which
   corrects `UStruct::MinAlignment` width for UE 5.6+. Current jmap commit
   `3f189715f08a646a8c341bf80c2fe06e44177ac3` was built natively and includes
   that fix. Its automatic resolver still selected the nearby zero-object
   address and emitted a 28-byte map. The read-only structural `.data` scanner
   found the sole valid live array (`76,943` objects, `2/65` chunks), and the
   explicit-address dump produced `44,859` reflection objects and `5,198`
   vtables. The resulting `2,141,952`-byte USMAP passed the fingerprint,
   manifest, payload, and required-name gate and is byte-identical to the prior
   manual dump. The previous mapping is therefore confirmed, not provisional.
7. Windows event evidence shows `0xE0434352` came from unhandled exceptions in
   our CLI tools, not a CLR fault. Both CLIs now catch top-level errors and
   return exit code `1`; current UAssetAPI and both error boundaries compile,
   and deliberate missing-input controls exit cleanly without a crash dialog.
8. The first semantic discriminator for the same simple package changes only
   `PrimaryActorTick.TickInterval` from its inherited default to `1.0` second.
   The original nine-byte CDO contains only `TickGroup = TG_PostUpdateWork`;
   the patch extends its nested `ActorTickFunction` unversioned header with
   mapped property index `6`. The `.uexp` is an exact `9 -> 15` byte CDO splice
   with every surrounding export byte preserved. Independent CUE4Parse UE5_8
   decoding reports both `TickGroup = TG_PostUpdateWork` and
   `TickInterval = 1.0`; `retoc verify` passes, and reverse conversion is
   byte-identical to the patched legacy pair. The separately named
   `VoyageCableUpdaterTickInterval_P` container loaded without a crash, but the
   user saw no obvious runtime change. The interval marker was therefore too
   weak to prove path liveness and was removed through its exact manifest.
   Production `BoatTotalDiesel_P` hashes remain unchanged.
9. The next discriminator deliberately corrupts only the generated class
   export-map `SuperIndex`: the exact native superclass import index `-7` is
   replaced with impossible export index `66559` in a five-export package.
   The source `.uasset` differs at exactly four bytes at `0x5FF`; its `.uexp`
   remains byte-identical. `retoc verify` passes, and isolated CUE4Parse UE5_8
   inspection exposes the invalid `Super` object path at export `66558` while
   preserving the serialized native `SuperStruct`. Voyage then crashed during
   initial game startup, before save loading, at `SecondsSinceStart = 0` with
   `EXCEPTION_ACCESS_VIOLATION` reading `0x00007feb957c3bc0`. Shipping did not
   emit the predicted textual `Bad export index 66559/5`; it dereferenced the
   invalid superclass reference instead. The prior exact package with a valid
   superclass loaded, while this candidate changed only that asserted index,
   so the result proves that the stock-path override wins and this Blueprint is
   read during startup. The crash marker was removed immediately through its
   exact manifest; no `VoyageCableUpdater*` containers remain installed.

Remaining gates before another feature experiment:

1. Choose a stronger but valid semantic CDO marker now that stock-path package
   loading is proven. Prefer a large TickInterval or disabling this actor's
   tick over changing components, class ancestry, or unrelated fields.
2. Separately repeat the unchanged layers on one SCS/component-rich Blueprint
   before trusting semantic edits to such assets.

Installed interval-probe hashes:

- `VoyageCableUpdaterTickInterval_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `VoyageCableUpdaterTickInterval_P.ucas`:
  `0149B497CC077AB3068F4DACEA93F19D7FB621C42EA87F61459490CEB90FEF90`
- `VoyageCableUpdaterTickInterval_P.utoc`:
  `03B92FF63D41CA06B654F0EEA3C46CA573A39E0F0FF15C9B28C91ABF0221D897`

Installation evidence is
`artifacts/tests/voyage-cable-updater-tick-interval-game-01/install-manifest.json`.

Installed controlled-crash hashes:

- `VoyageCableUpdaterBadSuperIndex_P.pak`:
  `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `VoyageCableUpdaterBadSuperIndex_P.ucas`:
  `5F9FC6C2A417C8F731328AD94060329B431D9A277CD352CC9351765E360E02C7`
- `VoyageCableUpdaterBadSuperIndex_P.utoc`:
  `AEBEC2A396C692AD7E55E635AEEFE20F94A5ED712D920B9CD1553855B1111135`

Controlled-crash installation evidence is
`artifacts/tests/voyage-cable-updater-bad-super-index-game-01/install-manifest.json`.

`/Game/Blueprints/BP_VoyageCableUpdater` is the mandatory known-hard canary:
developers report that current tools cannot process it correctly. Its
retoc-only roundtrip and separate UAssetAPI unchanged-save roundtrip both
passed in the game. This validates unchanged handling only. The separate
one-second TickInterval probe passed its narrow static gates and loaded, but
produced no obvious visual marker. The intentional bad-SuperIndex probe then
caused an immediate startup crash and was removed, proving the stock-path
package is live.

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
