# HarpoonCannon: accepted architecture and pitfalls

## Evidence boundary

User acceptance of clean-hud-02 + visual-v5-palette-02 on Steam25191271 / UE5.8
establishes the operator/optics/model checkpoint. Exact hashes and dependencies are
in GAME_DERIVED_SOURCES.md; package and cleanup identities in the active backlog.
Target-name/range display was accepted on ordinary objects and sharks. Displaying
an actor name does not establish hooking eligibility. Native firing and shark
kill were subsequently validated; see the direct-hit contract below.

Stable means the requested single-player entry/aim/HUD/exit test works. It does
NOT mean persistence, multiplayer, death/travel teardown, long-session operation,
cable attachment or all strong-motion edge cases have been validated.

## Owners

### Firing research boundary

Current-build stock BP_Module_Turret derives from VoyageModuleActor and has
OutputLocation plus CreateAbilityComponent/StartFiring/ActivateAbility events.
Its cooked ubergraph references VoyageCombatBlueprintFunctionLibrary.ActivateAbility
and ActivateBallisticAbility, and WeaponAbilityComponent.GetWeaponData. Individual
event wrappers only jump into that shared graph; the summary alone does not prove
their internal sequence, payload, hit ownership or collision exclusions. The
implemented direct-hit path below uses native movement and combat submission,
not the complete stock weapon ability.

Camera measurement (Steam25191271): user screenshots show character root drift
0cm, native GetActorEyesViewPoint drift below0.15cm, but FirstPersonCamera local
position changes from (53.980,8.241,34.687) to (42.140,-35.868,29.215)cm.
Native eye getter is therefore not interchangeable with this camera position
while operating the station. Runtime enumeration also found ThirdPersonCamera.
SK_Head component origin stayed (0,0,-102)cm; this says nothing about animated
bones/eye sockets. Following FirstPersonCamera position was subsequently accepted
in game, followed by clean HUD/circle and Toggle scope acceptance. Animation and
lifecycle edge cases are not comprehensively validated.

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

## Firing ownership (Steam25191271, UE5.8)

Stock BP_Module_Turret separates control from weapon execution. Its
CreateAbilityComponent creates a WeaponAbilityComponent from a template,
configures it before FinishAddComponent, and its firing graph calls
VoyageCombatBlueprintFunctionLibrary.ActivateBallisticAbility with ability
configuration, component location, forward vector, ammo and ability component.
The non-ballistic ActivateAbility path takes configuration, location, direction
and an actor instead. These are different contracts, not interchangeable helpers.

Native registration and exec thunk confirm the ballistic signature:
`bool ActivateBallisticAbility(const FVoyageAbilityConfig& Config,
const FVector& InOrigin, const FVector& InDirection, UVoyageItemAmmo* ItemAmmo,
UObject* Instigator)`, declared by VoyageCombatBlueprintFunctionLibrary.
The reflected parameter block is96bytes; input offsets are0/24/48/72/80.
The return is a BoolProperty; do not decode its offset with the ordinary
non-bool property layout.

The native body builds activation args itself. It distinguishes
VoyageEquipmentActor (controller reference at its own native member plus actor)
from an ordinary Actor (null controller plus actor); an unrelated UObject such
as an ActorComponent falls through to null actor/controller in this branch.
Thus the turret's component argument is not proof that a component supplies
shooter attribution. This does not establish the downstream collision policy.
Origin/direction are copied from the supplied vectors and ActivationMultiplier
is set to1. The function returns true after issuing its downstream virtual calls;
that alone is not proof of a spawned projectile or confirmed hit.
Activation args mappings list Origin, Direction, ActivationMultiplier,
InstigatorController, InstigatorActor, AttackID, TraceResult and ActivationIndex.

BP_Weapon_Projectile inherits native UtilityProjectile and uses
VoyageProjectileComponent / VoyageProjectileMovementComponent. OnSpawn receives
ProjectileAbilityComponent, VoyageWeaponDataStruct (serialized parameter size1152)
and VoyageEquipmentAbilityActivationArgs (size344). Spawning the actor alone is
not equivalent to ability-driven initialization. Do not invent padding or mirror
these structs partially as serialized defaults.

Its IgnoreCollisions function calls IgnoreActorWhenMoving for an actor array;
PerformAttack forwards an assembled attack to VoyageCombatSubsystem.RegisterAttack.
The initialization graph adds InArgs.InstigatorActor to IgnoreActorsArray,
appends a GetOwnedActors result, then calls IgnoreCollisions (bytecode7147-7405).
The producer/receiver of GetOwnedActors still needs tracing; this is not proof
that attachment to a ship makes the whole ship ignored.
GetImpartedMovementBaseVelocity is called on a CharacterMovementComponent;
its result feeds a vector addition with ProjectileMovement.Velocity and a Niagara
InheritVelocity parameter. A stationary vehicle lacking character movement cannot
be assumed to inherit ship velocity through this branch. Exact branch guards and
receiver acquisition still need tracing before reuse.
The base has configurable bounce, sweep damage, water and destruction handling;
it is not yet a validated harpoon attachment implementation.

DA_Ammo_Bolt_Rifle_762 supplies both a projectile class and an instanced projectile
template, weapon data, mesh and effects. Do not replace this setup with a bare
SpawnActor call or treat its bullet damage/speed as harpoon balance.
Evidence identities and remaining integration gates live in the active backlog.

### Direct-hit attack submission contract

