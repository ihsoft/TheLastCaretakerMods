# Voyage toolchain adoption evidence

This is a bounded report ledger, not a gameplay or binary-validation registry.
It records how real coding tasks used the public pipeline. Tool-development
synthetic tests are separate and do not prove adoption or repository-wide
coverage. The active improvement queue lives in `voyage-toolchain-backlog.md`.

## Measurement contract

- Current user-approved scope: game-asset work and release preparation only.
  Includes game fingerprint/mappings, extraction/inspection/modification,
  validation, build/cook/package, release verification, installation/restore.
  Source-only modeling, images/video and unrelated development are excluded.

- Count one intended recurring mechanical operation once, including its retries
  and diagnostic variants. Do not count every internal phase or assertion.
- A public entry point can be a maintained wrapper or a canonical CLI command
  with a documented supported invocation and contract. A wrapper is not needed
  merely to increase the metric. Ad-hoc invocation or fallback is not public
  coverage just because it uses the same underlying binary.
- A documented tool completing the operation after a retry counts as covered;
  record first-pass failure and source-inspection cost separately.
- An operation completed through an ad-hoc replacement counts as uncovered.
- Code editing, reasoning, documentation, and genuinely novel failure diagnosis
  are not mechanical operations. Unknown historical operations stay unknown;
  an empty denominator is N/A, not 100%.
- Source-model export, previews and video verification in R3 are outside the
  narrowed denominator. Actual import/cook/packaging for game releases remains
  in scope. Do not conflate source-content creation with preparing a release.
- Report collection is not itself coding work and must not generate recursive
  report requests. Each task/iteration is counted once.
- A historical sample establishes a baseline only. It cannot establish current
  adoption after later changes without a new, non-overlapping real-task sample.

## Current scoped baseline

Under the user's narrowed scope, R1 + R2 + R4 give **9/15 = 60%**. R3 contains
no game-asset/release operations, so its current scoped coverage is N/A (0/0).
This is a recalculation of the same historical reports, NOT measured improvement
or acceptance of newly added tools. Fresh in-scope reports are still required.

## Historical reports (original broader scope)

| Report | Task / bounded iteration | Covered | Eligible | Interpretation |
| --- | --- | ---: | ---: | --- |
| R1 | `01a06a3f-86f7-7253-b92d-d30a4162fa74`: completed retoc single-Blueprint producer/consumer investigation | 2 | 5 | 40%; below target, not a current global estimate |
| R2 | `01a0507c-01d0-71e0-a8a0-09348bb3c406`: delayed-marker interpretation, Telescope/operator research, then game-update stop | 3 | 5 | 60%; historical legacy-inspector path, not current-tool acceptance |
| R3 | `01a05140-478e-76e2-ba38-626d96aa08bb`: 2026-08-30 source-only Harpoon model iteration | 0 | 4 | Four mechanical model-artifact operations; no game operations |

Combined retrospective sample: **5/14 = 35.7%**. All three requested owners
reported (3/3); neither the sample nor that response rate represents every
historical or future coding task. The iterations use different historical tool
checkpoints and must not be presented as a current pipeline acceptance score.

R1 was supplied by the owning agent without new experiments or mutations.
It groups all four producer/consumer combinations as one packaging operation.

| R1 operation | Observed path | Result |
| --- | --- | --- |
| Fingerprint | `Get-VoyageBuildFingerprint.ps1` | Covered; documented execution-policy invocation succeeded |
| Source ownership/inventory | Ad-hoc retoc `list --path` over stock and selected installed mods | Uncovered; established ownership before allowing a mixed view |
| Exact asset extraction | `Extract-VoyagePackage.ps1` with explicit producer and reviewed additional-container allowance | Covered after failures; stock-only interface remains missing |
| Single-package packaging | Ad-hoc retoc `to-zen` producer/consumer matrix | Uncovered; same-producer cases passed, mixed layouts failed |
| Verification and result inventory | Ad-hoc retoc `verify` plus `list --path` | Uncovered; combined into one verification gate |

R1 source inspection had a legitimate trigger: a reproducible tool panic and
an insufficient stock-only extraction interface. Missing global ScriptObjects
and unavailable Windows symlink privileges defeated attempted alternatives.
The retained ignored evidence is under
`artifacts/retoc-single-bp-investigation/`. The useful layout finding and pending
fix are owned by the cooked-asset toolchain document and active backlog; this
ledger does not duplicate binary evidence or claim runtime validation.

| R2 operation | Observed path | Result |
| --- | --- | --- |
| Fingerprint | `Get-VoyageBuildFingerprint.ps1` | Covered; identified update and stopped old-input reuse |
| Mapping resolution | Prior manifest and recursive `.usmap` search | Uncovered; no reviewed resolver used in that old iteration |
| Asset/inventory lookup | Documented legacy `Inspect-VoyageAsset.ps1` queries | Covered for that historical interface |
| Structured export/inspection | Documented legacy inspector | Covered after permission retry; `dotnet run`/NuGet friction and old mapping acceptance remain relevant findings |
| Installed/process status | Ad-hoc process/file/hash commands | Uncovered; read-only status, not installation |

