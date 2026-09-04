# JSON save and engine selection checkpoint

Accepted after the user's interactive GUI confirmation and promoted on
2026-09-04 UTC. This supersedes the previous e362030 / 6b5ead3 / 234f4e5
tool checkpoint; it is not a new gameplay validation claim.

| Component | Source commit | Canonical file SHA-256 |
| --- | --- | --- |
| UAssetGUI | df18b5fd0d263d78fdb0cd5f49de1ee5cf6a9520 | 42837CD279A78DF57B537020A0169C5D2259A4570D3B62F4B96852F0F5C27F96 |
| UAssetAPI | 21c982fa8f04e12d5d216fdf330a2f206e81156f | 6DF2606BBA89987AEB4BF1EFBD3C64AC565DBC5D6113A0A7A5062C7CD8B249FD |
| retoc | 49b772135ddb967dc56795d311bd88fe81929f63 | 6F8F86AE3FD747A3B785E787A33C24F9A11735D03664948D6B619F18861650F5 |

Canonical paths are `.tools/bin/UAssetGUI.exe`,
`.tools/bin/UAssetAPI/UAssetAPI.dll`, and `.tools/bin/retoc.exe`.
Their publishers write `UAssetGUI.manifest.json`,
`UAssetAPI/publish-manifest.json`, and `retoc.manifest.json` respectively.
The GUI embeds retoc from the same source checkpoint, but its independently
built embedded executable has SHA-256
`3D17670FE4C45E610B915999AFBF6700DB7DB1F31DF0DF143486ABCF1607F33B`.

## Serialization contract

`SpecifiedEngineVersion` is runtime-only. JSON serialization omits it and
deserialization ignores it, including an obsolete field in an older JSON.
Existing object/custom version metadata is retained. Immediately before each
binary Save, GUI calls `SetSerializationEngineVersion(ParsingVersion)` and
then `Write(path)`. API callers should likewise set the serialization hint
after `DeserializeJson` and before writing. The existing Write API remains.

CLI syntax is `fromjson <json> <uasset> <mapping> [5.8]`; supply the final
version for Voyage. The hint controls the scoped import/FField behavior; it
does not rewrite ObjectVersion, ObjectVersionUE5 or CustomVersionContainer.
This is not general asset/schema migration across engine versions.

GUI extraction forwards the selected version to retoc. Explicit `UE5_8`
enables the Voyage expanded import writer. UE5_7, older versions and absent
library hints preserve upstream writer behavior. UE5_7 and UE5_8 have separate
match arms even where their object/container version values are equal.
Both validated import layouts are readable. No new Unreal object version was
invented to distinguish these layouts.

## Public commands and evidence

Run publishers through Windows PowerShell 5.1:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Publish-UAssetApiBinary.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Publish-UAssetGuiBinary.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Publish-RetocBinary.ps1
```

All three succeeded at the pinned commits. Public preparation also passed
through `Prepare-UAssetApiVoyageUe58.ps1 -OutputRoot <new ignored directory>`.
Parameter defaults are resolved in the script body for PS5.1 compatibility.

GUI regression:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/Test-UAssetGuiJsonRoundtrip.ps1 -CandidateGui .tools/bin/UAssetGUI.exe -InputAsset '<fingerprint-validated loose uasset>' -Mappings '<reviewed usmap>' -EngineVersion 5.8 -TestCanPlace
```

`-TestCanPlace` is specific to FabricationPlacementComponent's
GenerateAndSetLocation; omit it for other inputs. The test uses
`Invoke-VoyageBoundedTool.ps1`, writes only ignored artifacts and returns
`reportPath`. It checks unchanged JSON, obsolete-hint JSON, and the optional
CanPlace=True edit, including reopening without RawExport and preserving
object/custom versions. No-op export payload must match; total file equality
is reported separately because JSON omits OverrideNameMapHashes.

The bounded runner accepts one EXE and an argument array, with timeout and
memory limits; descendants belong to the same Windows job. It captures logs
and a result manifest under `artifacts/tool-runs`. Failure throws unless
`-AllowFailure` is explicit. Test it with `Test-VoyageBoundedTool.ps1`.

