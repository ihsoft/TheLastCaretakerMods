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

Fresh separate sample, 2026-09-05 Pacific / 2026-09-06 UTC:
Harpoon task `01a0507c-01d0-71e0-a8a0-09348bb3c406`, repository restart through
current-stock research, completed 13/14 intended operations through public
tools (92.9%). Four identity/inventory/status operations plus nine successful
exact-asset inspections; the tenth asset (turret) is blocked by optional
pseudocode generation. Retries/variants counted once per asset, no ad-hoc
replacement and no runtime test. This is not an update of the historical
baseline below. See [full consumer report](harpoon-cannon-tool-report-2026-09-05.md).
Later tool repair now returns valid turret JSON despite that optional formatter
failure, but synthetic verification does not rewrite this completed adoption
sample.

Five following non-overlapping Harpoon iterations are recorded as R10-R13 and
R15:
HC01 preparation 88.9%, HC01 installation 100%, HC02 preparation 83.3%, and
HC02 installation 100%, followed by HC03 preparation/installation at 100%.
Together with the 92.9% restart sample above, every one of the six latest
bounded real-task samples reaches the 80% target. They are kept separate rather
than pooled into a misleading repository-wide percentage.

The pre-repair historical R1 + R2 + R4 recalculation remains **9/15 = 60%**.
R3 contains no game-asset/release operations, so its scoped coverage is N/A
(0/0). This preserves the old baseline; later adoption does not rewrite it.

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

## R7: dynamic DonkLift asset and release workflow

Task `01a06fe3-b779-7da2-8650-f2fe9213cfa0`, reported 2026-09-05 UTC,
covered the real `v2-dynamic-prototype2` investigation, release production and
installation on Steam build `25056839`. The owning agent supplied the required
success, failure, fallback, validation and reusable-gap evidence. Source/code
editing and the unresolved gameplay hypothesis are not mechanical operations.

| Intended recurring operation | Actual interface | Coverage |
| --- | --- | --- |
| Installed-game fingerprint | `Get-VoyageBuildFingerprint.ps1` | Covered |
| Stock package inventory | `Get-VoyageAssetJson.ps1 -ListPackages` | Covered; 23,980 packages |
| Stock relationship and Blueprint structure inspection | `Inspect-VoyageAsset.ps1` references, mappings-property and pseudocode queries | Covered; repeated queries count once |
| Build, generation, narrow cook, package and release manifest | mod-owned `Build-DonkLiftRelease.ps1` producer | Covered after the producer itself was adapted; source editing is not another operation |
| Built-mod structural inspection | `Get-VoyageAssetJson.ps1 -Source Mod -ModContainer` | Covered |
| Manifest-gated installation | `Install-VoyageRelease.ps1` ValidateOnly then install | Covered; retry is not another operation |
| Installed state and fingerprint readback | `Get-VoyageInstallationStatus.ps1` | Covered |
| Determine Blueprint write accessibility of a reflected external property | Mapping report, then mirror compilation and cooked-bytecode inspection | Uncovered; public mapping output does not carry the needed access contract |
| Determine native type/function/call relation for persistence ownership | `VoyageExecutableInspector`, then manual architectural correlation | Uncovered; reported output was general engine noise and did not answer the relation query |

Reported and pipeline-reviewed coverage: **7/9 = 77.8% for R7**. A zero-match
`references` query returning exit 1 is a public-interface defect encountered
inside the covered inspection operation, not a tenth denominator item.
Likewise, an initial unsupported `-AssetPath` argument and PowerShell wildcard
mistakes are caller errors, not tool gaps. Static evidence that `DroppedActor`
controls newly fabricated forklifts did not prove behavior of an existing saved
actor. The later user test confirmed the distinction: a newly fabricated
forklift used the dynamic child, while an actor restored from the save retained
stock behavior through its separately serialized `PersistentWorldActor.ActorClass`.
That runtime result validates the reported static-analysis boundary; it does not
retroactively turn either uncovered R7 operation into public-tool coverage.
The later `Get-VoyageAssetSummary.ps1` implementation replaces the noisy full-
JSON search for generated class, functions, calls, external members, soft-object
constants and SCS components with focused compact output. Its regression is tool
evidence only; R7 remains 7/9 until a new real consumer adopts the interface.

