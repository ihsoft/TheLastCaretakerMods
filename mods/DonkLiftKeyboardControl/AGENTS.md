# DonkLiftKeyboardControl rules

These rules apply to the autonomous IoStore mod in this directory. Repository
rules in `../../AGENTS.md` also apply; this file owns every DonkLift-specific
contract and workflow detail.

## Sources of truth

- Treat this Unreal project, `GAME_DERIVED_SOURCES.md`, and the latest
  game-validated Git checkpoint as the implementation of record.
- Read `../../docs/donklift-autonomous-hud-backlog.md` after every context
  compaction and before changing source or installed files. It records whether
  the currently installed candidate is validated or still awaiting a test.
- `README.md` documents generation and packaging. `README.txt` is the
  user-facing mod README.
- Detailed evidence and historical failures live in
  `../../docs/game-architecture-observations.md` and
  `../../docs/research-pitfalls.md`. Do not restore an older control or HUD design
  when it disagrees with the current source without a new isolated experiment.
- `../DonkLiftKeyboardControlUE4SS` is a preserved historical alternative, not
  a runtime dependency, source component, or release payload of this mod.

## Game-version gate

- `GAME_DERIVED_SOURCES.md` is a hard build gate. Before generation, cook, or
  packaging, run `../../tools/Get-VoyageBuildFingerprint.ps1` and compare the
  Steam build ID and executable SHA-256.
- A fingerprint mismatch invalidates all hand-reconstructed Voyage headers,
  embedded generator contracts, action/mapping assumptions, package paths,
  relocation assertions, original Blueprint snapshots, mappings, and
  `scriptobjects.bin` until each is freshly extracted or revalidated.
- Every version-bound source file retained in Git must carry its own provenance
  header. Keep the complete inventory and current fingerprint synchronized in
  `GAME_DERIVED_SOURCES.md`.
- Never commit extracted/cooked originals, mappings, `scriptobjects.bin`,
  generated `Content`, staging trees, containers, or installed backups. They
  belong under ignored `artifacts/` paths.

## Source layout

- `Source/Voyage` is the minimal hand-reconstructed `/Script/Voyage` mirror.
  It exists only so cooked Blueprints bind to the game's native identities.
  Never stage its DLL or other native output into the mod container.
- `Source/DonkLiftGenerator` is our hand-written editor-only tools module. Its
  two `UCommandlet` classes must remain in an Unreal `Source` module so UHT
  and UnrealEditor can discover them; their native output is never shipped.
- `GenerateDonkLiftMod` creates the helper, X/C actions, and replacement
  forklift keyboard mapping context.
- `GenerateDonkLiftInheritance` creates the forklift placeholder/child,
  installs the helper, and supplies localized standard actions.
- Generated `Content`, `Binaries`, `Intermediate`, `Saved`, caches, and IDE
  files are owned by this directory's `.gitignore`.

## Runtime package architecture

- Ship exactly one container:
  `DonkLiftKeyboardControl_P.{pak,ucas,utoc}`. UE4SS and DML are not required.
- Freshly extract the complete original forklift and relocate it from
  `/Game/Blueprints/Vehicles/BP_Forklift_Possesable` to the equal-length
  `/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original`. The generated child at
  the original path installs the helper and extends standard actions.
- Equal-length substitution and the expected package-name occurrence counts
  are safety assertions. Never weaken them to accept an updated or approximate
  binary layout.
- The relocated original is a snapshot. A previously built mod does not
  inherit later developer changes to the forklift.

## Verified input contract

- Each helper targets only its exact owning forklift through the child actor's
  `GetParentActor()`. Integrate input only while that pawn reports
  `IsPlayerControlled()`; never return to the old global `GetPlayerPawn(0)`
  target, which loses the exited forklift and lets helpers from multiple
  instances write to the same active pawn.
- The game writes exact digital commands `-1`, `0`, and `1` to `ThrottleInput`
  and `SteeringInput`. The helper integrates analogue state using
  `DeltaSeconds` and writes it back to the same fields, driving both physics
  and the native percentage display.
- Never let a mod-produced active value equal an exact digital marker.
  `±0.9999` is the limit and renders as `100%`; exact `0` is neutral. The
  historical Lua-only `0.0001` wrapper sentinel is not needed here.
- Throttle changes by one third of the full range per second. Steering uses
  maximum speed `1.20`, acceleration `2.50`, and reversal braking `5.00`.
- `X` immediately writes neutral throttle. `C` immediately writes neutral
  steering and clears steering velocity. Do not add duplicate action-event
  handlers or diagnostic state for these keys; the helper's player-controller
  key checks are the behavior path.
- While the owning forklift is not player-controlled, write honest `0` to its
  native throttle and steering fields and clear integrated throttle,
  integrated steering, and steering velocity. This is the verified exit and
  parked-instance reset path.
- Accepted limitation: leaving the forklift stops active steering input but
  does not visually recenter wheels already turned. Do not broaden the mod to
  chase retained wheel pose unless the user explicitly requests it.

## Verified standard-HUD contract

