# Voyage vehicle and standard-HUD patterns (observed; transfer unproven)

This document describes reusable patterns for Voyage vehicles and possessed
stations. Concrete classes, fields, actions and widget lifecycles can differ
between consumers; implementation examples are references, not dependencies.

## Separate the runtime layers

Treat these as independent until a test proves otherwise:

1. Enhanced Input maps a physical key or controller axis to an `InputAction`.
2. A native vehicle pawn receives action values and stores raw input state.
3. Native or Blueprint vehicle logic converts that state into movement.
4. A provided-action system describes which controls should be presented.
5. Dynamic HUD widgets register with a provider/component, filter actions, and
   render key/label widgets.

A working mapping does not imply a visible standard hint. A changed HUD value
does not prove movement consumes the same field. Diagnose the producer and
consumer of each layer separately.

## Camera compass in a vehicle HUD

On Steam build `25982825`, the stock `WBP_CompassHUD` uses native
`VoyageCompassWidget` logic with a cached pawn reference. Its
`bUseControllerDirection` mode is not a guarantee of the active camera's
heading after possession changes. Reusing the visual widget in a vehicle HUD
therefore also requires verifying its direction source.

A camera-driven consumer can retain the stock widget tree through a child,
disable that display's native tick and update its material and degree text
from the owning player's `PlayerCameraManager.GetCameraRotation`. Cache the
owned display references at initialization and let the vehicle HUD control
visibility. Avoid a second native updater competing for the same presentation.
This does not establish another vehicle's HUD ownership or lifecycle.

## Actor attachment and save restoration

In an observed stock snapshot,
`VoyagePersistentSubsystem` exposes two Blueprint-assignable lifecycle events:

- `OnActorAttached(AActor* Child, USceneComponent* ParentComponent)` is
  broadcast by successful attachment. It also fires for existing Electric
  Wall Sockets during save restoration: the same synchronous handler supports
  newly built and restored sockets, including sockets with connected cables.
  A separate restore scan or polling loop is not required for that consumer.
- `OnActorRestored(AActor* Actor)` is a separate reflected event. Its signature
  is established, but its ordering relative to attachment and loader startup
  is not a runtime contract. Do not subscribe to both merely because both exist;
  account for duplicate processing if a consumer actually needs both.

Use the attached actor supplied by the event rather than rediscovering it.
The parent scene component identifies the immediate attachment target; for a
welded structure, resolve the physical root with
`VoyageMiscBlueprintFunctionLibrary.GetRootPrimitiveComponent` and
`bIncludeWeldedParent=true`, then inspect its owning actor. An immediate parent
need not itself be a Boat. Filter the intended class and vehicle before applying
mod behavior; an unrelated attachment is not a failure.

Module existence has its own events:
`VoyageModuleSubsystem.OnModuleRegistered` and `OnModuleUnregistered` are
Blueprint-assignable and carry one `VoyageModuleComponent* Module` parameter
through `VoyageModuleRegisteredDelegate`. Module registration is not a
substitute for completed actor attachment or proof of its final vehicle owner.