On Steam25191271 / EXE747DC255...F58B, stock PerformAttack copies
WeaponData.Attack, writes Hit, Target, Instigator (Controller), DamageCauser
(projectile self) and AttackID, then calls VoyageCombatSubsystem.RegisterAttack.
Native registration confirms `void RegisterAttack(FVoyageAttack Attack,
bool bAcceptDuration)`; Attack is by value, not an output/reference parameter.
The native parameter block is456 bytes; native VoyageAttack is448 bytes.
Cooked stock Blueprint metadata reports464 bytes for its local VoyageAttack and
264 for HitResult; these must not be treated as shipping-native memcpy sizes.
The native constructor initializes private trailing state beyond the reflected
AttackID at368. Any editor mirror must be bytecode/named-field-only, with no
native struct CDO/default serialization or invented padding.

Reflected AttackType values are Unknown0, Directional1, Radial2, AreaEffect3,
Force4. RegisterAttack routes Directional to0x145559dca and Radial/AreaEffect to
0x14555980e. Directional checks a Hit flag and deduplicates target against AttackID.
Zero AttackID is assigned from the subsystem counter at0x145559469..48a. A valid
DamageCauser weak reference is required before dispatch (0x1455594bb..4c8).
bAcceptDuration permits a separate duration path when DurationAttack.duration>0;
it is not a success output. Submitting an attack is not proof of health loss.

The rifle ammo export uses BP_DamageTypePhysicalForce, Damage5, AreaEffect,
InnerRadius/OuterRadius10 and ImpulseOverride5000. Those settings are not a
harpoon specification and must not be copied indiscriminately. The damage-type
Blueprint parent is VoyageDamageType, whose parent is Engine.DamageType.
VoyageCombatSubsystem derives from TickableWorldSubsystem, not plain UObject.
First direct-hit integration should preserve native construction of the attack,
use the original ReceiveHit result, explicit Directional type, valid shot causer
and firing controller, and distinguish submitted attack from observed damage.

Real-game validation on this fingerprint: the own swept projectile submitted
Directional attacks with fixed200 base damage, stock PhysicalForce damage class,
original ReceiveHit/target, shot causer and captured controller, with duration
disabled. User killed a shark after approximately four shots and reported normal
operation. This establishes actual damage delivery and kill, not merely aggression
or submission. It does not establish200 effective damage per hit,800 starting
health, universal target compatibility, or final weapon balance. Independent
per-click spawning is accepted; the single-hit guard remains per projectile.

### High-speed detection boundary

Current fingerprint rechecked: Steam25191271 /747DC255...F58B. Reviewed mappings
show VoyageProjectileMovementComponent derives from ProjectileMovementComponent.
Stock BP_Weapon_Projectile.ProjectileComp serializes MaxSpeed300000cm/s
(3000m/s), above the proposed2000m/s. This is a configured cap, not proof that
Voyage's runtime overrides or collision handling work correctly at that speed.
Local UE5.8 ProjectileMovementComponent.h documents bSweepCollision as swept
movement and straight-line movement as not normally needing forced substeps.
Disabling sweep means teleporting without blocking hits. The stock export does
not explicitly serialize bSweepCollision. Its registered bool setter at
0x142917c10 sets bit0x40 at component offset0x130. The engine constructor
0x142920b40 enables that bit (`or al,0x68`); Voyage's constructor0x1454d6eb0
calls it and does not write that byte. This establishes the native initial default
on this fingerprint, not that later initialization cannot change it.
Voyage also adds penetration, ricochet and water handling. Verify the live
high-speed path before claiming engine documentation validates those extensions.
At2000m/s displacement is33.3m/frame at60fps and66.7m/frame at30fps. A full segment
sweep can detect a thin stationary obstacle between endpoints; testing endpoint
overlap alone is insufficient. Moving targets remain a separate sampling issue.

On this fingerprint, a real-game isolated test of the native Voyage movement
component recorded blocking hits at both200m/s and2000m/s against a1cm-thick
stationary wall123.45m away, using a1cm-radius sphere, sweep enabled, explicit
UpdatedComponent and penetration/ricochet disabled. This supports native swept
movement for the first cannon-shot integration; it does not validate the complete
stock ability/projectile pipeline, damage, water, moving targets or range cutoff.
ReceiveHit actor age is not a fractional contact timestamp, and later actor
displacement is not HitResult.ImpactPoint. Use the actual hit result for contact
position; do not infer exact speed from those coarse diagnostic readings.

The subsequent muzzle-origin integration on the same fingerprint produced a
visible trajectory, a ReceiveHit-derived range of89.536349m against a
BP_DynamicMeshActor instance, and a separate empty-sky range-limit completion.
This validates reachability of both terminal paths from cannon input, not exact
range metrology or damage delivery. A technical dynamic-mesh actor name does
not establish gameplay target identity or damage eligibility. Stock attack
consumer integration was subsequently validated on a shark as described above;
this earlier collision observation alone did not establish damage.

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
# Authored HUD image sizing

When generating a texture and a UImage in the same editor commandlet, do not
derive the brush's desired size from texture platform dimensions before cook.
SetBrushFromTexture(matchSize=true) can persist ImageSize0x0 although the cooked
texture is valid. Author explicit FSlateBrush.ImageSize and audit the cooked
widget. UImage.SetDesiredSizeOverride only updates a live Slate image and is not
a persistent asset-setting substitute. Mask rendering still requires game QA.
