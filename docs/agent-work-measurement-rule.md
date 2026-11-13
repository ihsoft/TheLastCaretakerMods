# Measuring agent work on explicit request

Use this method only when the user explicitly requests process diagnostics.
It is not a default reporting requirement. Keep measurement cheaper than the
decision it informs; reuse available counters rather than building telemetry.
For an explicit session boundary, use the focused
[agent work measurement tool](../tools/agent-work-measurement.md).

## Scope and attribution

Define the task boundary, relevant sessions and question before measuring:
total resource use, useful review, repeated work, or workflow overhead.
Record actual model/effort from session metadata when available, not an agent's
self-description. Inspect only the relevant sessions and select metadata fields;
do not load entire transcripts, instructions or compiler logs for accounting.

Separate research, implementation/debugging, independent review, coordination,
and measurement overhead. Track scope changes separately from defect correction.
Session totals are not phase totals: a session can include reading, editing,
validation and waiting. If exact phase attribution is unavailable, report
combined totals and label time estimates as estimates.

Use cumulative counter differences at explicit start/end boundaries. Retain
baseline and final timestamps, including any reporting excluded by the cutoff.
For elapsed time use the task's wall-clock interval. Do not add overlapping
session intervals as task duration. Identify build waits separately;
wall time is not active inference or CPU time.

## Minimal useful measurements

- Input tokens, cached input subset, output tokens, and optional reasoning subset.
- Tool calls with a stated counting level; include waits and both ordinary and
  freeform calls. Do not compare top-level batches with nested operations.
- Retries and their causes, including measurement/reporting failures.
- Tool output volume and repeated source or artifact inspection.
- Initial/peak request context and a rough phase-time breakdown when available.

Summed input includes repeated context; it is not unique information. Input
minus cached can include reread context after cache misses. Reasoning tokens
are part of output, not an additional total. Character counts are not tokens.
Do not infer money or active compute from these counters without applicable
billing evidence. Missing counters remain unknown, not zero.

## Assessing benefit

For each substantial recheck, record what was inspected, why, approximate
effort, and whether it found a defect, confirmed a claim, or duplicated work.
Do not rerun builds or reread logs solely to strengthen a measurement.

Evaluate context reduction separately from total cost. Less visible output
does not establish lower total resource use. A workflow change needs a comparable
baseline before claiming savings. Counterfactual cost ranges must be labelled
estimates and account for differences in scope and retries.

## Reporting and retention

Give the requested results in the task response, with scope, exclusions and
uncertainty. Disposable scripts, samples and detailed accounting belong in
repository-local `Tmp/`, not shared documentation. Do not create a durable
results ledger unless the user requests one or a concrete continuing consumer
justifies it under the artifact policy. Promote only reusable conclusions to
the owning guidance; keep mod contracts with their mod. Do not embed session
IDs, per-run token tables, retry diaries or completed-task histories in `docs/`.
