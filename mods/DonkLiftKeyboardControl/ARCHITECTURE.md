# DonkLiftKeyboardControl architecture

This loader-free mod preserves the complete stock forklift package at its
canonical path and appends two compiled functions. An owned helper attached to each
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

The single `DonkLiftKeyboardControl_P` container overrides the stock forklift
package while retaining its native superclass, fields, CDO, SCS and component
template identities. `ReceiveBeginPlay` calls the inherited Actor function,
creates a deferred `ChildActorComponent`, sets its class to `ModActor_C`, then
finishes registration. This creates the helper without adding a reflected
instance field. `GetProvidedActionsBP` supplies localized X/C actions through
the native provided-action list. Each helper acts only on its owning forklift while it
is controlled. Loss of control clears native inputs and internal integration
state.

## Streamed map compatibility

Cooked map instances use numeric unversioned property indices. Adding even one
own class field shifts inherited fields and misinterprets serialized map data.
Retaining the property schema alone is insufficient: map components also
import the original package's public template and CDO-subobject identities.
Moving those exports to a parent package breaks these canonical references.
The assembler therefore preserves both contracts and leaves all non-class
original export payloads byte-identical.

The mod-owned `Tools/DonkLiftPackageAssembler` uses the published UAssetAPI to
append two function exports, remap their names and object references, and update
the class's `Children`, `FuncMap` and dependency lists. Function creation precedes
class serialization; owner-class creation precedes function creation. No
reciprocal serialization dependency or UberGraphFrame is introduced. Stock and
donor use separate mutable mapping contexts because their class names coincide.
The producer checks the packaged class against stock through independent asset
inspection and verifies the exact five-package container inventory.

## Control and standard HUD

The stock game writes digital `-1`, `0`, and `1` to `ThrottleInput` and
`SteeringInput`. The helper integrates gradual values using `DeltaSeconds` and
writes the same fields, which drive both physics and the native percentage
display. Active values stop at `±0.9999`, displayed as `100%`, so they stay
distinct from the game's exact digital commands. X writes neutral throttle;
C writes neutral steering and clears steering velocity. When the forklift is
not player-controlled, the helper clears its integrated values and velocity
and writes zero to the native fields.

The appended override targets `VoyageVehiclePawn.GetProvidedActionsBP`, the function's
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
mapping; `GenerateDonkLiftInheritance` compiles a donor child with the two
standalone functions and no own instance fields. A temporary relocated original
supplies donor parent context during assembly; neither that parent nor the donor
class replaces the stock class in the shipped package. Its `UCommandlet`
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
