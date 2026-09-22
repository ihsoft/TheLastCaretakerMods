# GyroKeyboardControl rules

Root ../../AGENTS.md applies. This file owns Gyro-specific contracts.

- The mod owns keyboard-control improvements for the stock Gyro. The current
  first phase is W/S integration of native TiltForwardInput, X reset, and the
  standard X HUD hint. Do not alter altitude throttle, A/D roll, yaw, camera,
  possession, physics tuning, save behavior, or other HUD/input behavior
  without a new user request.
- W increases, S decreases, and releasing both leaves the integrated value
  unchanged. X writes exact zero immediately.
- Assets/GyroKeyboardControl.ini owns the default PitchRampSeconds=3.0.
  Preserve existing installed settings and append only missing keys.
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
- The mod is not gameplay-validated until the user tests both directions,
  release hold, both limits, X, exit, and re-entry in the real game.
