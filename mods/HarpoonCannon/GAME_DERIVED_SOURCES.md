# Game-derived source registry

## Electrical buffer and debit signatures

Steam25191271 / UE5.8, executable
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
Revalidate on fingerprint change. Public enum/struct mappings, stock battery and
DistillationTower/FoodProcessor CDOs, and bounded native decoding establish:

- ModuleConfigData is a native struct; EVoyageModuleType Passive=0, Active=1.
  Source mirror is partial and requires tagged cooking, not ABI layout copying.
- VoyageModuleComponent.GetResourceAmount takes EModuleResourceType, returns
  double. RemoveResource takes Type, double RemoveAmount, RemovalType and returns
  bool. ConsumptionAfterModifiers=1 bypasses a second consumption modifier.
- Base debit checks balance and subtracts only on success, unless native
  overconsumption is enabled. Never use a HasPower flag as a debit receipt.
- SetCustomConsumption takes three doubles: InAcceptanceFilter,
  InMaxResourceAmount, InConsumptionON. It writes base configuration only.
  VoyageCustomModuleComponent overrides max capacity through its own maps;
  its use cannot be mixed with a base-only capacity setter.
- Native operation consumption divides W by 3600 times a runtime scale before
  applying elapsed seconds; electricity storage is Wh, not joules. Do not infer
  exact real-time supply rates from nominal W alone without the runtime test.

Evidence is ignored artifacts/harpoon-cannon/energy-*.json; bounded decoder
windows include base constructor14548B260, custom constructor14548A5C0,
RemoveResource1454B7690/1454B74D0, ConsumeOperationResources14549B690,
time factor1454A7C40, and SetCustomConsumption1454BB690. These are fingerprinted
research addresses, never hard-coded runtime calls. The mod emits reflected calls
and ships no mirror DLL. Integration and weak-grid timing remain runtime-pending;
current paired release manifests and test gates belong in the backlog.

## Runtime settings reader (current build revalidation)

Steam25191271/executable747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
VoyageEditorBlueprintFunctionLibrary.LoadFileToArray remains BlueprintCallable
static/native, flags0x04022401, FString InPath at0 and TArray<FString> return at16,
parameter size32. Bounded PE registration0x14b174110 and outer getter0x14524c1d0
confirm owner /Script/Voyage.VoyageEditorBlueprintFunctionLibrary. Evidence:
artifacts/harpoon-cannon/settings-reader-audit.json; method .py next to it, derived
from public executable-inspector string/pointer output, not an arbitrary scan.
Editor mirror never ships. Missing-file/parse behavior in this new consumer is
runtime pending; earlier autoload's reader acceptance is not new-version proof.
Revalidate on any game fingerprint change.

Current checkpoint: HC33 user-accepted on 2026-09-13. Fresh cleanup fingerprint
at23:05:01UTC still matches Steam25191271 and the hashes in the HC33 section.
Older dated entries are historical provenance, not current compatibility claims.
See [active backlog](../../docs/harpoon-cannon-backlog.md) for retained manifests;
retired extracted artifacts are recoverable by exact ZIP entry from the cleanup
archive. Their deletion does not justify consuming old inputs after a game update.

## HC33 display-only generalization, 2026-09-13

Fresh public fingerprint22:14:34UTC remains Steam25191271 / EXE
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B;
resolver selected reviewed mapping E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68.
All native mirror, entry, action, HUD and collision contracts unchanged from HC32.
User accepted HC32 entry/control/exit; no sharks nearby, their recognition not
retested. HC33 removes the display whitelist only and keeps exact reflected
VoyageModuleComponent.ItemAsset -> VoyageBaseDataAsset.Name optional lookup.
Engine GetObjectName supplies a runtime fallback before optional game fields;
Engine TextIsEmpty prevents replacing it with empty text. Signatures checked in
local UE5.8 headers. No new game native member, parent, physics or item schema.
Mouse scale is mod-owned 1.28 * 0.8 = 1.024. Revalidate native contracts on changed
fingerprint; tagged/cooked/independent and real-game gates remain required.

## HC32 correction: InteractiveInterface membership

2026-09-13 post-HC31 research, current Steam25191271/EXE747DC255...F58B:
common VoyageVehiclePawn's native FClassParams table contains six interfaces,
NONE InteractiveInterface; superclass is Engine.Pawn. HC29 editor inheritance
`APawn, IInteractiveInterface` is incorrect. HC31 station consequently contains
the correctly owned function but no explicit InteractiveInterface membership.
HC32 source removes fictitious base inheritance, explicitly implements the
interface on the own Blueprint, and gates both cooked membership and function
owner. Preserve all other current contracts. Evidence/method/revalidation on
fingerprint change: VEHICLE_ENTRY_RESEARCH post-HC31 section and
mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md. Fresh fingerprint06:45:35UTC and
reviewed mapping resolver confirm the same exact EXE and mapping E25620FB...A0F68.
Other native dependencies/HC31 collision contract unchanged. Independent cooked
verification and real-game entry remain required; source authoring is not validation.

## HC31 explicit collision delta, Steam25191271

Revalidated current fingerprint03:32:23UTC2026-09-13: Steam25191271 and full EXE
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B;
public mapping resolver again selected E25620FB...A0F68 before preparation.
All HC30 native identity contracts remain unchanged; only Engine collision-response
authoring/default comparison and a native Engine response getter are used.
Stock Steering JSON4EF724C2...63CB5 explicitly stores Interact Block. HC30 JSON
58434FD5...89029 lacks that named entry. Exact sources, UE5.8 default/delta semantics
and native channel14 evidence: VEHICLE_ENTRY_RESEARCH and selection tool report.
Editor-only default Ignore is a serialization device, not reconstructed game config.
Require fresh cooked explicit Interact Block and independent inherited/native audit;
read effective runtime response before claiming the fix. Revalidate on fingerprint change.

