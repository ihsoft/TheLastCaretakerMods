# Shared vehicle entry: architecture reconstruction

Current interpretation: HC33 accepted on 2026-09-13; HC32 entry contracts below
remain unchanged. Start with [architecture](RESEARCH.md) and the active backlog.
The dated sections are retained research evidence, not pending test instructions.
Older artifact paths now resolve through the verified cleanup archive inventories.
No native addresses below are portable beyond their recorded executable hash.

## Unavailable modern actions and teardown

For Steam25191271 / executable SHA256
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B,
GetInteractiveProvidedActions false means decline handling, not handled with no
actions. Consumer call0x1454671b2 branches on false to0x145467329 and can dispatch
legacy GetInteractIndices on the component owner at0x1454673bb. True plus an
empty action array instead skips legacy fallback and reaches0x14546796c.
An unavailable own modern provider must therefore explicitly return true/empty,
including during teardown; unconnected Blueprint guard exits are not equivalent.
Disable acquisition before destroying the station, but retain safe provider
responses for detector references acquired earlier. This does not justify adding
invented native interface inheritance or a partial legacy implementation.

The dismantle-completion crash null-read at0x1452217f0 is inside the registered
GetInteractIndices thunk0x145221720, with return0x1454673c0 on the stack. That
establishes the dangerous legacy route, not the exact live target UObject.
User confirmed dismantling works after explicit handled/empty guard responses
and acquisition shutdown were added. This validates the combined correction for
the reported exit-then-dismantle case, not each change independently or all teardown
paths. Exact candidate identity and transient evidence are in the active backlog.

## HC32 runtime acceptance (2026-09-13)

User reported the contextual-entry test works; only shark recognition could not
be checked because no sharks were nearby. HC32 candidate03 explicitly registers
InteractiveInterface on the own Blueprint while the common native parent does
not implement it. This validates the membership correction together with retained
HC31 acquisition, not a need for more collision changes. Native entry/exit, own
inputs/camera/HUD remain the baseline. Exact installed identities and recovery
evidence live in the active backlog. HC33 changes target display and tuning only.

## Post-HC31: false inherited-interface contract found (2026-09-13)

CURRENT CORRECTION: common VoyageVehiclePawn does NOT declare InteractiveInterface.
HC29's editor mirror `AVoyageVehiclePawn : APawn, IInteractiveInterface` was an
unsupported inference from concrete donor classes. Do not carry that declaration
into the next build. Exact declaring UFunction owner was correct but insufficient:
function presence and implemented-interface membership are separate contracts.

Fingerprint06:23:15UTC unchanged25191271/747DC255...F58B. Native class getter
145351EF0 phase branch145351FB2 passes FClassParams149B74B60 to ConstructUClass.
Matched to local UE5.8 UObjectGlobals.h FClassParams layout: interface table
14B19DD30, count6. Bounded decoding of its six16-byte entries identifies:

| Interface | Native offset |
| --- | --- |
| InteractiveConditionInterface | 328 |
| InteractiveTextInterface | 330 |
| PersistentInterface | 338 |
| VoyageDestructibleInterface | 340 |
| VoyagePlayerInputInterfaces | 348 |
| VoyageItemInterface | 350 |

All six native, not K2. Immediate superclass is Engine.Pawn. InteractiveInterface
getter1451F9B80 is NOT in this table. These are PE registration facts, not a new
live class query; raw report hc31-interface-registration.json is fingerprinted.

Hash-verified HC31 cooked BP_HarpoonOperator_C lists only VoyageActorWidgetInterface
as explicitly implemented K2 interface. Its GetInteractiveProvidedActions function
does exist with InteractiveInterface SuperStruct. The editor thought the interface
was inherited and therefore emitted the function without adding class membership.

Native helper145461D20 tests candidate class membership through getter1451F9B80
and14156A880 before145212A50 dispatch. It first considers component-parent(+D0),
then component-owner(+A8), then eligible owner components if the owner lacks the
interface. Owner interface success returns that owner's result directly. Dispatcher
145212A50 can FindFunction/ProcessEvent, but cannot rescue an actor rejected by the
earlier membership gate. Thus the missing membership explains ProviderSeenNO even
with correct acquisition. Do not replace modern entry with legacy methods merely
to bypass this authoring error.

Consumer1454670B0 tries modern actions first (call145461D20 at1454671B2); false
falls through145467329 to legacy indices. If indices remain empty, code at
145467443..47D inserts index0. It resolves text via1454620F0 and chooses a component
or detector default input action at1454675A1..5CD. This establishes a fallback route
compatible with the observed unlabeled E, NOT live proof of that exact hint's
producer/text return. Direct0 does not categorically bypass the modern provider.

NEXT HC32 design: remove fictitious interface inheritance from editor VehiclePawn;
explicitly implement InteractiveInterface on our generated station Blueprint using
the established ImplementNewInterface/interface-graph pattern (as own HUD already
does). Keep exact signature, provider-first marker, descriptor, guarded callback,
HC31 Block, query/hierarchy, native transport and occupied HUD/input unchanged.
Independent cooked gate must require BOTH own K2 interface membership AND exact
function signature/name. HC32 preparation corrected the initial proposed
SuperStruct gate: explicit interface graphs have no inherited UFunction superclass
(same as our already accepted HUD implementation). ImplementNewInterface builds
the graph against the exact interface; this differs from an inherited override.
Check native parameter identities/output/event flags, not a nonexistent SuperStruct.
No inferred interface inheritance from concrete donors.
One runtime check then separates provider acquisition from callback/native entry.
No source implementation/build/install yet in this research turn; this corrects
the provenance model before authoring. Owning report has failures and raw paths.

## HC31 runtime: acquisition and dispatch are now separated

On Steam25191271, HC31 with explicit cooked Interact=Block selected its own
HarpoonEntryQuery and resolved HarpoonInteraction as the native cached source.
External read verified both UObject serials and cache stability. The ready
station's post-spawn effective-Block marker was true. HC30's prior mount/null
acquisition is therefore no longer the observed failure at the tested viewpoint;
this does not establish acquisition from every side or the exact HC30 default.

