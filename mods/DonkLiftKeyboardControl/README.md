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
from the generators. The relocated original forklift, mappings,
`scriptobjects.bin`, cooked output, and final containers are current-game
artifacts and must stay under ignored `artifacts/` paths rather than Git.

The code is split by role:

- `Source/Voyage` is the minimal, version-bound game-API mirror;
- `Source/DonkLiftGenerator` is our hand-written editor-tools module containing
  the two Unreal commandlets.

See `GAME_DERIVED_SOURCES.md` before generation after any game update. A
changed executable fingerprint is a hard stop until the listed headers,
generator contracts, original assets, and packaging inputs have been
re-extracted or revalidated.

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
.\Build-DonkLiftRelease.ps1 -Version 1.0.0
```

It validates the game/engine fingerprint and clean Git source, builds the
editor modules incrementally, regenerates assets, cooks, extracts the original
forklift, builds and verifies the IoStore container, and creates a player-facing
ZIP plus `release-manifest.json` below ignored `artifacts/releases/`.

Useful explicit options:

- `-Install` backs up and installs the resulting three-file container after
  confirming that Voyage is closed, then reads every installed hash back;
- `-OriginalsRoot <path>` reuses an explicitly selected clean extraction only
  when its manifest proves the current build/hash, canonical filter, and no
  additional containers;
- `-AllowDirtySource` permits a development artifact from uncommitted DonkLift
  source and records that fact in the manifest. Omit it for a real release.

The script never uploads or publishes anything. The ZIP contains the three
container files and the player-facing `README.txt`; detailed logs, fingerprint,
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
`-CookSinglePackageNoRefs`, avoiding four redundant editor startups without
broadening the cook:

```powershell
.\Cook-DonkLiftAssets.ps1 `
  -UnrealEditor <UE-5.7>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe `
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
