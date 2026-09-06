# Harpoon research restart: tool-use report

Task `01a0507c-01d0-71e0-a8a0-09348bb3c406`; bounded iteration 2026-09-05
Pacific / 2026-09-06 UTC, repository refresh through current-stock research.
No build/cook/package/install/restore or gameplay source/model edit.

## Operations and evidence

- Fingerprint: `Get-VoyageBuildFingerprint.ps1 -OutputPath
  artifacts/cannon-research/resume-20260905/fingerprint.json`, PS5.1 public
  entry point, success. Steam 25056839, executable
  `CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
- Mappings: `Get-VoyageMappings.ps1`, PS5.1, success without generation;
  reviewed mapping SHA-256
  `5118549ACD3F34A03E790C307BCDB108632D5E1D87BCA72D7EE5B5EF081538BF`.
- Installed state: `Get-VoyageInstallationStatus.ps1 -HashModFiles`, PS5.1,
  success. No Harpoon-named top-level files and no unscanned subdirectories;
  process absent at 01:35 UTC, present at 01:36 UTC. Not a mount guarantee.
- Stock inventory: `Get-VoyageAssetJson.ps1 -ListPackages`, success;
  23980 packages, returned package-list SHA-256
  `6D76D8E2BE0C3210AAAD89D93667A3DCAD4501F2E6A94D15EC880852FC5BFF1E`.
  Consumed only the returned list path, never discovered the private catalog.
- Exact stock JSON: `Get-VoyageAssetJson.ps1` with exact /Game identities;
  nine successful targets: CameraDrone, Telescope, Binoculars, DA_CameraDrone,
  Cyclone item, medium-turbine actor, and ordinary/Ram/laser shark Blueprints.
  Used only returned jsonPath values. Later six requests ran through the
  Windows PowerShell 5.1 public interface; first three ran via the current
  PowerShell host. No caller-supplied mappings, source trees or cache roots.
- One unresolved exact target: module turret, described below. Other three
  requests in its initial sequential batch did not run after the terminating
  error; they were subsequently requested independently and succeeded.
- Optional readable pseudocode: `Inspect-VoyageAsset.ps1 -Query <exact path>
  -OutputRoot artifacts/cannon-research/resume-20260905/...`, PS5.1, for Drone
  and Binoculars. Unexpected mixed mounting; whole-export JSON parity against
  isolated Game JSON checked before using it as a reading aid. All conclusions
  remain grounded in the isolated JSON/control flow, not provider precedence.

Drone JSON SHA-256: `D4901219B5DE930906AC0830F344B0649CA98C29F9F90E8EE739C593C2978295`.
Binoculars JSON SHA-256: `2EDBE266142574F9438203CAB64BC061BCDA7558EC59B10A88DA80AF6242DCB8`.
The two independently produced diagnostic JSONs matched these exactly.
Diagnostic output/manifests remain under
`artifacts/cannon-research/resume-20260905/drone-pseudocode/` and
`artifacts/cannon-research/resume-20260905/optics-pseudocode/`.

## Failure HC-R1: optional decompilation rejects required JSON

Exact reproduction:

```powershell
& tools/Get-VoyageAssetJson.ps1 /Game/Blueprints/Modules/Utility/Weapons/BP_Module_Turret
```

Result: terminating error from wrapper line 641, no successful jsonPath.
Tool-returned diagnostic staging directory:
`artifacts/asset-cache/steam-25056839-CA84428CF456-game-41D0CB1DF381/_staging/asset-cc0d8197f7404e1e94cc115625982836`.

`errors.txt` names `ArgumentOutOfRangeException` in CUE4Parse
`BlueprintDecompilerUtils.FinalFunctionCleaner`, followed by
`GetLineExpression` / `UClass.DecompileBlueprintToPseudo` / Inspector Main.
`inspector.log` confirms exactly one requested stock package; only stock
containers mounted. A 996213-byte JSON was written, but was not consumed as
validated research input or manually promoted.

Allowed source-inspection trigger: failed black box. Read the wrapper's
failure gate and Inspector Program.cs export try/catch (approximately 271-297).
Serialization/write occurs before optional pseudocode inside one try/catch.
Both feed errors.txt; the wrapper then rejects the request. The inspector's
process return depends on match count rather than these per-asset failures.
The wrapper correctly fails closed; the operation conflates JSON and optional
pseudocode success. No implementation was changed, no staging/store mutation.

Potential pipeline work: separate required JSON validity from optional
decompilation status, retain warnings, and regression-test this exact asset.
This report is not authorization to change canonical tools during another
agent's checkpoint or to silently accept arbitrary partial JSON.

## Interface issues HC-R2 / HC-R3

- HC-R2: public `-File ... -ListPackages` default formatting truncates
  packageListPath to `...` and omits needed metadata in the table view. Calling
  the same public script as an object producer and explicitly piping to
  `ConvertTo-Json -Depth 5` returned the complete path and hashes. No store
  discovery was needed. JSON stdout or a documented automation recipe would
  improve the boundary; this is usability, not an inventory failure.
- HC-R3: documented `Inspect-VoyageAsset.ps1` exact-asset pseudocode calls mount
  installed mods too, unlike isolated `Get-VoyageAssetJson -Source Game`.
  Logs show 14 mounted files and 34258 virtual files, versus the isolated
  turret request's 34234 virtual files. Diagnostic Drone/Binoculars exports
  matched isolated stock hashes; that is per-export parity, not a general
  dependency-resolution or provider-precedence guarantee. A stock-isolated
  pseudocode interface (or optional pseudocode returned by the JSON tool) is
  a useful existing-workflow gap. Do not infer isolation from an exact query.

## Agent-side misses and local techniques

- Initial batched rule/backlog reads exceeded tool-output budgets; narrowed
  reads recovered the material. Later oversized projections also truncated;
  selected fields/ranges were reread. These are agent output-management errors,
  not asset pipeline defects. Long historical rules caused repeated work.
- A Windows `rg path/*.pseudo.cpp` invocation failed with OS error 123.
  Corrected to `rg -g '*.pseudo.cpp' <directory>` and used `--no-filename` to
  avoid repeated long paths. No pipeline tool defect.
- A first class-parent projection selected SuperStruct (function convention)
  and got null. Actual generated-class JSON uses Super; inspected that field
  before interpreting the parent. Null projection did not mean no parent.
- A final documentation-check command piped a PowerShell foreach statement
  directly and failed to parse. Corrected by materializing the result array
  before piping, as the existing tool index already requires. Agent syntax
  error, not a missing public tool. The corrected check found all six owned
  documents present, with no conflict markers or trailing whitespace.
- Local techniques were read-only PowerShell projections of returned JSON and
  SHA-256 comparison of diagnostic versus isolated exports. No new executable,
  script, asset writer, decoder or reusable tool was authored in this iteration.
- Old gameplay notes overclaimed timed Drone destruction from a non-restored
  visibility marker. Corrected the inference, retained the observation and
  recorded the missing independent latent-continuation discriminator.

## Validation / coverage

Public tools completed 13 of 14 intended operations (92.9%): fingerprint,
mapping resolution, installed-state snapshot, full inventory and nine exact
asset inspections. The tenth asset inspection (turret) remains blocked, not
counted as success. Retries and pseudocode variants are counted with their
asset inspection, not as extra operations. Failed-tool source diagnosis,
reasoning, documentation and repo reads are excluded. No operation was
completed by an ad-hoc asset-tool replacement. This is a bounded task sample,
not a repository-wide adoption figure or gameplay compatibility claim.

Validation was static JSON/bytecode and same-export hash comparison. No new
runtime result, binary save, native mirror renewal or package acceptance.
Tracked-document `git diff --check` passed (only repository CRLF conversion
warnings); untracked owned documents received the separate check above.
