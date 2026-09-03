DonkLift Keyboard Control
=========================

Improves keyboard control of the DonkLift-320 forklift while keeping the
native vehicle behavior and HUD.

Version
-------

Mod version: v2
Game-validated on The Last Caretaker 5.5 (Steam build 25056839, UE 5.8.1).

The game version is a tested compatibility record, not a hard requirement.
Future game updates may remain compatible. The previous v1 package is known to
be incompatible with game 5.5, primarily because it targets the earlier Unreal
Engine generation.

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

Copy `DonkLiftKeyboardControl_P.pak`, `.ucas`, and `.utoc` into:

`<Game>\Voyage\Content\Paks\`

The mod is self-contained and requires neither UE4SS nor DML. All three files
belong to one container and must be installed or removed together.

Configuration
-------------

The current package has no external configuration file. Its validated defaults
are:

- throttle rate: one third of the full range per second;
- steering maximum speed: 1.20 per second;
- steering acceleration: 2.50 per second squared;
- steering reversal braking: 5.00 per second squared.

Known limitation
----------------

Leaving the forklift clears its steering input, but wheels that are already
turned may remain visually turned until the forklift is entered again.
