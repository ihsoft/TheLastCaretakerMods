# Research workflow

Research should explain the producer, consumer, owner, lifecycle, identity,
and data contract behind game behavior. A patch that happens to work is not by
itself a complete explanation. This document covers research decisions and
evidence; generated output follows the shared
[artifact lifecycle](artifact-lifecycle-rule.md), and tool commands live in the
[tool index](../tools/tool-index.md).

After context compaction, recover from repository state rather than relying on
chat history. For a large active investigation, first read the backlog's
restart, current-state, and next-action sections. Use targeted searches and
bounded ranges of architecture or pitfall evidence; read a complete large
backlog only when those pointers are insufficient.

## Start with the question and its owner

1. State the behavior or contract that is unknown and identify the mod that
   owns the question. Read its current rules and relevant architecture or
   research notes. Use shared patterns and [research pitfalls](research-pitfalls-knowledge.md)
   only where they address that question; do not read every large document by
   default.
2. Decide which evidence is game-build-specific. For saved game-derived inputs,
   compare their provenance with the installed Steam build ID. A producer that
   checks the build and re-extracts affected stock inputs during its run covers
   this step. General engine behavior instead follows its engine,
   serialization, and structural evidence.
3. Use a reviewed mapping through `Get-VoyageMappings.ps1` when reflection data
   is needed. Generate a new mapping only after the resolver reports that no
   reviewed entry matches a genuinely new Steam build ID; follow the exceptional
   procedure in the [mapping guide](../tools/mappings.md).
4. Before an experiment, state the hypothesis, the one contract or variable to
   change, the distinguishable outcomes, and the decision supported by each
   outcome. If observation can answer the question, an in-game experiment is
   unnecessary.

Keep mod-specific evidence with the owning mod; preserve open hypotheses only
when needed in its ignored backlog. The
[documentation rule](repository-documentation-rule.md) prohibits committing
unverified claims without an explicit user request for those claims. Promote a
conclusion to shared `docs/` only when another mod can reasonably use it;
describe the reusable contract without turning one mod's trial history into a
general rule. Game-derived source inputs retained in Git follow the provenance
requirements in [the game-derived data rules](game-derived-data-rule.md).

## Working backlogs for large tasks

Work history is overhead. Use a backlog only for a large investigation or a
prolonged fix that needs continuity across compaction or work sessions. Keep it
local, with the owning mod, outside `Tmp/` so routine cleanup cannot remove it,
and ignore it in Git. Never commit the backlog or turn it into repository
documentation, including JSON metadata or a narrative of completed checks.

Keep a short restart section with the current checkpoint, established evidence,
pending gates, next action, and links to the owning sources. Include an open
hypothesis only when it affects the next decision or is needed to resume work;
do not log every suspicion or observation.
Update it as the investigation changes; keep raw logs and full readbacks in
their appropriate ignored storage instead of copying them into the backlog.
Research commits must explain their own evidence and validation boundary
without relying on this uncommitted note.

When the work ends, retain only verified contracts that change a future decision,
in their owning documentation or tool. Rewrite them as current constraints;
do not copy the investigation or duplicate its manifests and evidence. Delete
the backlog rather than archiving its chronology in the repository.

## Choose the next investigation step

Choose the smallest observation or experiment that can distinguish the live
hypotheses. Start with the relevant public tool from the
[tool index](../tools/tool-index.md), using the narrowest query and its compact
result. For Blueprint structure, a focused summary is usually cheaper than a
full export; for a specific package, narrow extraction is usually cheaper than
a broad inventory or binary scan. These are examples, not a fixed ladder:
choose by the question and by what the existing evidence already establishes.

Before asking the user to launch or test the game, weigh the information gained
against the time needed to start, observe, and close it. State what each
observable result would change about the next decision. Keep a known-good
baseline for a risky test and change one architectural variable at a time.
After one or two results that do not distinguish the hypotheses, return to
research or improve the observation instead of requesting another similar
gameplay test.

A controlled crash or visibly wrong result can be a useful discriminator when
the game process is disposable and a recoverable baseline exists. Keep its
observation and recovery boundary explicit.

Before a visibly heavy investigation, such as broad extraction, full indexing,
long decompilation, or wide binary scanning, explain its expected cost and
offer a lighter discriminating step if one exists. Escalate only when the
narrower step cannot answer the question or has already failed to do so.

## Record results and make research checkpoints

- Match every conclusion to its evidence and exact applicability. Keep per-run
  evidence in local output; preserve only the decision-relevant contract in
  documentation. Structural inspection, build, cook, and clean load are not
  proof of gameplay behavior; a compatibility claim needs the corresponding
  real-game test.
- Put stable mod-specific findings in its architecture or research notes and
  reusable cross-mod findings in shared documentation. Record an
  implementation constraint in [research pitfalls](research-pitfalls-knowledge.md)
  when it can prevent repeated mistakes, without recounting the failed attempts.
  One-off logs and measurements do not belong in committed guidance.
- Research may use intermediate checkpoint commits that are not intended to
  remain as permanent history. A later authorized operation may revert them or
  combine them into a durable result. Before each checkpoint, show the user its
  concrete scope, evidence, missing gates, and intended temporary or durable
  role, and obtain explicit commit authorization. Permission for one checkpoint
  does not authorize another or its later revert, squash, merge, amendment, or
  replacement.
- A checkpoint may record a useful intermediate fact or a reusable method whose
  claimed capability has been validated at the applicable level. It does not
  turn lower-level evidence into accepted gameplay behavior. Keep mod
  implementation, configuration, and producer changes uncommitted while their
  intended runtime behavior awaits a real-game test, unless the user explicitly
  authorizes that exact unvalidated checkpoint. Keep proposed current-state
  documentation uncommitted until the runtime gate passes.
  A successful build, structural comparison, installation, clean load, or
  other lower gate authorizes the next test, not a commit. State every
  checkpoint's evidence and validation boundary and do not present it as a
  finished mod or release.
- Do not create commits merely to preserve observations. After explicit
  authorization, commit eligible source,
  methods, and documentation through the repository Git workflow; generated
  output follows the [artifact lifecycle](artifact-lifecycle-rule.md).
