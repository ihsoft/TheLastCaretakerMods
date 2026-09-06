# HarpoonCannon: owner contracts and research boundaries

## Evidence scope

Current static inspection: Steam build `25056839`, executable SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
The reviewed mapping resolver selected the build-25056839 mapping and UE5_8
parser profile. Evidence from 2026-09-05 Pacific is indexed in
`../../docs/harpoon-cannon-tool-report-2026-09-05.md`.

The construction successes and operator crashes in the gameplay backlog were
on build 23962331. They are useful architectural evidence, not validation of
the current native mirrors. Subsequent current-build HC01/HC02 runtime evidence
is recorded below; it validates standalone Drone observation/acquisition only.

## Construction owner

Current stock data still connects Cyclone's
`/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium` to
`/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New`.
The item remains dismantlable; the actor remains a child of the small-turbine
Blueprint and its custom module component names that exact Cyclone item.
This refreshes the identity chain only, not the native parent constructor,
full unversioned property prefixes, carrier registration or persistence.

Historical runtime evidence favors a module shell as the structural owner.
The unfinished object is a different actor: the stock location fabricator
clones the item's mesh components. Base-mesh simple collision made its Q
cancel target discoverable; changing the final shell's separate collision box
could not do that. Preserve this boundary while renewing the current build.

## Telescope is an optical consumer, not an operator parent

Current `BP_Equipment_Telescope` retains the Binoculars parent and overrides
StateKey `Telescope`, InitialLockFOV `5`, FOV choices `[5]`, its telescope
overlay widget and `bMeasureDistance=false`.

Current `BP_Equipment_Binoculars` separates:

- camera effects: obtain the owning controller's Voyage camera manager,
  `AddOverridePPBlends(StateKey, ...)`, and `SetLockedFOV(StateKey, FOV)`;
- visual HUD: create the overlay for the owning player, add it to the player
  screen, and request game-state widget visibility `2`;
- exit cleanup: release the keyed FOV/post-process, restore visibility `0`,
  and remove the overlay.

These inspected optical functions do not possess another pawn. They are useful
consumers for a future occupied cannon, but do not supply its interaction,
driver fixation, first-person camera transform or mouse-input owner.
The literal 5-degree FOV is not 5x magnification. For base FOV F and 5x linear
magnification use `2*atan(tan(F/2)/5)` with consistent angle units; at F=90
degrees this is about 22.62 degrees.

Do not copy the stock cleanup graph uncritically. The current exported control
flow removes a valid overlay and returns; delegate-unregistration code is on
the other branch. This observation requires a separate lifecycle audit, not an
assumption that every stock cleanup action is unconditionally symmetric.
Montage/AnimNotify producers, widget native ancestry and exact camera native
signatures from the old research have not all been renewed in this iteration.

## Drone supplies several independent contracts

Current item `/Game/Data/Assets/Items/DA_CameraDrone` is a module whose dropped
actor is `/Game/Blueprints/Vehicles/BP_CameraDrone`; its localized source name
in the inspected item is `Photo Drone`.

The Blueprint's external provider calls `GetInteractActions`, returns both
`LootDrone` and `EnterDrone`, and binds them to stock `IAV_Interact` and
`IAV_InteractTwo`. Entry delegates to `InputAction_EnterVehicle`, which checks
the Drone module's resource amount (serialized resource type 0) is positive
before calling `OnEnterVehicle(InputAction.Controller)`.
Thus resource gating can block entry, but this graph still constructs both
actions: absent energy alone is not an explanation for no hints at all.

`SetDroneActive` changes gravity, active collision, light/material state and
optional deployment. It is not a complete creation/registration contract.
`ReceiveBeginPlay` initializes camera state, subscribes to a snap-socket
connection and schedules a one-second delayed snapping-state update.
`OnDriverChanged` separately owns activation, timers, visual first-person
parts, camera fade, rotation reset and buoyancy updates. This is a substantial
mobile vehicle lifecycle, not a neutral stationary operator.

The Blueprint CDO's VehicleMesh explicitly references `PhotoDrone_Body`.
The item's DropVariations separately references `SM_Camera_Drone_Full`.
Missing item-drop setup is therefore not proof that a directly spawned Drone
has no body mesh. In the inspected Blueprint, explicit DestroyActor calls
target HighlightActor, not a demonstrated two-second self-destruction branch.
This does not rule out native destruction, collection, streaming, registration
failure or a defect in the diagnostic continuation.

Current CDO component inspection additionally confirms DroneModuleComponent
already references DA_CameraDrone, specifies MaxResources Electricity=100 and
ConfigData DefaultResourceAmount/MaxResourceAmount=100. These are configuration
defaults, not proof of actual resource amounts in a directly spawned instance.
Do not add duplicate item identity/charge setup without a runtime discriminator.
The VehicleMesh response list explicitly overlaps `Interact`; the separate
ActiveCollisionSphere ignores `Interactive` and starts NoCollision. Do not
conflate those channel names, or actor-wide collision flags with the actual
component/channel hit path. HC02 observes natural hints and actor flags before
changing registration, resource or collision state.

Historical marker limits:

- barrel hiding was a positive immediate-spawn branch marker;
- the later marker reused that hidden state and restored it only on a valid
  delayed result; it had no independent visible continuation marker;
- no restoration cannot distinguish early actor invalidation from failure to
  reach the delayed branch or an incorrect stored-reference path;
- do not demand stopwatch measurements from the user. Either audit the owner
  path or design independently observable outcomes before another runtime test.

## HC01 standalone lifetime evidence (current-build runtime)