User saw an E key with no label. Own modern-provider marker stayed false (written
before filters), as did callback marker. Correct cached interaction is not proof
that the modern provider on its actor receives dispatch. Preserve explicit Block,
query geometry and mount: next research is Direct/legacy component routing versus
generic modern-provider routing. Stock helm/forklift use legacy text/action methods;
blank E is compatible with that fallback but does not identify its producer alone.
No input was requested and no entry validation follows from this observation.
Snapshot and paired cleanup evidence live in the active backlog/HC31 tool report.

## Post-comparison: collision-response serialization boundary, 2026-09-13

Fresh fingerprint03:32:23UTC remains Steam25191271, EXE747DC255...F58B.
Public Mod JSON of unchanged HC30 confirms query BodyInstance profile=Custom,
ObjectType=ECC_GameTraceChannel2, with NO named Interact entry in ResponseArray.
Visibility is explicitly Ignore. Stock BP_Steering InteractBox instead carries
profile Interactive and an explicit Interact=Block response. This is an observed
cooked-data difference, not a proven runtime value of the HC30 channel response.

The generator does call SetCollisionResponseToChannel(GameTraceChannel1, Block).
However, its editor DefaultEngine.ini declares that channel's default as Block.
UE5.8 FCollisionResponse::SetResponse removes a channel from ResponseArray when
the requested response equals the global default. UpdateArrayFromResponseContainer
likewise stores only deviations. On loading a Custom BodyInstance,
UpdateResponseContainerFromArray starts with the destination process's global
default response container and applies the named overrides. Thus a correct editor
in-memory mask is NOT sufficient provenance for the runtime mask in another game.
The omitted response inherits the GAME default, not our editor's default.
Stock's explicit Block is supporting evidence of a differing default, but does
not establish the exact current runtime global default by itself.

Native detector trace linkage is now narrower: update14547D558 reads the dword
at149BE7CC4 into the query-parameter byte at+9. The current PE initial value is14,
matching ECC_GameTraceChannel1 (Interact); parameter flows through1454610E0 to
145474930. Main sweep145474DEC and line145474ECE use that byte. Supplemental
sweep145474E36 and line145474F45 use3 (Visibility). These are decoded call-site
arguments and initial-image data, not a live trace-channel sample or full ranking
proof. HC30 explicitly ignores Visibility and depends on its main Interact response.

Geometry from exact cooked candidates plus recorded live attachments:
- Mount extent(75,75,50), centered at its own root origin; its authored relative
  Z50 is not an extra transform to add again after using the live root transform.
- Entry query extent(95,90,50), center shell-local(0,0,55): station(-170,0,140)
  plus interaction(170,0,-85), unit scale/essentially identity local rotations.
- Therefore query spans X[-95,95],Y[-90,90],Z[5,105], whereas mount spans
  X/Y[-75,75],Z[-50,50]. They overlap, but the query is not fully enclosed by
  the mount. The recorded gaze direction/intersection was not measured; this
  does not rule out the mount winning a particular view or ranking comparison.

Leading discriminating candidate, NOT implemented here: ensure the cooked query
explicitly preserves the intended Interact=Block (or set/read that exact response
after spawn) without modifying mount, extents, hierarchy, priority or occupied
vehicle logic. Before requesting a test, audit the serialized named response and
expose/read the effective channel response, not merely actor collision enabled.
Then reuse the proven stock/cannon cache observer. If actual Block and an eligible
query still yield mount/null, investigate selection/owner lookup; do not continue
blind channel changes. Preserve Q, attachment and the current native vehicle shell.

Game DefaultEngine.ini extraction through stock UnrealPak failed with missing
encryption key; no game config was obtained and no key search was pursued. Raw
inputs/results remain ignored; exact paths/hashes and caller/tool failures are in
mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md. No gameplay edits/build/install in
this research checkpoint.

## Post-HC30: common gaze acquisition reconstructed, 2026-09-13

### Runtime acquisition comparison: helm versus cannon

On the same Steam25191271 process, bounded external read-only observation resolved
the helm hit to InteractBox and the interaction cache to its InteractiveObject.
At the cannon base the native hit instead resolved to the shell's
ModuleMountCollision; the interaction cache was null. References were checked
against live UObject index/serial, with cache stability during the read. Exact
snapshots and install identities belong to the owning backlog/tool report.

The operator and its query were present and ready, with the intended live hierarchy:
HarpoonEntryQuery -> HarpoonInteraction -> station VehicleMesh -> shell
ModuleMountCollision -> boat VehicleMesh. Operator actor collision was enabled;
its provider marker (written before filters) remained false. This localizes an
observable difference to target acquisition before the expected station provider,
not merely HUD rendering. An attached child actor's interaction component is not
proven discoverable just because a hit on the shell is valid. Exact dispatch and
all lookup routes still require their separate evidence.

Do not overread this snapshot: it does not distinguish occlusion/ranking from
query eligibility, nor measure complete world bounds/collision state. It does not
prove that a priority change will fix entry or that the provider will work after
correct acquisition. Preserve mount collision/attachment and the Q cancellation
contract while investigating query selection and owner boundaries.

Scope: read-only stock Blueprint comparison and bounded native decoding, not a
new candidate. Public fingerprint01:15:59UTC still Steam25191271 / executable
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
Consolidated methods and raw-evidence archive recovery:
mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md.

### Shared detector, object-specific actions

The ordinary character owns native InteractiveDetectorPointerComponent.
BP_Base_Character sets MaxReach=200cm and RayShapeRadius=20cm, with
InteractIndexInputActions=[IAV_Interact,IAV_InteractTwo,IAV_InteractThree].
BP_FirstPersonCharacter_New supplies its crosshair widget data. These are stock
defaults, not measurements of a live save or guarantees against runtime overrides.
The detector is itself an action-provider participant: its native interface
vtable leads to the already investigated action aggregation consumer.

