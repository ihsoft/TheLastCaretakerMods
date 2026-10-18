# DonkLiftKeyboardControl rules

These constraints supplement the [repository rules](../../AGENTS.md). The
[architecture](ARCHITECTURE.md) describes current behavior; the shared
[release workflow](../../docs/release-and-installation-rule.md) owns candidate,
archive, installation, and version identities.

## Stock package replacement

- Relocate a freshly extracted, complete stock forklift package before creating
  the replacement child at its original virtual path. Keep equal-length package
  name substitution and exact occurrence-count checks as hard assertions. A
  changed stock layout requires diagnosis, not a relaxed binary replacement.
- Ship only the generated cooked assets in the single
  `DonkLiftKeyboardControl_P` container. The editor-only `Source/Voyage` mirror
  and `Source/DonkLiftGenerator` module are build inputs, not runtime payload.

## Control and HUD boundaries

- Each helper may control only its owning forklift, obtained through the child
  actor's parent. Integrate input only while that forklift is player-controlled;
  never target the globally current player pawn. Clear native input and helper
  state when ownership of player control ends.
- Mod-produced active throttle and steering values must stay distinct from the
  game's exact digital command markers. Keep neutral at exact zero.
- Keep the per-frame control path free of object searches, display work,
  logging, and diagnostic state. Supply X/C hints through the native
  provided-action path; do not replace or poll the HUD to impose a visual order.
- Production graphs must not retain probe markers, `PrintString`, `QuitGame`,
  or unused event-observation state. Removing apparently unused Blueprint nodes
  can change cooked bindings; compare against a known-good package and check
  behavior in game after such cleanup.

## Mod-specific game check

Before accepting changed control or HUD behavior, check throttle and steering
in both directions, their limits and native percentages, immediate X/C reset,
localized X/C hints before and after pause, and exit followed by re-entry.