The user tested the separately installed autoload candidate
`artifacts/harpoon-cannon/hc01-candidate-05`. Its readable world-space board
reported initial spawn VALID, final independent sample ALIVE at 8.002636
observation seconds and camera-to-Drone distance 455.186035 cm. Thus a stock
CameraDrone created through this Engine-only standalone path can retain a valid
actor reference for at least this observed window, without calling SetDroneActive
or binding it to the construction shell. The non-latent observer reached DONE.

This rejects necessary immediate/two-second invalidation for this setup; it
does not explain the old missing-barrel test, validate item setup/action hints,
or prove physical visibility, successful cleanup, save/load or travel safety.
The terminal text says cleanup requested and preserves a pre-cleanup sample.
Do not infer completed destruction from it. Runtime record and screenshot
identity are in the active backlog; no original asset snapshot was replaced.

## HC02 natural interaction evidence (current-build runtime)

The user screenshot from installed `hc02-candidate-01` visibly shows the Photo
Drone on the deck, sampled ALIVE at 21.406092 seconds, with hidden NO and actor
collision ON. The stock HUD offers Loot, Enter and Grab for Photo Drone while
the character aims at it. This validates natural target acquisition and the
stock interaction-hint path after standalone Engine-only spawning. No manual
item/charge/registration/collision/activation change was needed for those hints.

Do not promote hints to successful entry, resource initialization, exit,
cleanup or persistence. The old construction-shell test remains a different
owner/version path; neither a broken shell nor latent continuation is proved
the sole old cause. Next entry/exit probe must prevent timed destruction of
an occupied Drone before the user is instructed to press Enter.

## HC03 native entry/exit evidence (current-build runtime)

The unchanged standalone stock-spawn path, with observer destruction removed,
supports native Drone entry. The user reports successful entry and ordinary
Drone behavior; the HC03 screenshot shows GetPlayerPawn equal to the spawned
Drone, sticky control-observed YES, and the native vehicle view/action HUD.
Exit Vehicle, Change mode, Toggle light and Take Sample are offered. The
resource gate allowed entry without manual charge/item/activation mutation.
This validates possession and native view/HUD acquisition in this setup, not
every action, complete native cleanup, persistence, or a stationary Harpoon
operator. At the entry screenshot, return to the original pawn is still NO.
The user subsequently explicitly confirmed successful exit. Native user-driven
entry/exit is therefore game-validated for HC03, without an independent captured
post-exit pawn-identity sample or proof of every native cleanup operation.
The stock Drone's mobile lifecycle is inherited behavior, not yet constrained
to the requested cannon yaw/pitch, fixed mount or telescope optics.

## Drone movement/view split: HC04 discriminator

Fresh stock summaries and pseudocode on Steam25056839 show Blueprint flight
applying forces to MeshComponent and torque toward a desired rotation. The CDO
binds MeshComponent and RootComponent to the same native VehicleMesh object.
OnLookUpChanged and OnLookRightChanged call controller pitch/yaw input only
when IsActive and not IsSnapped. SetDroneActive(false) and forcing a snap state
are therefore poor isolation experiments: they can suppress the desired view
input. Disabling all Tick also conflates movement, zoom and other lifecycle work.

HC04 instead requests Engine PrimitiveComponent.SetSimulatePhysics(false) once
on the dynamically cast root after stock possession, preserving input and native
active state; restore the captured prior flag on exit. Static code supports this
as a discriminating test, not proof that native code cannot re-enable simulation,
that camera rotation survives, or that all movement is physics-driven. Observe
both simulation and world displacement. World stationarity is deliberately not
a moving-ship attachment solution. No native mirror is needed for this test.

HC04 user runtime result: mouse look works while translational movement does
not. This supports separating native view input from the physical flight body
without deactivating the Drone or suppressing all input/Tick. No screenshot of
physics flags or numeric drift accompanied the report; do not infer measured
zero displacement or a confirmed simulation flag. The user subsequently confirmed
working exit under HC04 as well. The behavioral test passes; the post-exit body
simulation flag and complete cleanup were not independently captured. This is
not yet a ship-relative mount.

## Shark classifier


The current full package inventory identifies the same three shark Blueprint
and gameplay-data pairs. Fresh Blueprint exports confirm:

| Blueprint | PlayerModuleComponent.ItemAsset |
| --- | --- |
| BP_NPC_Shark | /Game/Data/NPCData/DA_NPC_Shark |
| BP_NPC_RamShark | /Game/Data/NPCData/DA_NPC_RamShark |
| BP_NPC_Shark_Laser | /Game/Data/NPCData/DA_NPC_LaserShark |

Ordinary and laser sharks meet only at the general BP_NPC_Simple_Base;
RamShark derives from ordinary Shark. A simple ordinary-Shark class test misses
laser sharks; the general NPC base overmatches non-sharks. Prefer the exact
gameplay-data whitelist, not localized names. Name-based inventory discovery
is evidence for these identifiable variants, not proof that an unrelatedly
named future creature cannot be a shark. Renew the inventory after updates.

Trace origin/direction remain the optical-axis component. Distance remains
character location to hit impact point. This inspection did not implement
runtime tracing or promote the old generator to current-game compatibility.

## Next operator discriminator

Inspect the stock module turret's actual ownership before selecting a parent.
Its presence by name is not evidence of manual possession. That exact asset's
initial public JSON request failed in optional pseudocode rendering; the
pipeline subsequently repaired the public interface. Renew inspection through
the current public tool rather than using rejected staging output. Keep the
module shell, operator lifecycle and Telescope optics
as separate decisions until the native interaction boundary is understood.
