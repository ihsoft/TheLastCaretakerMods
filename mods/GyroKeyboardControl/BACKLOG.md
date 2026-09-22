# GyroKeyboardControl active backlog

## Restart

- Current game fingerprint: Steam build `25191271`, executable SHA-256
  `747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`.
- The first runtime candidate is rejected: relocating the stock Gyro Blueprint
  and replacing its original package with a child produced a world-bound Gyro
  whose driver could not exit.
- Root-node experiments were rejected as the explanation: the validated
  DonkLift child contains the same generated default-root pattern.
- Current source leaves the stock Gyro Blueprint in place, generates
  `/Game/Mods/GyroKeyboardBP/BP_GyroCopter_KeyboardCtl`, and patches
  `DA_Item_Module_GyroCopter.DroppedActor` to select that subclass.
- UBT, both generators, narrow cook, equal-length DroppedActor patch, package
  inventory, `retoc verify`, installation, and installed hash readback pass.
- Installed candidate: `artifacts/gyro-keyboard/build-20260922-023927`, archive
  SHA-256 `CD003C4D86F508BA931760F9A8282B5C3131796C52C00C2BBB71EAA3BBC169AD`.
- Installed semantic readback proves the container has exactly the helper,
  subclass, and patched item data asset; it does not override the stock Gyro
  Blueprint. The subclass directly inherits the stock Blueprint and the decoded
  DroppedActor path selects the subclass.
- The user confirmed the current subclass architecture, pitch controls, and
  standard X HUD hint work in the real game on 2026-09-22.
- The X hint is supplied through `IAV_GyroPitchReset`, an
  exact stock-mapping-preserving replacement of `IMC_GyroCopter_Keyboard`, and
  a child `GetProvidedActionsBP` override that calls the stock Blueprint parent
  implementation before appending reset.
- Editor module build, Blueprint generation, and four-package cook passed in
  `artifacts/gyro-keyboard/build-x-hint-20260922-03`; that agent run stopped
  safely before stock extraction because Voyage was running. The user then ran
  the public producer and validated the resulting package in Voyage.
- Current checkpoint: v1 is game-validated for this fingerprint. Revalidate the
  conditional F cargo hint explicitly after any future change to the stock
  `GetProvidedActionsBP` contract or keyboard context.

## Owning references

- `AGENTS.md`
- `GAME_DERIVED_SOURCES.md`
- `docs/research-pitfalls.md#управление-gyro`
