# StableGyro rules

Root ../../AGENTS.md applies. This file owns Gyro-specific contracts.

## Documentation map

- [README.md](README.md) describes the shipped controls, runtime behavior,
  user-facing settings, units, accepted ranges, and public build/install entry
  point.
- [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md) owns the game fingerprint
  gate, stock Gyro package/class/input/physics contracts, direct
  `PropellerVelocity` reference values, and tracked reconstructed inputs.
- [Vehicle and HUD modding patterns][postphysics-pattern]
  owns the reusable PostPhysics correction model, axis-local velocity writes,
  previous-command growth rejection, control-intent projection, state reset,
  and actuator-speed gating used by this mod.
- [Gyro research pitfalls](../../docs/research-pitfalls.md#управление-gyro)
  owns rejected Gyro architectures and deceptive no-op causes, including stock
  Blueprint relocation, child Tick replacement, component runtime identity,
  pure-node reevaluation, Shipping diagnostics, and PrePhysics correction.
- [Standard hint pitfalls](../../docs/research-pitfalls.md#стандартные-подсказки)
  and [standard action UI patterns][action-ui-pattern]
  own the provided-action/HUD distinction and exact UFunction-owner rules
  behind the X hint.

[postphysics-pattern]: ../../docs/vehicle-and-hud-modding-patterns.md#post-physics-kinematic-correction
[action-ui-pattern]: ../../docs/vehicle-and-hud-modding-patterns.md#discovering-voyages-standard-action-ui

- The mod owns control and flight-stability improvements for the stock Gyro.
  It includes W/S integration of native TiltForwardInput, X reset and HUD
  hint, arcade altitude stabilization, and kinematic horizontal-velocity
  damping guided by the two-axis control intent in the vehicle's local frame.
  Neutral controls reject physics-injected horizontal speed growth before
  decay. Apply horizontal correction as an additive XY-only velocity delta;
  never rewrite the full velocity vector or its Z component from the horizontal
  path. The helper must tick in `TG_PostPhysics`, after the stock rotor/Chaos
  force integration. Gate every kinematic horizontal or vertical stabilization
  correction on the stock rotor's measured `PropellerVelocity`; below the
  configured direct threshold, clear stabilization state and do not alter
  position or velocity. During vertical braking, reject positive
  Vz growth above the previous helper command before applying the configured
  constant-rate deceleration. Do not alter A/D integration, yaw, camera,
  possession, save behavior, or other HUD/input behavior without a new user
  request.
- W increases, S decreases, and releasing both leaves the integrated value
  unchanged. X writes exact zero immediately. Treat only exact native digital
  markers `-1`, `0`, and `1` as commands, clamp integrated pitch to
  `-0.9999..0.9999`, and reset helper plus native pitch state when the owning
  Gyro is not player controlled.
- Build-StableGyro.ps1 is the only public producer. It owns fingerprinting,
  editor build, generation, cook, clean item-data extraction, surgical patching,
  packaging, verification, ZIP creation, and optional -Install.
- `-Install` must preserve current values and comments in an existing
  `StableGyro.ini`, remove explicitly retired keys, append missing canonical
  keys, and hash-check the resulting settings file. Create it from canonical
  defaults only when it does not exist.
- Treat GAME_DERIVED_SOURCES.md as a hard fingerprint gate. Extract and patch
  a fresh current `DA_Item_Module_GyroCopter`; never commit extracted or cooked
  data. Keep the stock Gyro Blueprint at its original package identity. The mod
  subclass is selected only through the item data asset's `DroppedActor` path.
- Preserve every stock Gyro keyboard mapping when replacing
  `IMC_GyroCopter_Keyboard`; the only added mapping is the mod-owned reset
  action on X. The child `GetProvidedActionsBP` must append reset to the stock
  Blueprint's returned actions so the conditional F cargo hint survives.
- Gameplay validation covers pitch in both directions, release hold, both
  limits, X, exit/re-entry, full-throttle altitude braking/hold, and horizontal
  damping under climb, descent, neutral, directional, and reversing input.

## Settings contract

`Assets/StableGyro.ini` is the canonical packaged INI and sole source
of defaults, comments, order, and formatting. Comments occupy their own lines;
inline comments after values are rejected because the runtime numeric parser
would otherwise keep the baked default.
`Settings/StableGyro.settings.json` owns only the flat-section binding
and validation metadata. For each option:

- `id` is the generated C++ symbol;
- `runtimeName` is the generated Blueprint member and output-pin name;
- `key` is the exact name to the left of `=` in the INI;
- `type` creates a `double` or `bool` member on the generated `ModActor`; and
- `minimum` and `maximum` clamp a numeric value after parsing.

To add a setting, add the canonical value and adjacent comments to the INI,
then add one schema entry with matching identities. Include
`StableGyroSettings.h` and use the existing
`namespace Settings = StableGyroSettings;` alias. The public producer creates
the member, initializes it from the canonical default, and builds the BeginPlay
INI loader. Do not call `AddMemberVariable` for the setting and do not edit the
generated header. A consuming graph on `ModActor` reads the generated self-member
like this:

```cpp
UK2Node_VariableGet* Deceleration = AddRead(
    Graph, Settings::AltitudeStabilizationVerticalDeceleration,
    nullptr, X, Y);
Ok &= Connect(
    RequirePin(Deceleration,
        Settings::AltitudeStabilizationVerticalDeceleration),
    RequirePin(Consumer, ConsumerPin));
```

The null owner is required: every generated setting belongs to `ModActor`, not
the stock Gyro or the replacement child. `Settings::<id>` is an `FName`, not
the configured numeric or boolean value; it names the self-member and its value
pin, while the `VariableGet` output carries the runtime value. In the initial
helper generator the equivalent helpers are named `Read`, `Pin`, and `Link`;
the ownership and name contract is identical. After wiring the consumer, run
only the public producer, `Build-StableGyro.ps1`. It generates
`Intermediate/GeneratedSettings/StableGyroSettings.generated.h`,
validates the exact INI/schema key set, and rejects malformed defaults.

To remove a setting, remove its INI entry, schema entry, and every consumer.
If installed user files must be migrated, add only the exact old key and its
exact canonical comment lines to the retired lists in
`Update-StableGyroSettings`. Installation preserves values for active keys,
appends only missing canonical keys, and removes only those explicitly listed
retired lines.
