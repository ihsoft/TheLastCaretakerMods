# Harpoon pipeline: consolidated observations

Batch consolidation after accepted HC33, 2026-09-13. This is triage evidence,
not authorization to build more tools. See root tools/README.md for public routes.
Detailed historical reports retain checkpoint-specific evidence; raw outputs are
in the verified cleanup archive. Do not re-run failed recipes as instructions.

## Stable prevention rules

| Observation | Contract / next diagnostic |
| --- | --- |
| Sandbox UBT dotnet dialog, -532462766 / 0xE0434352 | Initial UE producer outside sandbox; classify launch permission before SDK/project investigation |
| Compile/cook/retoc accepted corrupt native CDO in HC18 | Tagged station packages AND independent semantic JSON inspection; partial mirrors cannot serialize guessed native defaults |
| HC31 right query hit but no provider | Verify class interface membership as well as exact UFunction owner/signature; explicit implementation may have no SuperStruct |
| HC30 collision mask looked correct in editor but delta absent | Inspect serialized ResponseArray, explicit Interact=Block; editor defaults are not runtime defaults |
| HC32 guards failed after successful cook | Preflight audits against known JSON; optional PropertyFlags and absent explicit-interface SuperStruct are legal |
| Marker existed in bytecode but not reached on misses | Check execution ordering; independent marker before optional guards |
| Name existed only in diagnostic widget | Verify actual HUD producer/own callback, not merely data acquisition |
| HUD hidden but player actions still live | Native possession/control ownership; rendering is not input routing |
| World diagnostic text moved relative to ship | Screen-space rendering and independent local-transform observations |
| Repeated no-op tests | Read acquisition/provider/callback state; stop field tweaking after one/two no-ops |

## Build / generator classifications

- HC23 EnhancedInput missing direct module dependency caused LNK2019/LNK1120;
  this is not the sandbox crash signature.
- GET_FUNCTION_NAME_CHECKED proves a C++ member exists, not that a UFunction
  exists. Use exact reflected names (GetTransform rather than helper
  GetActorTransform), semantic shared pin helpers and typed overload pointers.
- UK2Node_CallArrayFunction owns wildcard array inference; generic calls can
  compile an incorrect Array_Length graph. Class pins are not UObject pins.
- Declared MinimalAPI symbols need not be exported. Editor-only helpers are not
  runtime soft-class loading. Conv_SoftClassReferenceToClass retrieves, not loads.
- Changing module Editor to Runtime can leave stale UHT package flags despite
  up-to-date status. Inspect PKG_EditorOnly and cooked references.
- JSON bytecode may encode virtual versus final calls differently; audit semantic
  nodes/owners, not one string spelling. Do not recook unchanged payload merely
  to fix an inspection script.

## Tool gaps / unexpected output (historical; not all current defects)

- Asset summary omitted native references encoded as Path/ResolvedOwner even
  when full JSON contained valid ItemAsset/Name owners. Missing summary entries
  do not prove missing fields or distinguish valid from stripped references.
  Overview also reported zero generated classes for
  /Game/UI/Game/BP_DynamicPlayerInputHorizontalWidget while full JSON contained
  WidgetBlueprintGeneratedClass with VoyageDynamicPlayerInputWidget parent.
  Supported fallback: public Get-VoyageAssetJson returned jsonPath. Reproduce
  against current tools before treating either historical omission as open.
- Pseudocode exporter encountered WeakObjectProperty yet completed manifest with
  errorCount0 and missing expected output/errors.txt. Check artifact existence
  and diagnostics, not success metadata alone; confirm current status before repair.
- Enum description lost enum identity through InnerType. Bounded inspection of
  published parser metadata was used, not unnecessary mapping regeneration.
- Native inspector match count and RIP-reference report count were ambiguous.
- Non-asset DefaultEngine.ini extraction had no supported route; UnrealPak lacked
  a key. No config recovered; do not infer defaults from that failure.
- A temporary fingerprint-bound interface-registration table decoder helped
  prove native ancestry. Retained hc31-interface-registration.py is evidence;
  promote only if repeated work justifies maintenance cost.

## Caller friction, not game defects

Exact quoted paths matter: PowerShell -LiteralPath does not expand wildcard
filenames; brace expansion is not Bash; -File comma text is not a string array.
Use supported invocation, bounded rg directory/glob searches and known asset
identities. A widget instance name is not necessarily a package path.
Avoid nested-shell dollar expansion and replacement-string metacharacters.
Do not treat a rejected guessed type/member/path as proof the object is absent.
Broad Driver searches, full JSON/log dumps and repeated oversized context output
cost more than compact projections. This cleanup also encountered oversized doc
reads; bounded restart sections now replace routine reading of the full chronology.

## Cleanup-specific method and coverage

One-off scoped archive/cleanup script lives under ignored artifacts, not tools:
artifacts/harpoon-cleanup-20260913.ps1. It validates exact ownership/full paths and
reparse boundaries, archives first, reopens every entry and checks SHA256, verifies
sources unchanged, then deletes. It is intentionally not a new public pipeline.
Saved logs were archived; Intermediate is regenerable and not archived.
Active HC33 sources, three recovery candidates, shared asset store and other mods
are excluded. See cleanup receipt for measured files/bytes and validation.
No recurring tool family was added merely to improve a coverage score.

## Additional contracts recovered from the retired reports

