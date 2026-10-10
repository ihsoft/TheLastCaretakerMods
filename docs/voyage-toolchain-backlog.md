# Voyage toolchain active backlog

This file contains only unresolved cross-cutting pipeline work. Current public
interfaces live in [`tools/README.md`](../tools/README.md), accepted binary and
cooked-asset contracts in
[`voyage-cooked-asset-toolchain.md`](voyage-cooked-asset-toolchain.md), and
real-task measurements in
[`voyage-toolchain-coverage.md`](voyage-toolchain-coverage.md). Mod-specific
work belongs to the owning mod documentation.

## Current work / restart

The Railgun public build now keeps build staging and service reports below
repo-local `Tmp/Railgun/`, retains one exact artifact-versioned release ZIP in
the game's Paks directory after successful installation, and keeps only common
installation/restore evidence below `artifacts/`. The generic installer has a
narrow opt-in restore action that hash-validates but retains this canonical ZIP;
legacy manifests without the field retain their previous behavior. Both
Windows PowerShell 5.1 synthetic modes passed. `build-20261004-093343` completed
the full build/install pipeline: its TMP was removed, its ZIP has exactly the
documented five entries, installed hashes match, the user INI was unchanged,
and Restore ValidateOnly still reports four restores, no removals and one
retained archive after TMP deletion. This validates packaging and recovery
mechanics, not runtime gameplay.

The authorized Railgun release-duplicate cleanup is complete. Paks retains the
verified versioned ZIPs `073326`, game-validated `084803`, and current packaging
candidate `093343`. Installation evidence, backups and migrations remain
durable; there are no retained Railgun release directories below
`artifacts/railgun` and no failed Railgun build TMP. The owner intentionally did
not retain `065619`; do not reconstruct that archive from an installed triplet
or rewrite immutable manifests. Private caches, `.tools/bin`, DDC, reusable
Game summaries, real material exports/requests and unrelated installation
evidence remain outside cleanup.

## Prioritization gate

### Subagent coordination rule adoption

User-requested consolidation addresses repeated missing coder reports and
advance game-closed questions. Root `AGENTS.md` now routes delegation through
`docs/subagent-workflow.md`: one initial two-way handshake, explicit role/stage
ownership, automatic parent reports, and installer-owned process checks.
Expected delta: no user report-relay/reminder or advance game-status question
per assignment, at the cost of one handshake per connection and maintenance of
one shared protocol; no scheduler, new tool or global configuration is needed.
Validation: document/link review and whitespace checks passed; the active
coder received the rules and returned `RULES_ACK repo-subagents-20261009`,
confirming the parent-report route, review hold and direct game-process gate.
A clean new-agent startup under this complete protocol remains the next
adoption check; do not claim it has been tested merely because an existing
agent can exchange messages. Remove this adoption note once that check passes;
the stable contract belongs in the root rules and linked workflow.

### Pipeline changes

Before implementing a backlog item, record:

1. the recurring cost or correctness risk already observed;
2. the requested workflow that needs it;
3. the expected measurable change in calls, source reads, retries, output size,
   coverage, recoverability or false acceptance;
4. implementation, debugging, maintenance and caller-context cost versus the
   expected cumulative savings; and
5. the smallest discriminating validation and a stop condition.

Defer the item when the operation is a one-off, its trigger has not occurred,
or its expected return no longer exceeds implementation and maintenance cost.
Tool count and 100% coverage are not goals. Accumulate low-impact observations
until they recur or create a correctness, safety or provenance risk.

## Triggered work queue

| Gap | Current evidence | Start only when | Expected return |
| --- | --- | --- | --- |
| Interrupted installer recovery | Completed installs restore through the public command; a `recovery-failed` transaction retains staging but has no normal recovery entry point | A real interrupted transaction needs recovery, or the same manual recovery recurs | Recover or roll back from retained evidence without hand-editing installed files |
| Partial or obsolete installed-footprint transition | Sidecar-only and local-backup transitions fall outside the common complete-release installer | Another authorized workflow requires the same non-common add/remove/restore transition | One manifest-bound transaction or an owning-producer migration with recoverable exact-file mutation |
| Semantic equivalence of independent Unreal rebuilds | Stable identities and reviewed structure can coexist with changed package hashes | Hash churn blocks acceptance of another otherwise equivalent rebuild | One bounded equivalence report instead of ambiguous binary comparisons |
| Winning provider for duplicate virtual paths | Exact Game and exact Mod inspection are isolated and do not prove which duplicate provider wins at runtime | A release decision depends on provider order | One explicit winning-container result without removing mods or inferring precedence |
| Native reflected owner/function/call relation | Name and offset correlation cannot by itself prove ownership or call semantics | A second real task needs the relation and asset summaries cannot answer it | A narrow evidence-bearing relation query, only after a reliable source is demonstrated |
| Checkpoint-aware fork candidate build | Intentional UAssetAPI, retoc and GUI development must not replace accepted canonical binaries before review | Another fork candidate is requested before an accepted checkpoint exists | One bounded candidate-build call that leaves canonical binaries untouched |
| DmlAssetRegistryProbe release producer | Repeated probe iterations manually performed UE build, generation, cook and package assembly | That probe family receives another authorized iteration | A probe-owned producer; do not create a generic builder |
| UE5.8 AssetRegistry metadata reader | Existing readers provide partial metadata but not a supported bounded projection of group/chunk data | Another requested experiment depends on that metadata | Explicit partial/failure semantics instead of another forced-scan probe |
| Compressed SaveGame marker readback | One probe required a bounded ad-hoc Unreal-v2/zlib decoder | A second real task needs the same marker contract | One documented bounded reader instead of re-deriving compression and terminators |
| GitHub release preparation, publication and readback | The current measured release-publication sample covers 2/5 operations; immutable staging, publication and readback were manual | The next analogous release is requested and the remaining uses still repay implementation | The smallest safe reduction in repeated manual publication work; stop if a generic publisher does not pay back |
| Historical non-indexed package recovery | One legacy package required retoc plus partial UAssetAPI/CUE4Parse fallbacks because dependencies were absent | Another historical mod blocks a requested migration | Determine from a second sample whether a narrow recovery interface is reliable |
| Stock material shader fidelity | The public exporters preserve supported PBR bindings and report skipped texture/effect identities; procedural rust, damage and layered shader behavior remain approximations | A concrete requested asset needs one of those missing effects | Evidence-backed per-material baking or authored replacement, not speculative dependency archives |

## Deferred metadata decisions

- Full JSON is the fallback when a compact Blueprint summary omits a field. If
  a current consumer still receives a misleading omission, compare one focused
  summary with the public full JSON and fix only that projection.
- Native target-VA reference reports, not string-match counts alone, establish
  whether an address was found. Clarify the existing compact result only if the
  ambiguity affects another real decision.
- Voyage's user-facing display version has no proven local machine-readable
  source. Keep it nullable and distinct from Steam build, executable hash, UE
  version, mod version and artifact version until a release actually needs it.
- A shared compatibility/version schema needs an owning mod and release need;
  do not migrate existing metadata opportunistically.
- Build an all-binaries health check only if repeated individual manifest
  verification becomes a measured cost. Existing resolvers already validate
  their canonical binaries.

When a triggered item is completed, promote its stable contract to the owning
document and remove the row. Detailed execution evidence remains ignored under
`artifacts/` or in Git history, not in this active reading path.
