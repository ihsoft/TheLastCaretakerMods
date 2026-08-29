# DonkLift Blueprint experiment

This is an editor-only dummy project for authoring an autonomous IoStore mod
for The Last Caretaker. Use the same Unreal Engine version as the game (5.7).

The game class `VoyageVehicleForkliftPawn` is represented by a deliberately
minimal C++ stand-in. It exists only so Blueprint bytecode can reference the
real runtime class and its `ThrottleInput`/`SteeringInput` properties.

The dummy native class must never be included in the shipped container.

## Validated autonomous loading concept

The game-validated package contains three Blueprint packages:

- the complete original forklift copied to
  `/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original`;
- a small child Blueprint at the original game address
  `/Game/Blueprints/Vehicles/BP_Forklift_Possesable`;
- the control helper at
  `/Game/Mods/DonkLiftKeyboardControl/ModActor`.

The child inherits the renamed original and adds a `ChildActorComponent` whose
class is `ModActor_C`. It does not override the original `BeginPlay` or `Tick`.
Existing game references still request the original package address and
therefore receive the child, while all native forklift behavior remains in the
renamed parent. This loads the helper without DML or UE4SS.

`GenerateDonkLiftInheritanceCommandlet` creates an editor-only placeholder
parent and the child. At packaging time `Build-InheritancePackage.ps1`
replaces that placeholder with the extracted original cooked asset. The two
embedded package-name strings are changed byte-for-byte to an equal-length
path; the original `.uexp` is not rewritten.

The concept was validated in the real game on 2026-08-28: an existing save
loaded, the forklift retained its native behavior, the helper controlled
throttle, and the native HUD displayed the integrated value. Only the three
resulting `.pak`, `.ucas`, and `.utoc` files were installed.

## Generate and cook

Generate `ModActor`, then generate the placeholder parent and child in a clean
project content tree:

```powershell
UnrealEditor-Cmd.exe Voyage.uproject -run=GenerateDonkLiftMod -unattended -nop4 -nosplash -nullrhi
UnrealEditor-Cmd.exe Voyage.uproject -run=GenerateDonkLiftInheritance -unattended -nop4 -nosplash -nullrhi
```

Cook the three physical content directories for Windows. `CookDir` expects a
filesystem path, not `/Game/...`. Then build the autonomous container:

```powershell
.\Build-InheritancePackage.ps1 `
  -OriginalForkliftDirectory <legacy-original-asset-directory> `
  -ScriptObjects <target-game-scriptobjects.bin> `
  -OutputRoot <new-output-directory>
```

The original asset directory must contain the unmodified cooked
`BP_Forklift_Possesable.uasset` and `.uexp` extracted from the current game.

## Historical DML-only package

The earlier LogicMod experiment packaged only:

`/Game/Mods/DonkLiftKeyboardControl/ModActor`

DML resolves that asset as:

`/Game/Mods/DonkLiftKeyboardControl/ModActor.ModActor_C`

It requires DML to load the otherwise unreferenced helper and is retained only
as historical research.

### Build the historical container

Generate the Blueprint with `GenerateDonkLiftModCommandlet`, then cook only the
package named above with `-Package` and `-CookSinglePackageNoRefs`. Do not use a
`/Game/...` value with `-CookDir`: that option expects a filesystem directory.

Package the cooked asset with the target game's `scriptobjects.bin`:

```powershell
.\Build-LogicModPackage.ps1 -OutputRoot <new-output-directory>
```

The script deliberately stages only `ModActor.uasset`, `ModActor.uexp`, and
`scriptobjects.bin`. The editor-only dummy module and unrelated cooked engine
assets must never be shipped.