Post-publication evidence under `artifacts/tests/`:

- `gui-json-7c3bf6488c2e4871982e4386ff9157e2/summary.json`: three cases passed,
  zero RawExports, persistent CanPlace=True.
- `retoc-imports-2a7fe582f4c546b1a43cd7da446c3781/summary.json`: four cases
  passed; UE5_7 writer bytes match upstream 885a8da, UE5_8 matches old Voyage
  234f4e5. Converted containers verify and agree except the independently
  checked CookedHeaderSize field.
- `extraction-sources-a587ee27f925401d8420071767236071/summary.json`: six
  source-isolation checks passed using the coordinated extraction-wrapper
  changes. Those wrapper changes are a separate pipeline integration.

The import regression now requires `-LegacyRetoc <old 234f4e5 exe>` as well as
`-CandidateRetoc` and `-UpstreamRetoc`; the current canonical binary is no
longer a legacy control. The preserved local control is under
`artifacts/bytecode-repro/promotion-controls/retoc-234f4e5.exe`.

Inputs matched Steam 25056839 and executable SHA-256
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
Get-VoyageMappings resolved the reviewed mapping without generating a new one.
No installed game files were changed.

Tool-use report for promotion: public mapping resolution, all three publishers,
source preparation, JSON verification, import conversion/verification and
source-isolation regression succeeded. Earlier API 30/30 and targeted 5/5,
retoc 29/29, GUI 17/17, and interactive confirmation are prior evidence, not
new tests in this promotion. Source inspection was limited to publisher and
checkpoint development; the preparation hash was corrected for the canonical
checkout's line endings and its public entry point then passed. The existing
candidate-build entry-point gap remains; canonical publication is covered.

## GitHub RC3 publication

Published prereleases on 2026-09-04 UTC, after explicit user authorization:

- [UAssetGUI v1.1.1-rc3](https://github.com/ihsoft/UAssetGUI/releases/tag/v1.1.1-rc3)
- [UAssetAPI v1.1.0-rc3](https://github.com/ihsoft/UAssetAPI/releases/tag/v1.1.0-rc3)
- [retoc v0.1.5-rc3](https://github.com/ihsoft/retoc/releases/tag/v0.1.5-rc3)

API had no preceding fork release; its base follows the existing v1.1.0 source
tag. RC3 coordinates the bundle with GUI/retoc. All remote annotated tags
resolve to the exact source commits above. Canonical binaries were packaged
without rebuilding; the ZIPs include provenance manifests and license notices.

| ZIP | SHA-256 |
| --- | --- |
| UAssetGUI-Voyage-UE58-df18b5f-21c982f-win-x64-rc3.zip | 249ED59CD568B5B3E6A43DE19B6BBB213EABCFE628C09F51F3445F796239BC3D |
| UAssetAPI-Voyage-UE58-21c982f-net10.0-rc3.zip | 28A3C0FF1B6C951F27CD3F9709CB03C3F473F095B1F4597C214BCBF527A70577 |
| retoc-v0.1.5-rc3.zip | 313D3A3BC636372536D3B9E7ADC130A4201EFC9E61A9C6CFC3084E0EAE546C9F |

GitHub readback verified public prerelease state, tag commits, asset names,
sizes and server SHA-256 digests. Local evidence is retained in
`artifacts/releases/toolchain-rc3/verification.json` with exact release and
asset IDs. Drafts required listing releases: the tag lookup returned 404 until
publication. No mutation was repeated after that read-only failure.

New RC3 tags change git describe without changing commits. The two source
preparation/build scripts now accept the exact pre-tag description or RC3
tag while retaining the same required commit and source hashes. API source
preparation and a separate retoc build both passed via powershell.exe -File.
The smoke build did not replace the canonical or uploaded binary.

Release-iteration tool report: existing canonical binaries were hash-checked,
packaged with a local PowerShell 5.1 staging script, uploaded using gh, and
verified against GitHub digests and remote tags. No new game-asset tests were
run. Public repository automation does not yet cover third-party GitHub ZIP
assembly/publication; that gap is deferred, not authorization for a new tool.