## HC30 interaction-component identity, Steam25191271

Fresh public fingerprint2026-09-13T00:25:38Z: same EXE747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B
and reviewed mappingE25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68.
HC29/28/27 dependencies therefore retain their recorded fingerprint, not a new
compatibility claim. Current mappings confirm InteractiveObjectComponent derives
SceneComponent, 20 own properties; FVoyageInteractType Direct=0. Bounded native
registration/constructor decoding confirms native default InteractType=0 and
null overlay/dialog references. Detailed method/evidence in hc30 tool report.

New InteractiveObjectComponent.h is IDENTITY ONLY, zero own reflected properties
or functions; runtime supplies its complete native layout/constructor. Tagged
SCS template may author ONLY inherited Engine transform properties. No native
InteractType/overlay/default deltas, no indexed serialization, no guessed ABI.
Stock Turret hierarchy Root -> InteractiveObject -> InteractBox is the donor
contract, not donor physics/UI. Provider parameter is the interaction object,
not query shape. Revalidate identity/default/hierarchy on fingerprint change.
Independently inspect cooked class/template/ChildNodes and provider reference
before installing. Shell, occupied camera/inputs/HUD/exit stay unchanged.

## HC29 contextual entry, Steam25191271

Fresh public fingerprint2026-09-12T23:07:27Z still EXE
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B;
reviewed mapping E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68.
HC28/HC27 inputs retain the SAME game fingerprint. No new game compatibility
claim. HC29 report records current Drone/Turret interface+delegate JSON evidence.
Renewed one-function InteractiveInterface mirror uses exact interface owner.
Historical VehiclePawn inherited-interface assertion was WRONG; superseded by
the blocking post-HC31 membership correction above. Do not reuse that inference.
VoyageInputActionInstance extends Engine.InputActionInstance with Controller;
PlayerInputInterfaceOnAction signature has const input instance and OUT Text.
Only named bytecode fields/delegate binding, never native struct CDO or indexed
serialization. Native reflection cache/non-reflected ABI is NOT reconstructed.
Own Engine Box query uses current Interact channel1 / Interactive object2,
no overlaps, physics, visibility or camera block. Common native root remains
runtime NoCollision/non-simulating. SCS adds this ONE own Engine component;
no native component template/field overrides. Shell package remains unchanged.
Callback/selection/lifetime still require runtime validation; current evidence
does not prove the common vehicle's interaction acquisition accepts this box.

## HC27 optics/range renewal, Steam25191271

Public2026-09-12T05:21:07UTC fingerprint and reviewed mapping match the HC26
identities below. Public inventory remains23983 packages, SHA
12A70BF6D3EF38CC37D7FAC43AD46FB6BF1DAC61BFFA976815BF6DAB70903605.
Search found the same3 shark NPC/data identities; no fourth named variant.
Public Components summaries followed by required native CDO JSON inspection
renew module ItemAsset links; all3 NPC data JSON hashes match HC14 below.
Public mappings revalidate ModuleComponent ItemAsset7 ObjectProperty and
BaseDataAsset Name1 TextProperty, exact declared native owners. They remain
read-only mirrors, never instantiated or serialized native assets.
Reuse HC14 trace/classifier without broad through-obstacle selection. Output
owner changes to station text fields consumed by own HUD. Camera FOV uses
Engine-only angular5x calculation sampled from on-foot manager before entry.
Same9 tagged HC26 serialization gates apply; new HUD has own text widgets and
Tick graph, no Voyage-native property deltas. Runtime HC27 remains pending.

## HC26 tagged dedicated operator experiment, Steam25191271

2026-09-12T04:30:27Z public fingerprint renewed: executable
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B;
reviewed UE5.8 mapping E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68.
Public mappings:VoyageFastSceneComponent confirms StaticMeshComponent parent,
zero own properties. Current stock Forklift root VehicleMesh uses that class.
Partial common VehiclePawn mirror constructs only this exact root to avoid an
invented DefaultSceneRoot; shipping native constructor supplies other objects.
No native CDO field overrides are intended. Full vehicle38/widget47 schemas
are NOT reconstructed. Exact common enter/exit and widget-interface identities
retain the current-build evidence below and in VEHICLE_ENTRY_RESEARCH.md.

HC26 authoring exception uses ordinary tagged properties: UE5.8.2
CookCommandlet enables unversioned only for UNVERSIONED switch; SaveContext
sets SAVE_Unversioned_Properties conditionally. Producer omits that switch,
reopens all nine headers and rejects PKG_UnversionedProperties before retoc.
Independent cooked Mod JSON is required for correct own fields/parent/root and
absence of unintended native CDO/subobject deltas. Tagged package acceptance
and native widget lifecycle remain runtime questions, not compatibility claims.
Own Engine CameraActor uses reflected GetComponentByClass(CameraComponent),
not its non-UFUNCTION C++ GetCameraComponent accessor. Inputs are real Enhanced
Input events, no raw mouse polling. Revalidate all on fingerprint/engine change.

## HC24 native camera request, Steam25191271

2026-09-12 public fingerprint and mapping match EXE747DC255...F58B and
mappingE25620FB...A0F68 below. Fresh public mappings:VoyageVehiclePawn confirms
CameraComponent[6] ObjectProperty, bAllowFirstPersonCamera[10] BoolProperty,
bFirstPersonCamera[25] BoolProperty. Current stock Forklift CDO JSON
529CFD50ABA375678CF1156A3C9409D082BE15A4A696B25C7C37F2DAE6D2E045 identifies
Engine.CameraComponent, CameraAttachment/SprintArm chain and allow-first=false.
Field-reference-only mirror, NEVER native child/CDO serialization.

