# Voyage toolchain active backlog

## Material-library restart / used-only policy (2026-09-16 UTC)

User explicitly rejected speculative dependency archives. Export only images with
a supported use in the exported PBR material. Unknown/ambiguous maps and known
disabled emission stay as paths/reasons in SkippedTextures, not image payloads.
No archive-all option. Preserve only the converted normal actually used, not a
second raw image. Parameter inheritance still informs selection; this is NOT a
claim that the full original Unreal shader has been recovered.

Measured reason/return: previous sample was 211291308 bytes (201.503MiB), of which
193.740MiB was 21 unbound images. Small correction to existing adapter: lazy
decode selected bindings, remove unconditional archive append, schema2 used-variant
provenance, require every image/texture to have a material consumer. Public entry
and call count remain unchanged. Avoided building another tool or reducing mip
resolution. This fixes the actual user-visible size/cost regression.

New sample: artifacts/material-export/socket-library-used-only.glb, 8102532 bytes
(~7.73MiB / 8.10MB), 3 materials / 3 images, 1 textured PBR material, zero decoding
failures. Public PS5.1 export took 5.41s vs 48.35s. SHA256
3375805513AE5C03D8F660F1993D814F22C51D5AB55D7342B91DDB8CFA08835E.
Stock fingerprint/mapping gates passed inside Export-VoyageMaterialsGlb.ps1;
schema2 verify_material_library.py and inspect_glb.py passed, Blender2.80 imported
and rendered. 19 exporter self-checks passed; four in-memory validator corruption
checks reject orphan image/texture, disabled-emission payload and wrong image hash.
No model, game, shared producer or installed-file mutation. Old artifacts retained
as prior diagnostic evidence, not current output. No commit requested.

Tool-use report: intended publish/export/validate/import -> documented Publish.ps1,
Export-VoyageMaterialsGlb.ps1, verify_material_library.py, inspect_glb.py and
preview_glb.py all succeeded without fallback. Source edits addressed the observed
archive-all behavior; no upstream/library investigation or new dependency. Current
supported export is still one call / no implementation reads / compact summary.
Local tool-development evidence, not a new feature-agent adoption percentage.
Remaining limitation is approximate shader reconstruction, not image completeness.

## Superseded initial archive-all checkpoint (2026-09-16 UTC)

Direct user request: list of cooked materials -> one GLB for reuse in authored
models; explicitly prefer existing tools over custom decoding. Shallow upstream
survey found FModel/CUE4Parse material+texture extraction and Unreal editor GLTF
baking, but no verified ready batch-library GLB route for cooked Voyage. Reused
reviewed CUE4Parse inheritance reader and unmodified Conversion/AssetRipper texture
decoder plus SharpGLTF. New public route: tools/Export-VoyageMaterialsGlb.ps1;
contract, limits and one-time publisher in tools/VoyageMaterialLibrary/README.md.
No gameplay, generated assets, installed files or model edits in this work.

Cost/return: the requested recurring workflow otherwise needs per-material
parent/parameter inspection, texture decoding and manual GLB assembly. Target is
ONE public batch call, zero routine implementation reads and one summary instead.
Development required parser/conversion/API inspection because this output was
missing, a private executable+publisher and bounded PBR adapter, not a universal
shader converter. Initial three-material batch produces ~201.5MiB: preserve all
available dependencies, not just selected PBR maps. Further shader-specific work
is deferred until needed on a concrete material, not expanded speculatively.

Verified: existing Get-VoyageAssetJson -ListPackages -AsJson returns 23983 stock
packages; fingerprint and mapping resolvers select Steam25191271/UE5.8. Single
material and three-material JSON-list calls work in Windows PowerShell5.1. Three
socket materials yield 25 embedded images, no texture failures; only ONE material
has inferred PBR textures. Layered trim and glyph material retain dependencies
but import as flat approximations, NOT recovered shaders. SharpGLTF readback,
independent GLB structure/PNG hash/dimension/provenance checks, Blender2.80 import
and render passed. Twelve synthetic checks cover exact input identity, ambiguity,
normal-green convention, factors and disabled emission. Wrong-type mesh and
overwrite attempts fail without creating/replacing GLB. Evidence under ignored
artifacts/material-export; final published run uses socket-library-final.glb.

