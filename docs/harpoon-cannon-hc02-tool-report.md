# HC02: observation of natural Drone interaction acquisition

Iteration begins after user HC01 screenshot and request to continue.
The initial preparation did not modify installed HC01. The subsequent authorized
installation and runtime acquisition result are recorded first below.

## Subsequent runtime result: natural hints accepted

User screenshot `codex-clipboard-22e88fd6-01ed-41a7-8fbe-f4631d7aa353.png`
shows visible Drone, initial VALID/current ALIVE at 21.406092 s, distance
219.901586 cm, hidden NO/collision ON, plus stock Loot/Enter/Grab Photo Drone
hints. Acquisition and hint rendering are now game-validated; entry/exit,
occupancy-safe cleanup, save/travel and shell integration are not. Screenshot
is mid-observation and does not validate the 60s cleanup. No source/package
changes were made while recording it. Do not test entry using HC02's timed
unconditional destruction.

Recording-only report: documentation reads/updates, no asset/release operation,
coverage N/A, zero public pipeline calls or implementation/dependency reads,
no tool failures, fallback or new reusable tool. Historical preparation and
installation claims below describe their own earlier gate levels.

## Subsequent authorized installation

2026-09-06 03:28:09 UTC: unchanged candidate01 installed through the common
Install-VoyageRelease -AllowDirtySource. HC01 triplet/descriptor backed up;
new versioned HC02 ZIP added. Evidence/recovery:
`artifacts/installations/HarpoonCannonLifecycleProbe/20260906-032809-hc02-candidate-01-46805ed4/install-manifest.json`.
Independent status at 03:28:21UTC: filesMatch=true, fingerprint matches, 5 files,
zero mismatches, game absent. Other mods untouched; runtime was pending then.

Tool-use report: readiness -> Get-VoyageInstallationStatus; installation ->
Install-VoyageRelease; readback -> Get-VoyageInstallationStatus. All passed.
Three public calls, zero implementation/dependency files opened, no failed
calls/fallback/oversized output or new gap. Coverage 3/3 intended installation
operations (100%); separate from preparation metric. Recovery metadata read
from the returned manifest, not inferred from historical state.

## Tool calls and early findings

- Get-VoyageBuildFingerprint confirmed Steam25056839 and the unchanged
  CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933 executable.
- Get-VoyageAssetSummary /Game/Blueprints/Vehicles/BP_CameraDrone -Focus Functions
  under the current default shell (PowerShell 7) failed at line283: mandatory
  Get-OptionalPropertyValue Object cannot bind null. Narrow implementation read
  after that failure found the optional ChildProperties -> array pipeline path.
  No common tool/store edits. This is shell-specific evidence, not an invalid
  Drone asset or a claim that the PowerShell5.1 contract failed.
- The documented powershell.exe -NoProfile -File ... -Focus Functions -AsJson
  path succeeded: 72 functions, schema4 summary SHA
  00C5E39C0914CCF7D54DB48458AA55B12F8597EEF4556817EDD36F32B5964C0E.
  Its ~8872-token output exceeded the requested display budget. Selected
  function-name/call projections from the returned complete summary recovered
  the necessary scope. Prefer Overview first or exact FunctionName when known.
- Get-VoyageAssetJson -AsJson through PS5.1 succeeded, stock Drone JSON SHA
  D4901219B5DE930906AC0830F344B0649CA98C29F9F90E8EE739C593C2978295.
  Used only its returned jsonPath for necessary component/CDO property reads;
  no cache layout discovery or rejected intermediate data.
- A subsequent -Focus Calls -FunctionName GetInteractiveProvidedActions returned
  one compact record, GetInteractActions. This is the successful low-cost path.
- DroneModuleComponent already has current item identity, configured starting
  capacity/amount 100 and Electricity capacity100. Native runtime resource
  initialization remains unproved; no fabricated resource fix was applied.

## Implementation boundary / own methods

HC02 preserves HC01's spawn/activation/item/attachment behavior; increases the
observation window to60 and adds read-only whole-Actor hidden/collision flags
plus a world-space marker. Does not enter, recharge, register or attach a Drone.
Read current Engine headers for bHidden's BlueprintReadOnly/AllowPrivateAccess,
GetActorEnableCollision, SelectString and K2_SetWorldLocation signatures.
IsHidden is not a reflected callable; no guessed UFunction or Voyage mirror.
These are novel generator-source checks, not routine tool internals.