Public native name report executable-ff14f6137e774c9d99021a253a650574.txt locates
first-person descriptor149b73c60, bool setter145297590. Bounded32byte decode
hc24-camera-boolsetters.json verifies native state byte actor+0x3e8. This
establishes the field, NOT a complete mode-switch side-effect contract.
Exact reflection names were sufficient for this bounded request experiment;
no guessed native function was mirrored/called. CameraComponent FOV uses real
Engine UCameraComponent.SetFieldOfView. Observe manager GetFOVAngle to verify
actual gameplay result. Revalidate native ownership/type and stock camera
chain on game fingerprint change. Runtime first-person/optics pending.

## HC23 native input references, Steam25191271

2026-09-12 UTC public fingerprint/mapping renewed: same EXE747DC255...F58B and
mappingE25620FB...A0F68 as HC22 below. Fresh stock Forklift JSON
529CFD50ABA375678CF1156A3C9409D082BE15A4A696B25C7C37F2DAE6D2E045 plus public
mappings confirm VehiclePawn.InputControls[0] typed VoyageInputControlsComponent;
that class derives from Engine ModularGameplay.PawnComponent and its
InputContextAsset[0] points to VoyageInputContextAsset : Engine.DataAsset.
Forklift own LookRightInputAction[20]/LookUpInputAction[21] use InputAction type,
with stock IA_LookRight/IA_LookUp paths under /Game/Game/Input/Character.
Exact field owners/types referenced, not partial native CDO serialization.
Engine plugin dependency used only to compile matching parent/class identities.

Stock /Game/Game/Input/Vehicle/DA_Input_Context_Forklift JSON
97B117039B669C756E4F4270067EAA92D774210F36EB4990119AC68C645D0EF6 links keyboard
and gamepad mappings with priority10. Keyboard JSON
2DAA05735790280B0CB1FC044B3EF182A7D74790E4A59F6C63CDD1BE2E06497A has mappings
under Properties.DefaultKeyMappings.Mappings: MouseX -> IA_LookRight, MouseY ->
IA_LookUp (negate modifier), E -> IAV_VehicleExit (pressed trigger). This proves
configuration, not live activation/handler consumption. No stock asset modified
or copied. Runtime load/readback for all new references required before entry.
Assign before FinishSpawning to let native lifecycle own input setup; no call
to incompletely reconstructed SetupPlayerControls. Renew all identities, native
field owners/types and stock context mappings after fingerprint change.

## HC22 stock ExitAction dependency, Steam25191271

2026-09-12 UTC fingerprint and public mapping resolver renewed, identical to
HC21 below (EXE747DC255...F58B, mappingE25620FB...A0F68, UE5.8/editor5.8.2).
Inspect-VoyageAsset mappings:VoyageInputAction confirms InputAction superclass;
type-only UVoyageInputAction mirror omits Description and must never serialize
an authored asset/CDO. Engine EnhancedInput supplies real UInputAction parent.
Forklift field-reference mirror adds only VoyageInputAction ObjectProperty
ExitAction, native descriptor149b5b0d0 at actor+0x558. HC21 crash analysis in
VEHICLE_ENTRY_RESEARCH.md identifies its unguarded native action-provider use.
Stock Forklift CDO JSON SHA529CFD50ABA375678CF1156A3C9409D082BE15A4A696B25C7C37F2DAE6D2E045
sets /Game/Game/Input/Vehicle/IAV_VehicleExit.IAV_VehicleExit, whose stock JSON
SHA14AC9D8C3D3FC392A24B074875FCA7B9A87901C194E3364F5A8E9F8E684A8662
confirms VoyageInputAction. Runtime stock load/cast, exact-owner set and pointer
readback only; no stock asset copied or incomplete native CDO/child generated.
Position, physics, tag, native calls and HUD class selection unchanged HC21.
Revalidate field owner/type/offset, action path/type and parent on fingerprint
change. A successful build is not proof of complete Forklift setup or working E.

## HC21 combined operator point/native Forklift HUD discriminator

2026-09-11 fingerprint Steam25191271, UE5.8, executable SHA256
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B,
reviewed mapping E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68.
Public resolver/fingerprint renewed. Native ctor145409680 verifies
VoyageVehicleForkliftPawn : VoyageVehiclePawn; both inherit same entry/exit.
New Forklift header is TYPE ONLY, runtime-created shipping class, no CDO/child.
Base header adds only field-reference NameProperty ExitComponentTag (mapping[9],
native descriptor149b72d30 offset0x3a0). It remains unsafe for CDO authoring.
Existing native entry signatures unchanged. Engine ComponentTags uses real
Engine UActorComponent declaration; root is an owned new runtime component.
Current stock BP_VoyageHUD SHA C933A572CE80BB75C2E21D23DA0FAF0A10A5494BF19DE9FA56E48D9D7B8A1938
maps Forklift native class to stock HUD. No HUD asset is copied/reconstructed.
Constructor/callback/tag helper evidence in VEHICLE_ENTRY_RESEARCH.md and
mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md. Renew these exact
identities and class map after any fingerprint change. Runtime pending.

## HC20 existing canonical Forklift target, Steam25191271

Fresh fingerprint and reviewed mapping match HC19 identities below. Public stock
Get-VoyageAssetSummary BP_Forklift_Possesable Functions/Calls renewed canonical
class path, native Forklift parent and virtual OnEnterVehicle/OnExitVehicle calls.
Summary529CFD50ABA375678CF1156A3C9409D082BE15A4A696B25C7C37F2DAE6D2E045,
SHA7D617E64C03983B7BDCDFED953D7A1551D3163742289BC35780F27524D6D54FD.
HC20 references existing actors by class path then casts to common native base;
no relocated snapshot, native Forklift child/CDO, input structs or fields copied.
Current installed DonkLift may own canonical class; not overwritten or inherited
by this probe. Prior DonkLift logs/docs on25056839/23962331 are hypothesis sources,
not current compatibility evidence. Renew target/parent/call contracts on update.

