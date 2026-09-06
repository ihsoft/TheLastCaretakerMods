# Voyage toolchain active backlog

This file contains only unresolved cross-cutting pipeline work. Current public
interfaces and commands live in [`tools/README.md`](../tools/README.md), accepted
binary and cooked-asset contracts in
[`voyage-cooked-asset-toolchain.md`](voyage-cooked-asset-toolchain.md), and
real-task measurements in
[`voyage-toolchain-coverage.md`](voyage-toolchain-coverage.md). Mod-specific
research belongs to the owning mod backlog.

## Restart context after compaction

The pipeline goal is to reduce agent effort on ordinary Voyage game-asset and
release-preparation work. Public tools and black-box use are means; the measured
outcome is fewer calls, zero routine implementation reads, compact outputs,
reliable mutations, and at least 80% coverage in real coding-agent reports.

Resume in this order:

1. Read root `AGENTS.md`, then this file. Do not reconstruct state from chat.
2. Route the requested intent through `tools/README.md`; do not survey scripts or
   `.tools/` source checkouts first.
3. Resolve mappings and canonical binaries through their public resolvers. Do
   not rebuild or regenerate because a task merely needs them.
4. Preserve unrelated dirty work and use the repository Git semaphore for every
   index/HEAD/ref mutation.
5. At the next real coding-agent handoff, verify its structured report, add the
   bounded sample to the coverage ledger, and update a gap below only when the
   report supplies new evidence.
6. Implement a pipeline change only when a queue trigger has fired and the
   expected material return is stated before work begins.
7. Accumulate low-impact one-off observations and review them together at a
   natural report/commit checkpoint; do not interrupt agents or produce one
   rule, tool change, or commit per small remark.

Current accepted routing model:

- fingerprint/mappings: `Get-VoyageBuildFingerprint.ps1` and
  `Get-VoyageMappings.ps1`;
- stock inventory/JSON: `Get-VoyageAssetJson.ps1`, default `Game` source;
- compact Blueprint questions: `Get-VoyageAssetSummary.ps1`, then exact
  `Inspect-VoyageAsset.ps1` only when needed;
- one non-installed candidate: explicit `Inspect-VoyageAsset.ps1 -Source Mod
  -ModContainer <exact .utoc>`; this does not prove combined provider order;
- already-built release manifest, validation/install/restore/status:
  `New-VoyageReleaseManifest.ps1`, `Install-VoyageRelease.ps1`,
  `Restore-VoyageReleaseInstallation.ps1`, and
  `Get-VoyageInstallationStatus.ps1`;
- normal binary use: manifest-validated `.tools/bin/` outputs; publishers run
  only after an intentional accepted fork/source change.

There is no active common-tool implementation. R14 remains a deferred gap, not
an instruction to resume the abandoned publisher prototype. Stable checkpoints
include compact summary `f8451a04`, exact external candidate inspection
`74dc53c2`, Git serialization `42e49871`, and the adoption ledger through
`21529521` before R14.

## Current state

- The six latest bounded real Harpoon samples independently reached 92.9%,
  88.9%, 100%, 83.3%, 100%, and 100% public-tool coverage. HC03 supplied the
  newest 5/5 sample with no implementation reads or uncovered recurring
  operation. Do not pool them into a synthetic global percentage or rewrite
  older samples after later fixes.
- ScopeFix v1 publication R14 then reached 2/5 = 40%: fingerprint and manifest
  production were covered, while public-root preparation, GitHub publication,
  and remote readback were manual. It used 17 external calls and one premature
  implementation read. RC3 supplies the earlier occurrence of the same gap.
- DonkLift hint-provider probes/restoration R16 reached 5/6 = 83.3%. Inspection,
  production, manifest, installation and status used public routes; restoring
  the previous v2 plus removing the experimental sidecar was manual because the
  mod-owned producer retained only a local backup, not a common install manifest.
- Coding-agent reports remain mandatory. Measure intended recurring operations,
  public calls, implementation/dependency reads, retries caused by the interface,
  oversized output, validation, and reusable gaps.
- Normal work selects one intent-level public entry point from `tools/README.md`,
  consumes its compact result and returned evidence paths, and does not inspect
  source on a successful supported path.
- Git-state mutations are serialized through the `git-transaction` semaphore;
  normal commits use `tools/commit-repository-changes.ps1` with exact files.
- There is no active speculative common-tool implementation. R14 proved a real
  GitHub publication gap, but its synthetic publisher harness exceeded the
  break-even budget before becoming reliable. Resume only inside the next
  analogous requested release, using the smallest live-workflow improvement.

## Prioritization gate

Before implementing a backlog item, record:

1. the real recurring cost or correctness risk already observed;
2. the next requested workflow that needs it;
3. the expected measurable change in calls, source reads, retries, output size,
   coverage, recoverability, or false acceptance;
4. the expected implementation/debug/maintenance and caller token cost versus
   cumulative savings across realistically expected uses;
5. the smallest discriminating validation and a stop condition if it expands.

Defer the item when the expected return is marginal, the operation is a one-off,
the trigger has not occurred, or validation/repair cost crosses its break-even
budget. Tool count and 100% coverage are not goals.

Minor observations stay in their owning report until repeated by an independent
real-task iteration. Promote them into this queue only on repetition, blockage,
or a correctness, safety, or provenance risk.

