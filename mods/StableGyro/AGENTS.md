# StableGyro rules

Root ../../AGENTS.md applies. This file owns Gyro-specific contracts.

## Documentation map

- [README.md](README.md) describes the shipped controls, runtime behavior,
  user-facing settings, units, accepted ranges, and public build/install entry
  point.
- [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md) owns the game fingerprint
  gate, stock Gyro package/class/input/physics contracts, direct
  `PropellerVelocity` reference values, and tracked reconstructed inputs.
- [ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md) owns the version-bound native hover
  calibration formula, assistance-ownership order, recovery limits and exact
  gameplay-validation boundary.
- [StableGyro backlog](../../docs/stablegyro-backlog.md) owns the active restart
  state, candidate evidence, pending gates, and next validation action.
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
  Away from external contact, neutral controls reject physics-injected
  horizontal speed growth before decay. Apply horizontal correction as an additive XY-only velocity delta;
  never rewrite the full velocity vector or its Z component from the horizontal
  path. The stabilization helper must tick in `TG_PostPhysics`, explicitly after
  the stock Gyro tick and its rotor/Chaos force integration. Gate every
  kinematic horizontal or vertical stabilization
  correction on `bEngineRunning`, positive `CurrentThrottle`, and the stock
  rotor's measured `PropellerVelocity`; when any gate fails, clear stabilization
  state and do not alter position or velocity. External collision hits release
  both correction paths for 0.25 seconds. Enable hit notification once on the
  owned primitive components, and use a swept Z correction whose blocking-hit
  or initial-penetration result enters the same release interval. During that
  interval, perform no kinematic correction. During vertical braking away from
  contact, a positive configured deceleration rejects positive Vz growth above
  the previous helper command before applying the constant-rate deceleration.
  A zero vertical-deceleration setting disables that positive-Vz kinematic
  damping, including the growth rejection: while observed Vz is positive,
  perform no Z position or velocity correction and clear the previous vertical
  sample. It does not disable later exact-hold acquisition once Vz becomes
  non-positive or any horizontal stabilization. Space must preserve all
  positive Vz, including when current altitude is at or below the stored floor;
  raise the floor only when current altitude exceeds it. Do not require full
  throttle for stabilization.
  While Ctrl is held, clear vertical hold/braking state and let stock descent
  operate; Ctrl takes precedence over Space, while propulsion- and contact-gated
  horizontal damping may continue. Re-entering with Space released must brake positive Vz
  before exact hold, rather than discarding climb inertia. Do not alter A/D integration, yaw, camera,
  possession, save behavior, or other HUD/input behavior without a new user
  request.
- Native altitude-assistance ownership is independent of
  `EnablePitchCorrection`. In the PrePhysics helper, derive ownership from the
  same player-control, propulsion, input and contact eligibility used by the
  PostPhysics stabilization path. Cache the owned Gyro and sibling helper once,
  capture `bAutoHoverCollective` and `bAltitudeAssist` on first acquisition,
  disable them before the native Gyro tick, and restore the exact captured
  values on every ownership loss, controller change and EndPlay. On each change
  to a player controller, the PostPhysics helper must reset the owning Gyro's
  saved `HoverPitchScale` to the selected recovery baseline `1.0` through the
  validated Engine property setter. Reset only from the genuine
  controller-change event; BeginPlay binds the delegate but does not invoke the
  callback. Do not poll, reset per tick, or write `Collective`. The
  version-bound native formula and recovery boundary live in
  [ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md).
- When pitch correction is enabled, W increases, S decreases, and releasing
  both leaves the integrated value unchanged. X writes exact zero immediately.
  Treat only exact native digital
  markers `-1`, `0`, and `1` as commands, clamp integrated pitch to
  `-0.9999..0.9999`, and reset helper plus native pitch state when the owning
  Gyro is not player controlled. Run the pitch helper in `TG_PrePhysics`, order
  it after the player controller and before the stock Gyro with explicit tick
  prerequisites, and restore the stored integrated value before every controlled
  Gyro tick, including neutral input. When pitch correction is disabled, the
  pitch helper must not read or write native `TiltForwardInput` on any tick or
  lifecycle path and must not append the mod-owned X reset hint; inherited
  stock actions pass through. The independent PostPhysics horizontal damping
  path may still read the two native tilt axes as control intent.
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
  action on X. The child `GetProvidedActionsBP` must call the inherited native
  action provider and, only while pitch correction is enabled, append reset to
  its returned actions so base actions such as the conditional F cargo hint
  survive.
- Gameplay validation covers pitch in both directions, release hold, both
  limits, X, exit/re-entry, altitude braking/hold across partial and full
  throttle, Ctrl descent/reacquisition, and horizontal
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
- `type` creates a `double` or `bool` member on the generated settings owner; and
- `minimum` and `maximum` clamp a numeric value after parsing.

To add a setting, add the canonical value and adjacent comments to the INI,
then add one schema entry with matching identities. Include
`StableGyroSettings.h` and use the existing
`namespace Settings = StableGyroSettings;` alias. The public producer creates
the member, initializes it from the canonical default, and builds the callable
`ReloadSettings` loader. Both helpers call it at BeginPlay and synchronously on
player entry through the owning Pawn's controller-changed delegate, resetting
their defaults before every read. `ModActor` owns the full settings set for
PostPhysics stabilization; `PitchInputActor` owns only the duplicated
`PitchRampSeconds` and `EnablePitchCorrection` members needed by its PrePhysics
graph and action-hint gate. Do not call `AddMemberVariable` separately for a schema
setting and do not edit the generated header. A consuming graph reads its
own generated self-member
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

The null owner is required: every generated setting belongs to the helper whose
graph consumes it, not the stock Gyro or the replacement child. `Settings::<id>` is an `FName`, not
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