| Target | Acquired representation in authored stock data | Entry action contract |
| --- | --- | --- |
| Forklift | InteractiveObject with child Box, Interactive collision profile | Legacy GetInteractIndices/GetInteractText/InteractAction; modern provider false/empty |
| Boat helm BP_Steering | Cube -> InteractiveObject -> InteractBox; PartId200, Priority10000; box extent8 with parent absolute scale5 | GetInteractText returns ST_Interaction.PilotBoat; InteractAction -> FindBoat -> OnEnterVehicle(GetController); modern provider false/empty |
| Photo Drone | Native VehicleMesh with PhotoDrone_Body, Interact response Overlap; no authored InteractiveObject in inspected SCS/CDO exports | Modern GetInteractiveProvidedActions -> GetInteractActions; IAV_Interact for loot, IAV_InteractTwo for entry, OnTriggered delegates |

The drone's absent authored interaction component is not proof that no native
constructor can create one. Separately, decoded detector code proves both an
interaction-object path and a generic hit-component/owner provider path exist.
Do not copy drone physics or assume every transport must use one Blueprint graph.
Different visible E/F keys follow offered input-action identities and character
bindings; they are not evidence of different gaze detectors. GetInteractIndices
false/empty on Steering does not prove no fallback index: do not invent its
default-index rule without tracing that separate legacy branch.

### Native chain and corrected register ownership

Class getter1451FDB00 -> constructor wrapper1452099C0 -> ctor14544F080 establishes
the exact InteractiveDetectorPointerComponent class and its vtables. The update
method at14547D440 comes from the class vtable slot400. The action-provider
interface lives at object+250; its vtable slot10 points to1454670B0.
IMPORTANT: the old consumer's r15+110/+120 are interface-relative, corresponding
to concrete detector+360/+370, not concrete offsets110/120. PE unwind chain
links the old1454670FD excerpt back to real entry1454670B0.

Bounded decoded path:

1. Update gets owner/controller, computes a screen point from normalized pointer
   coordinates, then calls1454610E0 ->145474930 for trace/hit selection. The trace
   uses MaxReach/shape radius, multiple query results and a ranking pass, not
   simply a nearest-object overlap callback. Exact full filter/channel policy is
   not reconstructed; no collision-mask prescription follows from this alone.
2.1454610E0 resolves the selected hit component, tests its immediate attach parent
   for InteractiveObjectComponent, then children, then a same-owner sibling
   neighborhood. It does not blindly search every component of every actor.
   Parent/child access correlation follows Engine SceneComponent helpers; exact
   multi-level edge cases remain untested.
3. Update stores that interaction object at detector+360 (14547DAB2/AB9) and the
   selected hit component at+370 (14547D80C/81D or14547D864/86B).
4. Provider1454670B0 reads these caches. Interaction-specific helper145461D20
   passes the interaction object to GetInteractiveProvidedActions. Generic
   helper145450EA0 builds an attach-parent chain from the hit component and adds
   its owner; interface recipients are asked through145212A50. This second route
   need not pass our HarpoonInteraction identity (recipient component or original
   hit component can be passed). Do not generalize the HC30 equality guard into
   a universal interface contract.
5. Supplied action descriptors feed existing input/hint machinery; only invoking
   an entry action transfers control through native OnEnterVehicle. Occupied
   vehicle input/HUD remains a separate validated layer.

The ranking helper14545C840 compares geometric scores, rejects some candidates
and incorporates a native float from a nearby interaction component. Stock helm
Priority10000 is independently serialized evidence; the precise association of
that field with ranking offset2BC still needs reflected-offset confirmation.
Do not prescribe Priority10000 as the HC30 fix or claim closest-hit precedence.

### What this does and does not establish for Harpoon

HC30 ProviderSeen is written BEFORE readiness/component equality guards. Its
sticky NO therefore cannot be explained by those guards rejecting the action.
Prepared=1 only validates construction/pairing references, not a collision hit,
selected native source, interface dispatch, or runtime query bounds.

The subsequent live comparison above supplies HC30's actual hit and cached-source
identities: mount hit, null interaction. Query absence is ruled out for that
instance, but eligibility/ranking and exact provider recipient remain unresolved.
Keep native transport/HUD/optics/exit and accepted shell untouched. Follow the
serialization investigation at the top before another behavior-changing package.
Do not silently bypass native acquisition with a second custom raycast entry.

## HC30 runtime boundary: hierarchy repair was insufficient

HC30 tested native VehicleMesh -> InteractiveObjectComponent -> query Box,
provider comparison against the interaction parent, and sticky aggregate
diagnostics. User observed one prepared station but providerNO/callbackNO and no
entry hint. Thus the HC29 contract findings below are not a complete explanation
of entry failure. Do not equate valid SCS/component identity with live discovery.
The missing evidence is the native detector's actual hit target, shape-to-object
lookup, registration/filtering and cache producer before provider dispatch.
Return to that acquisition boundary before another candidate. Preserve the
validated occupied vehicle/HUD/input/exit architecture; no donor physics needed.
Exact rejected candidate and removal evidence belong to the active backlog and
HC30 tool report. No provider call was observed; the specific failing native
stage has not yet been proven.

## HC29 acquisition analysis: interaction component is not the query shape

Current Steam25191271 / executable747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B,
renewed by public fingerprint 2026-09-13T00:02:06Z. HC29 runtime entry failed;
no new runtime test accompanies these read-only findings.

There are at least two distinct components in the stock interaction contract:
the collision shape that can be hit, and the InteractiveObjectComponent used
to request actions. Public stock Turret SCS/JSON gives the exact hierarchy
Root -> InteractiveObject (Voyage.InteractiveObjectComponent) -> InteractBox
(Engine.BoxComponent). The box carries the Interactive/Interact collision mask.
Do not collapse these into a single Engine Box as HC29 did.

Bounded executable analysis identifies the reflected native helper
GetInteractiveComponentProvidedActions and its call to the interface dispatcher
GetInteractiveProvidedActions. The helper checks InteractType, searches interface
recipients, and passes its input interaction component unchanged as the provider's
Component argument. Its own input is not the hit box: InteractType is a native
InteractiveObjectComponent property (mapping plus reflected offset correlation),
and the consuming path obtains a cached interaction object before calling it.
Consequently, HC29's explicit Component == HarpoonEntryQuery filter rejects this
route even if discovery reaches the station. Merely enlarging the box cannot fix
that identity mismatch. Missing native interaction component and wrong provider
argument expectation belong to one acquisition contract repair.

