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

Keep mod-specific evidence and open questions with the owning mod. Promote a
conclusion to shared `docs/` only when another mod can reasonably use it;
describe the reusable contract without turning one mod's trial history into a
general rule. Game-derived source inputs retained in Git follow the provenance
requirements in [the game-derived data rules](game-derived-data-rule.md).

## Working backlogs for large tasks

Use a backlog when a task is large enough that its current state
could be lost between work sessions or context resets. It is an optional local
working note, not a required file for every question. Keep it with the owning
mod, outside `Tmp/` so routine cleanup cannot remove it, and ignore it in Git.
It is neither a retained generated artifact nor a document to commit.

Keep a short restart section with the current checkpoint, established evidence,
open hypotheses, pending gates, next action, and links to the owning sources.
Update it as the investigation changes; keep raw logs and full readbacks in
their appropriate ignored storage instead of copying them into the backlog.
Research commits must explain their own evidence and validation boundary
without relying on this uncommitted note.

When the task or investigation ends, review the backlog. Move durable conclusions to
the mod's architecture or research documentation, general findings to shared
docs, reusable failed approaches to research pitfalls, and recurring methods
to their owning tools. Then delete the backlog. Do not keep the chronology as
permanent project documentation.

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

- Record a useful conclusion with its evidence, exact applicability, and
  validation level. State what remains unknown. Structural inspection, build,
  cook, and clean load are not proof of gameplay behavior; a compatibility
  claim needs the corresponding real-game test.
- Put stable mod-specific findings in its architecture or research notes and
  reusable cross-mod findings in shared documentation. Record a failed or
  misleading approach in [research pitfalls](research-pitfalls-knowledge.md) when it can
  prevent repeated mistakes. One-off logs and measurements do not belong in
  shared guidance.
- Commit a coherent positive research checkpoint when it establishes a useful
  intermediate fact, method, or validated candidate, even if the investigation
  and feature remain open. Name its evidence and validation boundary; do not
  present it as a finished mod or release. This differs from implementation
  work, where a commit normally completes a logical piece of behavior.
  Combine minor observations at a natural checkpoint rather than committing
  each one. Commit eligible source, methods, and documentation through the
  repository Git workflow; generated output follows the
  [artifact lifecycle](artifact-lifecycle-rule.md).