## Triggered work queue

| Gap | Existing evidence | Start only when | Material expected return |
| --- | --- | --- | --- |
| Interrupted installer recovery | Installer fails closed and retains staging when a transaction reports `recovery-failed`; completed installs already restore through the public command | A real interrupted transaction must be recovered, or the same manual recovery recurs | Recover or roll back from retained evidence without hand-editing installed files |
| Partial/obsolete or legacy installed-footprint transition | C2 retirement and C7 sidecar-only addition required bounded manual mutations because the common installer owns complete releases; DonkLift R16 manually restored a local backup and removed `.autoload` because its producer had no common installation manifest | Another authorized release or DonkLift experiment requires the same non-common add/remove/restore transition | One manifest-bound transaction or owning-producer migration replacing repeated manual file-set mutation while preserving rollback evidence; implement only in that live workflow |
| Semantic equivalence of independent Unreal rebuilds | DonkLift rebuilt five packages with different hashes despite stable package identities, sizes and key pseudocode | Hash churn blocks acceptance of another otherwise equivalent rebuild | Replace five ambiguous binary comparisons and an unnecessary canary decision with one bounded equivalence report |
| Winning provider for duplicate virtual paths | Exact Game and exact Mod inspection are isolated; combined views still do not prove which duplicate provider wins | A release decision actually depends on runtime provider order and exact-source isolation is insufficient | One explicit winning-container result instead of removing mods or inferring precedence |
| Native reflected owner/function/call relation | R7's executable correlation returned names/offsets but could not prove persistence ownership | A second real task needs the same relation and asset summaries cannot answer it | A narrow evidence-bearing relation query instead of broad executable-output correlation; only if a reliable data source is first proven |
| Checkpoint-aware fork candidate build | Intentional UAssetAPI/retoc/GUI candidate development used direct SDK commands in more than one repair iteration; canonical publishers correctly reject dirty/uncommitted sources | Another fork candidate must be built before an accepted checkpoint exists | One bounded candidate-build call replacing repeated restore/build/output discovery while leaving canonical binaries untouched |
| DmlAssetRegistryProbe release producer | Test5 and Test6 each manually performed UE build, generation, cook and package/manifest assembly | That probe family receives another authorized iteration | A probe-owned one-command producer covering the four repeated preparation operations; do not create a broad common builder |
| UE5.8.2 AssetRegistry metadata reader | retoc rejects the cooked registry and CUE4Parse exposed useful entries before a serializer exception in repeated DML probes | Another requested experiment depends on registry group/chunk metadata rather than ordinary package inventory | One bounded metadata projection with explicit partial/failure semantics instead of another forced-scan probe |
| Compressed SaveGame marker readback | One probe used a bounded ad-hoc Unreal-v2/zlib decoder | A second real task needs the same marker contract | One documented bounded readback instead of re-deriving compression and terminator checks |
| GitHub release preparation/publication/readback | RC3 and ScopeFix v1 both used manual staging plus `gh`; R14 covered only 2/5 operations and used 17 external calls. A synthetic publisher was abandoned because its harness/debug cost crossed the expected break-even budget before reliable validation | The next analogous release is actually requested; improve that live workflow only if the remaining expected uses still repay implementation and maintenance | Smallest safe intent-level reduction in manual staging/publication/readback; stop rather than build a generic publisher if live evidence does not justify it |
| Historical non-indexed package recovery | ScopeFix UE5.7 recovery required retoc plus partial UAssetAPI/CUE4Parse fallbacks because dependencies were absent | Another non-indexed historical mod blocks a requested migration | Decide from a second sample whether a narrow property-recovery interface is reliable; do not promise general legacy support |

## Deferred metadata decisions

- Voyage's user-facing display version has no proven local machine-readable
  source. Keep it nullable and distinct from Steam build, executable hash, UE
  version, mod version, and artifact version. Investigate only when a release
  requires that field.
- A shared compatibility/version schema may be useful, but migration needs an
  owning mod and release need. Do not rewrite existing metadata opportunistically.
- A single all-binaries health check is justified only if repeated manual
  manifest verification becomes a reported cost. Individual resolvers already
  validate their canonical binaries.

## Recently closed; do not reopen without new evidence

- Compact Blueprint summary and exact focused queries: `f8451a04`.
- Exact non-installed Mod candidate inspection: `74dc53c2`.
- Common schema-2 release-manifest producer and manifest-gated install/restore:
  documented in `tools/README.md` and the cooked-asset toolchain checkpoint.
- Canonical manifest-validated binaries for retoc, jmap, UAssetAPI, CUE4Parse,
  UAssetGUI, VoyageAssetInspector, VoyageAssetPatcher, and
  VoyageExecutableInspector: current identities live in the cooked-asset
  toolchain document and sibling ignored publish manifests.
- Shared-index/ref race: Git mutation semaphore and exact-path commit wrapper in
  `42e49871`; concurrency, occupied-index rejection, and real Harpoon recovery
  commit were validated.
- Current adoption ledger through HC02: `21529521`.

When a triggered item is completed, promote its stable contract to the owning
document and remove it from this file. Preserve detailed history in Git and the
existing bounded reports, not in the active reading path.