Boundaries: this does not prove which object the user actually aimed at, nor that
the helper is the sole provider route. The executable has several interface
callers. Exact shape-to-interaction lookup/precedence remains to be checked before
authoring the correction; there is no whole-program reconstruction here. The
HC29 coordinator's last-station diagnostic also cannot prove no provider ran.
Do not add donor movement/physics, container UI, or arbitrary collision changes.

Next design is an own native interaction component plus child query shape,
checking the former in the provider. Verify its current native default/Direct
configuration without guessing fields or copying the Turret's WidgetOverlay
settings. Keep the accepted shell and occupied transport contracts unchanged.
Raw version-specific references and tool limitations are in the HC29 tool report;
decoded windows and stock JSON remain ignored artifacts.

## HC29 authored contextual entry (runtime failed; historical implementation)

Own station overrides inherited InteractiveInterface.GetInteractiveProvidedActions;
native delegate signature supplies InputAction.Controller to guarded native entry.
Independent candidate05 cooked JSON confirms exact owners/signatures, named action
writes and bound callback. Unlike the initial research plan, query acquisition
lives on one own Engine Box attached to station root over the gun base, preserving
the accepted shell package unchanged. No Fabricated or action-query spawn.
Engine coordinator owns exact-class discovery, Owner-based pairing and preparation;
station owns readiness, entry checks, occupied camera/HUD/input and exit safety.
This is an implemented candidate, not proof that native acquisition accepts the
new query component. HC29 markers separate preparation/provider/callback results.
Keep partial native structs tagged/bytecode-only; game-sized ABI not reconstructed.
Evidence, caveats and current installation: active backlog and hc29 tool report.

## Scope and evidence boundary

2026-09-11, Steam25191271, executable SHA256
747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
Fresh public fingerprint matches current Harpoon provenance. Read-only stock
assets and bounded executable decoding; no running-process memory writes,
gameplay changes, new package or compatibility claim. HC18 rebuild is deferred.
This is a partial reconstruction, NOT a finished native call graph.

## HC20 follow-up: inherited entry and missing concrete configuration

### HC29 contextual entry reconstruction (implementation pending)

Renewed Steam25191271 evidence distinguishes two entry producers. Forklift's
GetInteractiveProvidedActions returns false/empty; it uses legacy interaction
indices/text/InteractAction. Stock Turret and Drone instead implement native
InteractiveInterface.GetInteractiveProvidedActions, returning action descriptors
with delegates. Do not assume every vehicle's entry is the same Blueprint graph.

Modern provider signature is bool(Pawn* MyCharacter, SceneComponent* Component,
OUT TArray<PlayerInputInterfaceAction> OutActions). Bound global
PlayerInputInterfaceOnAction delegate takes const VoyageInputActionInstance&
InputAction and OUT FText Text. The instance extends Engine.InputActionInstance
with Controller; Drone's entry handler calls OnEnterVehicle(InputAction.Controller).
Drone binds OnTriggered for entry, Turret uses OnStarted for some other actions.
The physical key is described by an existing character-context action, not a
new key poll in the cannon. Own occupied E handler remains the separate HC28 path.

Current accepted shell generator deliberately has no interaction component/
action override. F8 bypasses this absent normal entry route. A separate
Interactive query shape/component exists on the stock Turret; its container
overlay configuration is not needed merely because it is a donor. Preserve
our mount/simple collision, no physical collision or donor physics needed.

Proposed ownership: coordinator prepares one station per built cannon; only
ready/vacant stations offer entry from shell. Provider queries are side-effect
free. Bound callback revalidates station/controller then enters through native
method. Never spawn in Fabricated or in action-list enumeration. Explicit
pairing, removal and stale-reference guards precede runtime testing. No new
candidate yet. Exact renewed hashes/signatures and pending implementation gates
live in mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md.

HC28 partial action mirror160 bytes is not a complete native action ABI (stock
metadata656): adding delegate fields requires exact current identities/signature,
not padding guesses or serialized partial defaults. Prior native interface
completion crashes remain a warning, not evidence that this new path works.

### HC28 runtime: own vehicle supplies a standard exit-action hint

2026-09-12 Steam25191271 user accepted requested native Exit Harpoon card,
E exit and restored walking test: "Все отрбаотало отлично!". This validates
the combined producer/consumer configuration below on our own common-base
vehicle, without Forklift inheritance, HUD or input context. No screenshot of
the individual diagnostic markers was provided. Rebinding, other devices,
pause/resume, repeated cycles and save lifecycle remain separate checks.

Keep physical input handler, action descriptor and rendering separate: the
existing own Enhanced Input E event still invokes native OnExitVehicle; the
base-owned GetProvidedActionsBP describes that SAME action for the stock
horizontal hint widget nested inside our own HUD. Character stats remain.
This is the accepted foundation for normal contextual entry from the shell;
it does not itself implement outside interaction or station lifetime.

#### HC28 preparation evidence

HC27 owns Enhanced Input E handler but no GetProvidedActionsBP override; its
custom HUD has only our TextBlocks. Current stock Forklift nests unchanged
BP_DynamicPlayerInputHorizontalWidget twice, supplying its own context and
Central/Context filters. The suffix _Bottom is an instance, not an asset path.
The stock widget's native parent is VoyageDynamicPlayerInputWidget; its native
tick renders a ContextInputActionsRoot horizontal box using WBP_InteractIndicator.

Prepared HC28 overrides exact VoyageVehiclePawn.GetProvidedActionsBP with one
Central description referencing the already-bound IA_HarpoonExit. The own HUD
soft-loads the stock widget, supplies our context before parenting and keeps
character stats. No Forklift HUD/defaults, duplicate E handler or global UI
suppression. Provider-called and nested-widget-created are separate markers;
neither marker alone proves key-card rendering. User test subsequently passed above.
Prior standalone-provider no-ops from DonkLift are not a reason to add arbitrary
Actor components: current station is already the possessed common vehicle.
If this coherent path fails, inspect registration/filter rather than iterate
more defaults. Ordinary E entry from shell is a separate lifetime contract.

