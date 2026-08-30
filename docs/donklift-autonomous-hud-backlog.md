# DonkLift active backlog

There is no active DonkLift research experiment.

The autonomous single-container `DonkLiftKeyboardControl` result was fully
validated in the game on 2026-08-29: throttle and steering in both directions,
limits, native percentages, immediate X/C, localized standard `E H X C`
hints, pause behavior, exit, and re-entry. Clean editor regeneration, six
narrow cooks, single-container assembly, and `retoc verify` also succeeded
after the final source/module cleanup.

Keep the branch `experiment/donklift-iostore-throttle` until the user
separately authorizes its deletion. Durable findings are in
`game-architecture-observations.md`; failed and deceptive approaches are in
`research-pitfalls.md`; reproducible extraction/build rules are in
`research-workflow.md` and the mod-local documentation.

## Compaction rule

After compaction, reread this entire file and the relevant permanent docs
before acting. While the user is playing, repository-only research and edits
are allowed; installed game files are not.
