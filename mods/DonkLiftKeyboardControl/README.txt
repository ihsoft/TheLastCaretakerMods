DonkLift Keyboard Control
=========================

Improves keyboard control of the DonkLift-320 forklift while keeping the
native vehicle behavior and HUD.

Controls
--------

- W gradually increases forward throttle.
- S gradually increases reverse throttle (or reduces forward throttle first).
- Releasing W/S holds the current throttle position.
- A/D gradually move the steering position.
- Releasing A/D holds the current steering position.
- X immediately resets throttle to neutral.
- C immediately centers the steering input.

The forklift HUD shows native-style X / Brake and C / Center hints. English and
Russian labels follow the language selected in The Last Caretaker's main menu.
Unsupported languages currently fall back to English.

Installation
------------

1. Install a compatible UE4SS Compact build for The Last Caretaker.
2. Copy the DonkLiftKeyboardControl directory into ue4ss/Mods.
3. Copy FText_Constructor.lua into ue4ss/UE4SS_Signatures.
4. Add or enable this line in ue4ss/Mods/mods.txt:

   DonkLiftKeyboardControl : 1

The supplied FText signature is version-specific. Revalidate or update it after
The Last Caretaker updates its executable.

Configuration
-------------

Acceleration and steering constants are grouped at the top of Scripts/main.lua.
The validated defaults are:

- throttle rate: 0.45 to 0.80 per second, ramped over 2 seconds;
- steering maximum speed: 1.60 per second;
- steering acceleration: 2.50 per second squared;
- steering reversal braking: 5.00 per second squared.

Known limitation
----------------

Leaving the forklift clears its steering input, but wheels that are already
turned may remain visually turned until the forklift is entered again.
