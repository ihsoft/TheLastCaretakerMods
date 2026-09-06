# HC03: native Drone entry/exit probe

Starts from committed HC02 (245710c) after repository coordination pause release.
Feature scope: remove observer-triggered destruction and observe current/ever
Drone possession and return to the original pawn. Stock creation and all native
input/camera/actions remain untouched. No model integration or new general tool.

## Tool use and validation

Prepared `artifacts/harpoon-cannon/hc03-candidate-01` through documented
Build-Probe -File under Windows PowerShell 5.1. First invocation passed build,
Blueprint compile, exact one-package cook, stock extraction, retoc packaging,
integrity/exact inventory and New-VoyageReleaseManifest/ValidateOnly gates.
Source hashes match build-provenance.json; no rebuild or manifest hand-edit.

Inspect-VoyageAsset -Query /Mods/HarpoonCannonLifecycleProbe/ModActor -Source Mod
-ModContainer <candidate payload/HarpoonCannonLifecycleProbe_P.utoc> completed
with one external candidate match. This freshly uses the fixed wrapper route,
not the old backend bypass. Returned log/manifest identifies only selected mod
plus stock mounts. Cooked inspection confirms Engine.Actor, 11 TextRender
components, original pawn capture, sticky Drone-control/return flags and absence
of destruction/timeout termination/forced possession or native state mutation.
Candidate static-validation.json records passed checks and source/JSON hashes.

Get-VoyageInstallationStatus -Summary -InstallManifest <HC02 evidence> confirmed
five matching files, current fingerprint and closed game at 04:15:18 UTC.
After explicit user approval, Install-VoyageRelease -AllowDirtySource installed
the immutable candidate manifest at 2026-09-06 04:18:06 UTC. Immediate process
gate and backup/hash readback passed. Recovery manifest:
`artifacts/installations/HarpoonCannonLifecycleProbe/20260906-041806-hc03-candidate-01-c7d1c1c9/install-manifest.json`.
The HC02 triplet and descriptor are recoverable; new HC03 ZIP had no predecessor.
Independent Get-VoyageInstallationStatus -Summary -InstallManifest <HC03 evidence>
at 04:18:15 UTC confirmed five matching files, zero mismatches, matching game
fingerprint and absent process. No other mod files were modified.

Recurring mechanical intent sample: build/release, candidate inspection,
pre-install readiness, installation and independent readback: 5/5 through
documented entry points (100% for this bounded sample, not a global score).
Nested build gates are not counted again. No tool implementation/dependency
source inspection or new reusable tool was required. Own generator editing,
novel static assertions and documentation are outside this metric. No uncovered
recurring operation was found. At installation, native entry/exit and save safety
were runtime-pending. Subsequent entry evidence is recorded below; no HC03
commit or complete entry/exit compatibility claim yet.

Own misses: initial batched rule/doc reads exceeded output budgets; narrowed
reads recovered the required portions. One apply_patch attempted delete+add of
the same README path and was rejected before changes; replaced with one update.
These are agent errors, not pipeline defects. Inspector output used PowerShell
default formatting and wrapped paths; consumed its returned manifest/log paths,
without repeating inspection or discovering a private cache. Prefer explicit
JSON projection for future display. No tool failure or new runtime crash in this
preparation iteration. Subsequent user runtime evidence is recorded below.

Pipeline feedback from prior HC02: shared Git index race was recovered without
file loss; final HC02 commit used commit-repository-changes with UseExistingIndex
and exact21 paths. No further Git write is needed for untested HC03. Current
native-inspection wrapper and external-candidate inspection routes are available;
do not repeat the old direct backend or PowerShell5.1 retry as normal research.

The model handoff remains pending and was acknowledged to its owner, with no
mesh/source/cable change in HC03. Separate installation approval was received
and used only after static checks. Test in a disposable session, start on foot,
do not loot or manually save; use the exit action displayed by the stock HUD.

## Runtime follow-up: entry and exit confirmed

User screenshot ending `36daa4b3-423e-4f65-94c7-ad5f938ccfe3.png` and explicit
report confirm successful entry with ordinary Drone behavior. Current pawn
equals Drone YES, control observed YES, return NO at 18.806339 seconds. Native
view/action HUD visible with E Exit Vehicle and 98 WH gauge. User subsequently
explicitly confirmed successful exit as well. HC03 native entry/exit passes;
post-exit diagnostic flags have no screenshot and wider lifecycle/save behavior
is not validated. No HC03 commit made in this evidence-recording follow-up.
Follow-up operations were repository document reads and exact apply_patch
updates to backlog, owning research and this report. No game-asset operation,
build/install, implementation fallback, new tool, Git mutation or failure.
This documentation-only follow-up adds no samples to mechanical tool coverage.
