# Changelog

## Next

- Fixed abnormally low fuel consumption by restoring the stock default.
- Replaced FuelConsumptionPerMinute with FuelConsumption, defaulting to 250.0 in StableGyro.ini.

## V5

- Added INI settings for fuel capacity and base fuel consumption, with stock game values as defaults.
- Added Nanite support for cable-port bases and reduced the geometry used for ray tracing.
- Altitude stabilization and pitch reset now follow the game's remapped controls.
- Added configurable descent braking for a smooth transition into altitude hold.
- Added separate INI switches for altitude stabilization and horizontal speed control.
- Improved lifting capacity with updated rotor-pitch tuning and an advanced HoverPitchScale setting, defaulting to 2.5.
- Raised the default throttle threshold for altitude stabilization from 50% to 65%.
