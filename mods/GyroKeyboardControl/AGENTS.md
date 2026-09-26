# GyroKeyboardControl rules

Root ../../AGENTS.md applies. This file owns Gyro-specific contracts.

- The mod owns keyboard-control improvements for the stock Gyro. The current
  phase includes W/S integration of native TiltForwardInput, X reset and HUD
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
  unchanged. X writes exact zero immediately.
- `Assets/GyroKeyboardControl.ini` is the canonical packaged INI and sole
  source of setting defaults, comments, order, and formatting.
  Comments must occupy their own lines; runtime parsing and build validation
  reject inline comments after setting values.
  `Settings/GyroKeyboardControl.settings.json` owns only the flat-section key
  identity, runtime binding, type, and numeric range. Add or remove an option
  in both files, then wire only its consuming behavior by hand; never edit
  generated output. `Build-GyroKeyboardControl.ps1` owns internal settings
  validation and generation. Installation preserves existing user values for
  active keys, appends only missing canonical keys, and may remove only
  explicitly declared retired keys plus their exact canonical comments.
- Build-GyroKeyboardControl.ps1 is the only public producer. It owns fingerprinting,
  editor build, generation, cook, clean item-data extraction, surgical patching,
  packaging, verification, ZIP creation, and optional -Install.
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