- `EPlayerInputInterfaceActionType` is exactly `Central=0`, `Context=1`, and
  `Hidden=2`. The lower E/H row uses `Central`; the center fork row uses
  `Context`. Never restore the nonexistent editor-only `Action` enumerator.
- The visible lower row is
  `BP_DynamicPlayerInputHorizontalWidget_Bottom.ContextInputActionsRoot`.
  `BP_VoyageIngameForklift_C.KeybindRoot` exists but is not the live host.
- `GetProvidedActionsBP` belongs to `VoyageVehiclePawn`, not
  `VoyageVehicleForkliftPawn`. The child must override the real base-owned
  UFunction and return enabled `Central` structs for the X/C actions.
- The native path deduplicates complete actions through a `TSet`; returned
  array order and equal priorities do not determine visual order. The game may
  repopulate the row after ESC/resume and choose a different `TSet` order.
- Accept the native action order. Do not replace the forklift HUD, poll its
  children, or reorder widgets solely for presentation.
- Read language from `VoyageGameUserSettings.CustomSettings.LanguageType`, not
  `KismetInternationalizationLibrary.GetCurrentLanguage`. Verified values are
  `English=1` and `Russian=11`; labels are `Brake / Center` and
  `Тормоз / Выровнять`, with English fallback.

## Performance and production hygiene

- Keep the helper's `ReceiveTick` graph limited to pawn cast, direction
  decoding, integration, clamping, X/C checks, and native-field writes. No
  logging, string work, object searches, or display logic belongs in the hot
  path.
- The provided-action override is a cold path; keep it independent from input
  integration.
- Production graphs must contain no `PrintString`, `QuitGame`, probe marker,
  diagnostic opacity, positional reorder, or unused event-observation state.
- Removing apparently unused Blueprint nodes can change cooked bindings. Keep
  a known-good package and require a real-game check after cleanup or
  optimization.

## Build and package workflow

- Prefer `Build-DonkLiftRelease.ps1 -Version <version>` for a complete local
  release. It owns the early gates, incremental editor build, generation, cook,
  clean-original selection, packaging, ZIP, evidence manifest, and optional
  backed-up installation. It never publishes externally.
- A real release requires clean tracked and untracked DonkLift source;
  `-AllowDirtySource` is only for development artifacts and must remain visible
  in `release-manifest.json`. Publication screenshots under `Slideshow` are not
  runtime source and are excluded from this gate.
- `-OriginalsRoot` may reuse only an explicit clean extraction whose manifest
  matches the current build/hash, canonical filter, and
  `allowAdditionalContainers=false`. The package builder repeats these checks.
- Build `VoyageEditor` with Unreal Engine 5.7.4 and `-NoUBA`.
- Run commandlets with `-ddc=NoZenLocalFallback` and a workspace-local
  `-LocalDataCachePath`; otherwise Zen can loop on an inaccessible data path.
- Generate from an empty ignored `Content` tree in this order:
  `GenerateDonkLiftMod`, `GenerateDonkLiftInheritance`.
- Use `Cook-DonkLiftAssets.ps1`. It sends five explicit package names to one
  Unreal process under `-CookSinglePackageNoRefs`. Unreal 5.7 accepts a
  `+`-separated `-Package` list and applies the narrow skip-reference options to
  the complete request. Do not replace this with broad `CookDir`, which follows
  editor dependencies into global shaders and unrelated Engine/OpenWorld assets.
- Use `Build-InheritancePackage.ps1` with a fresh original forklift directory
  and current `scriptobjects.bin`. Each path must remain under its
  canonical `Extract-VoyagePackage.ps1` root and manifest; never copy an asset
  into a manifest-less convenience directory. Require the manifest fingerprint
  and shadowing checks, relocation self-parent guard, exact packaged inventory,
  and `retoc verify` before installation.
- Prefer `Prepare-DonkLiftOriginals.ps1` over manually disabling the installed
  overrides and extracting the forklift. It owns the closed-process check,
  temporary disable of every non-base UTOC, canonical filter, and guaranteed
  name/hash restore. An interrupted leftover is a hard stop requiring recovery.

## Installation and real-game validation

- Installed test files are
  `P:\SteamLibrary\steamapps\common\Voyage\Voyage\Content\Paks\DonkLiftKeyboardControl_P.{pak,ucas,utoc}`.
- Confirm `VoyageSteam-Win64-Shipping.exe` is closed before replacing them.
  The user often continues playing while repository work happens; never alter
  the installed files until he explicitly reports that he exited.
- Back up the exact installed container and record hashes before every risky
  replacement. Repository and prepared artifact hashes must match installed
  hashes before handing off a test.
- Before committing gameplay behavior, validate in the real game: throttle and
  steering in both directions, limits, native percentages, immediate X,
  immediate C, localized X/C hints before and after pause, exit, and re-entry.
- A successful editor build, commandlet run, cook, `retoc verify`, clean log,
  load, or entry into the vehicle is not sufficient validation.
- Commit each validated checkpoint before the next experiment. On autonomous
  input, lost control, HUD corruption, or crash, restore the last checkpoint
  instead of layering another speculative fix.
