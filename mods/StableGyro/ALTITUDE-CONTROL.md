# Native altitude control and StableGyro ownership

## Version-bound native contracts

The calculation chain below was checked against the Shipping executable for
Steam build `25982825`, UE `5.8.2`, together with that build's reviewed mapping
metadata. Native addresses are image virtual addresses, not portable runtime
offsets. [Game-derived sources](GAME_DERIVED_SOURCES.md) records the executable
identity. This is a calculation contract, not a claim that every flight state
has been game-tested or a player-facing compatibility label.

Current mappings establish native owners and field types; the stock HUD
binding was independently extracted. Reflected access flags and numerical CDO
defaults explicitly identified as earlier observations below retain their older
evidence boundary. Do not silently treat them as newly inspected defaults.
StableGyro's configured `HoverPitchScale` entry baseline defaults and falls
back to `2.5`; that is mod tuning, not the native CDO default.

On a new Steam build, attempt the public candidate path first; investigate
these native details when a build or game test reveals a discrepancy. The
producer separately re-extracts and checks the stock cooked assets consumed by
each candidate build.

## Native calculation order and state ownership

The important quantities are distinct; none should be used as a synonym for
all of the others:

| Native field | Meaning and producer |
| --- | --- |
| `ThrottleInput` (`0x7E4`) | Signed input intent, not the resulting engine command. |
| `Collective` (`0x800`) | Integrated collective, also writable by native trim/calibration. |
| `BladePitch` (`0x808`) | Computed dimensionless pitch demand, including boost, assist and scale. |
| `HoverPitchScale` (`0x80C`) | Saved multiplier of pitch demand, not a percentage limited to one. |
| `CurrentThrottle` (`0x7FC`) | Engine command integrated by the rotor-speed governor. |
| `PropellerVelocity` (`0x89C`) | Measured rotor speed in the game's scaled units, not an engine-throttle setting or literal RPM. |
| `ClimbRate` (`0x898`) | Physical chassis world-Z velocity; positive is climbing. |

The native Tick at `0x1454a4d20` executes the relevant passes in this order:

1. Read chassis physical velocity Z into `ClimbRate` (`0x1454a4dbf`).
2. Update input/collective, altitude assist, calibration and `BladePitch`
   (`0x1454ac470`).
3. Update temperature-derived cold severity (`0x1454ac2b0`).
4. Compute governed rotor target, drag correlation and `CurrentThrottle`
   (`0x1454ad710`).
5. Sample rotor angular velocity/rate and submit rotor torque commands
   (`0x1454af0c0`).
6. Sample root world Z into `Altitude`, then submit rotor lift and update tilt
   (`0x1454acc90`).
7. Submit fuel demand and engine-resource production settings
   (`0x1454a4ec1..0x1454a4f5e`).

Thus the governor uses the rotor speed/rate retained from the preceding rotor
sample; its new target is also later than the authority check in the collective
pass. These are sequential feedback passes, not one simultaneous equilibrium
solution. Resource processing and physics integration are separate consumers;
setting throttle does not instantly assign a rotor speed.

The dependency loop is:

```text
input + assist/calibration -> collective/boost/assist/scale -> blade pitch
blade pitch -> drag + measured rotor speed -> governor -> engine throttle
engine throttle -> module energy -> rotor physics -> measured rotor speed
rotor speed + blade pitch + world lift axis -> lift -> physical climb rate
physical climb rate -> assist/calibration -> collective/assist/scale
```

StableGyro's eligible PrePhysics helper suppresses the native assistance before
this chain; its PostPhysics kinematic support runs after the stock Gyro tick.
Correcting final world Z cannot change the physical climb-rate sample already
read by that native tick.

## Input, collective and blade pitch

For normal positive `dt`, manual integration is:

```text
Cmin = min(MinCollective, 0)
Collective = clamp(Collective + ThrottleInput * CollectiveRate * dt, Cmin, 1)
boostTarget = ThrottleInput * CollectiveInputBoost
PitchBoost = FInterpTo(PitchBoost, boostTarget, dt, CollectiveBoostInterpSpeed)
```

The native lower bound is not unconditionally zero: a negative `MinCollective`
permits negative collective. `CollectiveInputDeadzone` supplies separate
up/down predicates for engine and assistance logic; it does not replace the
raw input in the integration above. Positive input can start a stopped engine;
held downward input at the lower bound can stop it subject to the native
hold-time and ground-clearance conditions.