### HC27 runtime: own native-selected HUD can display optical target data

On Steam25191271, user accepted HC27; screenshot shows Talon Shark / 270 m
under own reticle and x5 | E EXIT. The independent observer records own
GetHUDOverrideWidget callback YES, station controlled/attached YES, root
simulation NO. Station-owned optical trace outputs FText; own HUD Tick reads
those fields from controlled station. Target rendering therefore no longer
depends on the diagnostic panel or Forklift HUD. Preserve HC26 ownership split.

The E EXIT label is fixed text. Standard action/key cards are still absent:
input delegates and native HUD selection do not implement a provided-action
provider. User explicitly wants character stats at bottom center retained.
Contextual entry and registered own action hints remain separate next work.
Screenshot does not independently validate all shark variants, miss clearing,
exact zoom, exit restoration or save lifecycle. Evidence/candidate in backlog.

### HC26 runtime: dedicated common-base station is a usable foundation

2026-09-12 on Steam25191271 user reports the requested dedicated station test
works excellently: first-person/own HUD/mouse/E exit/restored character behavior
were the requested scope. This is a broad user acceptance, not instrumented
proof of every callback, prolonged ship motion or all exit conditions.

Authored child of VoyageVehiclePawn (NOT VoyageVehicleForkliftPawn), own
VoyageInputContextAsset/Enhanced Input mouse and E events, common native
OnEnterVehicle/OnExitVehicle, exact VoyageActorWidgetInterface.GetHUDOverrideWidget
returning own VoyageBaseUserWidget child. Camera is an owned Engine CameraActor
attached to station; select view target once per occupied transition, rotate
only this camera with own input. No native first-person flag repair, Forklift
HUD/context/drive dependency or player-HUD child suppression required for this
accepted test. Native entry still owns possession/character lifecycle.

Serialization matters independently: all9 packages used tagged properties,
not indexed unversioned serialization against incomplete native mirrors.
Current-game parser audit confirms own fields and no Voyage-native CDO deltas;
the exact native VehicleMesh root reference is retained, its property deltas
empty, active SCS nodes empty. This specific candidate passes runtime; it does
not authorize guessed schemas or arbitrary partial-mirror authoring.

E label is a fixed test label; dynamic provided-action hints/rebinding were NOT
implemented or validated. F8 debug entry and20s fallback remain. Restore x5/
shark trace next without changing this ownership split. Save/persistence,
ordinary contextual entry, removal while occupied and long sessions remain open.
Immutable candidate identity/recovery paths live in the active backlog.

### HC24 runtime: first-person state is not sufficient to select final view

On the same fingerprint, setting native bAllowFirstPersonCamera and
bFirstPersonCamera plus the owned CameraComponent FOV before FinishSpawning
left the view in third person. Independent runtime readback: stateYES,
requested26.812699 degrees, manager100 degrees, station possessed/attachedYES,
root simulationNO. User reports character rotation with mouse/camera.
Neither the rotation's component owner nor its introduction in HC24 is proven.
The flag survived, but the requested camera result did not. This does not
distinguish an inactive camera from a lifecycle/native camera update or a
different final-view producer. Resolve that consumer path before further
camera mutations; preserve HC23's validated native control/entry contract.
Do not infer zoom or first-person success from a stored field alone.

### HC23 runtime: native input configuration is sufficient for tested controls

User reports all requested controls working (mouse, E exit, restored walking),
with third-person camera rotation like Forklift. HC23 supplied the existing
native InputControls.InputContextAsset and Forklift look-action references
before FinishSpawning, keeping HC22 ExitAction and native enter/exit unchanged.
No explicit SetupPlayerControls call, raw mouse handling or synthetic E handler
was needed in this tested lifecycle. This establishes a usable stationary
transport control baseline without full Forklift Blueprint/wheels/physics.
It does not expose every internal activation stage or prove arbitrary late
context replacement works. Preserve configuration-before-lifecycle ordering.
Third-person is native camera behavior, not failure to possess the station.
Next isolate first-person/optics while retaining native possession/input.

### HC22 runtime corroboration

Follow-up: F8 exit and normal character controls restored. While occupied,
mouse and all other controls remained inactive; only observer F8 worked.
Thus ExitAction supplies native action enumeration/HUD but not complete input
activation/binding. Stock Forklift additionally sets InputControls.InputContextAsset
and LookRightInputAction/LookUpInputAction; the keyboard context maps MouseX,
MouseY and E to those known actions. HC23 tests that exact configuration before
BeginPlay/possession, preserving the working HC22 entry/exit/physics/HUD.
No explicit SetupPlayerControls call is justified by this evidence yet.

User's HC22 screenshot shows successful occupied native Forklift station at
4.419818seconds with attachmentYES/physicsNO. With only stock ExitAction
initialized and checked, HC21 entry crash did not recur in this attempt;
TEXT BLOCK and inert E Interact are replaced by stock E Exit Vehicle and
Forklift speed/fuel gauge, with no player hotbar. Character is now near deck/
cannon rather than high above it. This supports the native subclass HUD path
and tagged operator placement without spawning the full driving Blueprint.
The common quest/task/resource widgets remain normal stock layers, not proof
of failed possession. Speed gauge presence alone does not establish its data
source as the ship. Actual E handler, exit/restored walking, prolonged motion
and repeated entry remain unconfirmed for HC22. A custom vehicle HUD should
use native selection/provider contracts, not blanket widget hiding. Raw
screenshot/runtime details and cleanup anchors belong to the active backlog.

### Historical HC21 caution

HC21 runtime caution: bare shipping native Forklift subtype plus tagged root
prepared successfully but crashed on entry (null member read at RVA542cfaa).
It is NOT a validated shortcut to the stock HUD. Neither tagged placement nor
HUD selection was observed while occupied. Sharing native entry implementation
does not prove a bare subclass supplies the dependencies of the full Blueprint.
Follow-up below identifies the direct missing member as ExitAction during
action enumeration. Paired test packages have been removed.

### HC21 exact crash dependency: mandatory ExitAction descriptor

