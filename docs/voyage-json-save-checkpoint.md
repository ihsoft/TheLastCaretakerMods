# JSON save and engine selection contract

## Accepted binaries

| Component | Source commit | Canonical file SHA-256 |
| --- | --- | --- |
| UAssetGUI | `df18b5fd0d263d78fdb0cd5f49de1ee5cf6a9520` | `42837CD279A78DF57B537020A0169C5D2259A4570D3B62F4B96852F0F5C27F96` |
| UAssetAPI | `21c982fa8f04e12d5d216fdf330a2f206e81156f` | `6DF2606BBA89987AEB4BF1EFBD3C64AC565DBC5D6113A0A7A5062C7CD8B249FD` |
| retoc | `49b772135ddb967dc56795d311bd88fe81929f63` | `6F8F86AE3FD747A3B785E787A33C24F9A11735D03664948D6B619F18861650F5` |

Canonical paths are `.tools/bin/UAssetGUI.exe`,
`.tools/bin/UAssetAPI/UAssetAPI.dll`, and `.tools/bin/retoc.exe`. Their sibling
publish manifests are the authority for complete provenance. The GUI embeds
retoc from the same source checkpoint; that independently built embedded
executable has SHA-256
`3D17670FE4C45E610B915999AFBF6700DB7DB1F31DF0DF143486ABCF1607F33B`.

## Serialization contract

`SpecifiedEngineVersion` is runtime-only. JSON serialization omits it and
deserialization ignores it. Existing `ObjectVersion`, `ObjectVersionUE5` and
`CustomVersionContainer` metadata remain authored package data.

Immediately before each binary save, UAssetGUI calls
`SetSerializationEngineVersion(ParsingVersion)` and then `Write(path)`. API
callers must likewise set the serialization hint after `DeserializeJson` and
before writing. CLI syntax is:

```text
fromjson <json> <uasset> <mapping> [5.8]
```

Voyage callers supply `5.8`. The hint selects the reviewed filtered import and
`FField` layout; it does not rewrite stored object/custom versions and is not a
general asset/schema migration between engine versions.

UAssetGUI forwards its selected version to retoc extraction. Explicit `UE5_8`
uses Voyage's expanded filtered-import layout. UE5.7, older versions and absent
library hints preserve the upstream writer layout. Both reviewed layouts are
readable by the accepted tools. No synthetic Unreal object version is used to
distinguish them.

Full JSON intentionally omits UAssetAPI's internal
`OverrideNameMapHashes`. A no-op JSON write can therefore recompute name-map
hashes. Validate parsed exports, raw-export status, stored versions, `.uexp`
identity and intended semantics separately; do not claim universal
byte-identical `.uasset` JSON roundtrips.

## Public commands

Publish accepted binaries through Windows PowerShell 5.1:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Publish-UAssetApiBinary.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Publish-UAssetGuiBinary.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Publish-RetocBinary.ps1
```

Prepare reviewed UAssetAPI source for deliberate development with
`Prepare-UAssetApiVoyageUe58.ps1 -OutputRoot <new ignored directory>`; normal
asset work consumes the manifest-validated canonical binary instead.

Run the bounded GUI JSON regression with:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Test-UAssetGuiJsonRoundtrip.ps1 -CandidateGui .tools/bin/UAssetGUI.exe -InputAsset '<fingerprint-validated loose uasset>' -Mappings '<reviewed usmap>' -EngineVersion 5.8 -TestCanPlace
```

`-TestCanPlace` is specific to
`BP_FabricationPlacementComponent.GenerateAndSetLocation`; omit it for other
inputs. The test writes only ignored artifacts and returns `reportPath`. It
checks unchanged JSON, rejection of obsolete engine-hint assumptions, optional
`CanPlace=True`, reopen without `RawExport`, preservation of stored versions
and unchanged `.uexp` where applicable. Whole-file equality is reported
separately because name-map hashes may be recomputed.

Native tools run through `Invoke-VoyageBoundedTool.ps1`, which applies timeout
and memory limits to the process tree and retains stdout, stderr and a result
manifest below ignored `artifacts/tool-runs/`. Use `-AllowFailure` only for an
expected diagnostic failure and inspect the returned status explicitly.

## GitHub RC3 publication

The accepted source checkpoints are published as coordinated prereleases:

- [UAssetGUI v1.1.1-rc3](https://github.com/ihsoft/UAssetGUI/releases/tag/v1.1.1-rc3)
- [UAssetAPI v1.1.0-rc3](https://github.com/ihsoft/UAssetAPI/releases/tag/v1.1.0-rc3)
- [retoc v0.1.5-rc3](https://github.com/ihsoft/retoc/releases/tag/v0.1.5-rc3)

| ZIP | SHA-256 |
| --- | --- |
| `UAssetGUI-Voyage-UE58-df18b5f-21c982f-win-x64-rc3.zip` | `249ED59CD568B5B3E6A43DE19B6BBB213EABCFE628C09F51F3445F796239BC3D` |
| `UAssetAPI-Voyage-UE58-21c982f-net10.0-rc3.zip` | `28A3C0FF1B6C951F27CD3F9709CB03C3F473F095B1F4597C214BCBF527A70577` |
| `retoc-v0.1.5-rc3.zip` | `313D3A3BC636372536D3B9E7ADC130A4201EFC9E61A9C6CFC3084E0EAE546C9F` |

The source commit and published binary manifests, not a shared CLI version
string or later Git tag description, determine compatibility.