Here `FInterpTo` blends toward the target by `clamp(dt * speed, 0, 1)` for a
positive speed, snaps when sufficiently close, and uses the target directly
when the interpolation speed is nonpositive. This is different from constant
rate movement used by hover calibration.

For this fingerprint, the stock HUD Blueprint
`/Game/UI/Game/HUD/WBP_VoyageIngameGyroCopter` binds its `Throttle` slider's
`Value` directly to `GyroCopter.Collective`. The widget does not override the
Engine `Slider` range, whose native defaults are `0..1`; the displayed throttle
percentage is therefore `Collective * 100`. StableGyro's configurable minimum
altitude throttle compares `Collective >= configuredPercent / 100`. This is an
inclusive vertical-only gate and does not replace the separate governed
`CurrentThrottle > 0` propulsion gate.

After assistance and calibration, native control writes:

```text
BladePitch = (Collective / max(HoverCollective, epsilon)
              + PitchBoost + AssistPitch) * HoverPitchScale
```

The earlier reflected defaults were `HoverCollective=0.65`,
`CollectiveInputBoost=0.25`, native `HoverPitchScale=1`, and scale bounds
`0.5..2.5`. These explain the formula but are not newly verified current CDO
values. A scale above one is supported calibration/tuning, not invalid
normalized input. At equal rotor speed and other force factors, reducing this
scale reduces positive lift even when engine throttle is already saturated.

`Collective` alone is not a record of player intent: manual integration,
automatic trim and calibration share it. `ThrottleInput` identifies the current
input direction; `CurrentThrottle` identifies the governor output. The HUD
collective percentage cannot distinguish those causes by itself.

## Altitude assistance: feedback from vertical speed

Rotor authority here is `PropellerVelocity / GovernedRotorSpeed` when the
target is positive, otherwise zero; this assistance ratio is not clamped to one.
With `bAltitudeAssist`, neutral up/down input, a running engine, authority at
least `AssistMinRotorAuthority`, and the additional native eligibility predicate
at `0x145497420`, the control pass computes:

```text
assistTarget = clamp(-ClimbRate * AssistClimbRateDamping,
                     -AssistMaxPitch, AssistMaxPitch)
trimRate = clamp(-ClimbRate * AssistTrimGain,
                 -AssistMaxTrimRate, AssistMaxTrimRate)
Collective = clamp(Collective + trimRate * dt, Cmin, 1)
AssistPitch = FInterpTo(AssistPitch, assistTarget, dt, AssistInterpSpeed)
```

When assistance is ineligible, its target is zero and `AssistPitch` interpolates
back toward zero; this is not an unconditional immediate reset. This path uses
physical vertical velocity, not an error against a stored target height. For
negative `ClimbRate`, both pitch assistance and collective trim tend upward;
for positive `ClimbRate`, they tend downward.

Neither hull attitude nor the vertical projection of rotor lift appears as a
feed-forward term in this assistance calculation. Tilting the rotor or hull
can first reduce vertical lift; the response follows the resulting physical
descent measured by a later native tick. A rotor-speed governor alone need not
respond to that projection loss if rotor speed has not changed.

## Hover calibration and scale compensation

With `bAutoHoverCollective`, calibration requires:

- neither upward nor downward input beyond the native deadzone;
- a running engine and authority at least `AssistMinRotorAuthority`;
- `Collective / max(HoverCollective, epsilon) > HoverCalibrationMinPitch`;
- `abs(AssistPitch) < HoverCalibrationMaxAssistPitch`;
- `abs(ClimbRate) < HoverCalibrationClimbRate`; and
- `GroundClearance > HoverCalibrationClearance`.

The verified calibration block is `0x1454ac756..0x1454ac823`. It moves
collective toward `HoverCollective` using constant-rate interpolation. Only a
sufficiently positive candidate collective is committed together with the
compensated multiplier; otherwise the pair is left unchanged:

```text
newCollective = FInterpConstantTo(oldCollective, HoverCollective,
                                 dt, HoverCalibrationRate)
newScale = clamp(oldScale * oldCollective / newCollective,
                 MinHoverPitchScale, MaxHoverPitchScale)
```

For normal nonnegative calibration rate, `FInterpConstantTo` moves at most
`HoverCalibrationRate * dt` per pass. Without active scale clamping,
`oldCollective * oldScale = newCollective * newScale`: this preserves the base
collective contribution to pitch, not an exact total-force invariant when boost,
assist, ground effect or attitude changes. The final pitch write follows at
`0x1454ac82b..0x1454ac872`.

