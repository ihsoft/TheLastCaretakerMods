# Measuring agent work on explicit request

Use this method only when the user explicitly requests process diagnostics.
It is not a default reporting requirement. Keep measurement cheaper than the
decision it informs; reuse available counters rather than building telemetry.
Ordinary delegation decisions follow [the subagent workflow](subagent-workflow-rule.md).

## Scope and attribution

Define the task boundary, participating agents and question before measuring:
total resource use, useful review, context isolation, or delegation overhead.
Record actual model/effort from session metadata when available, not an agent's
self-description. Inspect only the relevant sessions and select metadata fields;
do not load entire transcripts, instructions or compiler logs for accounting.

Separate research, implementation/debugging, independent review, coordination,
and measurement overhead. Track scope changes separately from defect correction.
Role totals are not phase totals: a parent also coordinates, reviews and waits;
a coder also reads context, validates and reports. If exact phase attribution
is unavailable, report combined totals and label time estimates as estimates.

Use cumulative counter differences at explicit start/end boundaries. Retain
baseline and final timestamps, including any reporting excluded by the cutoff.
For elapsed time use the task's wall-clock interval. Parallel agent intervals
overlap and must not be added as task duration. Identify build waits separately;
wall time is not active inference or CPU time.

## Minimal useful measurements

- Input tokens, cached input subset, output tokens, and optional reasoning subset.
- Tool calls with a stated counting level; include waits and both ordinary and
  freeform calls. Do not compare top-level batches with nested operations.
- Retries and their causes, including measurement/reporting failures.
- Inter-agent message volume versus tool output kept in the child, plus source
  or artifact evidence independently read by the parent.
- Initial/peak request context and a rough phase-time breakdown when available.

Summed input includes repeated context; it is not unique information. Input
minus cached can include reread context after cache misses. Reasoning tokens
are part of output, not an additional total. Character counts are not tokens.
A fresh child still receives shared instructions/tools; isolation is not free.
Do not infer money or active compute from these counters without applicable
billing evidence. Missing counters remain unknown, not zero.

## Assessing benefit

For each substantial parent recheck, record what was inspected, why, approximate
effort, and whether it found a defect, confirmed a claim, or duplicated work.
Do not rerun builds or reread logs solely to strengthen a measurement. A defect
found before the coder finishes does not prove its final checks would miss it.

Evaluate context isolation separately from total cost. Reduced compiler noise
in the parent can coexist with increased combined tokens, duplicated discovery,
and expensive handoffs. A single delegated run cannot establish savings over
direct execution without a comparable baseline. Counterfactual cost ranges must
be labelled estimates and must account for differences in scope and retries.
Use the result to choose work boundaries, not to mandate delegation or telemetry.

## Reporting and retention

Give the requested results in the task response, with scope, exclusions and
uncertainty. Disposable scripts, samples and detailed accounting belong in
repository-local `Tmp/`, not shared documentation. Do not create a durable
results ledger unless the user requests one or a concrete continuing consumer
justifies it under the artifact policy. Promote only reusable conclusions to
the owning guidance; keep mod contracts with their mod. Do not embed session
IDs, per-run token tables, retry diaries or completed-task histories in `docs/`.
