# BoatHUDTotalResources active backlog

## Goal

Make the Boat's lower-right HUD fuel and electricity values show the total
Diesel and Electricity stored by all modules in the same ship grid. Petrol is a
different resource and must never be included. Socket cards intentionally stay
local; only the compact possessed-Boat HUD needs replacement.

## Validated architecture

The stock Boat HUD remains the lifecycle and layout owner. Its inherited
`PetrolTB` name is historical and does not identify the resource being shown.
A narrow stock-path child collapses each native-updated resource text in
`PreConstruct` and inserts a separate text-only direct child of `UUserWidget`
into the corresponding existing panel. Repeating aggregation stays in the
separate leaves, avoiding an unsafe Tick/CDO delta in the unversioned Voyage
HUD inheritance chain.

Each leaf resolves the possessed Boat's `VoyageModuleComponent`, asks
`VoyageModuleSubsystem` for all modules in the same grid, and sums only
`GetResourceAmount` for its explicit resource: Diesel for the fuel row and
Electricity for the battery row. Petrol is excluded. Electricity is converted
from the module's base amount to one decimal kWh, matching the stock row.

The generated fields must copy `Font`, `ColorAndOpacity`, and
`TextTransformPolicy` from the corresponding stock `PetrolTB` or `BatteryTB`
before insertion. On build `25056839`, both stock fields use size `10`,
`VoyageRoboto` Regular, and specified color `(0.5, 0.5, 0.5, 1.0)`. The old
custom Diesel leaf inherited UMG's Roboto Bold/white defaults and changed only
the size, explaining its visibly bright bold style.

The design was game-validated on Steam build `23962331`, where the compact HUD
changed from the main-tank-only `1911 L` to the confirmed total `18966 L`, and
again on build `24990438` / game UE `5.8.1`. The latter test also confirmed that
Petrol stored aboard the ship does not enter the total.

The current tracked game fingerprint is Steam build `25056839`, executable
SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`,
game UE `5.8.1`. Its accepted mapping is registered under
`mappings/Voyage/steam-25056839-ue5.8.1/`. On 2026-09-02 the user confirmed
that the two-resource current-build candidate works in the real game. The same
graphs were then rebuilt under the final `BoatHUDTotalResources` module,
package, widget, and container identities. That exact renamed container passed
the build, cook, four-asset inventory, and `retoc verify` gates, then loaded and
worked in the real game. This is the current release checkpoint.

## Rejected architecture

- Repeating logic bound directly to inherited Boat-HUD Tick crashed while
  loading the generated child CDO because of an unversioned native-ancestor
  property-index shift. Keep repeating work in the direct-`UUserWidget` leaf.
- A one-shot write to `PetrolTB` is ineffective because stock native logic
  restores the litre string. The live child must collapse the stock text and
  own a separate display leaf.
- `PetrolTB` is not evidence that Petrol should be aggregated. The exact Diesel
  enum identity is the resource contract.

## Completed validation

- The exact renamed `BoatHUDTotalResources_P` container loaded a real save and
  preserved the accepted two-resource HUD behavior. No current validation item
  remains open.

Completed UE 5.8 migration, toolchain repair, changed-save canary, and Diesel
socket investigations were deliberately removed from this active backlog.
Their durable conclusions live in `docs/voyage-cooked-asset-toolchain.md`,
`docs/boat-resource-socket-architecture.md`, and `docs/research-pitfalls.md`.

## Closed socket branch

The socket investigation established that every surveyed scan card reads its
local `ModuleOwner`; Electricity is not a special network aggregate. Preserve
that behavior. The rejected Actor-child, socket-component-class redirect, and
global interaction-consumer probes remain rejected and are not part of the HUD
extension.