The installed candidate and evidence identity reported by the owner is
`artifacts/installations/DonkLiftKeyboardControl/20260906-013217-v2-dynamic-prototype2-b6428129/install-manifest.json`.
This ledger records the report, not a fresh readback of ignored installation
evidence. The owning DonkLift backlog retains the feature conclusion.

## R8: rejected dynamic-child rollback

Task `01a06fe3-b779-7da2-8650-f2fe9213cfa0`, reported 2026-09-05 UTC.
After runtime validation rejected a mod-owned persistent child as uninstall-
unsafe, the owner restored two installation generations in reverse order and
verified the original `v2` replacement. Gameplay architecture belongs to the
DonkLift backlog; this ledger counts only the asset/release operations.

| Intended recurring operation | Actual interface | Coverage |
| --- | --- | --- |
| Inspect current installation state | `Get-VoyageInstallationStatus.ps1` | Covered; output was unnecessarily large |
| Restore validated predecessors | two `Restore-VoyageReleaseInstallation.ps1` calls | Covered; one intended operation across two manifests |
| Inspect restored mod asset structure | `Get-VoyageAssetJson.ps1 -Source Mod -ModContainer` | Covered |

Reported and pipeline-reviewed coverage: **3/3 = 100% for R8**. Routine cost:
four public calls, zero implementation/dependency files opened, and one
oversized status result. The mistaken attempt to use asset-specific `-AsJson`
on the status command was interface inconsistency, not an uncovered operation.
The pipeline owner subsequently added the semantically correct `-Summary`
contract to the already-JSON status tool; that synthetic validation does not
enter R8. The rejected architecture and user runtime result are not tool
operations and do not enlarge the denominator.

## R9: ScopeFix legacy recovery and current release candidate

Task `01a0747e-8680-7ea2-8c45-2821ad89e779`, reported 2026-09-06 UTC.
The owner recovered two values from an old non-indexed UE5.7 mod, built a
current-game candidate, validated its package, and installed it for user
testing. Historical recovery needed deliberate fallbacks; the current-game
release path exposed a repeated manifest-producer gap.

| Intended recurring operation | Actual interface | Coverage |
| --- | --- | --- |
| Game fingerprint | `Get-VoyageBuildFingerprint.ps1` | Covered |
| Mapping resolution | reviewed mapping path, after initial lookup | Covered |
| Current stock asset retrieval | `Get-VoyageAssetJson.ps1` | Covered |
| Exact current-package extraction | `Extract-VoyagePackage.ps1` | Covered |
| Structured asset inspection | documented canonical GUI/Inspector paths | Covered after diagnostic retries |
| Current asset modification/build | new mod-owned `Build-ScopeFix.ps1` | Uncovered in this first iteration; the producer was created during the task |
| Release packaging and schema-2 manifest | ZIP plus manually assembled manifest | Uncovered; no common producer existed |
| Container and changed-asset validation | bounded canonical tools plus hash comparison | Covered |
| Manifest-gated installation | `Install-VoyageRelease.ps1` | Covered |

Reported and pipeline-reviewed coverage: **7/9 = 77.8% for R9**. The direct
retoc/UAssetGUI/Inspector work on the old non-indexed container followed real
parse/index failures and is retained as novel legacy diagnosis outside this
recurring-operation denominator; it does not become a supported normal path.
Routine cost was 26 public/external calls, three implementation/dependency files
opened, repeated JSON/extraction attempts, and several oversized outputs.
Reading the private catalog was not a bypass: the path came from the public
`-ListPackages` result. Reading the status implementation and broad installer
implementation was unnecessary; only a narrow manifest-contract lookup had a
valid trigger.

