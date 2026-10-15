# Game-derived source contract

Supported identities:

- Steam build: `25929985`.
- Unreal Engine: `5.8.2`.
- `VoyageSteam-Win64-Shipping.exe` SHA256:
  `608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F`.
- Reviewed mapping: `mappings/Voyage/steam-25929985-ue5.8/Voyage-25929985.usmap`.
- Mapping SHA256: `4EA5D93EBF41FDE3C45C7DB938763E942179DC8EF24951A97EDB94FB1FA9E2FF`.

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

Revalidate mirrors, function/property owners, relocation assumptions and layouts
after a fingerprint change. Static compilation, container verification and
cooked readback do not substitute for runtime validation.

The installed `v6-test-20261015-065220` is user-validated for the requested
presentation and capacity setting, including a nondefault value. The release
default remains 200 kg. Exact installation/rollback evidence is under
`artifacts/installations/ContainerLidLabel/20261015-065622-v6-test-2dc3ea33/`;
`validation/` retains the original build/release manifests and cooked semantic
report. The versioned installed ZIP retains the payload independently of `Tmp/`.
See [current state](BACKLOG.md) for unenumerated edge cases.