# Game-derived source and build inputs

This file records source contracts whose correctness depends on a specific
The Last Caretaker build, whether extracted automatically or reconstructed by
hand. The current candidate pin is in `game-build.json`.

## Build-bound game identity

The pin in `game-build.json` binds the exact sources and build inputs; it is
not a runtime compatibility allowlist. A later Steam build may still run the
existing package. The published Nexus version lives in `version.txt`;
compatibility evidence is in `VERSION.json`. V3 is game-validated on Update 6;
V2 remains the Update 5.5 release.

Run the public producer first. On a pinned-ID mismatch, retry with
`-IgnorePinnedGameVersion`. It re-extracts current inputs and checks them during
the build. Use this source inventory for focused diagnosis if a build or game
test fails; historical conclusions here do not by themselves prove the new build.

## Game asset contracts

The reviewed mapping
`mappings/Voyage/steam-25056839-ue5.8.1/Voyage-25056839.usmap` confirms that:

- `VoyageVehicleForkliftPawn : VoyageVehiclePawn` is unchanged;
- `SteeringInput` and `ThrottleInput` remain `float` properties of
  `VoyageVehicleForkliftPawn`;
- `VoyageInputAction : InputAction` still owns `Description : FText`;
- `VoyageGameUserSettings.CustomSettings.LanguageType` still resolves through
  `EVoyageLanguageType`, with `English = 1` and `Russian = 11`;
- `EPlayerInputInterfaceActionType` remains `Central = 0`, `Context = 1`, and
  `Hidden = 2`;
- the first seventeen fields of `FPlayerInputInterfaceAction`, which are the
  fields visible to the generated DonkLift graph, retain their names, types,
  and order. Six native delegate fields now follow them and are intentionally
  omitted from the minimal editor mirror because the mod neither binds nor
  serializes them.

A current stock export of `BP_Forklift_Possesable` confirms the same native
parent and the same forward/backward/left/right action identities. A current
stock export of `IMC_Forklift_Keyboard` confirms the same fifteen mappings,
including W/S/A/D, E, T, mouse axes/buttons, H, Space, R, and F. Static
registration evidence in the current executable retains
`GetProvidedActionsBP` and the forklift throttle/steering getter and setter
identities.

The installed UE 5.8.2 editor is the build tool for the UE 5.8.1 game. Cooking
must use `-SkipZenStore` to preserve loose packages. The canonical patched
retoc at `.tools/bin/retoc.exe` still uses its explicit `UE5_7` compatibility
profile for the matching current IoStore format; extraction and packaging
must bind to the selected Steam build ID recorded in their manifests.

The builder freshly extracts the original forklift package and `scriptobjects.bin`,
checks exact relocation occurrences, and verifies the six-asset container and
replacement child's parent and native action owner. DonkLift supports input
integration, native percentage display, X/C hints, pause, exit and re-entry.
The older `v1` package is incompatible with Update 5.5 because it targets the
previous UE `5.7.4` generation.

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
  assumptions, staging layout, and IoStore packaging contract.

## Version-bound inputs deliberately not kept in Git

Recreate these below ignored `artifacts/` paths for every game build:

- original `/Game/Blueprints/Vehicles/BP_Forklift_Possesable` package;
- current `scriptobjects.bin`;
- reflection mappings (`.usmap`/`.jmap`), extracted assets, JSON, pseudocode,
  disassembly reports, cooked assets, staging trees, package inventories,
  installed backups, and final `.pak/.ucas/.utoc` files.

Use `tools/Extract-VoyagePackage.ps1`, `tools/Inspect-VoyageAsset.ps1`, and
`tools/VoyageExecutableInspector` to reproduce the evidence. Promote stable
results to `docs/`; never solve an update by committing a fresh extracted
snapshot.
