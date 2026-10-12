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
pitch. Default: 3.0 seconds. Settings are reread when you enter a Gyro; exit
and re-enter after editing the file.
Put comments on their own lines; inline comments after values are invalid.

StabilizationMinimumPropellerVelocity sets the minimum measured rotor speed at
which altitude and horizontal stabilization may operate, in the stock
PropellerVelocity units. Default: 390.0. Below the threshold, the mod does not
change position or velocity and clears stabilization state. Set 0 to disable
this gate.

AltitudeStabilizationVerticalDeceleration controls how quickly positive
vertical speed is reduced after releasing Space or entering stabilization with
Space released. Default: 100.0 cm/s^2. Physics-driven positive speed growth is
rejected while braking, and all kinematic corrections run after the stock
physics step. The current altitude is held after the climb stops. Hold Ctrl to
release vertical hold/braking for stock descent and landing; Ctrl wins over
Space. Horizontal damping remains independent. Exit and re-enter the Gyro after
editing the INI.

HorizontalVelocityDecayAcceleration controls kinematic horizontal braking.
With neutral controls, physics-injected horizontal speed growth is rejected and
all horizontal drift decays. While W/S and/or A/D is tilted, the permitted
direction comes from the controls relative to the Gyro; speed along it is
preserved while sideways and opposing speed decays. The correction is an
additive XY-only velocity delta and never rewrites vertical velocity. Default:
150.0 cm/s^2.
