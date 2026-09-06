# Harpoon HC01 preparation: tool-use report

2026-09-06 UTC. User authorized the next isolated autoload experiment.
Preparation sample below preceded installation; a subsequent screenshot now
validates the bounded standalone lifetime observation, not cannon gameplay.
This is separate from the earlier
research sample in `harpoon-cannon-tool-report-2026-09-05.md`.

## Subsequent runtime screenshot result

User-provided screenshot shows HC01 DONE / initial VALID / final ALIVE,
8.002636 observation seconds and 455.186035 cm camera distance. The observer
is readable and independently completed sampling. Cleanup is requested, not
verified by that final pre-cleanup sample. Interaction, shell and save/travel
remain unvalidated. No code/package/install changes followed this observation.

Recording-only tool report: repository documentation read/updated; no game-asset
or release operation required, coverage N/A (zero eligible operations), no
pipeline failure/fallback or new reusable tool. Routine cost: zero asset/release
public calls and zero implementation/dependency files opened. No new gap.

## Subsequent authorized installation: 2026-09-06 02:23 UTC

- Intended operations: readiness snapshot, manifest installation, independent
  readback. Entry points: Get-VoyageInstallationStatus -Summary (twice),
  Install-VoyageRelease -AllowDirtySource (once).
- All passed: closed-game gate, current fingerprint, unchanged candidate05,
  five installed hashes match, zero mismatches. No build or package changes.
- Evidence: `artifacts/installations/HarpoonCannonLifecycleProbe/20260906-022330-hc01-candidate-05-4bafd412/install-manifest.json`.
  Previous targets absent; restore removes only those owned files. No existing
  mod replaced, no predecessor backup invented.
- Failures/fallback/internal inspection: none. Installation manifest read for
  ownership/recovery evidence. Routine cost: three public calls, zero tool
  implementation/dependency files opened, no oversized tool output.
- Coverage: 3/3 intended operations via public tools (100%); this small install
  sample does not replace or inflate the preparation sample below.
- Reusable uncovered operation: none. Runtime was pending at installation;
  the subsequent bounded observation is recorded above.

## Result and evidence

Accepted preparation candidate: `artifacts/harpoon-cannon/hc01-candidate-05`.
Its immutable `release-manifest.json`, `static-validation.json`, `build.log`,
`generate-unreal.log`, `cook-unreal.log` and `inspection/` contain the detailed
evidence. Container verification:
`artifacts/container-checks/79ec20ff0d7e4d67a3a3e088f684d7ce/verification.json`.
Only `/Game/Mods/HarpoonCannonLifecycleProbe/ModActor` is packaged, plus the
matching autoload descriptor. No stock replacement, model, native DLL or HUD.
Current game fingerprint is Steam25056839 / executable CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.

## Pipeline operations

| Intent | Documented entry point | Result |
| --- | --- | --- |
| Fingerprint | Get-VoyageBuildFingerprint.ps1 | Current build/hash; candidate fingerprint.json |
| Reviewed mappings | Get-VoyageMappings.ps1 | Existing reviewed UE5.8 mapping; no generation |
| Process/installed state | Get-VoyageInstallationStatus.ps1, final -Summary | Process absent at 02:15:02 UTC; no Harpoon installed |
| Dependency package inventory | Get-VoyageAssetJson.ps1 -ListPackages | Stock Drone and Engine text font/material identities present |
| Exact packaging input | Extract-VoyagePackage.ps1 -Source Game (default) | One exact stock Drone package; only scriptobjects used in packaging |
| Prepare new probe | New mod-owned AutoloadProbe/Build-Probe.ps1 | C++/graph/cook/package/ZIP pipeline now runs in PS5.1; experimental producer |
| Container integrity/exact set | Test-VoyageContainer.ps1 | Passed; one exact package, no unexpected paths |
| Release validation | Install-VoyageRelease.ps1 -ValidateOnly -AllowDirtySource | Schema2, current fingerprint, triplet/sidecar/ZIP passed; no mutation |
| Cooked inspection | Get-VoyageAssetInspectorBinary.ps1; documented six-argument Inspector through Invoke-VoyageBoundedTool.ps1 | JSON+pseudocode; Engine Actor, Tick, five texts and intended spawn/sample/cleanup branches |