## Rotor-speed governor and automatic engine throttle

The governor at `0x1454ad710..0x1454ad929` derives its target from the chassis
primitive's native `GetMass`, world gravity and the lift coefficient:

```text
GovernedRotorSpeed = VehicleMesh.GetMass() * abs(worldGravityZ)
                     / max(PropellerLift, epsilon)
```

The `GetMass` identity is established by its Engine registration and thunk
`0x142915cc0`, which dispatches through the same virtual slot `0x958` used by
the governor. This resolves the otherwise ambiguous virtual query; it is not
an estimate based only on a nearby function name. It reads the chassis body
mass, not an explicit sum of suspended cargo attached by cables. Tilt, ground
effect and `HoverPitchScale` are not terms in this rotor target.

The pass retains the previous `RotorDragCoefficient` and calculates:

```text
Dnew = rotorAngularDamping + abs(BladePitch) * RotorPitchDrag
       + ColdSeverity * ColdRotorDrag
```

If `bCollectiveCorrelator` is enabled and both drag coefficients exceed the
native small-value guard, it first rescales the previous engine command:

```text
CurrentThrottle = clamp(CurrentThrottle * Dnew / Dold, IdleThrottle, 1)
```

With a running engine and positive target, it then integrates speed feedback:

```text
speedError = (GovernedRotorSpeed - PropellerVelocity) / GovernedRotorSpeed
speedRate = RotorSpeedRate / GovernedRotorSpeed
ThrottleCommand = clamp(GovernorGain * speedError - GovernorDamping * speedRate,
                        -MaxThrottleRate, MaxThrottleRate)
CurrentThrottle = clamp(CurrentThrottle + ThrottleCommand * dt, IdleThrottle, 1)
```

`ThrottleCommand` is the bounded rate of throttle change, not the throttle
itself. With the engine stopped or a nonpositive target, the command is
`-MaxThrottleRate` and throttle decays toward zero without the idle floor.
Higher pitch can therefore increase automatic throttle through both immediate
drag correlation and subsequent speed feedback, even with neutral player input.
There is no direct `CurrentThrottle = Collective` or proportional RPM assignment.

## Engine resources, rotor dynamics and measured speed

The rotor pass at `0x1454af0c0` measures angular velocity along the propeller's
world up axis and stores:

```text
PropellerVelocity = abs(dot(rotorAngularVelocity, rotorUp))
                     * PropellerVelocityScale
RotorSpeedRate = (newPropellerVelocity - previousPropellerVelocity) / dt
```

The rate is zero for nonpositive `dt`. The speed uses the game's scaling;
do not label it RPM without a separate units conversion. It is an observation
of physics, not an integration of the player input or of `CurrentThrottle`.

Native Tick writes engine production rates proportional to `CurrentThrottle`,
including `PhysicalEnergyProduction * CurrentThrottle`. The rotor reads
`CustomModule.GetResourceAmount(PhysicalEnergy)` and multiplies that available
amount by `PropellerTorquePerEnergy` for its engine torque command. The getter
returns an amount, not a production rate or a last-frame change. Its native
registration thunk `0x14532e790` dispatches through slot `0x548`; the enum value
is `5` for PhysicalEnergy (`1` for Petrol).

Consequently, the observed chain is throttle -> module production/resource
state -> torque -> physics -> measured rotor speed. It does not justify
replacing the available resource amount with the advertised production rate or
assuming that their consumers execute in one simultaneous calculation.

The rotor also applies pitch drag proportional to
`abs(BladePitch) * RotorPitchDrag * abs(dot(rotorAngularVelocity, rotorUp))`,
temperature-dependent drag and an explicit rotor-brake branch. Native
autorotation can add torque from downward airflow when its pitch/speed and
steady-torque conditions permit, including without a running engine. Thus a
stopped engine is not proof of a stopped rotor, and neither throttle nor pitch
alone determines the actual rotor acceleration. These are game torque commands,
not a complete SI-unit aerodynamic model.

## Lift vector, tilt and the feedback loop

For a valid chassis and `TiltControlComp`, the force block
`0x1454acf73..0x1454ad0be` uses the measured rotor speed, computed pitch, ground
clearance and root world altitude:

```text
q = clamp(GroundClearance / max(GroundEffectHeight, epsilon), 0, 1)
GroundEffect = 1 + GroundEffectLift * (1 - q)^2
heightFactor = exp(-max(Altitude, 0) * LiftReductionByHeight)
pitchForLift = BladePitch * GroundEffect    when BladePitch > 0
             = BladePitch                  otherwise
liftMagnitude = PropellerVelocity * PropellerLift * pitchForLift * heightFactor
liftAxis = worldUp(TiltControlComp)
forceVector = liftAxis * liftMagnitude
```

Force is applied to the chassis at the tilt component's world position. The
ground-effect multiplier boosts positive pitch; the nonpositive-pitch branch
keeps the signed pitch instead of receiving that boost. The native code is
linear in the stored rotor-speed value here; do not substitute a presumed
real-world RPM-squared lift law.

Both hull attitude and the tilt component's relative orientation affect its
world axis. As a consequence of the vector force, the vertical contribution
is `liftMagnitude * dot(liftAxis, worldUp)`, and the remaining components can
accelerate the craft horizontally. This projection is a consequence of the
applied force, not an additional compensation term in native altitude assist.
The force block submits lift before its later tilt update, so the newly
commanded tilt is not retroactively applied to that force submission.

Gravity, cargo/cable reactions, collisions and the other force paths then affect
physical motion. A later tick reads that motion into `ClimbRate`, closing the
native assistance loop. More scale can restore lift at the same measured rotor
speed, but it does not remove orientation, altitude, ground-effect or physical
load dependencies.

## Fuel request versus observed consumption

For Steam build `25982825`, native float `PetrolConsumption` belongs to
`VoyageVehicleGyroCopter` at `0x6f0` and is Blueprint-readonly. Constructor
`0x145477fa0`, registered through the Gyro's default-constructor wrapper
`0x1453c53c0`, initializes it to `0x437a0000`: the float representation of
`250.0`. The stock Gyro Blueprint has no override. The adjacent field at
`0x6ec`, initialized to `1/60`, is `RotorAudioRPMScale`, not fuel consumption.
For these float reflection descriptors the member offset is at byte `+50`
from the descriptor's name pointer; preceding bytes belong to the previous
descriptor and must not be used as this property's offset.

The Gyro's base native Petrol request is:

```text
basePetrolRequest = PetrolConsumption * CurrentThrottle
```

Tick submits that value to the custom module's `TemporaryConsumptions[Petrol]`
through `0x1454fc450`; the callee accumulates the request rather than directly
subtracting that amount from the tank. No blade pitch, rotor-drag or mechanical
work term occurs in this base request. At equal `CurrentThrottle`, changing
scale does not directly change it. An indirect change remains possible when
the governor changes throttle in response to pitch drag or rotor-speed error.

The request is not unconditional actual tank consumption. Module resource
processing, availability and other consumers or transfers remain separate.
`GetResourceAmount(Petrol)` measures the local stored quantity; a decrease can
also include transfer or leakage, while refill/supply can mask consumption.
The request's conversion to liters per real second is not established here:
temporary-request accumulation, module scheduling and resource multipliers
must be accounted for. Do not label the native parameter as a real-time rate.

StableGyro applies its native setting directly, defaulting to `250.0`:

```text
PetrolConsumption = float(FuelConsumption)
basePetrolRequest = PetrolConsumption * CurrentThrottle
```

The write uses `KismetSystemLibrary.SetDoublePropertyByName`, whose
native-float fallback preserves the property's reflected readonly flags. It
does not write throttle, collective, stored fuel or `WorkloadPetrolPerSecond`.

The stock Blueprint's `VoyageCustomModule_GEN_VARIABLE` component has native
class `VoyageCustomModuleComponent`, derived from `VoyageModuleComponent` and
then `ActorComponent`. Its writable `MaxResources` map is
`TMap<EModuleResourceType, double>`; the stock Petrol entry is `400`.
This is capacity, distinct from the stored amount returned by
`GetResourceAmount(Petrol)`.