Same installed fingerprint revalidated. Native Forklift constructor installs
VoyagePlayerInputInterfaces vtable at actor+0x348; its slot0x10 points to the
provider containing the fault. Provider prologue preserves that interface
pointer in rdi, so the faulting rdi+0x210 is actor+0x558, NOT actor+0x210.
Current reflected ExitAction descriptor binds exactly to0x558. The native
constructor zeros this property; the stock Forklift Blueprint supplies the
VoyageInputAction /Game/Game/Input/Vehicle/IAV_VehicleExit.IAV_VehicleExit.

The provider reads the ExitAction pointer and immediately accesses its data
at+0x28 without a null guard. HC21 omitted it. This matches the runtime read0x28
fault exactly. Its caller obtains VoyagePlayerInputInterfaces and invokes the
provider virtually while gathering actions. Preparation alone did not request
that occupied action list; a working F8 exit route does not make the action
descriptor optional. ForkTiltUpInputAction has an explicit null guard in the
decoded prologue; do not infer every native action pointer has the same null
contract. The complete provider's remaining requirements have not been audited.

Consequences: the observed crash is a missing action-descriptor dependency,
not direct evidence against attachment/tagged placement or the shared native
entry function. It does NOT prove HUD selection/position/exit now work, or
that no subsequent prerequisite is missing. A controlled successor can set
the exact stock ExitAction before entry, verify non-null/readback, and keep
the other HC21 variables fixed. E binding/handling additionally depends on
the input context; an action descriptor alone is not that complete contract.
No implementation/build/install performed by this research follow-up.

Read-only disassembly and stock Blueprint inspection on the SAME fingerprint
above establish the following. Raw evidence and decoder windows remain ignored;
see mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md.

- Forklift InteractAction forwards to ubergraph entry1355. It checks character
  validity, matching InteractiveObject.PartId and InteractIndex0, gets/casts the
  controller to VoyagePlayerController, invokes OnEnterVehicle and ends. The
  separate PlayerInteraction entry1353 is empty. No additional HUD setup occurs
  after OnEnterVehicle in this branch. This does not exclude earlier lifecycle
  initialization (HC20 normal-E-first priming remains untested).
- Native VoyageVehicleForkliftPawn calls the VoyageVehiclePawn constructor.
  Comparing both primary vtables proves identical entry gate, entry core and
  exit core. Forklift's twelve own native registrations concern fork/tilt,
  steering and throttle values, not entry/exit. Do not attribute its working
  entry to an unobserved derived OnEnterVehicle override.
- Common entry includes the previous-character interface notification
  OnOperatorEnter; exit includes OnOperatorExit. Both finish with OnDriverChanged.
  Names are tied through FName initializers to the called dispatch wrappers,
  not guessed from nearby strings. The core also contains conditional mounting,
  controller handoff and movement/camera operations. Some engine/virtual calls
  remain unnamed; this is not a complete executable lifecycle specification.

### Concrete placement dependency: ExitComponentTag

The shared placement helper initializes a fallback position from the root world
position plus2500cm on Z. With a nonempty ExitComponentTag it visits components;
the matching component supplies position and yaw (pitch/roll are cleared).
Both entry and exit call this helper. Current property descriptor binds
ExitComponentTag to native offset0x3a0; the native base constructor leaves it
empty. Stock Forklift CDO sets it to ExitComponentTag and has a correspondingly
tagged CharacterLocation scene component at local(approximately0,-70,165)cm.

This explains a concrete difference from HC19's bare native base and provides
a strong, not yet runtime-isolated explanation for the character floating high
above the cannon. Our additional140cm station offset is a separate small factor.
Do not fix this by adding physics or blindly copying Forklift placement offsets:
author a tagged operator point appropriate to the stationary cannon and test
native entry AND exit. Despite its name, this tag participates in entry too.

### HUD selection is separate from the entry call

Fresh BP_VoyageHUD CDO maps VoyageVehicleForkliftPawn to BP_VoyageIngameForklift;
it has NO VoyageVehiclePawn entry. The default Hud widget is BP_VoyageIngameHud.
This supports a generic-HUD fallback explanation for HC19, but lookup priority,
refresh trigger and actor override precedence remain unresolved. It does not
prove the exact owner of TEXT BLOCK or inert E Interact.

Forklift's native InputControls references DA_Input_Context_Forklift; its HUD
hint widgets reference the same context. The CDO also supplies ExitAction =
IAV_VehicleExit and concrete look/zoom/action identities. Separate Blueprint
controller/interface components provide Action/Menu contexts. These are concrete
configuration, not wheel physics, and a bare native CDO lacks the Blueprint's
authored values. A station needs a coherent context, handlers and HUD provider,
not merely visible action labels.

### Correction of historical SetupPlayerControls ownership lead

On25191271, reflected SetupPlayerControls AND TeardownPlayerControls belong to
VoyageInputControlsComponent. Verified by their function descriptors' outer
class constructor AND that class's nine-entry native registration table.
Setup consumes one object parameter and dispatches virtualslot0x4e8; the exact
parameter class and activation caller remain to resolve before any invocation.
The earlier lead placing these methods on VoyageInGameVehicleWidget is NOT a
current contract and must not guide a mirror or call. No claim is made about
whether the historical build changed or its old interpretation was incorrect.

Next bounded discriminator: preserve native entry/exit and the safe shell;
add the verified tagged operator-point contract, leaving HUD changes out of
that placement test. Independently resolve native HUD selection/override and
input-component activation before a minimal stationary vehicle child is cooked.
No new package was prepared or installed in this investigation.

## HC19 trial derived from this research

User subsequently approved a runtime discriminator. Native entry parameter
metadata decoded against current executable: OnEnterVehicle takes ObjectProperty
NewPossessor of Engine.Controller (not guessed VoyagePlayerController); void.
OnExitVehicle takes no parameters, void. Both flags0x0c020c00. Source registry
records exact descriptor evidence. Fresh Forklift Components summary reports55
SCS components, including separate controller/interface context components;
none implies copying wheel/constraint/drive implementation into Harpoon.