The common manifest producer added after this report addresses one demonstrated
gap but does not rewrite R9. Legacy recovery without index paths remains
deferred and distinct from the normal current-game path. The report was collected
before runtime validation; the user later accepted the candidate at checkpoint
`604fe6fd` as a qualitative long-range improvement with an accepted near-range
undershoot. That gameplay result does not change the historical 7/9 tool-coverage
measurement.

## R10: Harpoon HC01 preparation

Task `01a0507c-01d0-71e0-a8a0-09348bb3c406`, reported 2026-09-06 UTC.
The owner prepared an isolated Engine-only autoload probe through fingerprint,
mapping, installed-state, inventory, exact extraction, build/cook/package,
container verification, release validation, and cooked inspection operations.
Eight of nine used documented public entry points. The newly authored mod-owned
producer was conservatively uncovered in its first iteration.

Reported and pipeline-reviewed coverage: **8/9 = 88.9% for R10**. Own C++,
Blueprint and wrapper mistakes remain development evidence, not pipeline gaps.
The recurring manifest-producer and external-candidate inspection friction that
the report identified was subsequently addressed by the common manifest
producer and exact `Inspect-VoyageAsset.ps1 -Source Mod -ModContainer` route.
Those repairs do not rewrite R10. See
[the complete HC01 report](harpoon-cannon-hc01-tool-report.md).

## R11: Harpoon HC01 installation

The later bounded installation iteration used installation status, manifest-
gated installation, and independent installed-state readback. All three passed
through public tools with zero implementation/dependency reads, fallback, or
oversized output: **3/3 = 100% for R11**. It is separate from preparation and
does not enlarge R10's denominator. The user's later screenshot validated the
bounded lifetime observation, not complete Harpoon gameplay.

## R12: Harpoon HC02 preparation

The next preparation iteration reused the mod-owned producer and common release-
manifest generator as black boxes. Fingerprint, stock Drone inspection, mapping,
release preparation, and installed-state readback were covered. Exact inspection
of a non-installed candidate sharing a virtual package identity with installed
HC01 required lower-level diagnosis and was conservatively uncovered.

Reported and pipeline-reviewed coverage: **5/6 = 83.3% for R12**. Two tool
implementation reads followed actual failures: PowerShell 7 null handling in
the summary helper and the missing isolated external-candidate route. Commits
`f8451a04` and `74dc53c2` fixed both interfaces and reduced the broad Functions
result from roughly 8,872 tokens to a compact name index; synthetic validation
does not rewrite R12. See
[the complete HC02 report](harpoon-cannon-hc02-tool-report.md).

## R13: Harpoon HC02 installation

Readiness, manifest-gated installation, and independent status readback all
passed through the public tools: **3/3 = 100% for R13**. Three public calls,
zero implementation/dependency reads, no fallback, and no new reusable gap.
The user then observed a visible living Drone and stock Loot/Enter/Grab hints;
entry, exit, cleanup, save, and shell behavior remain outside that runtime gate.

## R14: ScopeFix v1 GitHub prerelease

Task `01a0747e-8680-7ea2-8c45-2821ad89e779`, reported 2026-09-06 UTC.
The user requested publication of the already accepted ScopeFix candidate as a
GitHub prerelease. The owner separated five intended recurring operations:

| Intended recurring operation | Actual interface | Coverage |
| --- | --- | --- |
| Current game fingerprint | `Get-VoyageBuildFingerprint.ps1` | Covered |
| Prepare immutable public release root, renamed ZIP, and notes/context | Manual PowerShell copy/hash/write | Uncovered |
| Produce and validate schema-2 manifest | `New-VoyageReleaseManifest.ps1` | Covered |
| Create remote tag/release and upload ZIP | Direct `gh release create` inside the repository lock | Uncovered; no intent-level publisher |
| Read back tag, prerelease metadata, and asset digest | Direct `gh release view` and `gh api` | Uncovered; no intent-level verifier |

