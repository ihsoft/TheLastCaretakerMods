# DonkLiftKeyboardControl rules

These constraints supplement the [repository rules](../../AGENTS.md). The
[architecture](ARCHITECTURE.md) describes current behavior; the shared
[release workflow](../../docs/release-and-installation-rule.md) owns candidate,
archive, installation, and version identities.

## Stock package replacement

- Preserve the freshly extracted stock forklift at its original virtual path:
  native parent, ordered instance fields, original export identities, CDO and
  component templates. Append only the compiled standalone functions and their
  required references. Do not add an instance field, SCS helper or UberGraphFrame
  to the stock class; streamed map instances use its original unversioned schema.
- Preserve all unchanged export payloads byte-for-byte. Validate the changed
  class and functions, remapped references and dependency phases. An unchanged
  opaque stock export is allowed only when its identity and bytes are preserved
  and independent inspection agrees; it does not justify opaque changed exports.
- The temporary relocated parent is a build-only donor dependency. Keep its
  equal-length substitution and exact occurrence-count checks; never ship it
  as a substitute for canonical component-template exports.
- Ship only the assembled forklift and generated supporting cooked assets in the single
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
For package/schema changes, also stream a POI containing a preplaced forklift
from an existing save; testing only a freshly spawned forklift is insufficient.