R2 includes no build, cook, package, install, restore, or fresh mapping generation.
The owner explicitly corrected an overconfident delayed-marker interpretation:
early invalidation remained a hypothesis, and a game update alone did not prove
mod incompatibility. Those feature conclusions belong to the owning task, not
to this coverage ledger. Existing newer mapping/source-isolation rules address
some historical gaps, but require fresh black-box acceptance evidence.

| R3 operation | Observed path | Result |
| --- | --- | --- |
| OBJ/MTL export | Ignored local Python helper | Uncovered |
| Static preview rendering | Local numpy/PIL helper | Uncovered |
| Turntable render/encode | Ignored helper plus local FFmpeg/OpenH264 | Uncovered |
| Model/video output verification | Ad-hoc PowerShell geometry checks and FFmpeg decode | Uncovered |

R3 excluded all operations performed by the separate logic owner and did not
claim UE integration or gameplay validation. Its original report supplied game
operations as 0/0 (N/A) and model operations as 0/4 separately. The former metric
counted the four source-artifact operations; the current metric excludes them.
Python discovery and the ffprobe-to-FFmpeg fallback were retries
inside those operations, not extra denominator entries.

## Additional pre-publication sample

R4: task `01a06a48-386e-7c50-bcda-1668a19773cc`, completed
`BP_FabricationPlacementComponent.GenerateAndSetLocation` investigation on
Steam `25056839` / API `6b5ead3`, from fingerprint through diagnostic
roundtrips. No fork repair, installation, or runtime validation; report-only
confirmation did not run new experiments. This iteration preceded the
canonical Inspector wrapper publication and is separate from R1-R3.

| R4 operation | Observed path | Result |
| --- | --- | --- |
| Fingerprint | `Get-VoyageBuildFingerprint.ps1` | Covered |
| Mapping resolution | `Get-VoyageMappings.ps1` | Covered; reviewed matching input |
| Selected-mod inventory | `Get-VoyageAssetJson.ps1 -ListPackages -Source Mod` | Covered after NuGet permission retry; target absent from the four-package mod |
| Binary extraction | `Extract-VoyagePackage.ps1` | Covered with explicit retoc-path workaround for PS5.1 default bug and reviewed additional-container allowance |
| Changed-save verification | Temporary C# harness with canonical API | Uncovered; missing public changed-bytecode/full-JSON roundtrip interface |

R4: **4/5 = 80%**, but with first-pass friction and a missing verification
interface. Source inspection followed an actual reported serialization failure;
the initial harness's wrong FName serializer was corrected with the API's own
serialization method. This was a diagnostic retry, not a sixth operation.
Findings and validation limits belong to the durable toolchain document and
backlog, not a new acceptance claim here.

Combined four-report pre-publication sample: **9/19 = 47.4%**. Four requested
coding owners supplied bounded reports (4/4); the original three-report
baseline remains 5/14 above. Do not compare these mixed samples as evidence of
improvement, or count notification acknowledgements as new coding reports.

## R5: post-publication real repair iteration

Task `01a06a48-386e-7c50-bcda-1668a19773cc`, turn
`01a06a8f-aa63-78b3-b19b-63112c1cdf3b`, completed 2026-09-04 UTC.
The user requested the full-JSON/Save fix and selected-UE5_7 retoc compatibility.
This is a new bounded coding report, not a repeat of R4 or notification work.

| Operation | Actual interface | Coverage |
| --- | --- | --- |
| Game fingerprint | `Get-VoyageBuildFingerprint.ps1` | Covered |
| Fresh extraction | `Extract-VoyagePackage.ps1` through the compatibility test | Covered |
| Packaging controls | Bounded `to-zen` cases in `Test-RetocImportCompatibility.ps1` | Covered through the documented matrix, not arbitrary shell commands |
| Changed-save/container/open verification | Compatibility matrix, `Test-UAssetGuiJsonRoundtrip.ps1`, GUI `stress-open` | Covered, with documented retries/limits |
| Candidate builds | Direct SDK commands for fork development | Uncovered; no candidate-build public entry point |

Reported and evidence-checked coverage: **4/5 = 80% for this iteration**.
Unit assertions do not add operations. `Test-VoyageContainer.ps1` was explicitly
not used, so no adoption is attributed to that new wrapper. This is a real
tool-repair task against game assets, not evidence that every feature/release
workflow now reaches 80%. Do not blend this post-publication observation into
the older 9/15 baseline to manufacture an improvement trend.

