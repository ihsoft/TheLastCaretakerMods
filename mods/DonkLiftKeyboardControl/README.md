# DonkLiftKeyboardControl Unreal project

DonkLiftKeyboardControl makes forklift throttle and steering gradual on a
keyboard, with quick reset controls.

This editor-only Unreal Engine 5.8 project generates the assets for the
self-contained `DonkLiftKeyboardControl` IoStore mod. The shipped mod does not
require UE4SS, DML, or any other loader.

## Controls and limitations

W/S gradually move throttle forward or reverse, and A/D gradually steer.
Releasing these keys holds the current input. X resets throttle to neutral; C
centers steering. The forklift HUD shows X / Brake and C / Center hints in
English or Russian according to the game language, with English fallback for
other languages.

The package has no external settings file. Its defaults are throttle travel of
one third of the full range per second, steering maximum speed of 1.20 per
second, acceleration of 2.50 per second squared, and reversal braking of 5.00
per second squared. On exit, the steering input clears, although already turned
wheels may remain visually turned until the forklift is entered again.

The project mirrors only the native Voyage types needed to compile the
Blueprints. Those C++ stand-ins are editor inputs and must never be included in
the shipped container; the running game supplies the real native classes.

## Provenance

The project scaffold (`Voyage.uproject`, `Config`, targets, and module setup)
is maintained for the installed Unreal Engine 5.8.2 editor. The
game-API mirror headers are reconstructed inputs, not copied engine headers;
each carries the Steam build and reconstruction method in its file header.
Investigate game-specific mirrors if a candidate build or game test fails after
a Steam build ID change.

## Versioning and compatibility

`version.txt` records the latest numbered Nexus release; `0` means there has
been no numbered `V1` release, regardless of earlier Nexus publication. `VERSION.json`
records compatibility metadata for the current build. `v1` and `v2` identify releases of this mod; they are not
aliases for game versions. The current release is `v2` for The Last Caretaker
`5.5`, Steam build `25056839`, running game UE `5.8.1`.

A supported game version is not an allowlist or a hard runtime check. A
future game update may remain compatible and does not automatically require a
new mod version. The builder compares the installed Steam build with
`game-build.json`; `-IgnorePinnedGameVersion` permits a structurally checked
candidate on a newer build. Fresh extractions still must match the installed
game. `v1` is known to be incompatible with game
`5.5`, primarily because it was cooked and mirrored for the earlier UE `5.7.4`
generation.

Commandlet-generated `Content` is deliberately ignored and must be recreated
from the generators. The relocated original forklift, mappings,
`scriptobjects.bin`, cooked output, and final containers are current-game
artifacts and must stay under ignored `artifacts/` paths rather than Git.

The code is split by role:

- `Source/Voyage` is the minimal, version-bound game-API mirror;
- `Source/DonkLiftGenerator` is our hand-written editor-tools module containing
  the two Unreal commandlets.

If a candidate fails, use `GAME_DERIVED_SOURCES.md` to investigate the
affected source contract. The normal path begins with the public builder.

## Runtime architecture

The single `DonkLiftKeyboardControl_P` container contains one transparent
package replacement. The complete original forklift is relocated from
`/Game/Blueprints/Vehicles/BP_Forklift_Possesable` to
`/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original`. A child Blueprint at the
original address installs `ModActor_C` and extends the native provided-action
list with localized X/C entries. The native HUD owns their display and order;
the mod does not replace any HUD package.

Each helper addresses only its owning forklift. While that pawn is
player-controlled it integrates digital throttle and steering and writes the
resulting analogue values to the native input fields. When control is lost it
clears both native inputs and all internal integration state, so an exited or
parked forklift cannot inherit another instance's input. X resets throttle
immediately; C resets steering immediately. The native vehicle and HUD
continue to consume those values.

## One-command release

For a normal release, use the orchestrator instead of invoking the individual
stages manually:

```powershell
.\Build-DonkLiftRelease.ps1
```