Reported and pipeline-reviewed coverage: **2/5 = 40% for R14**. The uploaded
`ScopeFix_v1.zip` was 2,869 bytes with SHA-256
`9233DFD8F0A247FD9E56BBE206F49E58D2C46B72A82055FB69B5830B67864601`;
remote tag target `21529521bf3999ae2a0eec52072ff9be9dcd38f8`, prerelease state,
notes, asset state, and server digest were read back. The earlier runtime result
was not repeated.

Routine cost was 17 external calls, two public game/release calls, one premature
implementation-file read, two sandbox-auth retries, and manual release assembly,
publication, and verification. RC3 had already exposed the same publication
gap, so R14 fires the backlog trigger for one manifest-bound public publisher.
This later repair must not rewrite R14.

## R15: Harpoon HC03 native entry/exit candidate

Task `01a0507c-01d0-71e0-a8a0-09348bb3c406`, reported and committed at
`a78baca1` on 2026-09-06 UTC. The owner separated five recurring mechanical
operations: mod-owned build/release production, exact non-installed candidate
inspection, pre-install readiness, manifest-gated installation, and independent
installed-state readback. All five completed through documented public entry
points: **5/5 = 100% for R15**.

The producer's nested build, cook, package, container and release-validation
gates are not counted again. There were zero implementation/dependency reads,
fallbacks, tool failures, or uncovered recurring operations. Two broad-read and
patch-shape mistakes were correctly reported as agent errors rather than
pipeline defects. The user confirmed native Drone entry and exit in game; that
runtime evidence validates HC03 behavior but is not another mechanical tool
operation. See [the complete HC03 report](harpoon-cannon-hc03-tool-report.md).

## R16: DonkLift hint-provider probes and restoration

Task `01a06fe3-b779-7da2-8650-f2fe9213cfa0`, reported 2026-09-06 UTC. This
sample covers the already completed `v2-hint-provider-probe6`, `probe8`, and
final restoration of the game-validated v2; it requested no new experiment or
code change. The owner separated six recurring mechanical operations:

| Intended recurring operation | Actual interface | Coverage |
| --- | --- | --- |
| Inspect installed candidate assets | `Get-VoyageAssetSummary.ps1` | Covered |
| Build, cook, package, and verify candidate | mod-owned `Build-DonkLiftRelease.ps1` | Covered |
| Produce schema-2 release manifest | same documented producer | Covered |
| Readiness, backup, install, and exact readback | same producer's `-Install` route | Covered |
| Independent installed status | `Get-VoyageInstallationStatus.ps1 -Summary` | Covered |
| Restore v2 and remove the experimental sidecar | Manual exact-file PowerShell fallback because the producer left only a local backup, not a common installation manifest | Uncovered |

Reported and pipeline-reviewed coverage: **5/6 = 83.3% for R16**. Probe7's
compile stop came from feature generator/API drift, not a pipeline failure. The
manual restore was triggered only after two expected runtime no-ops; it verified
the exact three restored hashes, removed the exact `.autoload` sidecar, and ran
with the game closed. No tool implementation or dependency source was read for
the fallback. The reusable gap joins the existing installed-footprint transition
queue; it does not justify an immediate common restore tool outside the next
requested DonkLift experiment.

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

The repair iteration reached 80%, followed by R6 at 100%. R7 and R9 each reached
77.8% and exposed defects or missing producers that were subsequently addressed;
R8 reached 100%. The six latest fresh Harpoon samples now independently reach
the 80% target: 92.9%, 88.9%, 100%, 83.3%, 100%, and 100%. This is current
adoption evidence, not a pooled global score or a promise that every future
workflow is covered. ScopeFix publication sample R14 fell to 40% and exposed
three repeated manual release operations, while the later HC03 sample R15
returned to 100% without a new gap. DonkLift sample R16 then reached 83.3%; its
one manual restoration is retained under the existing transition gap rather
than spawning immediate tool work. Continue collecting reports only from actual
in-scope work, and act on demonstrated recurring cost rather than synthetic
chores or marginal score polishing.