Evidence inspected: four producer/profile packaging reports and the successful
matrix under `artifacts/tests/retoc-imports-f5b79c5e01fa44479013ca48fc9cbd9a/`;
four GUI JSON cases under `artifacts/tests/gui-json-82a069c1889648c08ac2a1f6aaf5c7da/`
with zero RawExports. Their `.uasset` byte identity is false, not silently
accepted as true; the owner attributes header differences to JSON omission of
OverrideNameMapHashes and separately checks payload and versions. Full API
suite passed 30/30 after fixing missing fixture copying; final targeted
engine-selection tests passed 5/5. Prior candidate hierarchy test covered
17 Fabrication assets; at the end of R5 the final candidate awaited the user's
interactive GUI confirmation. Neither canonical publication nor runtime
validation occurred in that iteration; later confirmation/promotion belongs to R6.

Failures/costs retained: SDK access escalation, test-fixture staging, an empty
stress filter corrected to a folder, and rejection of a Voyage5.8 payload as a
native UE5.7 parser-control fixture. Fork source inspection was justified by the
requested defect repair. Candidate-build automation is a reported gap for
prioritization, not an automatic instruction to build another tool.

## R6: user-approved canonical promotion

Task `01a06a48-386e-7c50-bcda-1668a19773cc`, completed 2026-09-04 UTC,
after the user's interactive GUI confirmation and explicit commit/update request.
Iteration boundary: promotion of reviewed sources and post-publication checks,
ending at root commit `9cb271a0be59ffcff7a89e1f8a8694f91ef11621`.
The owner's final report groups five intended recurring operations:

| Operation | Actual interface | Coverage |
| --- | --- | --- |
| Input provenance and reviewed mapping resolution | `Get-VoyageMappings.ps1`, including fingerprint gate | Covered |
| Canonical build/publication | `Publish-UAssetApiBinary.ps1`, `Publish-UAssetGuiBinary.ps1`, `Publish-RetocBinary.ps1` | Covered; one category across three components |
| Extraction | `Extract-VoyagePackage.ps1` through import/source-isolation regressions | Covered |
| Legacy-to-IoStore packaging | `Test-RetocImportCompatibility.ps1` | Covered |
| Output verification | GUI JSON, retoc import and extraction-source regression entry points | Covered |

Reported coverage: **5/5 = 100% for this narrow promotion iteration**.
Repeated cases, source preparation, internal build stages, hash readback and
bounded execution do not add denominator entries. Earlier SDK candidate builds,
API 30/30 and 5/5, retoc 29/29 and GUI 17/17 are not new R6 operations.
`Test-VoyageContainer.ps1` was not used. The candidate-build gap remains;
canonical publication through the existing publishers is a different operation.

Pipeline owner independently checked canonical files against all three publish
manifests and inspected post-publication reports: GUI JSON 3 cases, retoc imports
4 cases and source isolation 6 checks, all passed. Exact commits, hashes and
evidence paths live in `voyage-json-save-checkpoint.md`. Root commit was read
back; no push or installed-game mutation was reported. Source inspection was
limited to deliberate checkpoint/publisher development. Preparation initially
needed a hash correction for canonical checkout line endings; the public PS5.1
preparation subsequently passed, not an ad-hoc replacement workflow.

An independent normal-path smoke after promotion resolved the existing reviewed
mapping and unchanged canonical Inspector, then obtained `BP_VoyageCableUpdater`
JSON through `Get-VoyageAssetJson.ps1` on Steam 25056839. It passed without a
source build or mapping generation; this pipeline-owner check is readiness
evidence only, not included in R6 or general adoption coverage.

R6 removes the pending interactive GUI/canonical-promotion gate. Its published
runtime-only engine-hint contract supersedes intermediate R5 candidate behavior;
do not implement from the historical candidate description. This is still not
a broad feature/release sample or a new gameplay validation claim.

## Current conclusion

Post-publication audit on 2026-09-04 UTC: all four known coding owners were
idle when inspected. Recent R1/R2/R3 activity was tool-change acknowledgement,
not fresh mechanical coding work. R4's owner supplied an explicit delta:
no new tool/harness/extraction/roundtrip operations after its recorded report
(0 eligible, N/A). The user did confirm the full-JSON GUI reproduction with
22 export errors, but that validation is not an extra agent-tool operation and
does not establish adoption. Do not request duplicate reports merely because
notifications update a task timestamp.

The subsequent assigned JSON/UE5_7 repair iteration is now recorded separately
as R5 above. The pipeline owner's earlier synthetic tests remain excluded.

The repair iteration reached 80%, followed by the separately bounded promotion
iteration R6 at 100%. Canonical binaries are now promoted after interactive GUI
confirmation. Broad current game-asset/release coverage remains unproven; do not
pool these different samples to imply repository-wide acceptance. Six bounded
reports from four requested owners are retained; this is not a complete activity
census. Continue gathering reports
from actual in-scope work, not synthetic chores or repeated idle-task requests.
