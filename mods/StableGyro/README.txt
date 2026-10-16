StableGyro

Copy StableGyro_P.pak, StableGyro_P.ucas,
StableGyro_P.utoc, and StableGyro.ini to:

Voyage\Content\Paks

When updating an existing StableGyro installation, preserve your current
StableGyro.ini instead of overwriting it with the archive default.

Controls:
W  - smoothly increase forward pitch
S  - smoothly decrease forward pitch
X  - immediately reset pitch to zero

These controls apply when EnablePitchCorrection=true. Set it to false to leave
stock W/S pitch control unchanged and hide the mod-owned X reset hint.
Releasing W or S keeps the current pitch value while correction is enabled.

PitchRampSeconds in StableGyro.ini sets the time from zero to maximum
pitch. Default: 1.5 seconds. Settings are reread when you enter a Gyro; exit
and re-enter after editing the file.
Put comments on their own lines; inline comments after values are invalid.

Altitude and horizontal stabilization require a running engine, positive
current throttle, and the measured rotor speed gate.
StabilizationMinimumPropellerVelocity sets that minimum rotor speed in the stock
PropellerVelocity units. Default: 390.0. If any propulsion gate fails, the mod
does not change position or velocity and clears stabilization state. Set 0 to
disable only the rotor-speed gate.

AltitudeStabilizationVerticalDeceleration controls how quickly positive
vertical speed is reduced after releasing Space or entering stabilization with
Space released. Default: 500.0 cm/s^2; accepted range: 0..10000. With a positive
value, physics-driven positive speed growth is rejected while braking. At 0,
the mod leaves positive Z velocity and position untouched and clears its stale
vertical sample; exact hold can still engage after vertical speed naturally
reaches zero or becomes negative. Zero is not a global altitude-stabilization
toggle, and horizontal stabilization remains independent. All kinematic
corrections run after the stock physics step. Hold Ctrl to
release vertical hold/braking for stock descent and landing; Ctrl wins over
Space. Holding Space preserves every positive upward velocity, including at or
below the stored altitude floor. Exit and re-enter the Gyro after editing the
INI. It still requires the propulsion
gates and an inactive collision-release interval.

AltitudeStabilizationMinimumThrottlePercent sets an additional inclusive
minimum for altitude stabilization using the displayed throttle indicator that
changes with Space/Ctrl. Default: 50.0; accepted range: 0..100. Below the
configured percentage, all Z corrections are skipped and vertical state is
cleared for fresh reacquisition. Horizontal stabilization is unaffected. This
is separate from the native governed current-throttle propulsion check.

During eligible neutral stabilization, the mod temporarily suspends the stock
automatic hover and altitude assistance before the native Gyro tick, then
restores their exact previous values when ownership ends. This prevents stock
hover calibration from learning the later kinematic hold. On player entry, the
mod resets saved HoverPitchScale to the selected baseline 1.0. Startup only
binds the entry event and does not reset an already controlled Gyro. It does
not reset this field every tick and does not write Collective.

External collision hits release both stabilization paths for 0.25 seconds so
stock physics can separate the Gyro from moving decks and other obstacles. A
blocking or initially penetrating altitude sweep enters the same release
interval and skips the following vertical-velocity correction.

HorizontalVelocityDecayAcceleration controls kinematic horizontal braking.
With neutral controls, physics-injected horizontal speed growth is rejected and
all horizontal drift decays. While W/S and/or A/D is tilted, the permitted
direction comes from the controls relative to the Gyro; speed along it is
preserved while sideways and opposing speed decays. The correction is an
additive XY-only velocity delta and never rewrites vertical velocity. Default:
150.0 cm/s^2.