First trial uses an Engine observer to request runtime creation of the SHIPPING
native VoyageVehiclePawn, rather than cooking a child against incomplete native
metadata. Deferred actor has physics/collision disabled before finishing and is
attached to safe shell; prepared-state gate precedes native entry. This imports
only class/call identities and preserves native game CDO. Native constructibility,
entry, HUD/context effects and return are runtime questions, NOT established by
the mirror. Full authored child/context/handlers remain a later checkpoint.

## Confirmed common ownership

### HC20 comparison uses existing complete Forklift, not cloned physics

DonkLift reports separate base-owned GetProvidedActionsBP from the registered
dynamic input component and filtered rows; standalone-provider probes6/8 failed
even after EnableInput. Do not resume that rejected branch for Harpoon hints.
Historical research-pitfalls placed SetupPlayerControls/TeardownPlayerControls
on VoyageInGameVehicleWidget. The current-build investigation above supersedes
that lead: the verified owner is VoyageInputControlsComponent.
Public current Forklift summary confirms canonical class and virtual entry/exit
calls; individual interaction events forward into a common ubergraph, so their
zero direct native calls do not prove absence of native entry.

HC20 targets the same already-built canonical Forklift first entered normally,
then via the HC19 native call under F8. No Harpoon shell, spawn, attachment,
physics/input-field or native HUD mutation; existing DonkLift left installed
and unchanged. It is a same-object/environment comparison, not an uncontaminated
stock-Forklift claim. Matching good UI/return would show complete transport
configuration works with this call; a normal-vs-F8 difference would motivate
looking at normal interaction's additional stages. If baseline already fails,
do not diagnose Harpoon from this comparison.

HC20 runtime: user confirms F8 matched normal E and exit had no artifacts.
Both screenshots identify SAME BP_Forklift_Possesable_C1492676524, with matching
Exit Vehicle/Horn/fork controls and no TEXT BLOCK/default Interact. Shared
quests/tasks/resources remain even in correct Forklift HUD, so their presence
alone is not failure. This validates our native caller on complete Forklift in
the tested sequence, not arbitrary cold-start setup. Normal E entry occurred
first; one-time initialization/cached HUD remains an unresolved alternative.
Posttest disk inventory finds DonkLift triplet absent although it was present
at install time. Our restore touched only HC20's5files; environment change timing
unknown. Do not credit DonkLift-specific runtime helper or assume frozen provider
identity from install-time hashes. No artifacts or offsets reused across builds.

### HC19 runtime boundary after native-base trial

User supplied screenshot da6f3fd6-d98b-4f4c-8c6f-527f8c8feebc: controller pawn
matches native station, it remains attached to shell, root simulation false.
User observes blocked locomotion and ship-following view/character, but excessive
height and unresolved HUD (TEXT BLOCK, inert E, character-looking panels).
Native runtime base constructibility/entry is observed. Full exit/actions not
yet validated. This discriminates against "all HUD artifacts merely mean no
vehicle possession": actual native possession coexists with those artifacts.
Concrete HUD selection/context/provider configuration remains necessary research;
the precise cause and identities of these widgets are not established.
Keep mounting/camera offsets separate from input/HUD ownership and safe shell.

### Base selected for Harpoon: VoyageVehiclePawn

User approved starting from the common transport base, and asked to check for a
more general game-specific ancestor. Renewed public mapping confirms direct
parent Pawn. Local UE5.8 Engine headers corroborate Pawn : Actor (+NavAgent
interface) and Actor : UObject. Thus the relevant chain is
UObject -> Actor -> Pawn -> VoyageVehiclePawn; there is no intermediate Voyage
base in this chain. Engine Pawn is precisely the lower level tested in HC17,
not another untested common transport implementation.

Select a dedicated VoyageVehiclePawn child as the intended operator, preserving
the existing buildable shell. This is a design selection, not a compiled or
runtime-validated child yet. Do not reuse the historical Harpoon23962331 empty
OnEnterVehicle mirror or DonkLift25056839 action-only mirror as a complete native
base schema. Both are incomplete for generating a new inherited CDO. The earlier
HC18 empty-native-widget serialization failure remains relevant.

The common constructor1454099f0..14540a0a1 creates native subobjects including a
mesh; it must not be described as inherently collision/physics-free. The current
Forklift CDO has VehicleMesh of type VoyageFastSceneComponent, InputControls,
DynamicPlayerInputComponent and camera-related native subobjects. Forklift's
full native-subobject list also contains derived objects; do not attribute every
Forklift subobject to the base without constructor evidence. New base child must
preserve required component identities and validate their stationary settings.

### Shared activation is not shared movement behavior

User correctly separates common entry/context ownership from per-transport
semantics. Current IMC_Forklift_Keyboard explicitly maps W/S/D/A to
IA_VehicleForward/Backward/Right/Left, and MouseY/MouseX to look actions.
This mapping establishes keys -> actions, not actions -> displacement.
Concrete classes/components/native handlers decide how action values affect
their transport. The data does not prove every vehicle directly polls keys, nor
that all vehicles require distinct action assets for the same key.

Harpoon should reuse the common entry/exit/context lifetime, supply its own
station actions/handlers and matching hint context, and not inherit Forklift
driving behavior. GetProvidedActionsBP describes supplied actions for the action
system; it is NOT by itself their execution handler. Input binding, handler and
hint registration must all be accounted for. No assumption that simply omitting
WASD handlers neutralizes inherited physical simulation.

Both current mapping chains converge on VoyageVehiclePawn : Engine.Pawn:

- VoyageVehicleForkliftPawn : VoyageVehiclePawn (32 own properties).
- VoyageBoatPawn : VoyageVehiclePawn (19 own properties).
- VoyageVehiclePawn has 38 own properties, including InputControls,
  DynamicPlayerInputComponent, mesh/camera references, Driver/DriverClass,
  bAutoPossess, bMountCharacterWithVehicle and IsPossessed.

Stock Forklift Blueprint directly derives from VoyageVehicleForkliftPawn. Stock
Steering calls FindBoat and then OnEnterVehicle with the controller; it does not
implement HUD replacement in its entry graph.