Development fixes: PS execution policy needed documented Bypass; publishing under
the host identity needed invocation-local Git safe.directory (no global config);
dotnet publish uses RestoreLockedMode MSBuild property, not restore-only CLI flag;
PS5 JSON arrays must be assigned before enumeration. Low emission is represented
in core factors so old Blender need not understand emissive-strength extension.
No changes to canonical/shared parser binaries. Full logs remain in evidence.

Tool-use report: intended list/fingerprint/mappings/extract/validate paths now have
documented entry points; local development exercised them, not a measured feature-
agent adoption sample. Supported export routine: one public call, no implementation
reads, compact summary+report link. Fallback/source reads were limited to the
missing conversion capability and observed publisher/input issues. Remaining gap:
exact procedural/layered/UV-dependent shader appearance and unsupported textures;
reported explicitly, not silently promised or a mandate to implement baking.
Final public batch took 48.35s; SHA256
B60CD07CDCF5C4B09A8794B776EE6B3C03B47A9817FC11D86F98FB930A05949D.

GLB-first model workflow is now a direct user requirement. Keep reusable GLB
inspection/conversion/preview tools and the stock mesh reference reader; extend
texture/material extraction when a real asset request needs it. The current
catalog is tools/glb/README.md. Do not confuse the prior rejection of speculative
model pipeline expansion with a ban on these requested utilities. Harpoon V1
is a local GLB baseline, not yet supported by the old OBJ shell build bridge;
consumer migration belongs to the model/gameplay owners. No full Unreal shader
or texture conversion is currently claimed.

Model-owned socket export, 2026-09-15 UTC: real request narrowed to original
electrical socket geometry with approximate colors. Public fingerprint/mapping
covered 2 operations; raw geometry extraction was an uncovered third (2/3 at entry).
JSON tools omit raw positions/indices. Narrow tools/VoyageMeshReference reader now
tested on one stock socket LOD0, Steam25191271, with 2932 triangles/3 slots and GLB
readback. Blender assembly/render/conversion is source-only, excluded from metric.
No evidence for arbitrary meshes; full materials/textures/Nanite remain deferred.
Do not restore a broad model pipeline. Method/limits in its README. Development
cost: SDK config permission retry, CUE API adjustment, duplicate-face preservation;
no game build/install/shared producer changes. Future adoption remains unmeasured.

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

1. Read root `AGENTS.md`, then this restart section, Current state, and the
   relevant queue row. Use targeted searches for older evidence; do not
   reconstruct state from chat or dump every large linked document.
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

### New-build mapping recovery (Mooring, 2026-09-11 UTC)

Resolved: a repeated capture at the main menu succeeded with the unchanged
canonical dumper. Map 25191271 is validated and registered; both mooring assets
parse. The PowerShell 5.1 process launch defect was corrected and smoke-tested.
Measured recovery: one main-menu generation call succeeded, versus two failed
calls in the original workflow; no Rust rebuild was required. Keep any future
null-pointer failure evidence separate rather than assuming object churn.
Full compact report: `docs/voyage-mapping-25191271-tool-report.md`.

Installed build 25191271 / executable
`747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B`
has no reviewed mapping. User started Voyage. Documented
`New-VoyageMappings.ps1` fails under Windows PowerShell 5.1 because
`ProcessStartInfo.ArgumentList` is unavailable (line 187); the narrow source
read confirmed the launch incompatibility. Retrying under installed PowerShell
7 passes launch but jmap exits 1 with `unexpected null ptr` / out-of-range
integral conversion after warnings about bad GUObjectArray entries. Evidence:
`artifacts/mappings/steam-25191271-747DC2553F7E-jmap-4f88d8a-20260911T030116Z/jmap.stderr.log`.
Neither failed result was promoted. The successful main-menu result resolves
the mapping blocker; rebuilding mod candidates still needs fresh asset inputs.
Repair scope/return: restore the real new-build mapping workflow (currently
zero successes in two public calls), first distinguish live-object churn from
a deterministic decoder failure before changing dumper internals. No tool
Rust implementation change was needed by the Mooring investigation.

Acceptance audit, 2026-09-06 UTC:

