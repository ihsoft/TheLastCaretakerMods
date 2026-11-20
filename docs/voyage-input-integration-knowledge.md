# Voyage input integration (observed stock contracts)

Use the game's action objects and input processing when extending controls.
Keep action identity, device mappings, input lifetime, camera processing and
action presentation as separate responsibilities. This complements
[vehicle and HUD patterns](vehicle-and-hud-modding-patterns-knowledge.md).

## Reuse actions, not default buttons

Two actions can share a default key and still have independent user bindings.
Select the exact action by its gameplay role, never by its default key or
localized label. Character action objects live under
`/Game/Game/Input/Character/`; vehicle actions under `/Game/Game/Input/Vehicle/`.

| Role | Stock action |
| --- | --- |
| Interact (2) | Character `IAV_InteractTwo` |
| Primary Action | Character `IA_PrimaryAction` |
| Aim Down Sight | Character `IAV_ADS` |
| Reload | Character `IA_Reload` |
| Look horizontally / vertically | Character `IA_LookRight` / `IA_LookUp` |
| Exit Vehicle | Vehicle `IAV_VehicleExit` |

An interaction provider supplies the exact action in
`FPlayerInputInterfaceAction.InputAction` and its callback in `OnTriggered`.
This uses the character's interaction dispatch; it does not need a separate
key listener. For independently bound action events, choose one execution
owner so provider callbacks and Blueprint events cannot perform the action
twice. Match the event phase to the stock trigger: `Started` is not completion
of a `Tap` trigger.

## Preserve mappings and their owner

An action reference alone does not activate its keyboard or gamepad mapping.
Use the live `VoyageInputContextAsset` and its device-specific mapping contexts.
Preserve action identity, player-mappable names, triggers, modifiers and context
settings; rebuilding equivalent-looking default keys loses these contracts.

When a mod needs only part of a stock context, duplicate the context asset and
its mapping contexts for the owning instance. Filter by an allowlist of exact
action objects, keeping the existing mapping entries for retained actions.
Snapshot the mapping array before removing entries. Do not mutate shared stock
assets or filter with a list of today's unwanted actions. Keep owned references
alive for the full input lifetime.

The native `VoyageInputControlsComponent` sets up input on pawn restart and
tears down input for the previous controller on controller changes. Its refresh
removes its context's mappings before adding the selected device mappings.
Configure owned contexts before input setup and possession. Avoid independent
add/remove paths competing with this native owner.

The gameplay `VoyagePlayerController` refreshes every input-controls component
on its current pawn when the device changes. The component's own device callback
is only one part of that lifecycle; do not replace controller coordination with
device polling or synthetic restarts. Reflected methods are not necessarily
Blueprint-callable: editor mirrors must reproduce the actual function flags.

## Action values and compiler-owned bindings

`EnhancedInputLibrary.GetBoundActionValue(Actor, Action)` reads the supplied
actor's `EnhancedInputComponent`. It needs an action-value binding for that
exact action; an event binding alone does not provide one. A helper that reads
another actor's controls must query the input owner, not itself. Missing value
bindings return zero, which can conceal a setup error as neutral input.
`Conv_InputActionValueToBool` tests nonzero magnitude for both boolean and
scalar action values.

The Blueprint compiler rebuilds `DynamicBindingObjects`, so a manually appended
binding does not survive recompilation. For maintained Blueprint graphs, keep
connected `/Script/InputBlueprintNodes.K2Node_InputActionValueAccessor` nodes
with the required `InputAction` identities on the input-owning actor. Those
nodes register value bindings during compilation. Check that the cooked class
retains the required value bindings alongside any event bindings; a direct
library call or a successful editor read alone does not establish that contract.

Trigger events and polled values have different lifetimes. In the inspected
engine, `EnhancedInput.bAlwaysGetRealValueFromActionInstanceData` defaults to
true: action instances return their current value independently of trigger
state. A `Pressed` mapping can therefore remain nonzero while held. A consumer
that needs one operation per press from a polled value must detect the rising
edge and clear its transient latch on release and the relevant ownership,
disable and teardown paths. Consume the edge before updating the latch; avoid
reevaluating a pure edge expression after changing its previous-value input.

