# StableGyro

StableGyro adds gradual pitch controls and steadier altitude and horizontal
flight to the Gyro.

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
PitchRampSeconds=1.5 means 1.5 seconds from neutral to either limit.
Accepted values are clamped to 0.05..60.0. Set
`EnablePitchCorrection=false` to leave stock W/S pitch control unchanged and
hide the mod-owned X reset hint. Settings are reread whenever the player enters
a Gyro, so exit and re-enter after editing.
Comments must be on separate lines; inline comments after values are invalid.
The INI is the sole source of defaults, comments, ordering, and packaged
formatting. Settings/StableGyro.settings.json declares only runtime
bindings, types, and numeric ranges; the public build validates both and
generates the Blueprint settings bindings internally.

Whenever the engine is running, current throttle is positive, and the measured
rotor-speed gate is satisfied, releasing Space after holding it starts a
kinematic braking phase for positive vertical speed.
Entering stabilization without Space held, including after releasing Ctrl,
uses the same braking path instead of discarding existing climb inertia.
`AltitudeStabilizationVerticalDeceleration`
sets that deceleration in cm/s^2; the default is `500.0` and accepted values
are clamped to `0..10000`. With a positive value, positive growth above the
previous commanded vertical speed is rejected before deceleration, so continued
rotor force cannot undo the configured braking rate. At `0`, both that growth
rejection and positive-vertical-speed damping are disabled: the mod leaves
positive Z velocity and position untouched and discards the stale vertical
sample. This is not a global altitude-stabilization toggle. When vertical speed
naturally reaches zero or becomes negative, the current altitude can still
become an exact hold target. Horizontal stabilization remains independent.
`AltitudeStabilizationMinimumThrottlePercent` sets an additional inclusive
minimum for altitude stabilization, using the displayed throttle indicator that
changes with Space/Ctrl. The default is `50.0` and accepted values are clamped to
`0..100`; `0` preserves the previous behavior. Below the configured percentage,
the mod skips all Z braking, exact-hold, and Space-floor corrections and clears
its vertical state so a later eligible frame reacquires from a fresh observation.
This setting does not gate horizontal stabilization and is distinct from the
native governed `CurrentThrottle` propulsion check.
Pressing Space returns to the one-sided altitude floor without clamping any
positive upward velocity, even
when the Gyro is still at or below its prior floor. Holding Ctrl releases
vertical hold and braking so the stock descent and landing controls remain in
charge; Ctrl wins if Space and Ctrl are both held. Horizontal damping remains
active while Ctrl is held when the propulsion and contact gates permit it.

The pitch-input helper runs in `TG_PrePhysics` before the stock Gyro tick and
while pitch correction is enabled restores the stored integrated forward tilt
every controlled frame. The
stabilization helper runs in `TG_PostPhysics` after the stock Gyro tick: stock
rotor and Chaos forces are integrated first, then the mod applies its kinematic
vertical and horizontal corrections. Explicit tick prerequisites enforce both
relationships.

Independently of `EnablePitchCorrection`, the PrePhysics helper temporarily
owns the native altitude-assistance booleans while neutral stabilization is
eligible. It captures the current `bAutoHoverCollective` and `bAltitudeAssist`
values, disables both before the native Gyro tick, and restores the exact values
when Space or Ctrl is held, propulsion or player control is lost, contact release
is active, the controller changes, or the helper ends. This prevents native
hover calibration from learning an equilibrium created by the later kinematic
hold. Whenever a player enters the Gyro, the stabilization helper also resets
the saved native `HoverPitchScale` to the selected baseline `1.0`. Startup only
binds the entry event and does not reset an already controlled Gyro. The reset
does not run every tick and does not write `Collective`.
See [ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md) for the version-bound native
formula, ownership order and recovery limits.

Both corrections require a running engine, positive `CurrentThrottle`, and a
measured rotor speed of at least
`StabilizationMinimumPropellerVelocity` in the native Gyro's direct
`PropellerVelocity` units. The default is `390.0`; accepted values are clamped
to `0..100000`, and `0` disables the rotor-speed part of this gate. When any
part of the propulsion gate fails, the helper clears its altitude-lock,
braking, and velocity-sample state without changing location or velocity.

An external collision hit releases both stabilization paths for 0.25 seconds,
allowing the stock physics response to separate the Gyro from moving decks and
other obstacles. The exact Z correction is swept; a blocking hit or initial
penetration enters the same release interval and skips the subsequent Z-velocity
correction. After the interval expires, stabilization reacquires from a fresh
observation. Horizontal speed-growth rejection and vertical braking apply only
away from that collision interval.

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
If only the pinned Steam build differs, retry with
`-IgnorePinnedGameVersion`. The build still requires fresh stock inputs and a
matching reviewed mapping; test a successful candidate in game before updating
`game-build.json`.

Build-StableGyro.ps1 is the only public producer; the other
PowerShell files in this directory are internal stages used by it. Use
-Install only while Voyage is closed. Installation preserves current values and
comments in an existing StableGyro.ini, removes explicitly retired keys, and
appends keys missing from the validated canonical template. A missing
StableGyro.ini is created from the canonical packaged defaults.
Build, cook, container verification, and clean load are not gameplay
validation.

StableGyro rereads settings on Gyro entry; `EnablePitchCorrection` switches
between the mod's smooth pitch control and stock W/S behavior. Altitude hold
does not adjust throttle, and each new player entry resets saved
`HoverPitchScale` to `1.0`. The entry-reset asset contract covers Steam build
`25982825`; asset-dependent inputs are freshly extracted and checked for each
build.
Strong waves, every collision shape and every saved calibration state are not
part of the established compatibility scope.
