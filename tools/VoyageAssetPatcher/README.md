# Voyage asset patcher

Small, assertion-heavy patches for isolated cooked-asset experiments. Game
assets and generated outputs remain under ignored `artifacts/`; this directory
contains only the reproducible transformation.

Official UAssetAPI support currently stops at Unreal Engine 5.7. The default
project reference uses reviewed fork commit
`b5c47b735076585513fe31b50e81cddf353341ed` under `.tools/UAssetAPI`.
`tools/Prepare-UAssetApiVoyageUe58.ps1` can copy the exact tracked source to an
ignored directory; override the project path only when deliberately testing
that prepared copy:

```powershell
dotnet build .\tools\VoyageAssetPatcher\VoyageAssetPatcher.csproj `
  -p:UAssetApiProject=D:\src\UAssetAPI\UAssetAPI\UAssetAPI.csproj
```

Voyage's UE 5.8 packages require all three fork fixes: filtered
`FObjectImport.PackageName` layout, filtered cooked `FField` layout, and
propagating the selected engine version to dependency schemas. The preparation
script verifies and records those exact sources without altering them. A
prepared source tree is still not certified for arbitrary UE 5.8 assets: an
unchanged roundtrip, binary comparison, `retoc` conversion, and real-game
canary remain mandatory.

The `break-bottom-action-filter` operation locates the exact
`BP_DynamicPlayerInputHorizontalWidget_Bottom` export, asserts that its
`bFilterByActionType` property is `true`, changes it to `false`, writes to a
different asset, and reopens that output to verify the change. It deliberately
breaks the native E/H hint row so a real-game test can prove which serialized
widget owns the visible panel.

For Voyage 5.7.4, UAssetAPI cannot structurally deserialize this game-specific
Widget Blueprint export and exposes it as 17 raw bytes. The operation accepts
only the fully known `01044B039FFFFFFF010D00000000000000` layout and changes
byte 8 from `01` to `00`. This is the serialized boolean between the
unversioned-property header and the unchanged CanvasPanelSlot package index.
Any different game-version layout is rejected.

The `swap-forklift-horn-to-exit` operation is a reversible producer probe. It
locates the original forklift CDO, asserts that `HornInputAction` references
`IAV_VehicleHorn` and `ExitAction` references `IAV_VehicleExit`, then copies
the existing exit-action package index into the horn field. No import is added.
If the native standard HUD is built from these CDO fields, `H / Horn` should
become a duplicate `E / Exit Vehicle` hint. The output is reopened and checked
before packaging.

UAssetAPI's normal writer is used only to create two temporary, equally
reserialized files: an unchanged baseline and a Horn-to-Exit variant. Their
diff must contain exactly one four-byte package-index replacement. The tool
then applies that int at the same verified offset in the exact original
`.uexp`, copies the `.uasset` byte-for-byte, and reopens the surgical output.
This avoids shipping unrelated round-trip normalization changes.

The `swap-hud-indicator-subclass` operation targets the exact current
`BP_VoyageIngameHud` CDO. It asserts that `IndicatorSubClass` references the
stock `WBP_InteractIndicator_C`, appends a distinct package/class import for
`/Game/Mods/BoatProbe/WBP_InteractIndicator_M`, and changes only that CDO
property to the marker class. The existing stock class import remains in place
for the embedded widget-tree instance. The written package is reopened and
both identities are verified before it can be packaged. Unlike the
Horn-to-Exit operation this necessarily rewrites the package header because a
new name and two imports must be added; downstream `retoc` and independent
CUE4Parse inspection remain required gates.

The `roundtrip-unchanged` operation makes no semantic mutation. It writes and
reopens a separate copy solely to distinguish UAssetAPI writer normalization
from a requested patch. A successful reopen is not runtime compatibility;
compare it with the exact input and inspect/package it independently.

The `export-json` operation parses an exact extracted `.uasset` with the
supplied mapping and writes UAssetAPI's complete formatted JSON representation
to a separate `.json` file. It never writes the source package. Keep the JSON
under ignored `artifacts/`, because it is reproducible game-derived data.

The `set-cable-updater-tick-interval` operation is a narrow semantic probe for
the current `BP_VoyageCableUpdater` package. It requires the exact nine-byte
CDO that serializes only `PrimaryActorTick.TickGroup = TG_PostUpdateWork`, then
extends the nested `ActorTickFunction` unversioned header with property index
`6` and a `1.0`-second `TickInterval`. The operation proves that the companion
`.uexp` is an exact CDO splice: every byte before and after that export remains
unchanged. It adds no import, name, component, graph, or class. Reopen,
independent CUE4Parse decoding, `retoc verify`, and a real-game moving-cable
test remain mandatory gates.

The `break-cable-updater-super-index` operation is an intentionally invalid,
controlled-crash discriminator for the same exact package. It asserts that
`BP_VoyageCableUpdater_C` still derives from
`/Script/Voyage.VoyageCableUpdater`, then replaces only its four-byte export-map
`SuperIndex` with the impossible export index `66559` in a five-export package.
The companion `.uexp` must remain byte-identical, and the `.uasset` may differ
at exactly that one four-byte field. The real-game probe crashed immediately at
`SecondsSinceStart = 0` with an access violation rather than emitting the
predicted textual `Bad export index 66559/5`; this still proved the stock-path
package was loaded. The package was removed by its exact manifest immediately
after that result.

The `swap-hud-indicator-existing-control` operation is a non-installable
structural control. It replaces the same raw CDO index and dependency with the
already imported `BP_VoyageWeaponHolsterWidget_C`, without adding names or
imports. Independent decoding must identify that changed value as
`IndicatorSubClass`; the deliberately incompatible widget class must never be
installed in the game.

The `swap-diesel-socket-component-class` operation targets the fresh current
`BP_Module_Diesel_Container`. It asserts that the native
`VoyageModuleSocketViewComponent` import is used as the export class by exactly
the two known Diesel socket templates, appends the distinct marker-component
package import, and redirects that one existing class import to
`BP_BoatDieselSocketTotal_C`. Because every serialized use of the original
class import belongs to those two SCS components, their property types, SCS
nodes, and templates change coherently while the original actor class and SCS
ownership remain untouched. Reopen, `retoc verify`, independent CUE4Parse
inspection, and a real-game marker result remain mandatory gates.