The runtime contract above covers Electric Wall Sockets on Boats, not every
actor type or arbitrary ordering of streamed levels. Check the reflected
signatures and lifecycle for a new consumer or when changed behavior gives
reason to doubt their applicability.
See [external socket ownership](boat-resource-socket-architecture-knowledge.md#external-socket-registration).

### Feedback before the HUD is available

The [stock message-card contract](voyage-hud-messages-knowledge.md#acceptance-readiness-and-throttling)
explains delivery results, HUD readiness and throttling. A card attempt cannot
guarantee user-visible failure feedback from an early-load callback.
Development-only Blueprint helpers such
as `PrintString` and `RaiseScriptError` are likewise not a Shipping failure
channel. Distinguish a useful development diagnostic from a production outcome;
do not silently treat an attempted notification as successful delivery.

## Post-physics kinematic correction

When a mod must cancel or reshape stock physics, the observation and correction
belong after the stock Blueprint and Chaos integration. An Actor helper ticks in
`TG_PrePhysics` by default even when ticking is merely enabled; explicitly set
its tick group to `TG_PostPhysics`. A pre-physics velocity write can be valid yet
be overwritten by stock force integration in the same frame.

Keep corrections axis-local and additive. Horizontal stabilization should read
the post-physics velocity, calculate the desired XY velocity, and add
`(desiredXY - observedXY, 0)` through `SetPhysicsLinearVelocity` with
`bAddToCurrent=true`. Rewriting a complete XYZ vector can interfere with a
separate vertical state machine even when its Z value was copied from the same
observation. A vertical correction likewise owns only Z velocity or Z position
and preserves the live X/Y state.

The consumer owns its stabilization algorithm, eligibility, braking rates and
disable semantics. Historical velocity or position samples must be invalidated
when their control ownership changes. A previous-command clamp cannot distinguish
actuator acceleration from a collision impulse; it is not a universal correction
policy. A coasting actuator also does not establish propulsion eligibility.

An unswept post-physics position correction can undo the solver's separation
from a moving obstacle. A swept correction must inspect its hit result and
yield on blocking contact or initial penetration before any subsequent velocity
write. The move's Boolean return means movement occurred, not that the path
was clear. An actor-root sweep tests the root primitive's represented collision
body. Treat welded child shapes as covered only when engine and component
evidence shows that they are represented by that body; independent child bodies
require separate handling. Contact notifications and the actual collision
hierarchy therefore remain part of the contract.

## Exact native identity matters

Cooked Blueprint references bind to exact `/Script/Module.Type` and parent
UFunction identities. An editor mirror must preserve the declaring class,
inheritance, function owner, struct fields, and enum values used by the graph.
A same-name function declared on the wrong stand-in class can compile and cook
yet never override the shipping callback.

Keep native mirrors minimal, provenance-marked and editor-only. Author the mod's
Blueprints through the maintained project and BPT; editor support must not
become a replacement runtime game module.

## Discovering Voyage's standard action UI

For a vehicle with dynamic hints, identify the whole path:

- which object constructs `FPlayerInputInterfaceAction` entries;
- which Blueprint extension point contributes additional actions;
- which component/provider is registered with the HUD;
- which enum/category filter selects each visible row;
- whether collection semantics deduplicate or reorder entries;
- which rendered widget property retains the original action identity.

Mappings and input-event nodes alone may be invisible because the standard HUD
renders the provided-action collection instead. Prefer a strong reversible
probe on a known native action or filter over repeated speculative new fields.

### Evidence levels for actions and hints

These findings describe reusable contracts, independently of the mod in which
they were established. Evidence labels distinguish what is known from what is
only a plausible explanation.

- **Static fact**: established by stock asset structure, generated source or
  cooked-package readback. This does not establish runtime call timing.
- **Runtime fact**: observed in the real game for the stated implementation;
  not a guarantee for every vehicle or input device.
- **Reasonable inference**: an explanation consistent with the evidence, not
  a proven native implementation or a requirement to copy into another mod.

A game update alone does not discard these concepts or require repeating this
whole investigation. Check the concrete provider, reflected identities and
lifecycle when integrating with a different consumer or investigating changed
behavior. Version-bound binary snapshots and editor mirrors retain their own
provenance and compatibility gates.

### Action execution and action presentation are separate contracts

**Static facts.** `VoyageVehiclePawn.GetProvidedActionsBP` is an extension point
for supplying `FPlayerInputInterfaceAction` records. Stock
`BP_GyroCopter_Possessable` uses it for Drop Cargo.

| Record or input | Role in the established path |
| --- | --- |
| Enhanced Input mapping and event | Receives the actual button press; not sufficient to create a hint |
| `InputAction` | Exact action object used by the standard hint machinery |
| `Name` / `Category` | Provider metadata; not the visible description or a safe substitute for object identity |
| `Text` | Explicit visible description, obtainable from `InputAction.ActionDescription` |
| `bEnabled` | Presentation availability; keep the record present when disabled |
| `Type` plus widget context/filter | Selects the relevant standard row; these controls use `Central` |

Stock Gyro's Drop Cargo record remains present and its `bEnabled` is the OR of
the two mooring sockets' `IsSocketConnectedOrHasCable` results. Its description
comes from the action asset. The same present-but-disabled pattern works with
inventory availability instead of cable state.

**Runtime fact.** An inventory action can remain visible but disabled when its
source is empty or the destination is full.

**Execution contract.** An Enhanced Input `Started` handler can own the mutation
without a second invocation through the provided-action delegate. Recheck live
conditions before mutation. Do not assume a disabled hint alone
blocks an independently bound input handler, or wire both paths to the same
mutation without proving dispatch ownership.

For a hold action, `Started` alone is insufficient. Match the native elapsed-time
threshold in both the input handler and hint, release once per press, and reset
press state on completion, cancellation and controller changes. For the stock
Gyro release action, the native threshold is `DropCargoTriggerTime`. The
[mooring socket contract](mooring-cable-and-socket-knowledge.md) explains why
adding endpoints can require changes to both release and availability consumers.

Capture the original character before possession; after entering a station,
the currently possessed pawn is not the character's inventory owner. Refresh
inventory-dependent availability initially and from both inventories'
`OnInventoryChanged`, unbinding on exit/EndPlay and before rebinding.
The provider reads availability; it does not transfer items or discover objects.
See [inventory transfer and notification contracts](voyage-item-fabrication-and-pickup-knowledge.md#manual-transfer-between-character-and-module-inventories).

### Generated property reads need an explicit object context

**Static fact with runtime confirmation.** A generated
`UK2Node_VariableGet` for `UInputAction.ActionDescription` did not acquire the
intended external object merely from an unlinked target pin's `DefaultObject`.
The bad cooked expression was a bare `EX_InstanceVariable`, without an
`EX_Context` for the intended action; labels were absent in game. An explicitly
linked typed object reference supplies the correct read and restores labels.

An explicitly connected typed action reference supplies the getter target in
the authored graph. The compiler can lower a literal object node into a generated
object-reference property whose CDO default points to the action, rather than
an inline `EX_ObjectConst`. Validation must resolve that reference instead of
requiring one literal bytecode shape or a fixed generated property name.
For each action record, verify that its `Text` expression reads
`ActionDescription` in the context of the *same* object as its `InputAction`.
Compilation and a correct-looking editor graph alone do not prove this.

### Embedding the stock hint widget in a custom HUD

**Static facts.** The standard horizontal hint widget is the game class
`/Game/UI/Game/BP_DynamicPlayerInputHorizontalWidget.BP_DynamicPlayerInputHorizontalWidget_C`,
whose native parent is `VoyageDynamicPlayerInputWidget`.

Inside that class, `ContextInputActionsRoot` is the horizontal row and
`IndicatorSubClass` is `/Game/UI/Game/Interact/WBP_InteractIndicator`.
Its `ButtonInfoContainer` uses
`/Game/UI/Game/Inputs/BP_ButtonInfoDetailed_Vertical`; the visible description
leaf is `DescriptionTB`. These are stock-owned layers, not mod-owned labels
placed alongside independent key icons. The stock generated class has
`bClassRequiresNativeTick=true`: event-driven mod availability does not mean
the native hint widget is tick-free. Do not copy the empty editor stand-in's
behavior flags into the shipped stock implementation.

The following composition is present in the stock Gyro HUD and has also been
validated in a custom possessed-station HUD:

| Layout contract | Composition |
| --- | --- |
| Root composition | `InvalidationBox -> CanvasPanel -> stock hint widget` |
| Hint instance creation | Serialized in the HUD `WidgetTree`, not created by HUD `Construct` |
| Bottom-row placement | Direct Canvas child, bottom-left anchor/alignment, autosize, consumer-owned offset |
| Context and filter | Consumer's context asset and action-type filtering |

The `InvalidationBox` child slot serializes as `PanelSlot`, not an invented
`InvalidationBoxSlot`. A referencing hint instance can serialize just its
`ContextAsset`, `bFilterByActionType` and `Slot`, with its class and archetype
referring to the stock package. The actual row, leaf widgets, native updating
and stock defaults remain game-owned. Additional overrides need their own
functional reason; they are not required merely to reuse the widget.

**Runtime facts.** This composition supports descriptions, enabled
state changes and entry lifecycle without the observed one-frame text overlap.
Changing the HUD base to `VoyageIngameGenericVehicleWidget` was not required.
The consumer does not need to rebuild the outer hint instance when availability
changes. A working custom HUD uses `VoyageBaseUserWidget` as its native base.
That says nothing about whether stock native code recreates its internal
action children. Nor does it prove that every other HUD can use this base.

**Editor/cook contract, statically checked and runtime-validated.** A
runtime-loadable game Blueprint is not automatically available in the editor
project. To serialize the external class reference, generation can create an
empty editor-only `UWidgetBlueprint` with the exact stock package, asset name,
generated-class name and confirmed native parent. Save it for subsequent cook,
but exclude it from both the cook package manifest and the shipped container.
Use tagged properties for the partial native mirror and serialize only the
required instance overrides. Do not copy the stock internal WidgetTree,
indicator class, native logic or CDO into the release.

The absence gate is essential: accidentally shipping this empty stand-in would
replace the real stock implementation rather than reference it. Read back the
owned HUD *and* verify the final package inventory. This is a bounded pattern
for an external stock reference, not permission to guess arbitrary Blueprint
parents, subobjects or default values.

### What the transient overlap does and does not prove

The observed overlap occurred during an enabled-state transition. Empty labels
or a permanently enabled action can hide that symptom without satisfying the
requested presentation. Serialized creation, direct Canvas placement and the
invalidation root were validated together, not independently. The result does
not establish that `InvalidationBox` alone fixes the issue, that all runtime
`CreateWidget` use is wrong, or that a forced prepass is required.

### Reuse checklist

1. Confirm the provider's declaring class and separate external interaction
   actions from controls of the possessed station.
2. Wire the real input mapping/handler and provided-action record separately;
   choose one mutation path and revalidate eligibility there.
3. Read the label through an explicitly linked action reference. Match action
   identity, description context, `Type`, widget context and filtering in readback.
4. Prefer the proven stock-widget composition above for a new owned HUD;
   exclude editor-only references from the release.
5. Drive owned availability from lifecycle/domain events, including an initial
   refresh. Leave stock widget internals to the game; do not add cosmetic scans.
6. Check visible labels, an enabled/disabled transition and exit/re-entry in
   game. If wrapping the whole HUD in an invalidation root, also check its other
   changing indicators and animations. Video frame inspection can distinguish
   a brief missing row from overlapping text; static readback cannot.

## Selecting a complete vehicle HUD

The full HUD class and the standard provided-action rows are separate
contracts. In an earlier game snapshot, both `BP_JetSki_Possessable` and
`BP_GyroCopter_Possessable` implement
`/Script/Voyage.VoyageActorWidgetInterface` and their Blueprint event
`GetHUDOverrideWidget()` returns the vehicle-specific
`VoyageBaseUserWidget` subclass. This is the leading extension point when a
vehicle needs a genuinely different screen composition, such as an optical
first-person view, rather than one or two additional action hints.

Treat the exact declaring UFunction identity as unconfirmed until native
registration or a narrow runtime marker proves it. The cooked Blueprint export
shows the interface implementation and return type but does not serialize a
`SuperStruct` for this function. A same-name function on a convenient vehicle
stand-in is therefore not sufficient evidence of a valid override.

Prefer this actor-selected HUD lifecycle over a free `AddToViewport` overlay
when the entire vehicle display changes. Still validate creation, pause/menu
visibility, actor handoff, exit teardown, and reload in game; static asset
inspection proves the producer path, not every consumer-state transition.

## Stable UI composition

Do not assume returned-array order survives native collection processing.
Voyage may deduplicate through a hash set and reconcile widgets from sparse
slots. If visual grouping must be corrected, scan the complete rendered
collection, identify only the mod's elements by exact action UObject identity,
and preserve every unknown element's relative order. Child index, key text,
description text, and localized label are fragile identities.

An `AddToViewport` overlay is useful as a lifecycle marker but is not equivalent
to a standard hint: it can ignore pause hiding, rebindings, input-device glyphs,
layout rules, and other native HUD state.

### Cooked widget animations

For the maintained UE 5.8 HUD animation path, the generated animation property
and its inner `MovieScene` must share the logical name: cooked UMG initialization
resolves the property by the scene name. Display rate and evaluation tick
resolution are separate; express authored phase times at sufficient tick
resolution for ordinary widget frames to advance playback. Image color alpha
and animated render opacity multiply and should retain separate responsibilities.

These serialization/timing constraints do not by themselves prove visible
playback; the cooked HUD remains the runtime validation boundary.

### Replacing one native-updated leaf

When native code continuously rewrites one inherited widget property, a
one-shot child-Blueprint text change is not a replacement mechanism. The stock
Boat HUD restores its fuel text after `PreConstruct`; an override can access
the live field without owning the later text update.

For a narrow value replacement, keep the stock HUD as lifecycle owner, collapse
only the native-updated leaf, and add a separate `UUserWidget` to the leaf's
existing parent panel. Repeating mod logic can then live on a widget derived
directly from engine `UUserWidget`, avoiding an inherited Voyage HUD Tick CDO
delta. A tested Boat HUD replacement used this insertion while preserving the
stock fuel icon, position and neighboring values. Validate insertion and teardown
with a static marker before adding the resource query.

For resource displays, [local and grid-wide resource producers](boat-resource-socket-architecture-knowledge.md#data-path)
are separate contracts. The inherited field name `PetrolTB` does not identify
the resource being shown; select the resource enum and producer explicitly.

## Language and lifecycle

Voyage can keep its selected language in game-specific user settings rather
than synchronizing Unreal's current internationalization language. Locate the
actual setting consumed by the UI and read it at the lifecycle point where the
action descriptions are produced. Recheck enum values when the settings contract
changes or current evidence disagrees.

Display-only HUD work must not write vehicle input state. Keep control and UI
experiments separate so feedback, autonomous input, and crashes remain
diagnosable.