- **Black-box adoption:** current Harpoon R15/R17 and DonkLift R16 used
  documented entry points with zero pipeline implementation or dependency
  reads. This criterion is supported for those current workflows, not proven
  forever for every future task.
- **At least 80% coverage:** the seven latest Harpoon samples and DonkLift R16
  each meet the target. Release preparation remains incomplete because ScopeFix
  GitHub publication R14 reached only 2/5 = 40%.
- **Coding-agent reports:** all four current owners have supplied bounded
  reports: Harpoon, DonkLift, ScopeFix, and model work (`0/0`, N/A). HC04 is
  already reported and must not be requested again.
- **Reduced routine effort:** compact summaries, stable binaries, intent-level
  producers, Git serialization, bounded compaction reads, and batched minor
  observations have current evidence. The remaining acceptance proof is a
  future real GitHub release workflow at or above 80% without resurrecting the
  abandoned speculative publisher.

The overall pipeline goal therefore remains active, not complete. Do not fill
the missing release evidence with synthetic work; evaluate and minimally
improve the next analogous user-requested release.

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
- Harpoon HC04 is committed at `10745abb` and recorded as R17: 9/9, zero
  pipeline implementation reads, with user-confirmed mouse look, blocked
  character movement, and exit. HC05 is prepared but not installed while the
  user plays; its current non-overlapping repository-only continuation is 3/3
  and remains pending the owning checkpoint. Do not request either report again.
- The model owner supplied its latest report: source-only blockout/render work
  was 0/0, N/A, with no game import/cook/package/install operation. The old
  untracked pipeline-owned `VoyageModel` export/render/test prototypes had no
  real-task adoption and were removed after source modeling left the coverage
  scope. Do not recreate them to improve this metric.
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
| Partial/obsolete or legacy installed-footprint transition | C2 retirement and C7 sidecar-only addition required bounded manual mutations because the common installer owns complete releases; DonkLift R16 manually restored a local backup and removed `.autoload` because its producer had no common installation manifest; HC05 needed one exact `Test-Path` because compact status did not expose `.autoload` removal after four public restores | Another authorized workflow requires the same non-common add/remove/restore transition or compact removal proof | One manifest-bound transaction or owning-producer migration replacing repeated manual file-set mutation while preserving rollback evidence; implement only in that live workflow |
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

### Inspection output: deferred verification

These are historical observations to reproduce only when the next real workflow
needs them, not confirmed defects in today's binaries. No implementation is
active.

| Question | Next useful check | Continue only if |
| --- | --- | --- |
| Does the compact Blueprint summary preserve native Path/ResolvedOwner references and WidgetBlueprintGeneratedClass identity? | Compare a focused current summary with the public full JSON for the same fingerprinted asset; use the horizontal input widget for class identity | A current consumer still gets a misleading omission; fix only that projection and regression-test it |
| Does native target-VA output distinguish string match count from address references? | Compare current compact counters with its returned reference report for one known address | The ambiguity still affects a real decision; clarify the existing result contract, not a new disassembler |

Full JSON is the existing fallback for summary omissions. Native reference
reports, not string-match counts alone, establish whether an address was found.
Stale UHT package flags and unsupported one-off config retrieval remain findings,
not pending experiments or automatic requests to extend the toolchain.

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

- Unreal producer sandbox launch boundary: repeated real Harpoon preparation
  again entered the restricted sandbox and failed before UBT initialized with
  the known `dotnet.exe` dialog / `-532462766` signature. The prohibition now
  lives in root `AGENTS.md` and is explicit in `tools/README.md`: request
  outside-sandbox execution on the producer's first invocation, not after a
  probe fails. Two BoatHUD-local copies were removed after a repository-wide
  rule search confirmed they were the only mod-specific duplicates.
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
- Current adoption ledger through Harpoon R17 lives in
  `docs/voyage-toolchain-coverage.md`.
- Compaction restart now uses compact status/next-action sections followed by
  targeted searches. HC03, HC04, and the pipeline owner each reproduced costly
  broad reads; the pipeline instance alone returned about 14k tokens before a
  narrow search supplied the needed evidence.

When a triggered item is completed, promote its stable contract to the owning
document and remove it from this file. Preserve detailed history in Git and the
existing bounded reports, not in the active reading path.