The executable registration establishes exact base ownership:
class constructor145351ef0 references VoyageVehiclePawn and registrar145369f30.
The registrar passes this class and11entries at149bb04c0. Entries include
OnEnterVehicle->145379590, OnExitVehicle->142ee2160,
GetProvidedActionsBP->145374620 and GetDriver->145372d40.
The other OnEnterVehicle-associated pointer145358b30 is a reflected-function
constructor, NOT another transport entry implementation. Names/pointers alone
were insufficient; decoded registration resolved the distinction.

## Entry path recovered so far

OnEnterVehicle exec thunk145379590 consumes one object argument and dispatches
virtual slot8b0. The base constructor1454099f0 writes primary vtable149bac020;
slot8b0 points to145439670. That short base method checks byte3b2, then dispatches
slot938->14541dcc0. Reflected bAutoPossess metadata points to setter1453591e0,
which writes byte3b2. Thus this flag gates base entry, not merely a guessed CDO
tuning parameter. Current native constructor initializes it true. Derived classes
may override the virtual methods or defaults; do not assume their implementations
are byte-identical to this base path.

The common core14541dcc0..14541e203:

- Rejects a null controller argument; handles controller pawn absent separately.
- Captures the previous pawn and its class into base state; writes possession
  state and retains a driver reference (field labels match current schema and
  constructor layout; do not copy offsets into mod code).
- Has a sizeable branch gated by byte3b0. Reflected
  bMountCharacterWithVehicle setter142f91740 writes exactly that byte. Placement,
  camera adjustments and later driver attachment work belong to this conditional
  branch, not to every vehicle physics implementation.
- Calls controller-targeted142997660 with the vehicle as argument. This is the
  possession-call candidate; its authority-checking prefix is decoded, but a
  symbol-level binding to Engine.Controller.Possess remains to be independently
  closed before using this address as such in a call graph.
- Makes additional calls on/with the previous character after the handoff,
  including1452137d0 (now resolved as OnOperatorEnter) and virtual9f8 (still
  unresolved). Direct Possess does not explicitly reproduce these operations.
- Finishes via14536cbb0, a ProcessEvent-style wrapper using FName14b5ee388.
  The OnDriverChanged name initializer at1411ff980 writes that same identity.
  This establishes a driver-change notification beyond controller possession.

There is an adjacent base exit gate145439690 checking the same auto-possess byte
and dispatching slot940->14541e5f0. Follow-up decoded the exit body through
14541eacc: paired OnOperatorExit, controller handoff, mounting/movement operations
and OnDriverChanged. Forklift inherits the same body. Exact cleanup semantics
for every virtual call/error path are NOT fully named; preserve native exit.

## HUD and action context: confirmed current asset contracts

These are collaborating layers, not a claim that OnEnterVehicle calls every
listed component directly or in this order:

| Layer | Current owner/evidence |
| --- | --- |
| Transport entry and driver state | VoyageVehiclePawn native registration/core |
| Input context | Forklift native InputControls subobject is VoyageInputControlsComponent; InputContextAsset is DA_Input_Context_Forklift |
| Device mappings | That VoyageInputContextAsset maps MouseKeyboard to IMC_Forklift_Keyboard and Gamepad to IMC_Forklift_Gamepad; priority offset10 |
| Action extension | GetProvidedActionsBP registered on VoyageVehiclePawn; existing DonkLift validated adding actions through this base-owned hook on its historical checkpoint |
| Hint presentation | Forklift Bottom and Center are instances of BP_DynamicPlayerInputHorizontalWidget; both ContextAsset fields point to the SAME DA_Input_Context_Forklift |
| Hint consumer | BP_DynamicPlayerInputHorizontalWidget derives from native VoyageDynamicPlayerInputWidget; class requires native Tick |
| Complete HUD selection | BP_VoyageHUD : VoyageHUD centrally maps VoyageCharacter/BoatPawn/VehicleForkliftPawn/PossessableModuleActor to character/boat/forklift/crane HUD classes |
| Optional HUD override | Current JetSki implements GetHUDOverrideWidget; its priority relative to the central map and lifecycle remain unverified |

VoyageDynamicPlayerInputWidget current schema includes bRegisterFromWidgetOwner,
ContextAsset, bFilterByActionType, InputActionType, ContextInputActionsRoot,
ButtonInfoContainer, bPreferIndicatorWidgets and IndicatorSubClass. The Bottom
instance serializes filtering=true; Center additionally serializes Context action
type. Absence of bRegisterFromWidgetOwner in these CDO deltas is NOT fresh proof
of its native default. Historical locator/component-registration findings are
useful leads, not renewed current-build call-graph evidence.

Bottom is an instance name INSIDE Forklift HUD, not a separate package. The public
summary unexpectedly reported0generatedClasses for the actual horizontal widget;
public full JSON reveals its WidgetBlueprintGeneratedClass and native parent.
This is a summary coverage defect, not an absent widget/class or corrupt store.

## Architectural consequence and next research gates

The user hypothesis of shared transport machinery is supported. Reuse/inherit
that machinery rather than mimicking its visible effects. A generic Engine Pawn
with direct Possess has neither demonstrated native driver-state lifetime nor
the context contract shared by stock input and hints. This does NOT yet prove
which missing operation causes each remaining HC17 HUD artifact.

Before choosing a Harpoon adapter:

1. Resolve base entry/exit and derived Boat/Forklift differences, including
   character notification and mounting prerequisites/default subobjects.
2. Identify who activates/deactivates InputContextAsset on possession/driver
   changes, who gathers provided actions, and how HUD binds the matching context.
3. Identify VoyageHUD refresh/teardown trigger and precedence of actor override
   versus class map; do not infer this from the presence of a GetHUD method.
4. Choose a station implementation preserving the already-safe buildable shell,
   reusing native entry/exit with the minimum verified transport dependencies.
   No physical Drone/forklift reuse merely to obtain control semantics.
5. Only then propose narrow reversible tests with distinct expectations for
   character actions, station actions/HUD, attachment and full exit restoration.

Detailed ignored evidence and tool-use limitations are indexed in
mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md. No new runtime acceptance.
