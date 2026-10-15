# Game-derived source contract

This generator is bound to Voyage Steam build `25929985`, Unreal Engine 5.8.2,
and `VoyageSteam-Win64-Shipping.exe` SHA-256
`608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F`.

The reviewed mapping is
`mappings/Voyage/steam-25929985-ue5.8/Voyage-25929985.usmap`, SHA-256
`4EA5D93EBF41FDE3C45C7DB938763E942179DC8EF24951A97EDB94FB1FA9E2FF`.
Current native registration metadata confirms
`VoyageEditorBlueprintFunctionLibrary.LoadFileToArray(FString)` returns
`TArray<FString>`. Current stock calls confirm
`VoyageInventoryWeightLimitedComponent.SetMaxWeightLimit(float NewDefaultMax)`
and `GetPercentWeightUsed()`. Current Blueprint summaries establish the exact
inventory member owner, `ResourceMeter`, Actor BeginPlay event, and
`BP_Container_Base.OnPersistentActorPostLoad(int32 Version)` override chain.

The release producer resolves the installed fingerprint before building and
freshly extracts
`/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large` with
the public stock-only extractor. It validates exactly two embedded occurrences
of the 69-byte source package path, replaces them with the equal-length
relocated path, preserves file length, and requires the `.uexp` payload to be
byte-identical. The extracted source, relocated bytes, cooked assets, reports,
and release outputs remain ignored under `Tmp/` or generated mod directories.

Revalidate every native mirror, parent/function owner, field owner, relocation
count, and serialized layout after any fingerprint change. Compile, cook,
container verification, and CUE4Parse readback are static gates.

The prior `v3-test` bytes built for Steam build `25880952` were user-validated
for the requested presentation: accepted icon size/positions, native-side
placement of a sole icon, and the lower name remaining visible with icons. Durable evidence,
including the reviewed build manifest, semantic report, exact code-source
snapshot and installed-file hashes, is retained under
`artifacts/installations/ContainerLidLabel/20261013-070846-v3-test-6b391a6f/`.
That evidence does not validate the new capacity behavior or the rebuilt
presentation on Steam build `25929985`; it also does not claim separate coverage
of long localized names, extreme count widths, lid animation, asynchronous icon
refresh, or save/load presentation.
