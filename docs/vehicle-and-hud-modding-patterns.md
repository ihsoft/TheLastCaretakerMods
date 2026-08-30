# Voyage vehicle and standard-HUD modding patterns

This document extracts reusable patterns learned from DonkLift-320. It is a
starting model for other Voyage vehicles, not proof that every vehicle uses
the same concrete class, field, package, action, or widget lifecycle.

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

## Language and lifecycle

Voyage can keep its selected language in game-specific user settings rather
than synchronizing Unreal's current internationalization language. Locate the
actual setting consumed by the UI and read it at the lifecycle point where the
action descriptions are produced. Revalidate enum values after game updates.

Display-only HUD work must not write vehicle input state. Keep control and UI
experiments separate so feedback, autonomous input, and crashes remain
diagnosable.