These binding/compiler and value-lifetime mechanisms were checked in UE
`5.8.2` source; the consumer evidence comes from Steam build `25982825`.
Recheck the affected mechanism if compiler output, configuration or observed
input behavior differs. Action identities and consumer-specific reset paths
remain owned by the mod.

## Reuse camera settings at the processing boundary

`VoyageMiscBlueprintFunctionLibrary::CalculateCameraMovementValue` takes a
Voyage player controller and a Vector2D and applies the game's camera settings,
including device sensitivity, inversion, acceleration and ADS scaling. It is
an executable Blueprint call, not a pure expression; wire its execution pins.
Feed it mapped action values so mapping modifiers remain in effect.

Its ADS input comes from the controller's current pawn, that pawn's
`VoyageWeaponHolsterComponent`, and the active equipment actor's native ADS
amount. A custom zoom boolean or a changed FOV does not establish that state.
Reuse native state updates rather than copying the interpolation formula.

Apply the processed result to the consumer's own motion model. Input processing
does not prescribe a vehicle orbit camera, a character controller or a direct
rotator. Resolve axis conventions at that boundary without stripping mapping
modifiers or implementing a second copy of the player's inversion preference.

## Native equipment state for ADS

For an animation-free station consuming the native ADS camera path, the holster
still needs a real equipment definition and actor. These are inspected UE 5.8
native contracts, not a guarantee for arbitrary equipment types or later updates.

- `OverlaySlotIndex` is derived from the first `Overlay` entry in `HolsterSlots`.
  Without it, the native overlay path returns. Do not fabricate internal slot
  indices. An overlay does not need a matching ordinary `BarSlotConfig`; native
  insertion can create the missing inventory slot record. Hand equipment must
  have `DesiredSlot` matching its holster slot type and a nonempty primary-asset
  identity.
- `AddOverlayState` takes a key, a `VoyageHolsterSlotOverride` containing a weak
  item reference and priority, and a force flag. It is not an arbitrary actor
  insertion API. The inspected creation path supports `VoyageItemHandEquipment`
  and `VoyageItemConsumable`; a bare `VoyageItemBaseEquipment` is insufficient.
  `RemoveOverlayState` is BlueprintCallable, returns bool, and returns false for
  an absent key. Reset ADS before removing the overlay on possession loss;
  removal does not establish immediate actor destruction or a finished transition.
- `DrawFromHolster` sets `bEquipped` through the equipment interface before its
  animation attempt. Missing animation instance/asset, or zero returned duration,
  selects a native fallback rather than unconditionally rejecting the draw.
- Attachment tries the equipment skeleton socket, then `SkeletonSocketTag`, then
  `EquipmentComponentTag`. If all are empty, it attaches to the owner's root and
  applies the equipment attachment transform; this branch needs no skeletal mesh
  or hand socket.
- `SetInterpState(type, enabled)` is a callable void setter;
  `HasInterpState(type)` is a pure bool query distinct from the interpolation
  amount. The equipment's native update owns transition timing. A setter call is
  not proof that ADS interpolation has completed.

## Present actions separately from context lifetime

Standard hints are driven by provided-action records, not merely by Enhanced
Input bindings. Multiple input contexts do not imply multiple hint widgets:
the stock renderer can show the provider's complete action list. Keep the
renderer count tied to the intended UI, not the number of input components.
For a small independently placed subset, the
[compact action-card row](vehicle-and-hud-modding-patterns-knowledge.md#a-compact-row-of-selected-action-cards)
describes stock card selection and layout without fixed per-card positions.

Validate remapping, settings changes, repeated possession and release, and
device switching as distinct behaviors. A successful keyboard test alone does
not exercise a gamepad's axis modifiers or device-change lifecycle.

Native APIs described here were observed in a UE 5.8 game snapshot. Try the
public candidate path first; revalidate an affected identity or lifecycle
contract when an Update changes it or the build or game test finds a mismatch.
