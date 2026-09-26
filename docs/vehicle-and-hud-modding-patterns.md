# Voyage vehicle and standard-HUD modding patterns

This document extracts reusable patterns learned from DonkLift-320 and
GyroKeyboardControl. It is a starting model for other Voyage vehicles, not
proof that every vehicle uses the same concrete class, field, package, action,
or widget lifecycle.

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

To prevent continuous stock force from rebuilding speed, retain the previous
velocity commanded by the helper. In each stabilized subspace, first reject
observed growth beyond that previous command, then apply the configured
constant-rate decay. On first entry there is no trustworthy previous sample;
seed it from the observation. Clear sample validity whenever control ownership,
the actuator-speed gate, throttle eligibility, or stabilization mode changes so
stale commands cannot create a transition jump.

Use local control intent, not a moving component's world orientation, to decide
which motion is permitted. Combine the vehicle's world-horizontal forward and
right axes using its native forward/side input values, normalize the result,
and preserve only positive velocity along it. Neutral input damps all horizontal
velocity; sideways and opposing components still decay under directional input.

For a smooth transition from physical climb to exact altitude hold, brake only
positive vertical speed at a constant configured rate. Reject renewed positive
growth relative to the previous command before each braking step. Once the
result becomes non-positive, capture the current Z and activate exact hold in
that same tick; an already descending vehicle enters hold immediately. Gate all
kinematic corrections on a measured live actuator state such as rotor speed,
and clear their state below the threshold, otherwise a disabled or obstructed
vehicle can be pinned unnaturally to an obstacle.

## Replacing a Blueprint while preserving original behavior

A loader-free IoStore mod can preserve a complex original Blueprint by:

1. freshly extracting the complete current-game package;
2. relocating it to another verified package path;
3. placing a generated child at the original path;
4. adding only the new component or override to that child.

This is inheritance from a snapshot, not from the developer's future asset.
Every game update requires a new fingerprint, fresh extraction, and renewed
compatibility testing. If relocation patches cooked bytes, assert exact path
lengths, occurrence counts, and container contents rather than accepting an
approximate match.

## Exact native identity matters

Cooked Blueprint references bind to exact `/Script/Module.Type` and parent
UFunction identities. An editor mirror must preserve the declaring class,
inheritance, function owner, struct fields, and enum values used by the graph.
A same-name function declared on the wrong stand-in class can compile and cook
yet never override the shipping callback.

Keep native mirrors minimal, provenance-marked, and editor-only. Separate the
game-identity mirror module from the hand-written editor commandlets that
generate mod assets.

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

## Selecting a complete vehicle HUD

The full HUD class and the standard provided-action rows are separate
contracts. In Steam build `23962331`, both `BP_JetSki_Possessable` and
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

### Replacing one native-updated leaf

When native code continuously rewrites one inherited widget property, a
one-shot child-Blueprint text change is not a replacement mechanism. On Steam
build `23962331`, a bound `UUserWidget::PreConstruct` override on the stock Boat
HUD successfully changed the inherited fuel text's render opacity, but native
logic restored its litre string afterward. This discriminates live field access
from update ownership: the override and field were correct, the write timing was
not.

For a narrow value replacement, keep the stock HUD as lifecycle owner, collapse
only the native-updated leaf, and add a separate `UUserWidget` to the leaf's
existing parent panel. Repeating mod logic can then live on a widget derived
directly from engine `UUserWidget`, avoiding an inherited Voyage HUD Tick CDO
delta. The Boat HUD `TOTAL SLOT` probe validated this exact insertion while
preserving the stock fuel icon, position, and all neighboring values. Validate
insertion and teardown with a static marker before adding the resource query.

The following aggregate Boat candidate also validated the data path on Steam
build `23962331`: its separate leaf obtained the possessed Boat's module,
enumerated `VoyageModuleComponent` instances in the same grid through
`VoyageModuleSubsystem`, summed only
`GetResourceAmount(EModuleResourceType::Diesel)`, and displayed the confirmed
total in the stock lower-right slot. Keep the resource enum identity explicit;
the inherited field name `PetrolTB` does not identify the resource being shown.

## Language and lifecycle

Voyage can keep its selected language in game-specific user settings rather
than synchronizing Unreal's current internationalization language. Locate the
actual setting consumed by the UI and read it at the lifecycle point where the
action descriptions are produced. Revalidate enum values after game updates.

Display-only HUD work must not write vehicle input state. Keep control and UI
experiments separate so feedback, autonomous input, and crashes remain
diagnosable.
