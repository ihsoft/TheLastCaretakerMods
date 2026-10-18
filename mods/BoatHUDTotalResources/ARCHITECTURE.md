# BoatHUDTotalResources architecture

The mod replaces the Boat HUD's two displayed resource totals while leaving
the stock HUD structure in place. A relocated stock Boat HUD parent supplies
the original behavior. Its generated child adds a separate `UUserWidget`
whose native tick aggregates modules in the possessed Boat's power grid.

The widget sums `Diesel` for the stock `PetrolTB` display and `Electricity`
for `BatteryTB`; the field names do not change the resource identities. It
copies each stock field's presentation style during `PreConstruct`. The
generated package and version-bound source identities are described in
[GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md). The [mod rules](MOD-RULES.md) own constraints
for changing these components.

## Runtime architecture

The stock Boat HUD remains the lifecycle and layout owner. A narrow child at
the stock package path collapses only the native-updated fuel and battery text
and inserts separate text leaves in their existing parent panels. Repeating
aggregation runs only on those leaves, whose direct parent is engine
`UUserWidget`.

Each leaf obtains the possessed Boat's `VoyageModuleComponent`, enumerates
modules in the same grid through `VoyageModuleSubsystem`, and sums its explicit
resource: Diesel or Electricity. The electricity amount is displayed as one
decimal kWh. Each generated leaf copies the corresponding stock text's font,
color, and text-transform policy before it is inserted.

This composition avoids adding Tick state to the unversioned inherited Voyage
HUD child. Shared cooked-asset contracts are documented separately in
[`../../docs/voyage-cooked-asset-toolchain-knowledge.md`](../../docs/voyage-cooked-asset-toolchain-knowledge.md).

The last pinned Steam build is in `game-build.json`; game-derived contracts
are in [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md). The builder uses a
fresh stock HUD package and the available UE 5.8.2 editor. The displayed
totals omit Petrol and preserve neighboring HUD elements.
