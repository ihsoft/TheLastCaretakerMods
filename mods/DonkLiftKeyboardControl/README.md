# DonkLiftKeyboardControl Unreal project

This editor-only Unreal Engine 5.7 project generates the assets for the
self-contained `DonkLiftKeyboardControl` IoStore mod. The shipped mod does not
require UE4SS, DML, or any other loader.

The project mirrors only the native Voyage types needed to compile the
Blueprints. Those C++ stand-ins are editor inputs and must never be included in
the shipped container; the running game supplies the real native classes.

## Provenance

The project scaffold (`Voyage.uproject`, `Config`, targets, and module setup)
was created for Unreal Engine 5.7.4 and is maintained as ordinary source. The
game-API mirror headers are reconstructed inputs, not copied engine headers;
each carries the Steam build, executable SHA-256, and current reproduction
tools in its file header. Revalidate every mirror after the game executable
changes.

Commandlet-generated `Content` is deliberately ignored and must be recreated
from the generators. Relocated original forklift/HUD assets, mappings,
`scriptobjects.bin`, cooked output, and final containers are current-game
artifacts and must stay under ignored `artifacts/` paths rather than Git.

The code is split by role:

- `Source/Voyage` is the minimal, version-bound game-API mirror;
- `Source/DonkLiftGenerator` is our hand-written editor-tools module containing
  the three Unreal commandlets.

See `GAME_DERIVED_SOURCES.md` before generation after any game update. A
changed executable fingerprint is a hard stop until the listed headers,
generator contracts, original assets, and packaging inputs have been
re-extracted or revalidated.

## Runtime architecture

The single `DonkLiftKeyboardControl_P` container contains two transparent
package replacements:

1. The complete original forklift is relocated from
   `/Game/Blueprints/Vehicles/BP_Forklift_Possesable` to
   `/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original`. A child Blueprint at the
   original address installs `ModActor_C` and extends the native provided-action
   list with localized X/C entries.
2. The complete original forklift HUD is relocated from
   `/Game/UI/Game/HUD/BP_VoyageIngameForklift` to
   `/Game/Mods/DonkLift/HUD_Forklift_Original`. A child Widget Blueprint at the
   original address identifies the X/C widgets by their exact input-action
   objects and appends them after the native E/H block.

The helper integrates digital throttle and steering at a fixed interval and
writes the resulting analogue values to the native input fields. X resets
throttle immediately; C resets steering immediately. The native vehicle and
HUD continue to consume those values.

## Generate

Start from an empty generated `Content` directory and run the commandlets in
this order:

```powershell
UnrealEditor-Cmd.exe Voyage.uproject -run=GenerateDonkLiftMod -unattended -nop4 -nosplash -nullrhi
UnrealEditor-Cmd.exe Voyage.uproject -run=GenerateDonkLiftInheritance -unattended -nop4 -nosplash -nullrhi
UnrealEditor-Cmd.exe Voyage.uproject -run=GenerateDonkLiftHud -unattended -nop4 -nosplash -nullrhi
```

`GenerateDonkLiftMod` creates the helper, X/C input actions, and the complete
forklift keyboard mapping context. `GenerateDonkLiftInheritance` creates the
temporary forklift parent and child. `GenerateDonkLiftHud` creates the
temporary HUD parent and child.

## Cook and package

Do not use a broad `CookDir` cook: Unreal follows editor dependencies and may
compile global shaders and unrelated Engine template maps. Cook exactly the six
production packages into a new staging root:

```powershell
.\Cook-DonkLiftAssets.ps1 `
  -UnrealEditor <UE-5.7>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe `
  -OutputRoot <new-cooked-staging-directory>
```

Then build the single container from that staging root:

```powershell
.\Build-InheritancePackage.ps1 `
  -CookedRoot <new-cooked-staging-directory> `
  -OriginalForkliftDirectory <current-game-original-forklift-directory> `
  -OriginalHudDirectory <current-game-original-forklift-hud-directory> `
  -ScriptObjects <current-game-scriptobjects.bin> `
  -OutputRoot <new-output-directory>
```

The two original directories and `scriptobjects.bin` must be freshly extracted
from the current game build. The builder rejects unexpected embedded package
path counts, relocates both originals by equal-length package-name
substitution, stages only the required cooked assets, creates
`DonkLiftKeyboardControl_P.{pak,ucas,utoc}`, and runs `retoc verify`.

Generated `Content`, `Binaries`, `Intermediate`, `Saved`, cooked files,
extracted game assets, and package output are intentionally ignored by Git.
The generators, builder, extraction tools, and verified architecture notes are
the reproducible source of truth.
