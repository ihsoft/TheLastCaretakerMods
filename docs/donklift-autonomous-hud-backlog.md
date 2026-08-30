# DonkLift active backlog

There is no active DonkLift research experiment.

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
