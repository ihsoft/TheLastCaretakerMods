# StableGyro

Loader-free IoStore mod for the stock Gyro. It leaves the complete current
Gyro Blueprint at its original package identity, adds a small generated child,
and redirects newly built Gyros through the stock item data asset's
`DroppedActor` class path.

StableGyro improves both direct controls and flight stability. With
`EnablePitchCorrection=true`, its pitch controls are:

- holding W increases TiltForwardInput linearly;
- holding S decreases it linearly;
- releasing W/S leaves the current value unchanged;
- X immediately resets pitch to zero;
- the standard vehicle HUD shows X as Reset pitch / Сбросить наклон;
- native Space/Ctrl throttle commands and mappings, A/D input behavior, yaw,
  camera, possession, save behavior, and all existing keyboard mappings remain
  unchanged.

Assets/StableGyro.ini is copied beside the container. The default
PitchRampSeconds=3.0 means three seconds from neutral to either limit.
Accepted values are clamped to 0.05..60.0. Set
`EnablePitchCorrection=false` to leave stock W/S pitch control unchanged and
hide the mod-owned X reset hint. Settings are reread whenever the player enters
a Gyro, so exit and re-enter after editing.
Comments must be on separate lines; inline comments after values are invalid.
The INI is the sole source of defaults, comments, ordering, and packaged
formatting. Settings/StableGyro.settings.json declares only runtime
bindings, types, and numeric ranges; the public build validates both and
generates the Blueprint settings bindings internally.

Whenever the measured rotor-speed gate is satisfied, releasing Space after
holding it starts a kinematic braking phase for positive vertical speed.
Entering stabilization without Space held, including after releasing Ctrl,
uses the same braking path instead of discarding existing climb inertia.
`AltitudeStabilizationVerticalDeceleration`
sets that deceleration in cm/s^2; the default is `100.0` and accepted values
are clamped to `1..10000`. During braking, positive growth above the previous
commanded vertical speed is rejected before deceleration, so continued rotor
force cannot undo the configured braking rate. Horizontal velocity is
preserved. When vertical speed reaches zero, or is already non-positive, the
current altitude becomes an exact hold target. Pressing Space returns to the
one-sided altitude floor without clamping upward velocity. Holding Ctrl releases
vertical hold and braking so the stock descent and landing controls remain in
charge; Ctrl wins if Space and Ctrl are both held. Horizontal damping remains
active while Ctrl is held when the rotor-speed gate is satisfied.

The pitch-input helper runs in `TG_PrePhysics` before the stock Gyro tick and
while pitch correction is enabled restores the stored integrated forward tilt
every controlled frame. The
stabilization helper runs in `TG_PostPhysics` after the stock Gyro tick: stock
rotor and Chaos forces are integrated first, then the mod applies its kinematic
vertical and horizontal corrections. Explicit tick prerequisites enforce both
relationships.
Both corrections require the measured rotor speed to be at least
`StabilizationMinimumPropellerVelocity` in the native Gyro's direct
`PropellerVelocity` units. The default is `390.0`; accepted values are clamped
to `0..100000`, and `0` disables this gate. Below the threshold the helper
clears its altitude-lock and velocity-sample state without changing location or
velocity. This prevents the Gyro from being held against an obstacle after a
rotor collision removes lift.

Horizontal speed is damped kinematically at the rate configured by
`HorizontalVelocityDecayAcceleration`, in cm/s^2. With both rotor-tilt axes
neutral, physics-injected growth relative to the helper's previous horizontal
velocity command is rejected before the complete horizontal velocity decays
toward zero. With W/S and/or A/D tilt, the permitted direction is derived from
the control values and the Gyro's world-horizontal forward/right axes rather
than from the rotor's absolute world orientation. Only positive velocity along
that direction is preserved; sideways and opposing velocity still decays. The
correction is added as an XY-only velocity delta, so this pass never rewrites
vertical velocity. The default is `150.0`, and accepted values are clamped to
`1..10000`.

The retired physical lift-compensation experiment is not present in the
runtime graph. Installation removes its obsolete `CompensateTiltLift` and
`TiltLiftCompensationMultiplier` keys while preserving active user settings.

## Build

The normal preparation entry point is:

    .\Build-StableGyro.ps1

To build and install in one run:

    .\Build-StableGyro.ps1 -Install

It fingerprints the installed game, builds the UE 5.8.2 editor project,
generates the two helpers, reset action, keyboard context, and replacement child,
narrowly cooks five generated packages, freshly extracts
and surgically patches the stock Gyro item data asset, packages and verifies
the six-asset container, and creates a ZIP under a fresh ignored
`Tmp/StableGyro` output by default.

Build-StableGyro.ps1 is the only public producer; the other
PowerShell files in this directory are internal stages used by it. Use
-Install only while Voyage is closed. Installation preserves current values and
comments in an existing StableGyro.ini, removes explicitly retired keys, and
appends keys missing from the validated canonical template. A missing
StableGyro.ini is created from the canonical packaged defaults.
Build, cook, container verification, and clean load are not gameplay
validation.

Steam build `25827816` remains the last broadly game-validated checkpoint. On
build `25880952`, the user verified settings reload on Gyro entry and the
`EnablePitchCorrection` enabled/disabled A/B switch; the remaining detailed
flight-stability cases still require proportional real-game validation.
