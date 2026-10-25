# Game-derived source and build inputs

This file records source contracts whose correctness depends on a specific
The Last Caretaker build, whether extracted automatically or reconstructed by
hand. The current candidate pin is in `game-build.json`.

## Build-bound game identity

The pin in `game-build.json` binds the exact sources and build inputs; it is
not a runtime compatibility allowlist. A later Steam build may still run the
existing package. The published Nexus version lives in `version.txt`;
source and release compatibility are in `VERSION.json`. Exact package identity
belongs to the release manifest. Use this source inventory to locate contracts
affected by a game update.

## Game asset contracts

The editor mirrors and generated graphs require these native identities:

- `VoyageVehicleForkliftPawn : VoyageVehiclePawn`;
- `SteeringInput` and `ThrottleInput` are `float` properties of
  `VoyageVehicleForkliftPawn`;
- `VoyageInputAction : InputAction` owns `Description : FText`;
- `VoyageGameUserSettings.CustomSettings.LanguageType` resolves through
  `EVoyageLanguageType`, with `English = 1` and `Russian = 11`;
- `EPlayerInputInterfaceActionType` uses `Central = 0`, `Context = 1`, and
  `Hidden = 2`;
- the first seventeen fields of `FPlayerInputInterfaceAction`, which are the
  fields visible to the generated DonkLift graph, must match their native names,
  types and order. Six native delegate fields follow them and are intentionally
  omitted from the minimal editor mirror because the mod neither binds nor
  serializes them.

The stock `BP_Forklift_Possesable` supplies the native parent and
forward/backward/left/right action identities. The generated keyboard context
must preserve the stock `IMC_Forklift_Keyboard` mappings, including W/S/A/D, E, T,
mouse axes/buttons, H, Space, R, and F. The provided-action override must bind
to `VoyageVehiclePawn.GetProvidedActionsBP`.

The installed UE 5.8.2 editor targets the current UE 5.8.2 game. Cooking
must use `-SkipZenStore` to preserve loose packages. The canonical patched
retoc at `.tools/bin/retoc.exe` uses its explicit `UE5_7` compatibility
profile for the matching current IoStore format. Source extraction uses `UE5_8`
to match the canonical UAssetAPI writer's filtered-import layout. These separate
source and target profiles and the selected Steam build ID are recorded in the
build manifest.

## Version-bound files kept in Git

The following hand-reconstructed mirror headers carry their own provenance:

- `Source/Voyage/PlayerInputInterfaceAction.h`
- `Source/Voyage/VoyageVehiclePawn.h`
- `Source/Voyage/VoyageVehicleForkliftPawn.h`
- `Source/Voyage/VoyageInputAction.h`
- `Source/Voyage/VoyageGameUserSettings.h`

The generator and builder implementations are ours, but their contracts must
also be reviewed after an update:

- `Source/DonkLiftGenerator/GenerateDonkLiftModCommandlet.cpp`: original
  forklift keyboard mappings, action paths, native input-field behavior.
- `Source/DonkLiftGenerator/GenerateDonkLiftInheritanceCommandlet.cpp`:
  Blueprint parent/function identity, language enum, action struct fields.
- `Build-InheritancePackage.ps1`: package paths, equal-length relocation
  assumptions for the temporary donor parent, staging layout, and IoStore contract.
- `Tools/DonkLiftPackageAssembler/Program.cs`: canonical stock schema/export
  preservation, function reference remapping and loader dependency phases.
- `Test-DonkLiftForkliftSchema.ps1`: packaged class and unchanged-export comparison.

## Version-bound inputs deliberately not kept in Git

Recreate these below ignored `Tmp/DonkLiftKeyboardControl/` for every game build;
retain only selected game-tested release archives through the release workflow:

- original `/Game/Blueprints/Vehicles/BP_Forklift_Possesable` package;
- current `scriptobjects.bin`;
- reflection mappings (`.usmap`/`.jmap`), extracted assets, JSON, pseudocode,
  disassembly reports, cooked assets, staging trees, package inventories,
  installed backups, and final `.pak/.ucas/.utoc` files.

Use `tools/Extract-VoyagePackage.ps1`, `tools/Inspect-VoyageAsset.ps1`, and
`tools/VoyageExecutableInspector` to reproduce the evidence. Promote stable
results to `docs/`; never solve an update by committing a fresh extracted
snapshot.
