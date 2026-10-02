# Voyage toolchain adoption measurement

This document records the current bounded baseline for recurring mechanical
Voyage game-asset and release work. It is not a gameplay-validation registry
and it does not count synthetic tool-development tests as adoption.

## Measurement contract

- Scope is game fingerprinting and mappings, asset retrieval, inspection and
  modification, build/cook/package, release verification, installation and
  restoration. Source-only modeling, images/video, reasoning, documentation
  and genuinely novel diagnosis are excluded.
- Count one intended recurring operation once, including retries and diagnostic
  variants. Do not count every internal phase or assertion of a producer.
- A maintained wrapper or documented canonical CLI invocation counts as a
  public entry point. An ad-hoc replacement is uncovered even when it invokes
  the same underlying binary.
- A documented tool that completes the operation after a retry remains covered;
  record avoidable retries, implementation reads and oversized output as cost.
- Unknown historical operations remain unknown. An empty denominator is N/A,
  not 100%.
- Keep bounded task samples separate. Do not pool unrelated workflows into a
  repository-wide percentage or rewrite an old sample after a later repair.
- The target is at least 80% coverage in each representative real workflow,
  with zero routine implementation/dependency reads on a supported path.

## Current bounded baseline

| Workflow sample | Covered | Eligible | Result |
| --- | ---: | ---: | ---: |
| Generic Fabricator model/material export | 5 | 5 | 100% |
| Harpoon current-stock restart and inspection | 13 | 14 | 92.9% |
| Harpoon first preparation | 8 | 9 | 88.9% |
| Harpoon first installation | 3 | 3 | 100% |
| Harpoon second preparation | 5 | 6 | 83.3% |
| Harpoon second installation | 3 | 3 | 100% |
| Harpoon native entry/exit preparation and installation | 5 | 5 | 100% |
| Harpoon occupied-root physics discriminator | 9 | 9 | 100% |
| DonkLift hint-provider inspection, production and restoration | 5 | 6 | 83.3% |
| ScopeFix GitHub release publication | 2 | 5 | 40% |

The covered Fabricator operations were stock catalog lookup, compact Blueprint
component summary, exact Blueprint JSON retrieval, generic model/material GLB
export and structural GLB validation. Its Blender preview was source-artifact
visual QA and therefore outside the denominator.

The Harpoon samples demonstrate the supported fingerprint, mapping, compact
inspection, exact candidate inspection, mod-owned producer, release manifest,
installation and readback routes. The DonkLift sample's uncovered operation was
restoring a local predecessor and removing an experimental sidecar without a
common installation manifest.

The ScopeFix sample is the current below-target release-publication baseline:
fingerprinting and schema-2 manifest production were covered, while immutable
public-root preparation, remote publication and remote readback were manual.
The same family of manual work also occurred during the toolchain RC3 release,
so the gap is real, but implementation remains deferred until another requested
release can justify and validate the smallest useful improvement.

## Interpretation

The representative game-asset, preparation and installation workflows meet the
80% target independently. GitHub publication does not. This is evidence about
the measured workflows, not a promise that every future task is covered.

Collect another sample only from actual requested work. Record intended
operations, public entry points, retries, implementation reads, validation and
reusable gaps in the coding agent's normal tool-use report. Do not request
duplicate reports or invent synthetic work to improve the percentage. Active
cross-cutting gaps and their start conditions belong in
[`voyage-toolchain-backlog.md`](voyage-toolchain-backlog.md).