## HC19 native vehicle TYPE/CALL ONLY, Steam25191271

EXE747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B,
reviewed mappingE25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68,
parserUE5.8/editor5.8.2. Current native registration/bounded metadata prove
VoyageVehiclePawn directly Engine.Pawn; OnEnterVehicle/NewPossessor is one
Engine.Controller ObjectProperty, void; OnExitVehicle no arguments, void.
Both UFunction flags0x0c020c00: public native BlueprintCallable/BlueprintEvent.
Property descriptor file9b6f450 -> name9b13560 NewPossessor; type registrar
1429646c0 -> UTF16 Controller at file8f6e660. Legacy FFunctionParams layout from
installed Engine UObjectGlobals.h; VEHICLE_ENTRY_RESEARCH.md owns interpretation.

AutoloadProbe VoyageVehiclePawn.h is class/call identity ONLY. NEVER generate a
child/CDO or access omitted38 native fields. Game instantiates its complete
native class; emitted assets contain only import, references/cast and native
calls. No editor DLL ships. Constructibility/default-subobject/entry safety need
runtime evidence. Renew identity/signatures/flags/parent/no-child gate on update.

## HC18 native HUD interface and static widget identity, Steam25191271

hc18-hud-fingerprint.json: executable
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B,
UE5.8 parser/editor5.8.2. Public stock JetSki JSON8BFF20D6...D5CAD1 shows
VoyageActorWidgetInterface and GetHUDOverrideWidget no inputs, return class of
VoyageBaseUserWidget. Current native registrar145216af0 calls class constructor
1451fba80 (VoyageActorWidgetInterface) and registers1name/thunk pair14996acd0:
GetHUDOverrideWidget /1452203e0. Read-only bounded decoded windows supplement
public executable inspector report66152b08cdef45a59c110f0f05c5e5fd. These VAs are
build-bound evidence, never runtime offsets. Native consumer timing awaits marker.

HC18candidate01 REJECTED before install: empty base stub despite zero own BP
fields produced invalid CDO bPlayerScreen/bRemoveWidgetAfterHideAnim/SoundData
(self-reference). Do not assume native fields can be omitted when subclassing.
Candidate02 interface return-type identity ONLY; null returned, no new native
widget instance/subclass in package. Native widget creation remains blocked
until full schema or another independently verified authoring path is available.

Editor interface exact /Script/Voyage identity, BlueprintNativeEvent returning
TSubclassOf<UVoyageBaseUserWidget>. Base widget mapping: UserWidget parent,47own
properties (hc18-widget-schema public report). Identity-only stub may be used
only for referenced return metadata, never widget generation or field access.
Revalidate registration, return type, parent/schema and no-derived-data gate
after fingerprint change. Never ship the editor native DLLs.

## HC17 engine Pawn handoff, Steam25191271

hc17-implementation-fingerprint.json matches747DC255...F58B/current reviewed
mapping. No new Voyage mirrors/data inputs: current shell/range prefixes remain
unchanged. New APIs are Engine Controller.Possess(InPawn), K2_GetPawn,
Pawn.GetController, authority/local checks, and Pawn auto-possession/rotation
defaults, checked against installed UE5.8.2 headers. ModActor root changes from
Actor to Engine Pawn; no native vehicle methods or structs are reconstructed.
Current stock Forklift/Steering OnEnterVehicle and HUD class maps are research
references only, not called by HC17. Vehicle-equivalent cleanup is runtime-pending.

## HC16 character HUD root identity, Steam25191271

Fresh hc16-fingerprint.json confirms same build/executable as HC15. Public JSON
BP_VoyageIngameHud3DE79084561BA7881905BC3075966BBE1030BD51CE393B56D1A33D0312713D23
reused via public resolver; parent VoyageInGameCharacterWidget. It contains
ActionBarWidget, ContextInputActionContainer, EquipmentActionRoot and CableOverlay.
VoyageHUD CDO separately maps VoyageCharacter/EHudWidgetType::Hud to this class.
New HC16 identity is /Game/UI/Game/HUD/BP_VoyageIngameHud.BP_VoyageIngameHud_C,
loaded soft-class lookup only; no copied Blueprint/native mirror. Revalidate on
update. User HC15 screenshot proves optical overlay coexists with native actions,
not that native input providers have stopped or all UI has one rendering owner.

## HC15 Playing HUD identity, Steam25191271

Fresh hc15-fingerprint.json matches747DC255...F58B. Public stock JSON:
BP_PlayingWidget819F05140B918FF3099361592851ABCAB1582D5B2CE1D58F829991A202390EEB,
BP_VoyageHUD C933A572CE80BB75C2E21D23DA0FAF0A10A5494BF19DE9FA56E48D9D7B8A1938,
BP_VoyageIngameHud3DE79084561BA7881905BC3075966BBE1030BD51CE393B56D1A33D0312713D23.
VoyageHUD CDO GameStateTopWidgetClasses maps Playing to exact
/Game/UI/GameStateWidgets/BP_PlayingWidget.BP_PlayingWidget_C. Widget parent is
VoyageGameStateWidget; serialized children include PawnOverlay, quests, battery,
messages, subtitles. HC15 resolves only that already-loaded class and reads/
sets existing UWidget visibility through Engine APIs. No native mirror added.
Revalidate identity/hierarchy/visibility lifecycle after game update; static tree
alone does not prove all native in-game UI belongs beneath this widget.

