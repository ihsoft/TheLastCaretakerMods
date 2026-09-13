# HarpoonCannon: accepted architecture and pitfalls

## Evidence boundary

HC33 user acceptance, 2026-09-13, Steam25191271 / UE5.8, is the latest stable
operator/optics checkpoint. Exact executable/mapping hashes and dependencies are
in GAME_DERIVED_SOURCES.md; package and cleanup identities in the active backlog.
The latest test could be performed on ordinary targets; shark-specific behavior
was not separately retested because no sharks were nearby. Historical HC27
confirmed Talon Shark name/range. Displaying a name is not hooking eligibility.

Stable means the requested single-player entry/aim/HUD/exit test works. It does
NOT mean persistence, multiplayer, death/travel teardown, long-session operation,
firing, cable attachment or all strong-motion edge cases have been validated.

## Owners

| Concern | Owner / accepted contract |
| --- | --- |
| Construction, deck mount, dismantle/item identity | VoyageModuleActor shell at Cyclone leaf; native module/default-subobject contract |
| Unfinished construction cancellation | Stock location fabricator clones item meshes; base simple collision enables Q acquisition |
| Gaze acquisition | Character native InteractiveDetectorPointerComponent, station interaction component and query |
| Enter action | Own explicit InteractiveInterface implementation; native descriptor/delegate |
| Driver fixation and possession | Common VoyageVehiclePawn.OnEnterVehicle / OnExitVehicle |
| Station motion | Nonphysical native root attached to shell; no inherited mobile-vehicle behavior |
| Aim inputs | Own Enhanced Input MouseX/MouseY handlers; E Started exits |
| Optical view | Owned Engine CameraActor and CameraComponent, attached to station |
| Complete HUD | Own VoyageActorWidgetInterface.GetHUDOverrideWidget |
| Occupied exit hint | Base-owned GetProvidedActionsBP, matching input context, stock horizontal hint widget |
| Target display | First blocking optical hit, optional game item Name, actor-name fallback and range |
| Diagnostics | Separate observer/widget; not evidence of gameplay state unless correctly sampled |

## Construction boundary

Keep Cyclone's item /Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium
aligned with leaf /Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New.
Never override its small-turbine/Whisper parent: that caused Bad export index4607/19
in the untouched child. Three-package and five-package variants both failed;
extra meshes or stock connector were not proved the cause of that particular crash.

The final module and unfinished fabricator are different actors. Q became usable
after base-mesh simple collision, not more mount overlaps or an independent box
on the completed actor. Preserve it. Historical current-build shell revalidation
retained that contract; user waived a redundant Q retest after cleanup.
Do not expand this into a claim that every future update preserves cancellation.

The model remains separately owned. No stock-head/cable/preview-material change
belongs to a logic cleanup. Preview wireframe appearance was not fixed by simple
collision and is not evidence that cancellation collision is wrong.

## Station lifecycle

Autoload scans the exact built-shell class and pairs one station by owner.
Prepare the station before making entry available: native context/root/attachment
checks, no spawn inside Fabricated or a hover/provider callback. Failed preparation
stays blocked instead of endlessly spawning replacements.
Native root VehicleMesh is non-simulating/NoCollision; shell owns physical mount.
Use native vehicle entry/exit, not direct generic Possess plus manual UI hiding.
Exit retains the native character handoff; own camera/HUD/input helpers must not
destroy an occupied station. F8 and20-second timeout are experimental exit guards.

Ordinary walking and helm occupancy are distinct: HC10 observed no root parent +
deck movement base + Walking before/after; VehicleMesh parent + no movement base +
None inside. Limited displacement in that sample was not a strong-motion proof.

## View, input and HUD

Use the common vehicle parent, not Drone/Forklift behavior. Enhanced Input actions
have their own execution handlers; GetProvidedActionsBP describes hints, not
movement execution. Matching context identity connects descriptor and hint widget.
HUD visibility alone never disables character actions: HC15/16 demonstrated that.
Generic Pawn possession improved routing but left fallback UI artifacts.
The own vehicle HUD/interface path resolved this in HC26 onward.

Camera is an owned Engine actor, not a borrowed Forklift third-person manager.
A native first-person flag was true in HC24 while actual FOV stayed100 degrees:
requested flags are not proof of final POV selection.
Compute x5 as 2*atan(tan(entryFOV/2)/5), not literal5 degrees. Re-entry samples
on-foot FOV again. Station-local yaw wraps360 degrees; pitch clamps -50/+10.
Current owned-axis scalar1.024 = HC32's1.28 *0.8. No raw player rotation writes.
Preserve useful central character stats as requested by the user.

## Optical hit display

LineTraceSingle: Visibility, simple collision, first blocking hit, <=100000cm.
Ignore shell and original character, preserve ignore-self. Do not select an actor
behind the first obstruction. Display fields clear before the trace every update.
A valid actor first writes GetObjectName fallback and rounded character-to-impact
meters. Optional VoyageModuleComponent.ItemAsset -> VoyageBaseDataAsset.Name
replaces it only if valid and nonempty. Missing component/item/name keeps fallback.
Historical DetectedSharkName property name is retained for widget compatibility;
its contents are now generic. This is display, not creature classification.
Technical actor names may contain generated instance identifiers and are not save IDs.

## Failed approaches / retained lessons

- Drone autoload spawn/hints/entry/exit worked; permanent pre-entry ship mounting
  later caused sinking/circling. Root simulation off did not isolate all native
  forces/collision/activation callbacks. Retire the physical donor, not more patches.
- World-space diagnostic text updated intermittently drifted/smeared on a moving
  ship. Measure parent/local transform separately and use screen-space diagnostics.
- Missing barrel was an immediate branch marker; no independent delayed marker
  meant it did not prove timed actor destruction. Do not ask the user to stopwatch it.
- HUD text placed after successful shark-hit branches was unreachable on misses.
  Markers must precede filters or use independent execution; bytecode presence is
  not reachability. HC33 fallback/range also precedes optional item gates.
- Partial native mirrors plus unversioned serialization can compile/cook/retoc yet
  corrupt inherited properties. Tagged headers AND independent semantic inspection
  are mandatory for this station; no guessed padding or convenient native members.
- An interface method of the right name is insufficient without class membership.
  HC31 acquired the right component but never called the provider; HC32 explicit
  K2 membership fixed entry. See VEHICLE_ENTRY_RESEARCH.md.
- Stock assets are useful contracts, not a complete parent to copy. Forklift's
  ExitAction was mandatory in its native provider; missing it caused null+0x28.
  That donor-specific field is not a new dependency of our dedicated station.
- Multiple no-op gameplay tests are costly. After one/two no-ops, localize acquisition,
  provider, callback, native handoff, input and render separately before another test.

## Recovery / research methods

Public tool routing is in tools/README.md. Reusable external interaction observer
is tools/Read-VoyageInteractionState.py with fresh UObject discovery and serial/cache
validation. No writes/injection. Native addresses are fingerprint-bound research
anchors, never runtime mod code. The exact source and old evidence archive is
indexed by mods/HarpoonCannon/PIPELINE_OBSERVATIONS.md; do not reopen it routinely.
