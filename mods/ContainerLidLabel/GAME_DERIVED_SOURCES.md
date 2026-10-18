# Game-derived source contract

These asset contracts support the published V1 for Update 6 beta
(`Voyage.exe` version `5.8.2.0`). The exact accepted source-build pin is in
`game-build.json`.

Editor-only mirrors reproduce native identities confirmed by current registration
metadata and stock Blueprint calls: `LoadFileToArray(FString) -> TArray<FString>`,
`GetClassDefaultObject(Class) -> UObject`, `SetMaxWeightLimit(float NewDefaultMax)`
and `GetPercentWeightUsed()`. Blueprint summaries establish inventory ownership,
`ResourceMeter`, and the exact PlayerBox BeginPlay/PostLoad call chain. These
mirrors are build inputs; no mirror native module ships in the release.

The producer fingerprints the installed game and freshly extracts
`/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large` using the
public stock-only extractor. It relocates exactly two embedded occurrences of
the 69-byte package path to an equal-length path, preserves file length and
requires the `.uexp` payload to remain byte-identical. Extracted, relocated and
cooked files are generated output, not tracked source.

On a changed Steam build, try the public producer with
`-IgnorePinnedGameVersion`; investigate mirrors, function/property owners,
relocation assumptions and layouts if the build or game test fails. Static
compilation, container verification and cooked readback do not substitute for
runtime validation.

The release provides the documented presentation and configurable capacity,
including nondefault values. Its default remains 200 kg. Retained installation
and rollback evidence belongs to the installation artifact family.