## HC14 classifier and display-name read-only prefixes

Steam25191271/747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B,
public hc14-fingerprint.json. Full23983 package inventory and fresh shark CDOs:
BP_NPC_Shark/RamShark/Shark_Laser all have PlayerModuleComponent of exact native
VoyageModuleComponent class with ItemAsset pointing respectively to
DA_NPC_Shark/DA_NPC_RamShark/DA_NPC_LaserShark under /Game/Data/NPCData.
Module prefix through ItemAsset7 was revalidated in hc13-25191271-audit.
New hc14-name-audit public mapping: VoyageBaseDataAsset:PrimaryDataAsset has7own
fields, Type0=PrimaryAssetType and Name1=FText. Only these read-only prefixes are
mirrored in AutoloadProbe/Source/Voyage. Never instantiate/serialize them or ship
native binary. They declare exact runtime owners for field reads, not overrides.
Data source names: Talon Shark; Ram Shark; Talon Shark – Dorsal Model. Runtime HUD
reads FText, not these literals. Fresh data JSON SHA C20019D8997AA5FE1C56ECEB9CE8E7D4BC12B0F33ABC11FD4C4446639D00B213,
7E1D7DE8D8B2758459F555337F37204941342413C844852A9E725255BAD6123B,
BE300CC8C4AFFC3E3ED324AE1062B7F79ECCF2AECB06C590492562C9596B8CB7.
Revalidate inventory/field owners/layout and data identities after fingerprint
change. Known3 matches do not prove no future/unrelatedly named shark exists.

## Current shell and HC13 renewal: Steam25191271,2026-09-11UTC

Public resolver accepts reviewed mapping E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68
for executable747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
Engine targetUE5.8, editor5.8.2; game patch version not independently established.
Own audit artifacts/harpoon-cannon/hc13-25191271-audit: fresh public mapping reports
for module actor/component/custom/dynamic/persistent/destructible/item hierarchy.
Actor4 fields unchanged; module69 with ItemAsset7; custom20 fields/order unchanged;
dynamic2 bools unchanged; persistent/destructible each10, ActorComponent parents.
Item:VoyageItemBase:VoyageBaseDataAsset identity retained. Only previously reviewed
shell-prefix usage is renewed, not dormant vehicle/operator/interface definitions.

Fresh stock JSON small turbine/Cyclone/item hashes are EXACTLY the prior three
hashes listed in the historical shell section below. Named RF_DefaultSubObject
templates and classes, custom module override and ItemAsset identity unchanged.
This renews serialized identities/layouts, not all native runtime implementation.
Shell validation still must reject executable Blueprint functions, unexpected
native properties and changed base collision. No geometry or behavior edits.
HC13 is Engine-only; public producers must extract NEW scriptobjects for25191271.
Native camera/fixation/loader runtime must be retested before compatibility claims.
C8 is a separate Blueprint loader, unchanged external prerequisite, not bundled.
Descriptor ABI retained from owning STATE.md; runtime on25191271 not yet proved.

Revalidation applies only to new builds, not metadata edits of old release files.

## HC13 cannon-local aiming boundary

Public hc13-fingerprint.json2026-09-10 renews Steam25056839/executable
CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933,
game5.8.1/editor5.8.2. No new Voyage mirror/asset identity. Mod-authored shell root
coordinates reused read-only; camera point(0,0,180) is provisional tuning.
Engine5.8.2 PlayerController.GetInputMouseDelta and Actor relative setters supply
input and local transform writes; Kismet NormalizeAxis/FClamp own bounds. Engine
SceneViewport MouseDelta.Y subtracts screen CursorDelta.Y. Native sensitivity/
inversion is not copied. Revalidate loader/scriptobjects/shell after fingerprint
change; runtime local-follow and raw mouse interaction still require testing.

## HC12 owned camera boundary

Public fingerprint2026-09-10UTC: Steam25056839/executable
CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933,
artifacts/harpoon-cannon/hc12-fingerprint.json; game5.8.1/editor5.8.2 unchanged.
No new Voyage mirror or stock asset reference. Engine5.8.2 headers establish
Controller.GetViewTarget/GetControlRotation, PlayerController.SetViewTargetWithBlend,
CameraComponent.SetFieldOfView and Actor.bHidden (IsHidden is C++ only).
Own CameraComponent/UMG reticle use ordinary Engine/UMG script identities; new
generator code is mod-authored. Autoload and scriptobjects remain game-derived
inputs renewed by the public producer. Revalidate these and shell identity after
fingerprint change. Native camera-manager behavior remains runtime-pending.

## HC11 fixation boundary

Public fingerprint2026-09-10UTC renewed Steam25056839 and executable
CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933;
artifacts/harpoon-cannon/hc11-fingerprint.json. Same gameUE5.8.1/editor5.8.2.
No new Voyage mirror or class import: trace target class path and mod-authored
ModuleMountCollision root name identify the unchanged accepted shell. After any
fingerprint change revalidate that shell, the class path and autoload C8/scriptobjects.
Engine5.8.2 source establishes SetMovementMode(None) clears floor/base, velocity,
jump state and forces; Walking finds floor/base again. Attachment explicitly welds
FALSE; character collision alone is temporarily disabled. Engine evidence is not
proof of all Voyage native overrides or runtime safety. HC11 runtime is pending.
Retained HC10 now has a user's three-state snapshot, recorded in its tool report.

## HC10 observer boundary

