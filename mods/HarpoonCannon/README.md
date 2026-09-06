# HarpoonCannon Unreal project

Current checkpoint: **HC02 standalone autoload probe installed; stock Drone
acquisition/hints observed in game**. Its isolated current-build source and
preparation contract are in [AutoloadProbe](AutoloadProbe/README.md). HC03
entry/exit is pending; do not enter the HC02 Drone because timed cleanup is
not occupancy-safe. No current Harpoon construction/model package is installed.

The retained root project/mirrors/probe recipes below
are historical UE 5.7 / Steam 23962331 work, not build instructions validated
for the installed Steam 25056839 / UE 5.8 game. Start with `AGENTS.md`,
`RESEARCH.md` and the active section of `../../docs/harpoon-cannon-backlog.md`.
Old present-tense checkpoint statements below describe their original tests.
No current Harpoon shell compatibility or working cannon entry/optics is claimed.

This isolated Unreal Engine 5.7 editor project is the source for a future
stationary harpoon cannon mod. Sharks are the first target family, while the
generic optical trace, distance, and HUD layers remain extensible to other
creatures. It currently generates two compile-only assets:

- `BP_HarpoonCannon`, a `VoyageVehiclePawn` child with a fixed mount,
  yaw pivot, pitch pivot, optical axis, collision probe, and 5x camera;
- `WBP_HarpoonCannon`, a first-person reticle and hidden target-distance
  readout skeleton derived from `VoyageBaseUserWidget`.

For the current cancellation experiment the generator also creates a disposable
`VoyageModuleActor` shell at the leaf Cyclone actor path
`/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New`. When
the probe container is installed, the existing Cyclone item places that shell,
whose collision-bearing root and dynamic-collision component own
`bAutoWeld=true`. The shell reproduces the native `ModuleComponent`,
`PersistentComponent`, and `DestructibleObjectComponent` default-subobject
names and overrides the inherited module component to
`VoyageCustomModuleComponent`, as the original turbines do. Its placeholder
geometry is direct shell-owned SCS content; no child actor or vehicle pawn is
referenced by this experiment. This isolates the stationary module attachment
contract while retaining the game's normal Cyclone recipe, unlock, preview,
and placement flow. The replacement remains a test adapter, not the final item
identity.

The shell-only checkpoint passed real-game preview and completed-object
attachment validation. Exact item alignment then game-validated
construction-phase attachment and completed-object dismantling. Cancellation
of an unfinished object now works after adding simple collision to the cloned
base mesh; enabling overlap generation on the mount was a game-validated
no-op. Static inspection proves that cancellation
is provided by the temporary `BP_Location_Fabricator`, which clones only mesh
components from the item actor. The current cancellation probe therefore adds
one deterministic simple-collision box to `SM_HarpoonCannonBase`; yaw/pitch
meshes, render geometry, materials, shell attachment, and item identity remain
unchanged. That exact collision-only package is installed, and the user
confirmed that `Q Cancel (Self)` now appears and
successfully cancels unfinished construction. Simple collision on the base mesh
is therefore part of the validated construction contract. The attempted move from Cyclone to
Whisper is rejected: replacing Whisper's small-turbine Blueprint also replaces
the parent package of the untouched Cyclone Blueprint and crashes its loader.
The restored leaf-level visual candidate is installed and game-validates
startup, construction-menu loading, completed construction, and assembly of
the base/yaw/pitch meshes. The cancellation overlap hypothesis failed; motion,
dismantling, persistence, and later operator logic remain pending.
Enter/exit, input, trace, shark classification, and HUD selection remain
unimplemented. See `../../docs/harpoon-cannon-backlog.md`.

The accepted mod-authored OBJ remains under `SourceAssets/`, and the generator
can reconstruct three self-contained meshes at the intended component
ownership boundaries: static mount, yaw assembly, and pitch assembly. The
first restored leaf checkpoint includes the OBJ's mod-authored connector proxy
inside the pitch mesh. Its direct reference to the game's
`/Game/AssetSets/Sockets/SM_Mooring_CableSocket_Out` is deliberately disabled
until this visual carrier loads in game; the stock connector will be a later
one-variable checkpoint.

The first five-package visual candidate is not installed and must not be
reinstalled. Despite passing build, cook, `retoc verify`, and exact-inventory
checks, it caused an async-loading assert in the original medium-turbine child:
`Bad export index 4607/19`. Restoring the preceding exact three-package Whisper
overlap package produced the identical assert, proving that the unsafe variable
is the synthetic override of the small-turbine parent package itself. The game
has therefore been restored by removing all three Harpoon container files.
Future visual work cannot use the Whisper parent path as its adapter boundary.

Material slot names from the rough model are preserved, but this checkpoint
uses Unreal's default surface material rather than claiming final texturing.
The modeled cable route is likewise only a non-functional blockout cue; no
duplicate drum or replacement cable gameplay component is added. Functional
cable work remains bound to the game's `BP_ModuleCable_Mooring` and
`VoyageCableComponent` architecture.

The `Voyage` module is an editor-only identity mirror despite its Unreal module
type. Never ship its native binary. The
`HarpoonCannonGenerator` module and commandlet are also generation
tools, not runtime payload.

## Generate

After verifying `GAME_DERIVED_SOURCES.md`, build `VoyageEditor` and run:

```powershell
UnrealEditor-Cmd.exe Voyage.uproject -run=GenerateHarpoonCannon -unattended -nop4 -nosplash -nullrhi
```

Generated `Content` and all Unreal build output are ignored. The probe cook and
package scripts build a disposable container under ignored `artifacts/`; they
do not install it into the game.

```powershell
.\Cook-HarpoonCannonPlacementProbe.ps1 `
  -UnrealEditor 'K:\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  -OutputRoot '..\..\artifacts\harpoon-cannon\placement-probe-cook'

.\Build-HarpoonCannonPlacementProbe.ps1 `
  -CookedRoot '..\..\artifacts\harpoon-cannon\placement-probe-cook' `
  -ScriptObjects '..\..\artifacts\extracted\steam-23962331-6A9AE86E5CE5\Vehicles_BP_Forklift_Possesable\scriptobjects.bin' `
  -OutputRoot '..\..\artifacts\harpoon-cannon\placement-probe-package'
```

Both scripts are fail-closed on the current game fingerprint. The current
rough-visual inventory contains the three generated visual meshes, cannon pawn,
and temporary leaf Cyclone shell. The editor-only Cyclone item stub and the
disabled stock-head reference stub must not enter the cook or inventory.
Installation remains a separate, explicitly gated step for every replacement;
the current prepared candidate passed that gate and is installed.