Bounded conservative adoption sample: 8/9 (88.9%). The new producer is counted
as uncovered until adopted, even though its documented orchestration passed;
do not inflate adoption by calling newly authored code an existing black box.
Retries are folded into their operation. Source editing, novel graph design,
failure diagnosis, documentation and bespoke semantic assertions are excluded.
No installation/restoration was attempted or counted. No tool written to fill
an adoption quota.

## Failures, unexpected behavior and own fixes

- Initial direct UBT build: auto* cannot deduce from UE5.8 TObjectPtr elements.
  Own generator error; explicit UEdGraph/UEdGraphNode pointers fixed it.
- Candidate01: own wrapper assumed Get-VoyageBuildFingerprint returned a PS
  object; it returns JSON text. ConvertFrom-Json corrected the consumer.
- Candidate02: default FText on a by-reference SetText pin is invalid. Own
  generator now wires Conv_StringToText. Compilation failed closed before cook.
- Candidate03: UTextRenderComponent.SetText is reflected but not BlueprintCallable;
  correct entry is K2_SetText. Generator now checks reflected callable flags.
  These two graph errors are development errors, not Inspector/UE incompatibility.
- Candidate04 prepared successfully, but editor's enabled-by-default Android
  File Server created an unwanted source Config/DefaultEngine.ini. Disabled
  that unused plugin and removed only that generated config via apply_patch.
  It is reproducible and was not committed. Candidate05 is the replacement;
  source hash gate passed and no config reappeared. No game files removed.
- One tool-index apply_patch used an invalid extra context line; retried only
  the narrow routing insertion. No shared content was overwritten.
- A parent-field projection assumed Super and returned null. Inspecting the
  actual output showed SuperStruct for this generated Engine Actor. Static
  assertions now use the observed field; null projection was not missing data.
- Some initial documentation/source output was oversized/truncated; later
  reads narrowed to exact sections/log errors. This is agent-side context cost.

## Fallback / internal inspection and routine cost

Tool source reads: Get-VoyageBuildFingerprint.ps1 after candidate01 type failure;
Get-VoyageMappings.ps1 tail to check the adjacent return contract (unnecessary
extra read); existing DonkLift release producer's manifest fields because the
schema2 documentation lacks a complete minimal producer example. No shared
tool implementation was changed. Engine headers and other generators were
read for novel source development, not as the normal asset/release interface.

Routine cost: five Build-Probe public -File attempts (candidate01-05; 04 and
05 passed), plus an initial direct compiler development attempt. Additional
public gate/inventory/extraction/status/inspection calls and nested gates are
listed above; their total invocation count was not instrumented, so no exact
total is claimed. Three existing tool implementation files were opened; new
generator and Engine API development reads are separate. Full logs stayed in
artifacts except narrow diagnostics; initial oversized reads remain a cost miss.

The documented non-installed Inspector recipe mounts the normal Paks set plus
the candidate: logs show 16 mounted files. This is not the newly fixed isolated
Game wrapper. The unique HC01 package was the sole match and exactly owned by
the verified candidate; dependency provider precedence is NOT established by
this inspection. Do not use this as proof of isolated stock exports.

## New methods and reusable gaps

- New experimental Build-Probe prepares one fresh candidate end-to-end, hashes
  source inputs before/after, records dirty-source truth, uses public extraction,
  bounded retoc and shared validation. Not promoted to a general release system.
- New Engine-only C++ Tick observer provides independent initial/ongoing/final
  lifetime evidence without model visibility, latent Delay or screen widgets.
  Gameplay readability/lifetime/cleanup still require real-game validation.
- Added ActorLifecycleGraphNames.h for semantic Engine pin names; existing shared
  BlueprintGraphNames.h and other agents' code were not edited.
- Reusable friction: schema2 producer interface requires reading an existing
  producer to assemble its minimal manifest; a documented example/schema would
  reduce recurring source reads. No automatic request to build another tool.
- Existing documented non-installed Inspector route is lower-level and mounts
  other mods. Record this distinct scope, not a reopening of the now-fixed
  installed-source HC-R3. A future wrapper may help if this workflow recurs.

## Validation boundary

PS5.1 parser and public -File smoke passed. C++ build, Blueprint compile, exact
single-package cook, bounded retoc, integrity/inventory, schema2 validate-only,
and cooked JSON/control-flow assertions passed. Source hash readback matches.
No game launch, install, possession, construction or save test; no claim of
HarpoonCannon compatibility or completion. Other dirty work preserved; no commit.