Same Steam25056839/gameUE5.8.1/executable fingerprint below, renewed by public
producer2026-09-10UTC. Engine5.8.2 headers establish reflected
CharacterMovementComponent.GetMovementBaseObject and MovementMode, Actor.GetTransform
(not C++ helper GetActorTransform), SceneComponent.GetAttachParent. HC10 generates
only Engine/UMG readers and own widget/state writes; no native Voyage mirror,
stock class reference or stock Blueprint replacement. Autoload C8 contract and
current extracted scriptobjects remain the only game-side preparation inputs.
Current cooked inspection validates imported getter/field owners, not actual
helm-state semantics. Runtime observation is pending; renew inputs after updates.

## Current shell-only revalidation: 2026-09-10 UTC

Only `GenerateHarpoonCannon -ShellOnly` is enabled on Steam25056839 / game
UE5.8.1 / editor5.8.2, executable SHA256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
Public fingerprint, reviewed mappings and current stock JSON were renewed.
Audit manifests: `artifacts/harpoon-cannon/shell-return-audit/`.
No operator, vehicle, interface, HUD or shark mirror is renewed by this gate.
Historical helper code remains compiled but cannot be selected/emitted by Main.

Consumed native contracts, reconstructed via Inspect-VoyageAsset mappings and
Get-VoyageAssetJson stock exports (not inferred from the old package):
- VoyageModuleActor : Actor has the same four own fields in order:
  bCollectable, ModuleComponent, PersistentComponent, DestructibleObjectComponent.
- Current small-turbine CDO has all three exact named default subobjects with
  RF_DefaultSubObject, corresponding component classes, and a custom-module
  override. This revalidates the constructor identity contract without native
  disassembly; no claim about the constructor's full implementation.
- VoyageDynamicCollisionComponent : ActorComponent has bAutoWeld then
  bAutoweldIgnoreNormal. Stock small turbine serializes bAutoWeld=true.
- VoyageCustomModuleComponent retains20 own fields and the parent
  VoyageModuleComponent retains69 own fields; ItemAsset is parent field7.
  The retained mirror represents the prefix through ItemAsset only. Custom
  container inner types remain alignment-only placeholders and MUST NOT emit
  nonempty values. Cooked verification must reject every custom-module serialized
  field except ItemAsset and every unrenewed inherited/component property.
- VoyagePersistentActorComponent and VoyageDestructibleObjectComponent still
  derive from ActorComponent and each have10 own fields. Their mirrors are
  identity-only empty templates: no properties may be serialized in this probe.
- VoyageItem : VoyageItemBase : VoyageBaseDataAsset : PrimaryDataAsset remains
  the reference-stub hierarchy. Stub is never cooked or packaged.
- Current Cyclone ModuleComponent points to
  /Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium; leaf remains
  /Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New.

JSON hashes: small turbine
`B56C5CE3365AF790B191B761255F36661877634E7D5249F6BAD9CCA494052BBA`,
Cyclone `A63A55332C0701CAEABF1C48EB60E12875D069E9DA1225B805FE1ADC75D756A8`,
item `7AA676CD6DA61E1D6899CF271DE9BA396CBA649271702168FFFA00C81BBE5A62`.
Revalidate these consumed layouts, class/DSO/item identities and scriptobjects
after any fingerprint change. Current construction/gameplay remains pending.
The old fingerprint below owns historical unused definitions, not this scoped
shell renewal; do not interpret the entire registry as current-compatible.

## Retained non-shell source gate: historical, build blocked pending revalidation

Restart on 2026-09-05 found installed Steam build `25056839`, executable hash
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
The public resolver returns reviewed mappings successfully. `RESEARCH.md`
records partial current-stock inspection; this does NOT renew the serialized
native definitions below. Keep their old fingerprint explicit until each
affected contract is revalidated, then separately migrate and verify authoring.
An update invalidates evidence; it is not by itself proof of incompatibility.

## Original source fingerprint

HC08 renewed fingerprint on2026-09-07 UTC: same25056839/executable below.
Public summaries resolve BP_FirstPersonCharacter_New -> BP_Base_Character ->
/Script/Voyage.VoyageCharacter. Blueprint indexes expose no direct walking/jump
input handler; exact native behavior remains a runtime question. Engine5.8.2
source confirms Controller.SetIgnoreMoveInput is a stacked counter and Pawn
movement input respects it unless forced; Character jump checks do not generally
depend on that counter. HC08 emits only Engine/UMG calls to gate the original
controller, not a stock-class reference/override or native Voyage mirror.
Current player extraction supplies scriptobjects only. No Drone runtime reference.

The separate `AutoloadProbe/` HC01-HC07 experiments do not consume the retained
native definitions below. Its engine-only generator targets editor UE5.8.2
and current game build 25056839 with the executable hash above. The only game
class reference is the current-stock inspected soft class
`/Game/Blueprints/Vehicles/BP_CameraDrone.BP_CameraDrone_C`; no game Blueprint
snapshot is shipped. Extraction supplies current `scriptobjects.bin` only to
retoc. Renew this reference, script-object input and autoload contract after
fingerprint changes. Build manifests record tool/mapping/source hashes; static
success cannot establish native spawn/persistence or interaction safety.
Subsequent HC01/HC02 user screenshots validate bounded lifetime and natural
stock action hints only; HC03 subsequently validates native entry and exit.
HC04 re-inspected this same fingerprint through public summaries and stock
inspection: Drone RootComponent and MeshComponent both resolve to VehicleMesh;
look handlers require active and not snapped. The sole new runtime call is an
Engine PrimitiveComponent simulation toggle, guarded by a runtime root cast.
No VoyageFastSceneComponent mirror or assumed native field layout is generated.
Full cleanup, physics-toggle behavior and save safety remain pending.