`ApplyFuelSettings` finds that component on the owned Gyro and passes its
instance `MaxResources` property by reference to `BlueprintMapLibrary.Map_Add`
with the Petrol key and configured `FuelCapacity`. It preserves the other map
entries and the stored fuel amount, including an amount above a lowered
capacity. `VoyageModuleComponent.SetCustomConsumption` is not this capacity
path: its native body at `0x14555ccc0` writes three ConfigData fields rather
than the custom module's map. Settings ownership and reload timing belong in
[architecture](ARCHITECTURE.md#fuel-settings).

## Conflict with kinematic height support

The mod can hold or restore altitude while native collective remains low. The
native controller then observes a small climb rate and can treat this externally
supported state as a calibration equilibrium. Raising low collective toward its
hover target reduces `HoverPitchScale`; lowering high collective toward that
target increases it. The changed scale affects the next manual ascent, when the
held upward input inhibits native calibration.

`AltitudeExactHoldActive=false` does not prove that no vertical correction is
occurring. The Space floor branch can still restore position and zero
nonpositive vertical velocity when current altitude has not exceeded its stored
floor. This can support a craft whose current physical lift is insufficient.

Do not use a cached `SceneComponent.ComponentVelocity` value as evidence of
Chaos rigid-body vertical velocity. It can remain zero while the actor is
visibly moving. Diagnose this path with physical velocity or the native
`ClimbRate`, and preserve the tick-order boundary between the native observation
and StableGyro's later correction.

## Reflected state and setter route

Earlier reflection established the following access flags and CDO defaults;
current mappings revalidate field names/types, not all of those flags/defaults.
`HoverPitchScale` at offset `0x80C` is a protected,
BlueprintReadOnly **SaveGame** float. `Collective` at `0x800` is also saved.
The CDO values do not reset values already stored on a vehicle.

`bAutoHoverCollective` at `0x5EC`, `bAltitudeAssist` at `0x60C` and
`HoverCalibrationRate` at `0x5F0` are public, editable, BlueprintReadOnly and
not SaveGame. The reflected booleans use one-byte native-bool storage and have
CDO value `true`.

UE 5.8.2 exposes the static native/public/BlueprintCallable internal-use
function
`KismetSystemLibrary.SetBoolPropertyByName(UObject, FName, bool)`. It resolves
an `FBoolProperty` and writes it without treating BlueprintReadOnly as a setter
restriction. StableGyro calls this real Engine function and keeps the mirror
properties' actual readonly flags. The generator validates the exact function,
property names and types; the shipped graph does not add a runtime setter
readback or invent writable native flags.

## Shipped ownership and restoration

The helpers read `IA_GyroCopterThrottleUp` and `IA_GyroCopterThrottleDown`
through the owning Gyro's Enhanced Input value bindings. Space and Ctrl below
refer to these actions' defaults, not runtime key tests. Down takes precedence
when both actions are active. These input values express player commands;
native changes to `Collective` or `CurrentThrottle` do not activate them.

The generated pitch helper runs in `TG_PrePhysics`, before the native Gyro. At
BeginPlay it caches its parent Gyro and the sibling PostPhysics stabilization
helper through the owned child components; it does not rediscover them per tick.

Before each native control pass, it computes the same neutral stabilization
eligibility used by the PostPhysics helper: the Gyro is player-controlled, a
player controller exists, the engine is running, `CurrentThrottle` is positive,
`PropellerVelocity` meets the configured threshold, `Collective` meets the
configured displayed-throttle percentage, `EnableAltitudeControl` is true,
neither Ctrl nor Space is held, and the collision-release deadline has expired.
On first acquisition it
captures the exact current values of `bAutoHoverCollective` and
`bAltitudeAssist`, then temporarily sets both false. This prevents the native
controller from learning an equilibrium created by the mod's neutral hold or
braking path, including on the first acquisition frame.

The helper restores both captured values when any eligibility condition is
lost, including when altitude control is disabled, on controller change and on EndPlay. A contact first observed in
PostPhysics starts the release deadline; the next PrePhysics pass restores the
native values before the next native consumer tick. The mod never permanently
changes these tuning booleans.

The PostPhysics helper separately owns collision-safe kinematic correction. An
external hit releases both vertical and horizontal paths for `0.25` seconds and
clears their state. Exact Z movement is swept; a blocking hit or initial
penetration enters the same release interval before any following Z-velocity
write. Engine-running, positive-throttle and measured-rotor gates clear the
state when propulsion is ineligible.

Positive vertical braking defaults to `500 cm/s^2`. A positive configured value
retains previous-command growth rejection followed by constant-rate damping.
At `0`, positive observed Vz bypasses both the damping and growth rejection,
performs no Z position or velocity write, and clears the old vertical sample.
This does not disable horizontal stabilization or later exact-hold acquisition
after Vz naturally becomes nonpositive.

Descent-release braking separately defaults to `500 cm/s^2`. Ctrl clears
vertical hold, braking and previous-sample validity before the minimum
Collective gate. With Ctrl released and Space neutral, fresh eligible hold
acquisition snapshots physical Vz before writes and selects descent braking
when Vz is negative, ascent braking when positive, or exact hold at zero.
This also applies when native Collective recovers to the configured threshold
several frames after Ctrl release; it does not require remembered input state.
Descent braking rejects physics-injected growth in downward speed against the
previous helper command, applies constant-rate damping toward zero, and acquires
exact hold only when corrected Vz reaches or crosses zero. At `0`, negative observed Vz bypasses
both damping and growth rejection, performs no Z position or velocity write,
and clears the old vertical sample; exact hold remains available once physics Vz
becomes nonnegative. The setting is reread on the next player entry with the
other flight settings.

While Ctrl is held, an independent upward-speed cap prevents excess native
lift from causing a hop when height support is released. PrePhysics captures
the initial physical velocity before the first freed physics step. PostPhysics
uses these values, with positive Vz pointing upward:

```text
initial cap = max(physical Vz, 0)
outputVz = min(observed physics Vz, cap)
velocityDelta = (0, 0, outputVz - observed physics Vz)
next cap = max(outputVz, 0)
```

The cap only decreases while Ctrl remains eligible. Existing climb inertia can
decay naturally, while fresh upward acceleration is rejected. Negative Vz is
unrestricted, including slowing a descent from a larger to a smaller downward
speed. After upward motion stops, the zero cap prevents upward reversal. This
guard does not apply either configured braking rate or change position, throttle,
collective, pitch scale or native forces. Velocity clipping does not undo
displacement already integrated during a physics step.

The guard requires altitude control, player control, engine-running,
positive CurrentThrottle, sufficient measured rotor speed and expired contact
cooldown. It runs below the minimum Collective percentage and independently of
the pitch and horizontal control flags; Ctrl overrides Space. Release,
controller changes, either helper's EndPlay, invalid references and eligibility
loss clear its transient state. Ordinary native-assistance restoration during
held Ctrl leaves the cap intact.

The distributed INI sets the minimum displayed-throttle threshold to `65%`.
The schema fallback for a missing or invalid value remains `50%`. Below the
configured inclusive threshold, PostPhysics still performs eligible horizontal
damping and the eligible Ctrl-held cap. With Ctrl released, it skips Z braking,
exact hold and Space-floor correction, then clears vertical state for fresh
reacquisition. Native-assistance ownership uses the threshold for neutral
stabilization and restores captured assistance booleans while that ownership is
ineligible.

## Entry reset and remaining limits

On every change to a player controller while `EnableAltitudeControl=true`, the
PostPhysics helper writes the entry-reloaded `HoverPitchScale` INI setting to
the saved native field through the Engine's `SetDoublePropertyByName` route.
This advanced setting defaults and falls back to `2.5`, accepts `0.5..2.5`,
and is listed under the INI's advanced-settings warning. When
altitude control is false, it skips this write and leaves native altitude and
automatic-throttle assistance state in charge. The configured baseline is mod
tuning, independent of the earlier recorded native CDO default `1.0`.
BeginPlay only binds the controller-change delegate;
it does not invoke the callback for an already controlled Gyro. The reset runs
only from a genuine later player-entry event and never from polling or a
per-tick write. It does not write `Collective`, calibration limits, or any other
saved flight field.

The reset deliberately discards a scale learned in an earlier session or while
another controller owned the Gyro. Native calibration can still adjust the
scale later whenever StableGyro does not own neutral altitude control; the next
player entry writes the freshly reloaded configured value again.

Writing the configured baseline on player entry is the recovery policy.
Altitude, orientation, ground effect and load still affect the
equilibrium reached by later native calibration. The yaw-related recovery
mechanism remains unproven.

## Compatibility boundary

The altitude-hold path does not adjust native throttle. Game checks cover smooth
descent braking and hold reacquisition after Ctrl release, including delayed
eligibility at the configured Collective threshold, and suppression of the hop
when Ctrl releases hover support. Static generation and cooked readback also
establish the ascent/descent graph structure and settings contract. Separate
game checks remain necessary for zero descent deceleration, later exact-hold
acquisition after Vz becomes nonnegative, every Ctrl-cap lifecycle transition
and unchanged horizontal damping. They must also verify both control
flags independently and together: entry-reloaded altitude disablement restores
native assistance and preserves saved `HoverPitchScale`, while horizontal
disablement performs no XY correction and does not disable the vertical path.
Strong waves, every collision
shape and every saved calibration state are outside the established
compatibility scope. A changed game Update or a failed build/game test calls
for review of the affected input and calibration contracts.
