# DonkLift active backlog

There is no active DonkLift experiment.

## Completed native-HUD cleanup

The exact lifecycle regression was reproduced in the real game: initial
forklift entry showed `E H X C`, while ESC/resume repopulated the same native
row as `C E H X`. The row is backed by a native `TSet`, so its order is not a
stable presentation contract.

The user accepted the game's native ordering. The production mod must keep X/C
as standard localized actions but must not replace the forklift HUD, reorder
its children, or poll it solely for presentation. The HUD generator, mirrored
widget types, HUD extraction, relocation, cook, and packaged replacement have
therefore been removed.

A simplified six-asset container (five generated/cooked assets plus a clean
same-build relocated original forklift) was built, verified, backed up, and
installed on 2026-08-29. Installed hashes match the prepared container:

- `pak`: `75E7144577253917F6DA7312EF5E585B12FB728226A22B0938323751A6B555CD`
- `ucas`: `BFFF2851767625350063FC97457FC7FD577B12F1263A3434EF603BBD201ECD8A`
- `utoc`: `C35D10BC5297DA9238CE758F42C43356A0D2239CD2A41D7941F03F0CE84BD301`

Fresh extraction was deliberately not forced because the unrelated installed
`HarpoonCannonPlacementProbe_P` container tripped the shadowing guard. The
builder instead consumed the most recent clean forklift extraction whose
manifest proves the same current Steam build/executable fingerprint and no
additional containers. The current game fingerprint was independently checked
again before test handoff.

The simplified package passed real-game validation on 2026-08-29: controls,
X/C behavior, standard localized hints, ESC/resume, exit, and re-entry all
work. Native hint order is explicitly not a validation requirement. This is
the current production checkpoint and publication may continue from it.

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

The corrected candidate was validated in the real game on 2026-08-29 and was
the gameplay checkpoint before the native-HUD cleanup above superseded it. The
packaging tool now rejects an alleged original that already references the
relocated-parent path, and the extraction tool rejects a filter that produces
zero assets.

Repository-only hardening was then completed. New
extraction manifests record additional-container state; the builder requires
 canonical filter, one validated fingerprint, matching extraction provenance
 and `scriptobjects.bin`, the relocation self-parent postconditions, and the
 exact six-asset output inventory. `Prepare-DonkLiftOriginals.ps1` owns the
 closed-game check, exact additional-container allowlist, temporary installed
 UTOC disable, one canonical extraction, and guaranteed UTOC restoration. Its
 running-game rejection and all script parsers were tested, followed by a fresh
 positive end-to-end extraction/package run. Independent `retoc` builds used a
 different physical chunk order, but their sorted semantic
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