HC04 subsequently passed the requested mouse/blocked-translation/exit behavior.
HC05 adds only Engine trace/hit-component and attach/detach calls, with dynamic
root/support validity checks. It introduces no new game class or reconstructed
native layout. TraceTypeQuery1 uses the default Visibility slot; actual deck
acquisition is a runtime discriminator, not a renewed game collision-profile
contract. Missing support refuses mutation. Component display names are only
diagnostic, never a Boat identity classifier. Native ship/module welding and
save ownership are not implemented by this component attachment experiment.

HC06 changes only diagnostic presentation to a mod-authored Engine/UMG UserWidget,
adding /Game/Mods/HarpoonCannonLifecycleProbe/ProbeHUD to exact inventory. No new
game-derived asset or native mirror; same game fingerprint gate and Drone soft
class. Stock HUD/input contracts untouched. Display and startup behavior require
real-game validation independently of static widget compile/cook checks.

HC07 renewed the fingerprint and stock Drone function summary/pseudocode on
2026-09-06 UTC: same build/exe. OnDriverChanged activates, starts snap-state
sampling and changes actor/camera rotation; the no-driver branch clears timers,
restores visibility and updates buoyancy/input. OnDroneExit Blueprint plays a
sound. SetDroneActive changes gravity/collision/visual deployment, not an explicit
SetSimulatePhysics in that function. Native internals are not thereby proven.
HC07 uses the same Engine attachment calls once before entry, observing native
overrides rather than changing the stock Blueprint or reconstructing its types.

- Steam build: `23962331`
- Unreal Engine mapping version: `5.7.4`
- `VoyageSteam-Win64-Shipping.exe` SHA-256:
  `6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`
- Fingerprint tool: `../../tools/Get-VoyageBuildFingerprint.ps1`

Any fingerprint change invalidates every entry below until it is re-inspected
and the backlog records the new evidence.

## Reconstructed editor-only identities

- `Source/Voyage/VoyageVehiclePawn.h`
  - identity: `/Script/Voyage.VoyageVehiclePawn : /Script/Engine.Pawn`
  - evidence: current mappings and the inspected forklift, JetSki, and
    GyroCopter Blueprint parent chains
- `Source/Voyage/VoyageBaseUserWidget.h`
  - identity: `/Script/Voyage.VoyageBaseUserWidget : /Script/UMG.UserWidget`
  - evidence: `GetHUDOverrideWidget` return metadata and current widget class
    mappings
- `Source/Voyage/VoyageDynamicCollisionComponent.h`
  - identity: `/Script/Voyage.VoyageDynamicCollisionComponent :
    /Script/Engine.ActorComponent`
  - current mapping exposes exactly `bAutoWeld` followed by
    `bAutoweldIgnoreNormal`; Cyclone's SCS root component sets only
    `bAutoWeld=true`
- `Source/Voyage/VoyageModuleActor.h`
  - identity: `/Script/Voyage.VoyageModuleActor : /Script/Engine.Actor`
  - the current mapping exposes four own properties (`bCollectable`,
    `ModuleComponent`, `PersistentComponent`, and
    `DestructibleObjectComponent`)
  - targeted disassembly of the gated executable confirms that the native
    constructor creates the three named component fields as default subobjects;
    the base turbine generated class then overrides the inherited
    `ModuleComponent` template with `VoyageCustomModuleComponent`
  - the current editor-only mirror reproduces that actor-level field order,
    false `bCollectable` default, named default-subobject construction, and
    Blueprint component-class override
- `Source/Voyage/VoyageModuleComponent.h` and
  `Source/Voyage/VoyageCustomModuleComponent.h`
  - identities:
    `/Script/Voyage.VoyageModuleComponent : /Script/Engine.ActorComponent`
    and `/Script/Voyage.VoyageCustomModuleComponent :
    /Script/Voyage.VoyageModuleComponent`
  - current mappings provide the exact eight-property base prefix through
    `VoyageModuleComponent.ItemAsset` and the exact 20-property derived prefix
    of `VoyageCustomModuleComponent`
  - the mirror reproduces those property counts and order so the generated
    unversioned `ItemAsset` value lands at the runtime index; derived container
    inner types are editor-only alignment placeholders and are never serialized
  - cooked Cyclone CDO inspection resolved the value at runtime identity
    `/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium.0` and confirmed
    that no neighboring derived field is accidentally overridden
- `Source/Voyage/VoyageItem.h`
  - identities:
    `/Script/Voyage.VoyageBaseDataAsset : /Script/Engine.PrimaryDataAsset`,
    `/Script/Voyage.VoyageItemBase : /Script/Voyage.VoyageBaseDataAsset`, and
    `/Script/Voyage.VoyageItem : /Script/Voyage.VoyageItemBase`
  - current mappings and the extracted turbine items establish this inheritance
    chain; the class is used only to author a project-local reference stub at
    the exact item path, and that stub is excluded from cook and packaging
- `Source/Voyage/VoyagePersistentActorComponent.h` and
  `Source/Voyage/VoyageDestructibleObjectComponent.h`
  - class identities and native default-subobject names are confirmed by the
    current mappings, original Cyclone CDO export, and module-actor constructor
    disassembly; their internal runtime schemas remain intentionally absent