Build-Probe now consumes New-VoyageReleaseManifest, keeping the extra engine/
mapping/sourceHash evidence in its own build-provenance.json. New wrapper
source was not inspected. Shared graph names added only in the owned
ActorLifecycleGraphNames header. Existing shared BlueprintGraphNames and all
model/other-mod generators untouched.

One multi-file apply_patch failed from an unnecessary trailing context hunk;
verified no partial changes and reapplied the exact edits. Agent error, not a
pipeline defect. No generated/native binary is intended for Git or shipping.

## Non-installed candidate inspection: diagnosed public-interface gap

The tools index's six-argument recipe mounts all installed containers plus the
candidate. Here installed HC01 and uninstalled HC02 intentionally share the
same virtual asset identity; using that recipe cannot safely establish provider
identity. Did not remove, hide or rename installed containers to inspect HC02.

Canonical resolver + bounded Inspector --help returned usage (exit1, expected
for this unsupported help flag). Help exposed selection/selected-container
arguments. The first attempt combined extra-paks directory with Mod selection
and failed clearly: extra directories supported only with All. Evidence:
`artifacts/tool-runs/646bbae6b54548548f3bbad939169f7c/result.json`.
After this failure, narrow Program.cs lines374-407 showed Mod itself adds the
exact selected file. Correct call uses sixth argument `-`, seventh `Mod`,
eighth the exact non-installed candidate .utoc, primary game Paks and reviewed
mapping. It succeeded, with stock containers plus only HC02, one matching asset.
The wrapper's installed-Paks restriction does not apply to this raw CLI route.

Success: `artifacts/tool-runs/6d33e5d1c5e6404caff291844f85daba/result.json`.
No provider precedence guess: HC01 was excluded and HC02 markers (60 seconds,
eight text components, new hidden/collision reads) were verified in its output.
This is a reusable documented-recipe gap, not a request for a new backend or
automatic authorization to expand tooling. Retain the exact successful method
for the pipeline owner to document/route.

## Final preparation report

- Worked: public Build-Probe -File on Windows PowerShell5.1 passed at first
  invocation. C++/Blueprint/cook, bounded packaging, exact inventory and new
  New-VoyageReleaseManifest producer passed. No manual schema2 construction or
  producer/installer source inspection; extra hashes in build-provenance.json.
- Evidence: `artifacts/harpoon-cannon/hc02-candidate-01/release-manifest.json`,
  `build-provenance.json`, `static-validation.json`, and stage logs.
  Container report `artifacts/container-checks/0840df6ac97140b1811414e911bfcf56/verification.json`.
  Cooked JSON SHA5575D9C460AF2F3284CD5CDB0A04528D1B6D331796B373E0E2EC36CB4D191784.
- Static assertions: Engine Actor parent, eight text components, 60s threshold,
  marker location writes target observer component; reads target owned Drone;
  no activation/possession/hidden/collision mutation. Source hashes match.
- Installed-state readback at 03:24:24UTC confirms HC01 unchanged (5/5 files,
  current fingerprint, game process absent). At that preparation gate HC02 was
  not installed or runtime-tested; subsequent results are recorded above.
- Routine cost: 15 direct public entry-point calls including helper/resolver
  calls and diagnostic retries; nested producer gates not added to this count.
  Two existing tool implementation files opened only after failures:
  Get-VoyageAssetSummary.ps1 and VoyageAssetInspector/Program.cs. Four Engine
  headers read for new generator code, not routine mechanical pipeline work.
  Functions output was oversized (~8872 tokens); an attempted nonexistent
  Inspector README lookup also failed harmlessly. No hidden-source build.
- Conservative coverage: 5/6 eligible intents (83.3%): fingerprint, stock Drone
  inspection (including summary retries), mappings, release preparation and
  installed readback covered; non-installed candidate inspection counted as a
  diagnosed lower-level recipe gap until the correct invocation is documented.
  Code/semantic checks/docs/novel diagnosis excluded; no install operation counted.
- Reusable gaps: PowerShell7 null optional-property handling and oversized
  unfiltered Functions output; correct isolated non-installed Inspector recipe.
  New manifest producer's real consumer adoption is successful. No new general
  tool was authored; changes belong to the mod's diagnostic generator/orchestrator.
