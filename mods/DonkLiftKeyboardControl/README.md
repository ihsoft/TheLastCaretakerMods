# DonkLiftKeyboardControl Unreal project

This editor-only Unreal Engine 5.8 project generates the assets for the
self-contained `DonkLiftKeyboardControl` IoStore mod. The shipped mod does not
require UE4SS, DML, or any other loader.

The project mirrors only the native Voyage types needed to compile the
Blueprints. Those C++ stand-ins are editor inputs and must never be included in
the shipped container; the running game supplies the real native classes.

## Provenance

The project scaffold (`Voyage.uproject`, `Config`, targets, and module setup)
is maintained for the installed Unreal Engine 5.8.2 editor. The
game-API mirror headers are reconstructed inputs, not copied engine headers;
each carries the Steam build, executable SHA-256, and current reproduction
tools in its file header. Revalidate every mirror after the game executable
changes.

## Versioning and compatibility

`VERSION.json` is the source of truth for the player-facing mod version and
tested game versions. `v1` and `v2` identify releases of this mod; they are not
aliases for game versions. The current release is `v2`, game-validated on The
Last Caretaker `5.5`, Steam build `25056839`, running game UE `5.8.1`.

A tested game version is evidence, not an allowlist or a hard runtime check. A
future game update may remain compatible and does not automatically require a
new mod version. The exact Steam build, executable hash, and UE version remain
hard build-time provenance gates for reconstructed headers, extracted assets,
and cooked-package production. `v1` is known to be incompatible with game
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
.\Build-DonkLiftRelease.ps1
```

The default artifact label is the current mod version from `VERSION.json`.
`-Version v2-test` may be used for a development artifact without changing the
underlying mod version recorded separately in the release manifest.

It validates the game/engine fingerprint and clean Git source, builds the
editor modules incrementally, regenerates assets, cooks, extracts the original
forklift, builds and verifies the IoStore container, and creates a player-facing
ZIP plus `release-manifest.json` below ignored `artifacts/releases/`.

Useful explicit options:

- `-Install` backs up and installs the resulting three-file container after
  confirming that Voyage is closed, then reads every installed hash back. It
  also keeps the exact release archive in the Paks directory as
  `DonkLiftKeyboardControl_<artifact-version>.zip` and verifies its hash;
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