- `Source/HarpoonCannonGenerator/CannonAssetNames.h`
  - current shark data identities from current package inventory and inspected
    `PlayerModuleComponent.ItemAsset` values
  - current leaf probe identity:
    `/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New`,
    confirmed by `DA_Item_Module_WindTurbineMedium.DroppedActor` and the
    inspected Blueprint package; package inventory found no child Blueprint
    depending on this leaf path
  - current leaf item identity:
    `/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium`, confirmed by
    `DA_Skill_Engineering_Cyclone_Wind_Turbine`, the item export, and the
    original medium-turbine component CDO
  - the rejected Whisper experiment used the parent
    `/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Small`; replacing
    that package broke its untouched medium-turbine child with `Bad export
    index 4607/19`, so it must not be a packaged identity again
  - future loaded connector identity:
    `/Game/AssetSets/Sockets/SM_Mooring_CableSocket_Out`, freshly confirmed by
    exact package extraction under the current fingerprint; the first restored
    leaf visual checkpoint deliberately keeps this hard reference disabled and
    includes only the mod-authored connector proxy in the pitch mesh
  - the generated `/Game/Mods/HarpoonCannon/SM_HarpoonCannonPlaceholder` cube
    was a historical mod-authored visual marker and contained no game-derived
    data
  - the current `SourceAssets/harpoon_cannon_blockout.obj` and its generated
    base/yaw/pitch meshes are mod-authored visual data, not game-derived data;
    source hashes and coordinate contract are recorded in
    `SourceAssets/README.md`
  - first-person FOV is a mod tuning contract, not extracted game data

These native definitions exist only to compile generated Blueprint assets.
Their binaries must never be included in a mod container; the running game
supplies the real `/Script/Voyage` classes.
## HC25 dedicated input authoring gate, Steam25191271 (2026-09-12 UTC)

Fresh Get-VoyageBuildFingerprint: Steam25191271, executable
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
Get-VoyageMappings: reviewed UE5.8 mapping
E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68.
Editor5.8.2. Public schema evidence under ignored artifacts/harpoon-cannon/
hc25-schema/steam-25191271-747DC2553F7E-game/mappings_VoyageInputContextAsset.
Current stock context JSON97B117039B669C756E4F4270067EAA92D774210F36EB4990119AC68C645D0EF6
confirms map values InputMappingContext and modifier VoyageInputAction types.
Published CUE4Parse reflection recovered missing enum identities from that same
reviewed mapping after Inspector Describe erased EnumName with InnerType.
Context7 fields/order: InputMappingContext (EInputDeviceType -> InputMappingContext),
ContextType (EInputControlContextType), BindType (EInputControlBindType),
InputPriorityOffset (int), ShiftAction/CtrlAction/AltAction (VoyageInputAction).
Enums: device Unknown0/MouseKeyboard1/Gamepad2; context Default0/Interface1/System2;
bind Default0/Elevated1/Demoted2/Ignore3. Terminal native MAX values use normal
UHT generated sentinels. No modifier action object/CDO is instantiated.
All five assets are mod-authored, not copied from the stock context. Only context
serialization is under test; base vehicle/widget mirrors remain forbidden for
child authoring. Renew schema/types/enum values and cooked semantic audit after
any fingerprint change. Never ship editor DLLs.
# HC28 action-hint extension provenance

2026-09-12 fingerprint06:58:27UTC: Steam25191271, UE5.8,
EXE747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
Reviewed mapping E25620FB2D0674B26A3E4EF059AC2C5AF58E28CE1B4F07307860F7FADB9A0F68.
Base-owned GetProvidedActionsBP registration is already decoded on this exact
fingerprint (VEHICLE_ENTRY_RESEARCH.md, registrar145369f30, thunk145374620).
DonkLift's older proven graph is precedent, not current provenance by itself.
Public current mappings confirm PlayerInputInterfaceAction first17 named fields
plus six delegate fields; Central=0/Context=1/Hidden=2. Only named bytecode
MakeStruct fields used; no native struct default/CDO serialization permitted.

Public stock JSON renewed:
- BP_DynamicPlayerInputHorizontalWidget:
  3C3F195927F1A51416A91696C57D9178AEC12AF9F8CD4CCB2F28CBD2FD827AB7.
  Parent VoyageDynamicPlayerInputWidget; native tick, ContextInputActionsRoot
  horizontal box and stock WBP_InteractIndicator identity. No stock copying.
- BP_VoyageIngameForklift:
  174013697A2B72635ABC275486AFB35011E42256837E196C6703E19BD714973F.
  Bottom instance ContextAsset points to matching Forklift context, filtertrue,
  Central default. HC28 supplies own context to runtime stock widget before
  AddChild; no Forklift context, HUD or physics used.
Native widget mapping: VoyageBaseUserWidget parent, eight own fields; ContextAsset
and bFilterByActionType field-only mirror, never instantiated/cooked as native
child. Public evidence paths in mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md.
Revalidate all contracts after EXE/mapping change. Runtime discovery still pending.

## Electrical socket authoring contract

Steam25191271, UE5.8, EXE
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
Public Inspect-VoyageAsset mapping queries confirm:
- VoyageModuleSocketViewComponent : ModuleSocketComponent : BoxComponent.
  DataAsset is SoftObjectProperty on the view; SocketID (UInt32),
  bAutoInitialize and Port (ModuleSocketIOData) belong to ModuleSocketComponent.
- ModuleSocketIOData.DefaultDirection uses EModuleSocketType; ST_Input=1.
- VoyageModuleComponent owns bUseSocketCustomTarget and SocketCustomTarget
  (Engine.ComponentReference). The target must name the actual socket component.
- Consumption maps use EModuleResourceType enum keys (Electricity=0), double values.

Stock FoodProcessor confirms electric DataAsset path and input DefaultDirection.
The generator writes a soft path directly: it must neither load stock assets
into the editor nor instantiate UObject/DataAsset reference stubs for that path.
These partial native mirrors REQUIRE tagged properties. Build-Shell verifies
every inventory package header after cook; Validate-Shell independently reads
the candidate container and checks socket reference, input direction and resource
maps. Compile/cook/readback are authoring evidence, not runtime energy validation.
Renew mappings, stock reference and cooked readback after any fingerprint change.
