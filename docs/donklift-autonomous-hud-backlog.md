# DonkLift active backlog

There is no active DonkLift research experiment.

## Validated checkpoint and packaging regression

The first package made from commit `10946e4` crashed immediately with
`EXCEPTION_STACK_OVERFLOW`. It had accidentally relocated an extracted copy of
our child forklift instead of the original game Blueprint, creating a
self-parent cycle. The previous validated package was restored before the
cause was investigated.

The original forklift and HUD were then freshly extracted from the base game
with the installed override temporarily disabled. A corrected package was
built and verified with `retoc`, and its prepared/installed hashes match:

- `pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `ucas`: `A27E52428ACB7F738B5D5CEE00042981D208EBA39E598E1BE35DFA8C2E6390F1`
- `utoc`: `8EC4A80AE05F20FE68CC4588E1D4B2D1A6EBAD3D26249AA1756F50A282E90D9D`

The corrected candidate was validated in the real game on 2026-08-29. It is
the current gameplay checkpoint. The packaging tool now rejects an alleged
original that already references the relocated-parent path, and the extraction
tool rejects a filter that produces zero assets.

Repository-only hardening was then completed. New
extraction manifests record additional-container state; the builder requires
canonical filters, one validated fingerprint, matching extraction provenance
and `scriptobjects.bin`, the relocation self-parent postconditions, and the
exact eight-asset output inventory. `Prepare-DonkLiftOriginals.ps1` owns the
closed-game check, exact additional-container allowlist, temporary installed
UTOC disable, two canonical extractions, and guaranteed UTOC restoration. Its
running-game rejection and all script parsers were tested, followed by a fresh
positive end-to-end extraction/package run. Independent `retoc` builds used a
different physical chunk order, but their sorted eight-asset semantic
inventories matched exactly; the builder now emits that deterministic sidecar.

The running game memory-maps the installed `.ucas` and Windows refuses an
in-place replacement. Repository work may continue while the user plays, but
the complete container can only be installed after both Voyage processes have
closed.

The exit-input regression fix was validated in the game on 2026-08-29. Each
helper now targets its exact parent forklift, integrates only while that pawn
is player-controlled, and clears native input plus internal state otherwise.
Throttle stops when the player exits. The known visual wheel-pose limitation
remains accepted: clearing steering input does not physically recenter wheels
that were already turned.

The original autonomous experiment branch and the exit-reset fix branch must
remain until the user separately authorizes deletion. Durable findings are in
`game-architecture-observations.md`; failed and deceptive approaches are in
`research-pitfalls.md`; reproducible extraction/build rules are in
`research-workflow.md` and the mod-local documentation.

## Compaction rule

After compaction, reread this entire file and the relevant permanent docs
before acting. While the user is playing, repository-only research and edits
are allowed; installed game files are not.
