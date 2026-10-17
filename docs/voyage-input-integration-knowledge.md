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

## Present actions separately from context lifetime

Standard hints are driven by provided-action records, not merely by Enhanced
Input bindings. Multiple input contexts do not imply multiple hint widgets:
the stock renderer can show the provider's complete action list. Keep the
renderer count tied to the intended UI, not the number of input components.

Validate remapping, settings changes, repeated possession and release, and
device switching as distinct behaviors. A successful keyboard test alone does
not exercise a gamepad's axis modifiers or device-change lifecycle.

Native APIs described here are bound to Steam build `25956314`, UE 5.8.
Revalidate game-specific identities and lifecycle contracts on a Steam build change.
The [Railgun architecture](../mods/Railgun/ARCHITECTURE.md) owns its concrete
station, filtered-context constraints and equipment implementation.
