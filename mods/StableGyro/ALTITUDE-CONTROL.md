# Native altitude control and StableGyro ownership

## Version and evidence boundary

The native contracts in this document were decoded from Steam build `25956314`,
`VoyageSteam-Win64-Shipping.exe` SHA-256
`0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A`.
Names, owners, offsets, property types and flags were matched to reflection from
that build. Revalidate them after an executable fingerprint change. The public
producer separately re-extracts and checks the stock cooked assets consumed by
each candidate build.

## Native producer and calibration

The Gyro control pass samples physical vertical velocity into `ClimbRate`, then
updates collective, altitude assistance and hover calibration. StableGyro's
kinematic correction follows in `TG_PostPhysics`. Correcting final world Z does
not change the velocity that the native controller observed earlier in the same
frame.

Manual throttle input changes `Collective`; it is distinct from governed engine
`CurrentThrottle`. With neutral input, a running engine, sufficient rotor
authority and the native control predicate, altitude assistance trims collective
according to negative climb rate and supplies a damped `AssistPitch`. Engine
throttle can therefore rise without new player input. Kinematic support is not
engine propulsion.

With automatic hover calibration enabled, calibration additionally requires:

- neither upward nor downward input beyond the native deadzone;
- a running engine and adequate rotor authority;
- `Collective / max(HoverCollective, epsilon)` above
  `HoverCalibrationMinPitch`;
- absolute assist pitch below its calibration limit;
- absolute climb rate below its calibration limit; and
- ground clearance above its calibration limit.

For this fingerprint, the native calibration routine at
`0x1454aa4c0..0x1454aa8d3` applies:

```
newScale = clamp(oldScale * oldCollective / newCollective,
                 MinHoverPitchScale, MaxHoverPitchScale)
BladePitch = (Collective / max(HoverCollective, epsilon)
              + PitchBoost + AssistPitch) * HoverPitchScale
```

The compensation and pitch writes are at `0x1454aa84f..0x1454aa8c2`.
Reflection verifies `HoverCollective=0.65`, scale bounds `0.5..2.5` and a native
CDO `HoverPitchScale` of `1`. Full upward input supplies `PitchBoost=0.25`.
The formula explains why a reduced scale can leave full input unable to produce
the previous lift even at full rotor speed.

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

For this fingerprint, `HoverPitchScale` at offset `0x80C` is a protected,
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

The generated pitch helper runs in `TG_PrePhysics`, before the native Gyro. At
BeginPlay it caches its parent Gyro and the sibling PostPhysics stabilization
helper through the owned child components; it does not rediscover them per tick.

Before each native control pass, it computes the same neutral stabilization
eligibility used by the PostPhysics helper: the Gyro is player-controlled, a
player controller exists, the engine is running, `CurrentThrottle` is positive,
`PropellerVelocity` meets the configured threshold, neither Ctrl nor Space is
held, and the collision-release deadline has expired. On first acquisition it
captures the exact current values of `bAutoHoverCollective` and
`bAltitudeAssist`, then temporarily sets both false. This prevents the native
controller from learning an equilibrium created by the mod's neutral hold or
braking path, including on the first acquisition frame.

The helper restores both captured values when any eligibility condition is
lost, on controller change and on EndPlay. A contact first observed in
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

## Recovery and remaining limits

StableGyro prevention does not write `HoverPitchScale`, `Collective`, calibration
limits or saved data. It has no automatic reset or migration for a scale already
stored by an earlier session. Native calibration can still operate when the mod
does not own neutral altitude control, but that is not a guaranteed recovery
procedure for every vehicle or physical load.

The rotor-force path at `0x1454ab013..0x1454ab10e` multiplies rotor speed by
`PropellerLift`, positive blade pitch, ground effect and an exponential altitude
factor, then applies the force along the rotor's up direction at its position.
Reflection verifies `PropellerLift=75` at offset `0x5CC`. The governor at
`0x1454ab760` derives its target from a component virtual query times absolute
world gravity divided by this lift coefficient. The virtual query is consistent
with mass, but its exact Shipping identity is not independently verified, so a
complete mass-normalized formula is not established.

Native CDO scale `1` is a startup baseline rather than proof that every saved
vehicle should be reset to `1`; altitude, orientation, ground effect and load
affect equilibrium. The reported yaw-related recovery mechanism also remains
unproven.

## Gameplay validation boundary

On Steam build `25956314`, the user reported that the intermittent takeoff
failure no longer reproduced in his test and that throttle no longer adjusted
itself during hold; he preferred the resulting behavior. After the requested
default-`500` and zero-setting update, he reported that the latest installed
build works. This validates the reported workflow at the level the user tested.
It does not establish a separate scenario matrix for both `0` and `500`, strong
waves, every collision shape, every saved calibration state or compatibility
with another executable fingerprint.
