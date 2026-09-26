Gyro Pitch Control

Copy GyroKeyboardControl_P.pak, GyroKeyboardControl_P.ucas,
GyroKeyboardControl_P.utoc, and GyroKeyboardControl.ini to:

Voyage\Content\Paks

Controls:
W  - smoothly increase forward pitch
S  - smoothly decrease forward pitch
X  - immediately reset pitch to zero

Releasing W or S keeps the current pitch value.

PitchRampSeconds in GyroKeyboardControl.ini sets the time from zero to maximum
pitch. Default: 3.0 seconds. Restart the game after editing the file.
Put comments on their own lines; inline comments after values are invalid.

StabilizationMinimumPropellerVelocity sets the minimum measured rotor speed at
which altitude and horizontal stabilization may operate, in the stock
PropellerVelocity units. Default: 390.0. Approximate references: 392 at 10%
throttle, 784 at 20%, and 1176 at 30%. Below the threshold, the mod does not
change position or velocity and clears stabilization state. Set 0 to disable
this gate.

AltitudeStabilizationVerticalDeceleration controls how quickly positive
vertical speed is reduced after releasing Space at full throttle. Default:
100.0 cm/s^2. Physics-driven positive speed growth is rejected while braking,
and all kinematic corrections run after the stock physics step. The current
altitude is held after the climb stops; horizontal velocity is handled
independently. Restart Voyage after editing the INI.

HorizontalVelocityDecayAcceleration controls kinematic horizontal braking.
With neutral controls, physics-injected horizontal speed growth is rejected and
all horizontal drift decays. While W/S and/or A/D is tilted, the permitted
direction comes from the controls relative to the Gyro; speed along it is
preserved while sideways and opposing speed decays. The correction is an
additive XY-only velocity delta and never rewrites vertical velocity. Default:
150.0 cm/s^2.