- Required JSON and optional pseudocode are separate outcomes. The original
  BP_Module_Turret request failed because FinalFunctionCleaner threw after JSON
  was written; the wrapper rejected it. Do not consume diagnostic staging JSON
  as validated output. Conversely, BP_Steering exported JSON but had no pseudocode
  despite completed/errorCount0 metadata. Preserve partial-failure truth in both
  directions; a current public JSON success is the supported fallback.
- Exact asset query does not imply isolated providers. Older raw Inspector
  recipes mounted installed mods; matching one export hash proved only that
  export's parity. Current documented exact Mod container selection is required
  when installed and candidate packages share an identity. Resolve candidate
  containers to absolute paths; relative ModContainer resolves under game Paks.
- Historical PowerShell7 Functions summary failed on optional null ChildProperties;
  documented Windows PowerShell5.1 succeeded. This did not prove a bad Drone asset
  or a failure of the supported5.1 contract. Recheck current tooling before repair.
- Get-VoyageBuildFingerprint returns JSON text; decode steam.buildId and
  executable.sha256. Table output can truncate packageListPath or hide later mixed
  objects. Consume returned structured paths/compact JSON, not formatted blanks.
- Node reflection must also be BlueprintCallable: TextRender.SetText is reflected
  but K2_SetText is callable. By-reference FText inputs need a connected conversion,
  not a literal default. Array wildcard inference must accommodate all elements;
  Actor first accepts Pawn, while Pawn-first inference can reject Actor later.
- Keep Engine headers outside anonymous namespaces in implementation fragments;
  otherwise UObject forward declarations can acquire different identities.
  Disable unused default plugins that generate source config unexpectedly; source
  immutability gates must reject those changes rather than bless the artifact.
- An orchestration tool returning an exec result does not throw for every nested
  nonzero exit. Explicitly gate dependent installs on required audit exit/status;
  do not continue because the JavaScript wrapper itself returned normally.
- Repair assertions against previously returned, hash-verified same-candidate
  JSON instead of repeating nine exports/cooks. This is bounded evidence reuse,
  not a persistent Mod cache. Normalize GUID noise only after inspecting the diff.
- Installation status with the exact install manifest includes autoload. After
  removal, filesMatch=false with owned files missing is expected, not failure.
- Telescope is a data-only Binoculars child; InitialLockFOV5 means five degrees,
  not x5. Equipment animation notifies target the equipped slot and later cleanup;
  playing that animation is not a safe station-camera lifecycle. Call-site shapes
  alone do not prove native ref/out flags or ownership/arbitration semantics.

Historical gaps are not all still open: the public index owns current interfaces.
Detailed stage counts already reside in voyage-toolchain-coverage.md; do not
reconstruct an invented aggregate total from incompatible per-turn denominators.

## Accepted checkpoint recovery

HC33 is user-accepted; its completed paired installation was removed. Exact live
release and restore manifests remain in ../../docs/harpoon-cannon-backlog.md.
No gameplay rebuild or source refactor was bundled with documentation cleanup.

Ignored archive root: artifacts/archive/harpoon-hc33-20260913.
Each ZIP has a sibling inventory JSON with original relative paths, lengths and
SHA256. Verify the archive before extracting selected entries into a fresh ignored
directory; never overwrite active sources or auto-install historical candidates.

| Archive | Files | SHA256 |
| --- | --- | --- |
| hc33-source-and-methods.zip |106|113FF3DA6283628558CFAA048C0023043C827E70EF7C739B4858C7EDD0B759E7|
| research-history.zip |44|64FB30198695BF85B627796F9C7CCB9F4EDF0A8EF04E5FE058A462C4FFA0FA93|
| retired-experiments-and-logs.zip |3490|08FBB28E70C328DE826B9416EE7BCAEC0D91CA150DBF96600CBB6D028B156E35|

The first cleanup removed3876 files /6,357,377,332 bytes, including386 regenerable
Intermediate files not archived. Archive/evidence storage272,795,191 bytes left
net6,084,582,141 bytes removed (file lengths, not allocated-disk measurement).
plan.json and cleanup-result.json record that boundary. All83 checked active
C++/header/build/config/project files matched the pre-cleanup source inventory.
Public Install-VoyageRelease ValidateOnly with explicit AllowDirtySource passed
HC33, HC32 and shell; immutable manifests remain unchanged. No mixed-tree commit
was made; source ZIP preserves the accepted dirty source checkpoint.

The second cleanup retired42 standalone report documents, including the cleanup
report itself, after consolidating their useful facts here and in architecture,
provenance and active backlog. The41 older reports match research-history.zip
inventory hashes. No redirect stubs or duplicate report archive was added to docs.
The native/provenance documents retain technical evidence, not report recipes.

Documentation cleanup tool-use: scoped reads, apply_patch consolidation/deletion,
archive hash comparison and reference/whitespace validation. No game-asset/release
operations, tool implementation changes or new tool family in the second pass;
recurring coverage denominator is zero. One repeated Windows rg wildcard-path
error and oversized report projection were caller friction, corrected with
directory -g and bounded reread. No game/install/source mutation.
An overlarge whole-document JSON projection was truncated and could not be parsed;
no changes followed that result. Recovered with a read-only changed-line projection
and applied only explicit hunks, rather than loading entire consumer documents.

Baseline commit preparation: documented Git wrapper rejected trailing blank
lines in six newly tracked files and rolled its staging back. Prior unstaged
diff-check did not cover untracked files; this was formatting, not a build defect.
Removed terminal empty lines only before retry. Python was absent from PATH;
the bundled runtime ran all six offline observer tests successfully. No rebuild,
new gameplay claim or speculative pipeline work followed these caller issues.
