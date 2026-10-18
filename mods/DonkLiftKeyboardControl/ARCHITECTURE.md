# DonkLiftKeyboardControl architecture

This loader-free mod replaces the stock forklift package with a generated
child of a freshly relocated stock original. An owned helper attached to each
forklift integrates the native digital throttle and steering commands into
gradual control values, keeping the native movement and percentage display.
It also supplies X/C reset actions through the game's standard provided-action
HUD path. The shipped payload is one IoStore container; editor mirrors and
generator binaries are build inputs, not runtime dependencies.

W/S gradually move throttle forward or reverse, and A/D gradually steer.
Releasing these keys holds the current input. X resets throttle to neutral; C
centers steering. The HUD shows the reset hints in English or Russian according
to game language, with English fallback. There is no external settings file.
On exit the steering input clears; already turned wheels may remain visually
turned until the forklift is entered again.

The single `DonkLiftKeyboardControl_P` container replaces the stock forklift
package. The complete original is relocated to
`/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original`; a child at the original
address installs `ModActor_C` and adds localized X/C actions to the native
provided-action list. Each helper acts only on its owning forklift while it
is controlled. Loss of control clears native inputs and internal integration
state.

## Control and standard HUD

The stock game writes digital `-1`, `0`, and `1` to `ThrottleInput` and
`SteeringInput`. The helper integrates gradual values using `DeltaSeconds` and
writes the same fields, which drive both physics and the native percentage
display. Active values stop at `±0.9999`, displayed as `100%`, so they stay
distinct from the game's exact digital commands. X writes neutral throttle;
C writes neutral steering and clears steering velocity. When the forklift is
not player-controlled, the helper clears its integrated values and velocity
and writes zero to the native fields.

The child overrides `VoyageVehiclePawn.GetProvidedActionsBP`, the function's
declaring owner, to supply enabled `Central` actions for X/C. `Central` is the
lower E/H row; `Context` is the center fork row. The visible lower row is
`BP_DynamicPlayerInputHorizontalWidget_Bottom.ContextInputActionsRoot`, not
`BP_VoyageIngameForklift_C.KeybindRoot`. Native action collection uses a `TSet`,
so action order can vary, including after pause. Labels use
`VoyageGameUserSettings.CustomSettings.LanguageType` (`English=1`,
`Russian=11`), with English fallback.

## Editor-only source layout

`Source/DonkLiftGenerator` is a hand-written Unreal commandlet module.
`GenerateDonkLiftMod` creates the helper, X/C actions and replacement keyboard
mapping; `GenerateDonkLiftInheritance` builds the relocated parent and child,
installs the helper, and adds localized provided actions. Its `UCommandlet`
classes remain in the Unreal `Source` tree for Unreal Header Tool discovery.
Shared engine-level generator names live in
`../../tools/UnrealEditorGeneratorCommon/Public`.

`Source/Voyage` is an editor-only hand-reconstructed mirror of selected
`/Script/Voyage` identities. Its scaffold is authored locally; reflected
types have version-bound provenance in their headers and in
[GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md). Neither module's DLL,
C++ source, generated headers, nor editor dependencies enter the released
container; only generated cooked assets do.

The [mod rules](MOD-RULES.md) contain the few required local boundaries.
Version-bound game identities are in [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md).
