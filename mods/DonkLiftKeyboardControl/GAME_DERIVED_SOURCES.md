# Game-derived source and build inputs

This file is the update gate for anything whose correctness depends on a
specific The Last Caretaker build, whether it was extracted automatically or
reconstructed by hand.

## Build-bound game identity

This Steam build ID binds the sources and build inputs used to produce the current
package. It is not a runtime compatibility allowlist: a different game build
requires review before rebuilding or claiming support, but may still run an
existing package correctly. Player-facing mod and tested-game versions live in
`VERSION.json`.

- Steam build: `25056839`
- Unreal Engine: `5.8.1`

Run `tools/Get-VoyageBuildFingerprint.ps1` before generation or packaging. If
the current Steam build ID differs, do not reuse this checklist as proof of
compatibility: re-extract the build inputs and revalidate every source contract
below against the new version.

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
replacement child's parent and native action owner. DonkLift `v2` supports
input integration, native percentage display, X/C hints, pause, exit and
re-entry on game version `5.5`. The older `v1` package is incompatible with
that version because it targets the previous UE `5.7.4` generation.

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
