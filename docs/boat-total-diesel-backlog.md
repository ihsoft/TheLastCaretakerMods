# BoatTotalDiesel active backlog

## Goal

Make the Boat's lower-right HUD fuel value show the total Diesel stored by all
modules in the same ship grid. Petrol is a different resource and must never be
included. The wall and console fuel widgets already provide the intended total;
only the compact possessed-Boat HUD needs replacement.

## Validated architecture

The stock Boat HUD remains the lifecycle and layout owner. Its inherited
`PetrolTB` name is historical and does not identify the resource being shown.
A narrow stock-path child collapses that native-updated text in `PreConstruct`
and inserts a separate text-only direct child of `UUserWidget` into the same
panel. Repeating aggregation stays in the separate leaf, avoiding an unsafe
Tick/CDO delta in the unversioned Voyage HUD inheritance chain.

The leaf resolves the possessed Boat's `VoyageModuleComponent`, asks
`VoyageModuleSubsystem` for all modules in the same grid, and sums only
`GetResourceAmount(EModuleResourceType::Diesel)`. Petrol is excluded.

The design was game-validated on Steam build `23962331`, where the compact HUD
changed from the main-tank-only `1911 L` to the confirmed total `18966 L`, and
again on build `24990438` / game UE `5.8.1`. The latter test also confirmed that
Petrol stored aboard the ship does not enter the total.

The current tracked game fingerprint is Steam build `25056839`, executable
SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`,
game UE `5.8.1`. Its accepted mapping is registered under
`mappings/Voyage/steam-25056839-ue5.8.1/`. The feature has not received a new
explicit behavior sign-off for this fingerprint, so the earlier cooked package
must not be called current-release validated merely because the game starts.

## Rejected architecture

- Repeating logic bound directly to inherited Boat-HUD Tick crashed while
  loading the generated child CDO because of an unversioned native-ancestor
  property-index shift. Keep repeating work in the direct-`UUserWidget` leaf.
- A one-shot write to `PetrolTB` is ineffective because stock native logic
  restores the litre string. The live child must collapse the stock text and
  own a separate display leaf.
- `PetrolTB` is not evidence that Petrol should be aggregated. The exact Diesel
  enum identity is the resource contract.

## Remaining validation

- On the current game fingerprint, verify the total-Diesel value in a real save.
- Leave and re-enter the Boat and confirm that exactly one correctly positioned
  value remains, with no duplicated leaf.

Completed UE 5.8 migration, toolchain repair, changed-save canary, and Diesel
socket investigations were deliberately removed from this active backlog.
Their durable conclusions live in `docs/voyage-cooked-asset-toolchain.md`,
`docs/boat-diesel-socket-architecture.md`, and `docs/research-pitfalls.md`.
