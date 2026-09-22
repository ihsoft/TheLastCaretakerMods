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
