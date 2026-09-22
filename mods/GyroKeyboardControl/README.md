# GyroKeyboardControl

Loader-free IoStore mod for the stock Gyro. It leaves the complete current
Gyro Blueprint at its original package identity, adds a small generated child,
and redirects newly built Gyros through the stock item data asset's
`DroppedActor` class path.

The first implemented keyboard-control improvement is pitch:

- holding W increases TiltForwardInput linearly;
- holding S decreases it linearly;
- releasing W/S leaves the current value unchanged;
- X immediately resets pitch to zero;
- the standard vehicle HUD shows X as Reset pitch / Сбросить наклон;
- Space/Ctrl altitude throttle, A/D roll, yaw, camera, possession, save
  behavior, and all existing keyboard mappings remain unchanged.

Assets/GyroKeyboardControl.ini is copied beside the container. The default
PitchRampSeconds=3.0 means three seconds from neutral to either limit.
Accepted values are clamped to 0.05..60.0. Restart Voyage after editing.

## Build

The normal preparation entry point is:

    .\Build-GyroKeyboardControl.ps1

To build and install in one run:

    .\Build-GyroKeyboardControl.ps1 -Install

It fingerprints the installed game, builds the UE 5.8.2 editor project,
generates the helper, reset action, keyboard context, and replacement child,
narrowly cooks four generated
packages, freshly extracts and surgically patches the stock Gyro item data
asset, packages and verifies the five-asset container, and creates a ZIP under
ignored `artifacts/gyro-keyboard` output.

Build-GyroKeyboardControl.ps1 is the only public producer; the other
PowerShell files in this directory are internal stages used by it. Use
-Install only while Voyage is closed. Installation preserves an existing
GyroKeyboardControl.ini and only appends keys missing from a newer template.
Build, cook, container verification, and clean load are not gameplay
validation.
