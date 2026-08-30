# Game-derived source and build inputs

This file is the update gate for anything whose correctness depends on a
specific The Last Caretaker build, whether it was extracted automatically or
reconstructed by hand.

## Validated game fingerprint

- Steam build: `23962331`
- Unreal Engine: `5.7.4`
- `VoyageSteam-Win64-Shipping.exe` SHA-256:
  `6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`

Run `tools/Get-VoyageBuildFingerprint.ps1` before generation or packaging. If
the current fingerprint differs, do not reuse this checklist as proof of
compatibility: re-extract the build inputs and revalidate every source contract
below against the new version.

## Version-bound files kept in Git

The following hand-reconstructed mirror headers carry their own provenance:

- `Source/Voyage/PlayerInputInterfaceAction.h`
- `Source/Voyage/VoyageVehiclePawn.h`
- `Source/Voyage/VoyageVehicleForkliftPawn.h`
- `Source/Voyage/VoyageInputAction.h`
- `Source/Voyage/VoyageGameUserSettings.h`
- `Source/Voyage/VoyageIngameForkliftWidget.h`
- `Source/Voyage/VoyageDynamicPlayerInputWidget.h`
- `Source/Voyage/VoyageButtonInfoContainerWidget.h`
- `Source/Voyage/InteractIndicator.h`

The generator and builder implementations are ours, but their contracts must
also be reviewed after an update:

- `Source/DonkLiftGenerator/GenerateDonkLiftModCommandlet.cpp`: original
  forklift keyboard mappings, action paths, native input-field behavior.
- `Source/DonkLiftGenerator/GenerateDonkLiftInheritanceCommandlet.cpp`:
  Blueprint parent/function identity, language enum, action struct fields.
- `Source/DonkLiftGenerator/GenerateDonkLiftHudCommandlet.cpp`: HUD parent,
  widget field names, action-widget identity chain and lifecycle timing.
- `Build-InheritancePackage.ps1`: package paths, equal-length relocation
  assumptions, staging layout, and IoStore packaging contract.

## Version-bound inputs deliberately not kept in Git

Recreate these below ignored `artifacts/` paths for every game build:

- original `/Game/Blueprints/Vehicles/BP_Forklift_Possesable` package;
- original `/Game/UI/Game/HUD/BP_VoyageIngameForklift` package;
- current `scriptobjects.bin`;
- reflection mappings (`.usmap`/`.jmap`), extracted assets, JSON, pseudocode,
  disassembly reports, cooked assets, staging trees, package inventories,
  installed backups, and final `.pak/.ucas/.utoc` files.

Use `tools/Extract-VoyagePackage.ps1`, `tools/Inspect-VoyageAsset.ps1`, and
`tools/VoyageExecutableInspector` to reproduce the evidence. Promote stable
results to `docs/`; never solve an update by committing a fresh extracted
snapshot.
