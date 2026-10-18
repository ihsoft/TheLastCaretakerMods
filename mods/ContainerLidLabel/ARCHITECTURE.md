# Container Lid Label architecture

Supported fingerprint and native-mirror provenance are owned by
[GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md). The public configuration is
in [CONFIGURATION.md](CONFIGURATION.md).

## Package and presentation ownership

The mod overrides only
`/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large`.
A generated child inherits a freshly relocated stock Large Blueprint under
`/Game/Mods/ContainerLidLabel/StockOrigin/`. These are the two runtime packages;
editor-only mirrors and a replacement stock text-widget package are not shipped.

Under inherited `PlayerBox_Lid`, the child adds `LidLabelWidget` and two static
icon planes. `InitTextWidget`, `UpdateTextWidget` and `UpdateIconMaterial` call
the exact stock parent first, then update only the added components. The label
uses the stock text widget class and keeps the name visible with either icon.
Icon materials and side visibility come from the corresponding stock sides;
a sole icon retains its left/right slot. The original side label is untouched.
The stock side widget receives its inventory name through
`OnInventoryNameUpdated` and `PostLoadUpdateName`; the lid duplicate uses the
same parent update hooks rather than polling. Parent-created dynamic icon
materials are shared with the lid planes, so later material updates reach both.
The numeric count has a separate inventory binding and remains stock.

Moving the stock side widget's transform would leave its SCS attachment on
the body rather than the moving lid. The lid-owned duplicate avoids changing
the side widget's attachment or visibility policy.

Lid geometry: label center `(0, 66.4, 13.73)`, draw size `200x110`, scale
`0.12`, Voyage Roboto Bold size 12, wrapping disabled on both text blocks.
Icon centers are `(-17, 62.27709, 13.73)` and `(17, 62.27709, 13.73)`, scale
`0.05`. Count, capacity bar and lid mesh remain stock.

## Capacity and cache

`ReceiveBeginPlay` and `OnPersistentActorPostLoad(int32 Version)` call the stock
`BP_Inventory_PlayerBox` implementation first, then `ApplyConfiguredCapacity`.
Do not bind directly to Actor BeginPlay or ContainerBase PostLoad: that bypasses
PlayerBox initialization. The inventory member belongs to
`BP_Inventory_Container_Base_WeightLimited`.

`VoyageMiscBlueprintFunctionLibrary.GetClassDefaultObject(Class)` returns the
exact generated Large CDO. Its transient loaded flag and configured float are
shared by all instances. `EnsureCachedSettings` reads the INI once per loaded
CDO using `VoyageEditorBlueprintFunctionLibrary.LoadFileToArray(FString)`, which
returns `TArray<FString>` in the shipping game. Missing/invalid input is cached
as 200 kg. Parsing has no world, latent, inventory or presentation side effects.
Class unload/reload may reread; the documented user workflow is game restart.

The per-instance apply path calls
`VoyageInventoryWeightLimitedComponent.SetMaxWeightLimit(float NewDefaultMax)`
on the existing inventory, then sets `ResourceMeter` material scalar
`ProgressLevel` from `GetPercentWeightUsed()`. No inventory recreation or stack
trimming occurs. The value is a mass limit, not an item-slot count. Validation
checks positivity after conversion to float as well as before conversion.

The mod adds no Tick event, timer or recurring object search. Its added
VoyageWidgetComponent inherits engine widget servicing; absence of mod Tick
does not establish zero extra per-frame rendering work.
