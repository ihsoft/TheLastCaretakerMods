# Repository rules

These rules apply to the entire `TheLastCaretakerMods` repository.

## Sources of truth

- Treat `mods/DonkLiftKeyboardControl/Scripts/main.lua` and the latest
  game-validated Git checkpoint as the control implementation of record.
- Treat the HUD section of `mods/DonkLiftKeyboardControl/Scripts/main.lua`
  together with `ue4ss/UE4SS_Signatures/FText_Constructor.lua` as the
  game-validated hotkey HUD implementation of record.
- Treat older control/HUD designs in `docs/` as historical research when they
  disagree with the current source. In particular, do not restore the old
  `ReceiveTick`, setter-hook, `Acceleration`/`Steering`, or direct HUD-widget
  architecture without a new, isolated experiment.
- The installed test copy is
  `P:\SteamLibrary\steamapps\common\Voyage\Voyage\Binaries\Win64\ue4ss\Mods\DonkLiftKeyboardControl\Scripts\main.lua`.
  The repository copy and installed copy must have matching hashes before a
  test is handed to the user.

## Verified DonkLift input contract

- The reliable interception points are the pre-hooks for
  `/Script/Voyage.VoyageVehicleForkliftPawn:GetThrottleInput` and
  `/Script/Voyage.VoyageVehicleForkliftPawn:GetSteeringInput`.
- The game writes exact digital commands `-1`, `0`, and `1` into
  `ThrottleInput` and `SteeringInput`. The mod reads those exact values as
  direction events, integrates its own state on a fixed 50 ms clock, and
  writes the integrated value back before the native getter reads it. This
  path drives both the vehicle and the native HUD.
- Never let a mod-produced active input equal an exact command marker.
  `0.9999` is the positive/negative limit and renders as `100%`; `0.0001` is
  the active neutral sentinel and renders as `0%`. Exact `0` is reserved for
  the game's release command and for shutting down an exited vehicle.
- `X` resets throttle immediately and `C` resets steering immediately. Both
  reset their direction/ramp or velocity state as well as the input field.
- On acquisition, initialize both mod states to `0.0001`. On exit, write an
  honest `0` to both input fields and clear the cached state.
- UE4SS may return different Lua wrappers for the same UObject on consecutive
  `context:get()` calls. Never use Lua wrapper identity (`==`) as persistent
  actor identity. A direct `IsPlayerControlled()` check is the verified hot
  path filter; `GetFullName()` may be used only on cold acquire/release paths.
- Known accepted limitation: writing `SteeringInput = 0` on exit stops the
  mod's steering input but does not visually recenter already turned wheels.
  The forklift appears to retain wheel position elsewhere. Do not broaden the
  control mod to chase this unless the user explicitly requests it.

## Verified DonkLift hotkey HUD contract

- The visible lower-left `E/H` hints are children of
  `BP_DynamicPlayerInputHorizontalWidget_Bottom.ContextInputActionsRoot`.
  `BP_VoyageIngameForklift_C.KeybindRoot` is real but its nested action
  container is empty at runtime; do not use it as the visible hint host.
- Create display-only hints by copying the native `WBP_InteractIndicator_C`
  class through `WidgetBlueprintLibrary:Create`, adding it to the bottom
  `ContextInputActionsRoot`, then configuring its nested text blocks on a later
  poll. The named child widgets are not ready immediately after `Create`.
- Set `ButtonInfoContainer.bAutoUpdateKeyRebindings = false` on copied hints or
  a later native refresh may restore the template's `E / Interact` content.
- Creating new `FText` requires the version-pinned custom signature at
  `ue4ss/UE4SS_Signatures/FText_Constructor.lua`. Its pattern is unique for the
  validated Voyage executable. Revalidate uniqueness and constructor semantics
  after every executable update; never install an approximate or multi-match
  signature.
- Voyage does not keep its menu language synchronized with Unreal's
  `KismetInternationalizationLibrary.GetCurrentLanguage`. Read the live
  `VoyageGameUserSettings.CustomSettings.LanguageType` enum instead. Cache it
  only while a valid forklift HUD exists, clear the cache when that HUD is
  destroyed, and read it again after the next game load.
- `EVoyageLanguageType` values verified for the current labels are `English=1`
  and `Russian=11`. The production hint labels are `Brake / Center` and
  `Тормоз / Выровнять`; unsupported languages fall back to English.
- Ship control and its `X/C` hints as the single `DonkLiftKeyboardControl` mod.
  Keep their internal loops and state independent: the HUD section must never
  write throttle or steering state.
- Separate temporary mods are acceptable while isolating a risky experiment,
  but merge a validated feature back into the owning user-facing mod when it
  has no standalone purpose. Do not leave probe/diagnostic mods in `mods/` or
  in the installed game after the experiment ends; keep reusable inspectors
  under `tools/` instead.

## Performance rules

- Lua is interpreted and both native getter hooks may run several times per
  frame. Keep their steady-state paths free of `pcall`, closures, logging,
  object searches, string construction, `GetFullName()`, and redundant helper
  calls.
- Put integration, ramping, clamping, lifecycle validation, and other work that
  does not require getter timing in the fixed 50 ms loop.
- Optimize measured or structurally hot work, but do not assume UObject wrapper
  identity or remove a defensive boundary without a real-game test. Preserve a
  known-good Git checkpoint before every optimization experiment.
- Production logging should be limited to one-time initialization failures.
  Temporary diagnostic logging must be removed after the experiment.

## Change and validation workflow

- Confirm that the game process is closed before replacing the installed Lua
  file. Never hot-copy this mod into a running game.
- Before testing the HUD section, ensure the repository and installed
  `DonkLiftKeyboardControl` Lua hashes match and the installed
  `FText_Constructor.lua` matches the repository signature file.
- Keep control-path and HUD experiments separate. Do not change both in one
  experiment because feedback between them previously caused autonomous
  throttle/steering and crashes.
- Before committing gameplay behavior, require real-game validation of:
  throttle, steering, both directions, limits, immediate `X`, immediate `C`,
  native HUD values, exit, and re-entry. A successful load or clean log is not
  sufficient.
- Commit each validated checkpoint before starting the next experiment. If an
  experiment removes control, causes autonomous movement, or crashes, return
  to the latest checkpoint instead of layering speculative fixes on it.
- Preserve unrelated worktree changes and stage only the exact files belonging
  to the validated change.
