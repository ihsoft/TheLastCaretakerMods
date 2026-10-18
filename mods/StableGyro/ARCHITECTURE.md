# StableGyro architecture

The loader-free mod leaves the stock Gyro Blueprint at its original package
identity and generates a child selected through the stock item data asset's
`DroppedActor`. One helper handles pitch input before the stock physics tick;
another applies eligible altitude and horizontal stabilization after physics.
The child retains stock mappings and provided actions while adding an optional
X pitch-reset action.

`Assets/StableGyro.ini` is the packaged settings source, and
`Settings/StableGyro.settings.json` defines runtime bindings and value ranges.
[ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md) explains the native hover and
stabilization boundary. [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md)
identifies version-bound inputs; [mod rules](MOD-RULES.md) give the exact
behavior and settings constraints.
