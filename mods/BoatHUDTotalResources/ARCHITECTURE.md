# BoatHUDTotalResources architecture

The mod replaces the Boat HUD's two displayed resource totals while leaving
the stock HUD structure in place. A relocated stock Boat HUD parent supplies
the original behavior. Its generated child adds a separate `UUserWidget`
whose native tick aggregates modules in the possessed Boat's power grid.

The widget sums `Diesel` for the stock `PetrolTB` display and `Electricity`
for `BatteryTB`; the field names do not change the resource identities. It
copies each stock field's presentation style during `PreConstruct`. The
generated package and version-bound source identities are described in
[GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md); the public build and install
path is in [README.md](README.md). The [mod rules](MOD-RULES.md) own constraints
for changing these components.
