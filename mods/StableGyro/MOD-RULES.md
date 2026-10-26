# StableGyro rules

Root ../../AGENTS.md applies. This file owns Gyro-specific contracts.

## Documentation map

- [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md) owns the source provenance,
  stock Gyro package/class/input/physics contracts, direct
  `PropellerVelocity` reference values, and tracked reconstructed inputs.
- [ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md) owns the version-bound native hover
  calibration formula, assistance-ownership order, recovery limits and exact
  gameplay-validation boundary.
- [Vehicle and HUD modding patterns][postphysics-pattern]
  owns the reusable PostPhysics correction model, axis-local velocity writes,
  previous-command growth rejection, control-intent projection, state reset,
  and actuator-speed gating used by this mod.
- [Gyro research pitfalls](../../docs/research-pitfalls-knowledge.md#gyro-controls)
  owns rejected Gyro architectures and deceptive no-op causes, including stock
  Blueprint relocation, child Tick replacement, component runtime identity,
  pure-node reevaluation, Shipping diagnostics, and PrePhysics correction.
- [Standard hint pitfalls](../../docs/research-pitfalls-knowledge.md#standard-input-hints)
  and [standard action UI patterns][action-ui-pattern]
  own the provided-action/HUD distinction and exact UFunction-owner rules
  behind the X hint.

[postphysics-pattern]: ../../docs/vehicle-and-hud-modding-patterns-knowledge.md#post-physics-kinematic-correction
[action-ui-pattern]: ../../docs/vehicle-and-hud-modding-patterns-knowledge.md#discovering-voyages-standard-action-ui

- Runtime flight input must use the exact `IA_GyroCopterThrottleUp`,
  `IA_GyroCopterThrottleDown` and `IAV_GyroPitchReset` actions. Space/Ctrl/X
  references below denote their defaults. Query bound action values on the
  owning Gyro, never its helper or raw keyboard state. Keep Up/Down separate
  and retain Down precedence. Generate the three value bindings from connected
  Blueprint accessor nodes; validate that all three survive cooking alongside
  the cargo event bindings. Preserve stock player-mappable identities,
  triggers, modifiers and mapping settings.
- The mod owns control and flight-stability improvements for the stock Gyro.
  It includes W/S integration of native TiltForwardInput, X reset and HUD
  hint, arcade altitude stabilization, and kinematic horizontal-velocity
  damping guided by the two-axis control intent in the vehicle's local frame.
  Away from external contact, neutral controls reject physics-injected
  horizontal speed growth before decay. Apply horizontal correction as an additive XY-only velocity delta;
  never rewrite the full velocity vector or its Z component from the horizontal
  path. `EnableHorizontalVelocityControl=false` must branch before every
  horizontal physics-velocity read or write, clear both the stored horizontal
  sample and its validity flag, and continue into the independent vertical path.
  It must not disable altitude or W/S pitch control. The stabilization helper must tick in `TG_PostPhysics`, explicitly after
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
  raise the floor only when current altitude exceeds it. After horizontal
  damping and the altitude-enable gate, process Ctrl before the minimum
  `Collective` gate. Gate neutral braking, Space and exact hold on
  `Collective >= AltitudeStabilizationMinimumThrottlePercent / 100`. Below that
  threshold with Ctrl released, clear only vertical lock, exact-hold, Space,
  braking and sample state; do not clear or gate horizontal state. Keep the setting inclusive,
  configurable over `0..100`, and retain the schema fallback `50` rather than
  hardcoding a full-throttle requirement. The distributed INI sets it to `65`;
  do not replace the schema fallback with that distributed value.
  While Ctrl is held, clear vertical hold/braking/sample state and let stock
  descent operate with an upward-speed cap. On first eligibility, capture
  `cap = max(physical Vz, 0)` in PrePhysics before the freed physics step;
  PostPhysics applies `outputVz = min(observedVz, cap)` and then
  `cap = max(outputVz, 0)`. Preserve existing upward inertia and its natural
  decay, reject renewed upward acceleration, and leave negative Vz free to
  increase or decrease. Apply an additive Z-only velocity delta, never a
  position correction or configured braking rate in this Ctrl-held guard.
  It requires altitude control, player control, propulsion, rotor speed and
  expired contact cooldown, including below the minimum Collective percentage.
  It is independent of pitch and horizontal control flags. Ctrl takes precedence
  over Space, while eligible horizontal damping may continue. Clear the cap on
  release, controller/EndPlay changes, invalid references and eligibility loss;
  ordinary native-assistance restoration must not clear an active held-Ctrl cap.
  On fresh eligible hold acquisition with Space released, snapshot physical Vz
  before writes and select braking direction from its sign. This includes
  delayed reacquisition after native Collective recovers to the threshold;
  do not depend on a remembered Ctrl-release flag. Brake negative Vz toward zero at
  `AltitudeStabilizationDescentDeceleration`, rejecting physics-injected growth
  in downward speed against the previous helper command before applying the
  constant-rate correction. Acquire exact hold only when corrected Vz reaches
  or crosses zero. A zero descent-deceleration setting disables negative-Vz
  damping and growth rejection while observed Vz remains negative, clears the
  previous vertical sample, and still allows exact hold once observed Vz becomes
  nonnegative. Re-entering with Space released must brake positive Vz before
  exact hold, rather than discarding climb inertia. Do not alter A/D integration, yaw, camera,
  possession, save behavior, or other HUD/input behavior without a new user
  request.
- `EnableAltitudeControl=false` must branch after the independent horizontal
  path and before every mod-owned vertical position or Z-velocity correction.
  Clear vertical lock, exact hold, Space and Ctrl-cap state, both braking
  modes, locked altitude and previous vertical samples. Do not reset saved
  `HoverPitchScale`, and do not acquire native altitude-assistance ownership;
  restore any captured `bAutoHoverCollective` and `bAltitudeAssist` values.
  Disabled altitude control leaves horizontal damping and W/S pitch integration
  available. Both control flags default to true and reload on player entry.
- Native altitude-assistance ownership is independent of
  `EnablePitchCorrection`, but requires `EnableAltitudeControl=true`. In the PrePhysics helper, derive ownership from the
  same player-control, propulsion, neutral-input and contact eligibility used by
  PostPhysics neutral altitude stabilization, including its configured minimum
  `Collective` percentage. Cache the owned Gyro and sibling helper once,
  capture `bAutoHoverCollective` and `bAltitudeAssist` on first acquisition,
  disable them before the native Gyro tick, and restore the exact captured
  values on every ownership loss, controller change and EndPlay. On each change
  to a player controller, the PostPhysics helper must reset the owning Gyro's
  saved `HoverPitchScale` to the entry-reloaded `HoverPitchScale` INI setting
  through the validated Engine property setter. Its default and invalid-or-missing
  fallback are `2.5`, its accepted range is `0.5..2.5`, and the canonical INI
  keeps it under the advanced-settings warning. Reset only from the genuine
  controller-change event; BeginPlay binds the delegate but does not invoke the
  callback. Do not poll, reset per tick, or write `Collective`. The
  version-bound native formula and recovery boundary live in
  [ALTITUDE-CONTROL.md](ALTITUDE-CONTROL.md).
- When pitch correction is enabled, W increases, S decreases, and releasing
  both leaves the integrated value unchanged. The reset action writes exact
  zero once per press. Derive its edge from the current bound value and a
  transient previous-active flag; update that flag only after both pitch writes.
  Clear it on invalid ownership, disabled/uncontrolled paths, controller changes
  and EndPlay. Do not assume a Pressed mapping's bound value is a one-frame pulse.
  Treat only exact native digital
  markers `-1`, `0`, and `1` as commands, clamp integrated pitch to
  `-0.9999..0.9999`, and reset helper plus native pitch state when the owning
  Gyro is not player controlled. Run the pitch helper in `TG_PrePhysics`, order
  it after the player controller and before the stock Gyro with explicit tick
  prerequisites established at BeginPlay and controller changes, without
  re-adding them per tick, and restore the stored integrated value before every controlled
  Gyro tick, including neutral input. When pitch correction is disabled, the
  pitch helper must not read or write native `TiltForwardInput` on any tick or
  lifecycle path and must not append the mod-owned X reset hint; inherited
  stock actions pass through. The independent PostPhysics horizontal damping
  path may still read the two native tilt axes as control intent.
- `ModActor` owns `FuelCapacity` and `FuelConsumption`, with defaults,
  units and ranges in [architecture](ARCHITECTURE.md#fuel-settings). Apply both
  after settings reload at helper BeginPlay and genuine player entry, before
  any flight-control enable gate. Change only the existing custom module's
  Petrol capacity map entry and the Gyro's base consumption property; preserve
  stored fuel and other resource entries. Do not refill, clamp stored fuel,
  poll settings/components on Tick, or modify throttle or workload consumption
  from this path. Keep the new native mirrors as graph-reference identities,
  never serialization templates for authored component instances.
- Build-StableGyro.ps1 is the only public producer. It owns fingerprinting,
  editor build, generation, cook, clean item-data extraction, surgical patching,
  packaging, verification, ZIP creation, and optional -Install.
- `-Install` must preserve current values and comments in an existing
  `StableGyro.ini`, remove explicitly retired keys, append missing canonical
  keys, and read back the resulting settings file. Create it from canonical
  defaults only when it does not exist.
- Treat GAME_DERIVED_SOURCES.md as source provenance, not a mandatory manual
  audit before a candidate build. On a pin mismatch retry the public producer
  with `-IgnorePinnedGameVersion`. Extract and patch a fresh current
  `DA_Item_Module_GyroCopter`; never commit extracted or cooked
  data. Keep the stock Gyro Blueprint at its original package identity. The mod
  subclass is selected only through the item data asset's `DroppedActor` path.
- Preserve every stock Gyro keyboard mapping when replacing
  `IMC_GyroCopter_Keyboard`; the only added mapping is the mod-owned reset
  action on X by default. The child `GetProvidedActionsBP` must call the
  inherited native
  action provider and, only while pitch correction is enabled, append reset to
  its returned actions so base actions such as the conditional F cargo hint
  survive.
- Always construct the two extra mooring socket endpoints with their stable
  component names, SocketIDs, templates and native `VehicleMesh` attachment so
  saved connections can resolve in either setting mode. When four-port mode is
  disabled, leave the four custom plates and inherited stock-part layout out,
  bind both extra endpoints for initial and late-restored connections, and use a
  coalesced next-tick cleanup after every connection event regardless of its bool
  payload. Cleanup must keep its guard set through both sockets, call the stock
  non-destroying disconnect helper, keep the endpoint objects registered,
  and disable only this Gyro's socket visuals and interaction collision. Do not
  destroy sockets, poll, use an arbitrary load delay, or traverse cable and
  remote-endpoint references for visual cleanup. Capture each socket's child
  component array once before traversing it.
- Four-port mode must retain the stock Drop Cargo action identity, native hold
  threshold and once-per-press behavior. Use the same live threshold for input
  and hint; include any connected Mooring port in the hint predicate and stock
  disconnect path. Disabled mode keeps the native two-port action and hint.
- Preserve the extracted mooring mesh's corner normals, UV channels, vertex
  colors and material override when splitting its two sides. Do not manually
  remove zero-area triangles or invent tangents absent from the source GLB.
  Prepare runtime meshes through the shared StaticMesh producer with the
  Nanite, fallback and ray-tracing policy in [architecture](ARCHITECTURE.md).
  Keep editor reference meshes out of the runtime payload and preserve every
  cooked companion emitted by the common producer. Validate the exact final
  container's representations, source attributes and material references before
  installation; geometry preservation and game appearance are separate gates.
- Gameplay validation covers pitch in both directions, release hold, both
  limits, X, exit/re-entry, altitude braking/hold across partial and full
  throttle, Ctrl descent/reacquisition with positive and zero configured descent
  deceleration, and horizontal damping under climb, descent, neutral,
  directional, and reversing input. Test both new flags independently and
  together, including entry reload, restoration of native altitude assistance,
  no `HoverPitchScale` reset while altitude control is disabled, and no XY
  physics-velocity access while horizontal control is disabled.

## Settings contract

`Assets/StableGyro.ini` owns distributed values, comments, order, and
formatting. Inline `#` comments are supported.
`Settings/StableGyro.settings.json` owns the flat-section binding, required
schema fallbacks, and validation metadata. For each option:

- `id` is the generated C++ symbol;
- `runtimeName` is the generated Blueprint member and output-pin name;
- `key` is the exact name to the left of `=` in the INI;
- `type` creates a `double`, `bool`, or string member on the generated owner;
- `default` is the fallback for missing or invalid runtime input; and
- `minimum` and `maximum` are optional inclusive bounds. Out-of-range input
  uses the schema fallback and is never clamped.

To add a setting, add the canonical value and adjacent comments to the INI,
then add one schema entry with matching identities. Include
`StableGyroSettings.h` and use the existing
`namespace Settings = StableGyroSettings;` alias. The public producer creates
the member, initializes it from the schema fallback, and builds the callable
`ReloadSettings` loader. Both helpers call it at BeginPlay and synchronously on
player entry through the owning Pawn's controller-changed delegate, resetting
their defaults before every read. `ModActor` owns the full settings set for
PostPhysics stabilization, including both independent control-enable flags;
`PitchInputActor` reads `EnableAltitudeControl` through its cached sibling only
for native altitude-ownership eligibility and does not duplicate the setting.
`PitchInputActor` owns only the duplicated
`PitchRampSeconds` and `EnablePitchCorrection` members needed by its PrePhysics
graph and action-hint gate. The replacement Gyro child alone owns
`EnableFourMooringPorts`; it reads the setting once per loaded replacement-class
CDO before parent construction and all of its layout, cargo-input and
provided-action branches use that same cached value. Class unload/reload may
read again, so changing this setting requires a game restart. Existing helper
loaders must explicitly exclude it. Do not call `AddMemberVariable` separately for a schema
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

The null owner is required for helper-owned settings: each belongs to the helper
whose graph consumes it. The child-owned four-port setting is the narrow
exception and is read from the exact replacement-class CDO. `Settings::<id>` is an `FName`, not
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