The default candidate label is the next version after `version.txt`, including
`v1-candidate` when the file contains `0`.
`-Version v3-test` may distinguish a development build without changing the
published version.
If the default run stops only at the pinned ID, retry with
`-IgnorePinnedGameVersion` and test a successful candidate in game.

It validates the game/engine fingerprint and clean Git source, builds the
editor modules incrementally, regenerates assets, cooks, extracts the original
forklift, builds and verifies the IoStore container, and creates a player-facing
ZIP plus `release-manifest.json` below ignored `Tmp/DonkLiftKeyboardControl/`.

Useful explicit options:

- `-Install` backs up and installs the resulting three-file container after
  confirming that Voyage is closed, then reads every installed hash back. It
  installs only the game-consumed container files; the ZIP stays outside the game;
- `-OriginalsRoot <path>` reuses an explicitly selected clean extraction only
  when its manifest proves the current Steam build ID, canonical filter, and no
  additional containers;
- `-AllowDirtySource` permits a development artifact from uncommitted DonkLift
  source and records that fact in the manifest. Omit it for a real release.

The script never uploads or publishes anything. The Vortex ZIP contains the
three container files at its root; detailed logs, fingerprint,
inventory, backup, and hashes stay beside it as local evidence.

The remaining commands describe the internal stages and are useful for
diagnosis.

## Generate manually

Start from an empty generated `Content` directory and run the commandlets in
this order:

```powershell
UnrealEditor-Cmd.exe Voyage.uproject -run=GenerateDonkLiftMod -unattended -nop4 -nosplash -nullrhi
UnrealEditor-Cmd.exe Voyage.uproject -run=GenerateDonkLiftInheritance -unattended -nop4 -nosplash -nullrhi
```

`GenerateDonkLiftMod` creates the helper, X/C input actions, and the complete
forklift keyboard mapping context. `GenerateDonkLiftInheritance` creates the
temporary forklift parent and child.

## Cook and package

Do not use a broad `CookDir` cook: Unreal follows editor dependencies and may
compile global shaders and unrelated Engine template maps. Cook exactly the
five production packages into a new staging root. The cook script passes the
five explicit `-Package` values to one Unreal process with
`-CookSinglePackageNoRefs -SkipZenStore`, avoiding four redundant editor
startups without broadening the cook. `-SkipZenStore` preserves the loose
`.uasset/.uexp` output consumed by the checked relocation pipeline:

```powershell
.\Cook-DonkLiftAssets.ps1 `
  -UnrealEditor <UE-5.8>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe `
  -OutputRoot <new-cooked-staging-directory>
```

Then build the single container from that staging root:

First prepare the original forklift in one new artifact root. The preparer
requires a closed game, temporarily disables every non-base `.utoc` that could
shadow the original, uses the canonical filter, and restores every container
by original name and SHA-256 even if extraction fails:

```powershell
.\Prepare-DonkLiftOriginals.ps1 `
  -OutputRoot <new-extraction-root>
```

Then pass the asset directories and `scriptobjects.bin` from those exact
extraction roots:

```powershell
.\Build-InheritancePackage.ps1 `
  -CookedRoot <new-cooked-staging-directory> `
  -OriginalForkliftDirectory <current-game-original-forklift-directory> `
  -ScriptObjects <current-game-scriptobjects.bin> `
  -OutputRoot <new-output-directory>
```

The builder locates each `extraction-manifest.json` and rejects inputs produced
with additional containers, noncanonical filters, different fingerprints, or
unrelated `scriptobjects.bin`. It also rejects a supposed original that already
references the mod's relocated-parent path, which would create a self-parent
cycle. After equal-length relocation it stages only the required cooked
assets, creates `DonkLiftKeyboardControl_P.{pak,ucas,utoc}`, runs
`retoc verify`, and verifies the exact six-asset production inventory. It
also writes a sorted `.inventory.txt` sidecar for semantic build comparison;
the sidecar is evidence, not part of the three-file installed payload.

Generated `Content`, `Binaries`, `Intermediate`, `Saved`, cooked files,
extracted game assets, and package output are intentionally ignored by Git.
The generators, builder, extraction tools, and verified architecture notes are
the reproducible source of truth.
