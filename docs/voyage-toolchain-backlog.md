# Voyage toolchain active backlog

## Common release-manifest producer (2026-09-06 UTC)

Two independent release preparations, Harpoon HC01 and ScopeFix, assembled the
common schema-2 manifest manually and inspected an existing producer or the
installer contract. ScopeFix reported this as the only uncovered release-
packaging operation and explicitly classified its broad installer source read
as avoidable. This is demonstrated recurring friction, not quota-driven work.

`New-VoyageReleaseManifest.ps1` now takes one exact release root, container,
ZIP and source scope; fingerprints the installed game; records scoped Git
provenance and exact hashes; refuses unapproved dirty source and existing
output; and publishes the manifest only after the common installer accepts it
in `-ValidateOnly` mode. ZIP construction and mod-owned build/cook/package
remain with the owning producer. Windows PowerShell 5.1 regression passed 12
checks against a synthetic Steam tree without game-file mutation, including
dirty-source authorization, immutable output, containment, ZIP disagreement,
and no manifest after failed validation. This validates the tool contract, not
retroactive adoption by either completed feature workflow.

## Canonical VoyageAssetPatcher boundary (2026-09-06 UTC)

The public routing table advertised assertion-heavy patch operations while its
normal command still used `dotnet run`. That contradicted the stable-binary
contract and exposed routine work to restore, NuGet configuration, and source
build costs. The patcher now has an intent-level wrapper with reviewed mapping
resolution, read-only manifest/hash/input resolution, compact output evidence,
and an explicit checkpoint-gated single-file publisher. The stable EXE is
`.tools/bin/VoyageAssetPatcher.exe`, SHA-256
`4298009F9034E9F5D93BFF343ED9613D35F1F501EB7F4B6AAB5A390EDDBBA1A9`.

Windows PowerShell 5.1 regression passed nine checks on Steam `25056839`:
script parse, idempotent publisher reuse with `dotnet` intercepted, executable
usage, public mapping/extraction inputs, real UE5_8 unchanged roundtrip and JSON
export of `BP_VoyageCableUpdater`, compact result, existing-companion and legacy-
mapping guards, and unchanged installed executable/container hashes.
Evidence is under ignored
`artifacts/tests/asset-patcher-binary-1d05ad6bf1984d2ba4f9891710aa06eb/`.
The first publish needed normal NuGet.Config access; a documentation-only
`UAssetAPI.xml` beside the candidate was correctly excluded from required
runtime sidecars. Documentation-only patcher files are likewise excluded from
the binary input fingerprint, as are UAssetAPI PDB/XML/deps files that do not
enter this direct-reference executable, so those changes do not force a rebuild.
This is tool validation, not a new feature-agent adoption sample.

## Canonical VoyageExecutableInspector boundary (2026-09-06 UTC)

The documented native-correlation path still launched its source project with
`dotnet run`, exposing routine research to restore, NuGet configuration, build,
and oversized-output costs. It now uses one intent-level wrapper that resolves a
manifest-validated single-file EXE, fingerprints the installed game, protects
existing reports, preserves full evidence under ignored `artifacts/`, and
returns a compact structured result. Query-only zero matches are a normal
`no-match` result unless the caller requests strict `-RequireMatch` behavior.

The stable EXE is `.tools/bin/VoyageExecutableInspector.exe`, size `196,700`
bytes and SHA-256
`3A33483362EF5BA122C370C76322A6A4012BEB298D58A07C67CD6045A2F6C718`,
from source checkpoint `79d01b3`. Its explicit contract remains narrow: names,
bytes, pointers, references, and correlated offsets do not prove reflected
ownership, call relations, or persistence lifecycle.

Windows PowerShell 5.1 regression passed six checks: public-script parse,
publisher reuse with both `dotnet` names intercepted, canonical usage smoke,
pre-scan overwrite protection, one real current-game query, and retained game
identity. The single read-only scan of the 198,655,048-byte executable found two
`VoyageInputControlsComponent` matches; installed game files remained unchanged.
Evidence is under ignored
`artifacts/tests/executable-inspector-binary-fdac0d8c9ca345c1b3f325357b711e67/`.
This is tool validation, not a new feature-agent adoption sample.

## C8 freeze: common tooling checkpoint (2026-09-06 UTC)

User requested freezing C8, retiring temporary probes and all old rollback data,
and committing reusable tool fixes. Shared scope: published Inspector resolver/
publisher and wrapper routing; explicit Game/Mod extraction; manifest install/
restore including optional sidecars; read-only installed-state checks; bounded
container validation and their regression scripts. Model/Harpoon work is not
part of this checkpoint. Existing source-only model changes remain untouched.

Fresh Windows PowerShell 5.1 validation passed: installer legacy and sidecar
suites; status 9 checks; container 9 checks; Inspector 6 checks (including no
dotnet launch and unchanged publisher reuse); narrow stock and C8-owned Mod
extraction. Prior accepted source-isolation 6-check and four-case retoc import
reports were reread, not represented as newly rerun full matrices.

Repository rules/index changes preserve the user-approved narrowed mechanical
coverage scope, route supported release producers, and replace obsolete source-
launch/extraction instructions with public binary/source-selection contracts.
Game/asset identity and closed-game/backup protections remain intact in the tools.
The user's explicit deletion retires this task's old backups; it does not remove
backup safety from future installations. Current C8 manifest stays as readback
evidence, but its deleted predecessor files must not be used for rollback.

Research conclusions from the retired probe live in voyage-autoload-research.md.
The repeated manual retirement/sidecar-only and marker-decoder gaps remain
deferred. No extra tool was added merely to increase coverage. Synthetic tool
tests do not count as real-task adoption; real cleanup uses one status family
and one bounded manual retirement family (1/2), matching the existing gap.
No tool internals needed diagnosis, no game launch or new mod runtime logic.
Auto-review initially blocked broad deletion; user subsequently confirmed the
exact inventory. Immutable C8 prerelease/tag are not rewritten by this cleanup.

Autoload C7 / BoatHUD integration (2026-09-05): adding one descriptor to an
already installed, separately owned mod has no sidecar-only public installer.
The common release tool supports sidecars only with the whole triplet/ZIP.
Used closed-game, manifest/hash-gated, no-overwrite single-file creation and
readback without rewriting HUD containers; evidence under
artifacts/autoload-candidate/c7/boat-hud-entry-only. Legacy HUD installation
hash comparison also remained manual (not the common manifest schema).
Deferred scope, not authorization to add tools or migrate the other mod.

Autoload C2 (2026-09-05): common installer handled unchanged third-party DML
as three separately manifested families, without modifying its interface.
Renaming the candidate's active container footprint required exact C1 retirement:
normal Restore would reinstate an obsolete test14 override. Bounded four-file
backup/hash/process-guarded retirement ran successfully via PS5.1; method and
evidence in mods/VoyageAutoLoader/README.md and ignored C2 artifacts. A generic
retire-without-predecessor operation is a deferred gap, not new tool authority.
Conservative C2 sample: public fingerprint/verify/inspect/install/status 5/10
families, manual experimental build/generate/cook/package/retirement 5/10.

Autoload candidate C1 (2026-09-05): the common release installer/restorer now
accept one exact-basename `.autoload` sidecar, including zero bytes, in payload
and ZIP; it shares container hash, backup and rollback gates. PS5.1 synthetic
legacy and sidecar suites passed; the sidecar suite also rejects another
container's marker. Runtime descriptor parsing is mod behavior, not covered
by these installation tests. Manual new-generator build/cook/package and the
probe marker decoder remain the existing deferred workflow gaps.

Deferred metadata-reader gap (DmlAssetRegistryProbe, 2026-09-05): canonical
retoc asset-registry rejects the UE 5.8.2 cooked registry with
`invalid FAssetRegistryVersion: 24`. No parser change is authorized solely by
this report. The current investigation must first select a runtime registration
mechanism; dumping a mod-only registry over the global game registry is not
an additive metadata test. Findings and pending scope live in the probe README.

## Scope

Build a reproducible, versioned tool pipeline for game-asset work and release
preparation. The user explicitly narrowed the 80% target to those operations;
source-only modeling, images/video and unrelated development are excluded.
This backlog owns common tooling only; mod-specific feature research belongs
in the owning mod backlog.

Current priority after the user's video-work interruption: reliability and
adoption of the existing cooked-asset toolchain. Do not resume multimedia
expansion from an automatic continuation or from the historical R3 report.
No video was generated or video tool implemented; only the existing FFmpeg
encoder inventory was queried. Existing uncommitted model helper work is
preserved, not an instruction to extend it or integrate models.

## Operating model

- This backlog is the active queue for the cross-cutting Voyage tool pipeline,
  not for any mod feature. Keep feature implementation and gameplay hypotheses
  in their owning mod documents.
- Normal work consumes published binaries from `.tools/bin/` as black boxes.
- Fork source trees under `.tools/` are inspected or rebuilt only after a tool
  fails, returns an unexpected result, or an intentional fork change has been
  committed and its accepted checkpoint updated.
- A publisher must verify the exact clean source commit, validate its output,
  write provenance and hashes, and leave an already current valid binary
  untouched.
- Tracked wrappers and manifests are the method. Published binaries remain
  ignored local artifacts.
- Periodically promote settled results into `AGENTS.md`,
  `docs/voyage-cooked-asset-toolchain.md`, or another owning durable document,
  then remove the completed narrative from this file. Keep only active gaps,
  pending decisions, and evidence still awaiting validation.

## Success metrics

- Routine effort: ordinary predictable work should require one intent-level
  public call where practical, zero implementation/dependency files opened, and
  only a compact structured result in active context. Detailed evidence remains
  available by returned path. Track calls, source files opened, retries caused
  by interface ambiguity, and oversized output in coding-agent reports; do not
  substitute guessed token counts for observable evidence.
- Tool-first use: agents select the documented public entry point from
  `tools/README.md` and do not inspect implementations on a successful supported
  path. This is a means to lower routine effort, not the end metric by itself.
- Coverage: public tools complete at least 80% of eligible recurring mechanical
  game-asset/release-preparation operations reported by coding agents. The
  numerator is successful operations
  through documented public entry points; the denominator also includes ad-hoc
  fallbacks for operations that should be reusable. Count each requested
  operation once, not retries or internal phases. Keep synthetic tool tests
  separate from feature-agent adoption, and do not turn unknown/empty samples
  into a passing percentage.
- Reporting: 100% of coding-agent handoffs include the report contract from
  `tools/README.md`, including both what worked and what failed or was missing.
- Improvement loop: every recurring fallback is either turned into a public
  tool/contract or remains here with an ownerable next experiment; completed
  entries are promoted and removed rather than accumulating indefinitely.
- Safety floor: effort reduction never weakens fingerprint, provenance,
  validation, mutation, backup, restore, or real-game evidence gates.

## Stable binary registry

| Tool | Canonical path | Source checkpoint | State |
| --- | --- | --- | --- |
| UAssetGUI | `.tools/bin/UAssetGUI.exe` | UAssetGUI `df18b5f`, UAssetAPI `21c982f` | accepted |
| retoc | `.tools/bin/retoc.exe` | retoc `49b7721` | accepted |
| jmap | `.tools/bin/jmap_dumper.exe` | jmap `4f88d8a` | accepted |
| UAssetAPI | `.tools/bin/UAssetAPI/UAssetAPI.dll` | UAssetAPI `21c982f` | accepted |
| CUE4Parse | `.tools/bin/CUE4Parse/CUE4Parse.dll` | CUE4Parse `ec6595e` | accepted, managed-only |
| VoyageAssetInspector | `.tools/bin/VoyageAssetInspector.exe` | tracked source/input fingerprint in sibling publish manifest | published; wrapper smoke passed |
| VoyageAssetPatcher | `.tools/bin/VoyageAssetPatcher.exe` | patcher `9d32697`, UAssetAPI `21c982f`, sibling input/hash manifest | published; PS5.1 real-asset smoke passed |
| VoyageExecutableInspector | `.tools/bin/VoyageExecutableInspector.exe` | source `79d01b3`, sibling input/hash manifest | published; PS5.1 current-EXE smoke passed |

Exact binary hashes and bundle contents are recorded after successful local
publication in `docs/voyage-cooked-asset-toolchain.md` and in ignored sibling
publish manifests.

## Current iteration

- [x] Add idempotent, checkpoint-gated publishers for retoc, UAssetAPI, and
  CUE4Parse.
- [x] Publish all three accepted binaries to their canonical paths.
- [x] Make extraction use canonical retoc by default.
- [x] Make `VoyageAssetPatcher` and `VoyageAssetInspector` consume the stable
  UAssetAPI and CUE4Parse bundles by default, retaining explicit source-project
  overrides only for tool development.
- [x] Smoke-test retoc, both managed bundles, and the two consuming projects.
- [x] Record hashes and update tool routing documentation.
- [x] Add the same stable publisher boundary for jmap and make mappings
  generation consume only the canonical dumper.
- [x] Make the asset JSON store a private implementation detail: remove the
  public root override and require callers to consume only the returned path.
- [x] Split asset retrieval into default cached `Game` and explicit uncached
  `Mod` sources; mount only selected containers and key the cache solely from
  the installed game fingerprint and stock tool inputs.
- [x] Expose the already useful complete package inventory through
  `Get-VoyageAssetJson.ps1 -ListPackages` for both sources instead of requiring
  callers to discover or read the private `_catalog` layout.
- [x] Add `Get-VoyageAssetSummary.ps1` so routine Blueprint class/function/call/
  member/soft-object/SCS questions return a focused compact result and complete
  summary path without reading or searching the full export JSON.
- [x] Make reviewed mapping reuse the default black-box path through
  `Get-VoyageMappings.ps1`; consumers resolve it automatically, while jmap
  generation is reserved for a confirmed unmatched game fingerprint.
- [x] Add a common manifest-driven install-only path for already-built
  standalone IoStore releases, with mutation-free validation, exact archive
  provenance, staging, backup, hash readback, transaction evidence, and handled
  rollback.
- [x] Add a common manifest producer for already-built triplets and ZIPs so a
  new mod producer does not reproduce schema 2 or inspect installer internals.
  It records exact source scope and delegates final acceptance to the installer.
- [x] Add the paired hash-guarded restore/remove path with its own validation,
  staging, transaction evidence, and rollback to the installed state.
- [x] Publish the tracked Inspector as a single-file EXE; move both wrappers off
  `dotnet run`. Windows PowerShell 5.1 regression passed all six checks with
  `dotnet` and `dotnet.exe` intercepted to throw: parse, publisher no-op,
  narrow legacy query, stock inventory (23,980 packages), JSON retrieval and
  reuse. Canonical SHA-256:
  `FD38374E1F402C15D9F1D788D1DA0FB89A96D3006AE77496AF1D418CA5C6BABE`.
  Evidence: ignored `artifacts/tests/inspector-binary-28888824081a4aae9664043b07cb36c5/`.
  The tool migrated stale provenance in the existing game store; no new store,
  mapping generation, or game-file mutation. New real-task adoption is pending.
- [x] Remove the obsolete normal-Inspector `dotnet run` exception from routing
  docs, and move duplicated PowerShell implementation advice out of root
  rules to its existing tool-owned section. Preserve the PS5.1 test contract
  and explicit publish-time permission diagnosis. Six-check regression above
  validates the replaced runtime contract; rule changes do not weaken game
  mutation, mapping, or provenance gates.
- [x] Publish `VoyageAssetPatcher` as a manifest-validated single-file EXE and
  replace the documented normal `dotnet run` path. Its PS5.1 regression blocks
  source-project launch and exercises the intent-level wrapper on fresh UE5_8
  unchanged roundtrip and JSON-export paths.
- [x] Publish `VoyageExecutableInspector` as a manifest-validated single-file
  EXE and replace the documented normal `dotnet run` path with one compact,
  fingerprinted wrapper. Preserve the explicit correlation-versus-proof
  boundary; do not count this synthetic validation as feature-agent adoption.

Acceptance evidence:

- a second invocation of every publisher returned `Rebuilt = False`;
- canonical retoc reported `retoc_cli 0.1.5`;
- canonical jmap reported `jmap_dumper 0.2.0` and a second publication returned
  `Rebuilt = False`;
- `New-VoyageMappings.ps1` resolves repository-relative defaults after parameter
  binding, so its documented Windows PowerShell `-File` invocation reaches the
  intended running-game precondition instead of failing on an empty
  `$PSScriptRoot`;
- both managed consumers rebuilt successfully from the canonical bundles;
- the canonical CUE4Parse bundle itself contains `Microsoft.Bcl.Memory 10.0.11`
  despite the unchanged upstream project declaring vulnerable `9.0.0`;
- an explicit-`UE5_8` UAssetAPI unchanged roundtrip of
  `BP_ToolAbility_Maintenance_Dismantle` was byte-identical for both
  `.uasset` and `.uexp`;
- a narrow CUE4Parse `list:BP_VoyageCableUpdater` query mounted the current
  containers and resolved exactly one package;
- the smoke tests exposed and fixed two wrapper contracts: Patcher now carries
  one explicit engine version through every reopen, and Inspector wrappers use
  `-` rather than a Windows-PowerShell-elided empty argument when mappings are
  absent.

Store-integrity and source-isolation follow-up:

- the earlier `with-additional-*` design was removed because any mod change
  made that cache disposable and allowed unrelated mods to affect research;
- game mode now registers only stock `global` and `pakchunk*` containers even
  when mods remain installed, while mod mode requires one exact `.utoc`, proves
  package ownership from that container alone, and writes an uncached run;
- `-ListPackages` returns a validated list path, count, and hash without
  requiring mappings; game lists reuse the game cache and mod lists remain
  one-off diagnostics;
- the public wrapper now rejects `-CacheRoot`, returns only the asset identity,
  validated JSON path, and content hash, and produces the same game identity
  under Windows PowerShell 5.1 and PowerShell 7;
- a Windows PowerShell `-File` generate-then-repeat test reused one path after
  normalizing `Get-Content` results to plain strings; its diagnostic view was
  removed after validation;
- cache schema 2 invalidates and rebuilds pre-isolation indexes and entries,
  because the older inspector silently mounted every installed container even
  for paths labeled as base-game data.

Install-only acceptance evidence:

- `Test-Install-VoyageRelease.ps1` passed under Windows PowerShell 5.1 against a
  unique synthetic Steam/Voyage tree, covering validate-only, successful
  install, previous-file backup, installed hash readback, injected failure
  after the first replacement, complete rollback, successful predecessor
  restoration/removal, and an injected mid-restore failure with complete
  rollback to the installed state; rejection cases cover fingerprint,
  archive/payload mismatch, directory targets, changed installed files, and
  damaged backups, and a failed-recovery case proves staging-copy retention;
- mutation-free validation accepted the real DonkLift v2 release manifest and
  exact triplet/archive against installed Steam build `25056839` and executable
  SHA-256 `CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`;
- no installed game file was changed by these checks. The first authorized real
  installation through the common wrapper remains a runtime canary.

Windows PowerShell and Inspector-launch follow-up:

- public build/release/extraction/package scripts now have an explicit Windows
  PowerShell 5.1 contract: repository-relative defaults are resolved after
  parameter binding, multi-segment paths use nested `Join-Path`, and the real
  `powershell.exe -File` entry point is part of verification;
- a restricted `dotnet run` can fail before Inspector startup when NuGet cannot
  read `%APPDATA%\NuGet\NuGet.Config`; this is a launch-environment failure and
  must not be interpreted as asset, mapping, or parser evidence;
- duplicate identical `matches.txt` rows are accepted for a selected mod only
  after `ModOnly` proves exact ownership and every combined-view row is the
  requested virtual path. That gate does not prove provider precedence.

## Later pipeline work

- JSON/engine-selection fix is implemented locally, ownership transferred
  from pipeline to the Fabrication task. Fork commits and canonical publication are complete. User confirmed the old GUI's 22-export failure. Candidate executable:
  `artifacts/bytecode-repro/gui-review-build/UAssetGUI.exe`, SHA-256
  `95A51ED530B312AC7D3A50F8DED9A28A1E549A2312D415A3F9305772FADE805D`.
  User confirmed this candidate works after interactive testing on 2026-09-04.
  This closes the requested GUI verification, not a gameplay or general
  cross-engine migration validation gate.
  Per user decision, the engine hint is no longer in JSON (obsolete fields are
  ignored too). It applies current selection only before binary Save, preserves
  object/custom versions, and embeds engine-gated retoc SHA-256
  `3D17670FE4C45E610B915999AFBF6700DB7DB1F31DF0DF143486ABCF1607F33B`.
  Validation: full API suite 30/30 after fixing fixture copy configuration
  (`artifacts/tool-runs/4bdb7950cd2745f5a71034fd9c701393/`); final five targeted
  JSON/selection tests 5/5 (`artifacts/tool-runs/4b782f303709475d98bc865d95d32d8b/`).
  Retoc library 29/29 (`artifacts/tool-runs/269118cdf092459eb8f014450e5eaaf3/`);
  fresh four-case matrix at
  `artifacts/tests/retoc-imports-f5b79c5e01fa44479013ca48fc9cbd9a/summary.json`
  proved UE5.7 writer identity with upstream and UE5.8 with canonical Voyage.
  Final GUI CLI JSON regression: three passing cases, zero RawExports, retained
  versions, unchanged no-op `.uexp`, persistent CanPlace=True at
  `artifacts/tests/gui-json-cb5284b94b4f4368aaf704077415b62e/summary.json`.
  No-JSON-version delta report: fresh fingerprint matched existing input
  provenance; public Test-UAssetGuiJsonRoundtrip passed under PS5.1; targeted
  API tests passed through the bounded runner; direct SDK candidate builds
  required approved escalation. No new extraction/packaging/retoc tests were
  needed. Recurring operations: fingerprint and verification covered, build
  uncovered = 2/3 (66.7%); candidate-build entry-point gap remains. No new
  failures or installed/canonical changes in this delta.
  The preceding candidate passed 17/17 GUI hierarchy opens and unchanged-save
  checks in `artifacts/bytecode-repro/gui-stress-fabrication/`; its extraction
  code/resource is unchanged in the final Save-selection candidate.
  Latest rebuild split UE5_7/UE5_8 match arms into separate lines without
  changing their values; prior automated reports identify their own binaries.
  Completed: coordinated API, retoc and embedded-GUI commits and publication; Voyage extraction uses UE5_8.
  Do not reuse the earlier candidate's unqualified writer-identity claims.
  Limitations/diagnostic corrections: API fixture files originally were not
  copied to test output (Content Include fixes this); initial GUI stress
  filename prefix selected zero assets (rerun at Fabrication directory);
  whole JSON `.uasset` equality fails because OverrideNameMapHashes is ignored;
  a Voyage5.8 payload extracted with UE5.7 layout is not a native UE5.7 API
  fixture and fails older FField parsing, so that invalid control was rejected.
  New reusable verification: Test-UAssetGuiJsonRoundtrip.ps1, tested through
  Windows PowerShell5.1. No installed game files changed by this task.
  Iteration tool report: fingerprint/mapping-input provenance -> public
  fingerprint plus matching existing manifests; fresh extraction/conversion/
  package/verify -> Test-RetocImportCompatibility and public wrappers; native
  execution/tests -> bounded runner; GUI JSON validation -> new public test.
  Builds used direct SDK commands for intentional fork development (GUI needed
  SDK-access escalation). Coverage of recurring game/release operations in
  this iteration: fingerprint, extraction, package, verification covered;
  candidate builds direct = 4/5 (80%), counting each operation once across
  tools/profiles. Synthetic unit cases are evidence, not extra adoption.
  Remaining reusable gap: checkpoint-aware candidate build entry point;
  normal canonical publishers intentionally reject these uncommitted sources.

- Validate improvements on new, non-overlapping real coding iterations.
  Current narrowed-scope historical baseline is R1 + R2 + R4: 9/15 = 60%.
  Source-only R3 is now excluded (N/A); this is a scope recalculation, not
  improvement or proof of current tool adoption.
  All four requested reports are received and retained in
  `docs/voyage-toolchain-coverage.md`: original retrospective baseline 5/14;
  with the separate Fabrication investigation, 9/19 = 47.4%, including
  mechanical source-model operations. This mixed pre-publication sample is
  not current acceptance or a complete activity census. Synthetic self-tests
  do not enter that score. All four owners were notified of the new Inspector
  binary boundary; notification acknowledgements are not new coding samples.
  A post-publication audit found no new completed mechanical work in those
  notifications. R4's explicit delta is 0/N/A; the user's GUI reproduction
  confirms the defect but is not an agent-tool coverage sample. The subsequent
  JSON/UE5_7 repair has now supplied R5: 4/5 = 80% for that real coding iteration,
  recorded with inspected evidence and limitations in the coverage ledger.
  Candidate builds used direct SDK commands; the new container-check wrapper
  was not used. Canonical promotion and interactive validation are complete;
  this one sample is not broad current workflow acceptance. Collect later
  reports from actual work, not repeated idle-task notifications.
- Prioritize common game-asset/release paths exposed by those reports:
  stock-only extraction, correct changed-asset saves, and documented
  inventory/package/verify/install. Source-only model work below is out of scope.
  `Test-VoyageContainer.ps1` now covers the recurring combined integrity and
  exact-package-set gate exposed by R1, using canonical retoc with bounded
  execution. Nine PS5.1 checks passed on a copied, fingerprint-validated
  DonkLift v2 container, including corrupt UCAS rejection, missing/unexpected
  packages, prefix normalization and preservation of original files.
  Evidence: `artifacts/tests/container-tool-733e023866c54e5092d628e03db077a5/summary.json`.
  Expected lists must be independent release contracts; test-derived lists are
  not release-scope evidence. PAK is hash-only, and no runtime/adoption claim
  follows. Contract lives in tools README; fresh coding-agent adoption pending.
- Source-only model work is outside the user-approved scope. Existing helper
  contracts and retained local test evidence live in `tools/model-artifacts.md`;
  do not resume rendering/video or scene integration from this backlog.
- Read-only installed-container/process status is now available through
  `Get-VoyageInstallationStatus.ps1` (contract in `tools/README.md`). Its
  ordinary `-Summary` result avoids returning full stock/process arrays. All
  eleven Windows PowerShell 5.1 synthetic checks passed; evidence:
  `artifacts/tests/installation-status-a334543c54d24385ac4e4d61f5ea12ff/summary.json`.
  A read-only installed-game smoke on build `25056839` also passed. No game
  files changed. R2's ad-hoc status gap now has a public entry point; fresh
  feature-agent adoption remains pending. Top-level inventory explicitly
  discloses unscanned subdirectories; it is not a runtime mount inventory.

- Retoc promotion completed with the coordinated JSON/engine-selection checkpoint
  above, owned by task `01a06a48-386e-7c50-bcda-1668a19773cc`. The earlier
  three-case candidate is superseded; do not publish it independently.
- Bounded diagnostic execution is available through
  `Invoke-VoyageBoundedTool.ps1`; its eight-case Windows PowerShell 5.1
  regression passed. Contract and usage live in `tools/README.md`. Always use
  it for the previously failing conversion fixtures, including future regressions.
  Do not count tool-development tests as feature-agent adoption.
- On the next explicitly authorized closed-game install, use
  `Install-VoyageRelease.ps1` for the real mutation and review its transaction,
  backup, installed hashes, and archive evidence before promoting the installer
  from synthetic-validated to game-install-validated. Exercise the paired
  restore path when a real rollback/removal is actually requested.
- Add guarded recovery for abruptly interrupted or `recovery-failed` common
  installation/restoration transactions. The current restore command covers
  completed installations; it must not pretend an unfinished transaction is a
  completed install. Preserve retained staging/backup evidence until recovery
  has been verified.
- Define and automate semantic equivalence for independent Unreal rebuilds:
  identify volatile SavePackage/GUID metadata, normalize only proven volatile
  fields, compare package identity/ID and structural JSON or Blueprint
  pseudocode, and state which differences force another real-game canary.
  Separately investigate why identical DonkLift gameplay source changed all five
  generated cooked asset hashes while package sizes, IDs, relocated original,
  and key Blueprint pseudocode remained stable.
- Investigate a reliable local source for Voyage's user-facing display version
  and, if found, add it to `Get-VoyageBuildFingerprint.ps1` with explicit
  provenance. Until then, keep user-reported display version nullable and
  distinct from machine-verified Steam build, executable hash, and UE version.
- Define a small reusable compatibility/version metadata schema that separates
  mod version, artifact version, tested game identities, evidence status, and
  provenance of a user-facing game version. Decide migration scope with the
  owners of existing mods rather than rewriting all metadata opportunistically.
- Make mod override resolution emit the winning provider/container and read
  order, then assert that an exact stock replacement resolves from the selected
  mod rather than merely accepting duplicate identical virtual paths.
- Add a single read-only health check for every canonical binary and publish
  manifest if repeated manual verification becomes error-prone.

## Latest readiness audit

Read-only audit on 2026-09-04 UTC used the documented public entry points:
`Get-VoyageMappings.ps1`, `Get-VoyageAssetInspectorBinary.ps1`, and
`Get-VoyageAssetJson.ps1` for `BP_VoyageCableUpdater`. All exited successfully
on Steam `25056839` / executable `CA84428CF456...`. Mapping hash remained
`5118549ACD3F...`, Inspector hash `FD38374E1F40...`; the wrapper returned a
validated JSON path without a source build or mapping generation. No installed
game files or canonical binaries were changed. This readiness check is not a
new adoption sample and does not enter the coverage numerator.

Removed duplicate candidate documentation and superseded repair/promotion
instructions after checking the four-case matrix, four-case GUI JSON report,
API 30/30 and targeted 5/5 logs, retoc 29/29 log, and candidate/canonical hashes.
Interactive confirmation and coordinated promotion are complete; broader real release adoption still needs
reports from actual work. Do not repeat completed diagnostic work from the old
candidate or count this documentation cleanup as mechanical coverage.

Release routing audit: the common tool index covered verification/install but
did not route agents to existing build/cook/package producers. Added the
`tools/README.md` release-producers route to the two documented release
workflows, keeping exact commands and rules mod-owned. Explicitly preserved
closed-game preparation requirements and the common installer's schema-2
boundary; no build, cook, extraction, install or publisher was run by this
audit. Development probes are not advertised as production release builders.
This closes a discoverability gap, not a new measured adoption sample.

## JSON save promotion completed: 2026-09-04 UTC

User-confirmed GUI behavior and canonical API/GUI/retoc publication are complete.
See [the accepted checkpoint](voyage-json-save-checkpoint.md) for commits,
hashes, current serialization contract and post-publication evidence.
Earlier pending promotion statements describe the historical candidate stage.
The candidate-build wrapper gap and broader real-game validation remain separate.

## R7 minimum follow-up: asset query contracts

The dynamic DonkLift report is recorded as R7 in
`voyage-toolchain-coverage.md`: 7/9 = 77.8%. Prioritize these narrowly scoped
changes only when the affected public tool is next changed or the operation
recurs; the report is evidence, not blanket authorization for a new analyzer.

The `VoyageExecutableInspector` launch and documentation boundary is complete:
normal work uses its canonical binary through one compact wrapper, while name,
string, reference and offset correlation is explicitly not proof of a reflected
owner, call relation, or persistence lifecycle. A structured reflected-type/
function/call mode remains deferred until a second real task requires it or a
bounded experiment proves a reliable data source.

PowerShell wildcard mistakes and unsupported `-AssetPath` were caller misuse.
Keep examples literal-path based where practical, but do not count or implement
them as asset-tool coverage gaps. The user has now completed the discriminating
existing-versus-newly-fabricated forklift test: new creation followed the patched
`DroppedActor`, while the restored actor retained stock behavior through its
separately serialized class. This confirms the static-analysis boundary rather
than closing it. Research of pre-spawn saved-class resolution or a safe migration
hook remains feature-owned; do not present runtime actor replacement as a
transparent class redirect or promise a generic save migration tool.

The zero-reference-match contract reported by R7 is complete and therefore no
longer active backlog work. `Inspect-VoyageAsset.ps1` returns a structured
`no-match` result by default and offers strict `-RequireMatch`; per-package parse
errors still fail. HC-R1, HC-R2 and HC-R3 are also complete: exact asset JSON
survives a proven optional pseudocode formatter failure, direct `-File -AsJson`
returns full paths without a caller conversion pipeline, and normal structural
inspection mounts stock containers only unless one exact mod is explicitly
selected. The R7 compact-summary gap is also implemented: Overview plus focused
Functions/Calls/Members/SoftObjects/Components views reuse the validated JSON,
record a complete hash-verified summary, and preserve explicit boundaries around
writability, native call graphs, lifecycle, dispatch and provider precedence.
On the real turret fixture, the source export was 996,213 bytes, the complete
summary 130,019 bytes, and the ordinary Overview result 884 UTF-8 bytes; focused
function queries return only their matching records.
The thirteen-check Windows PowerShell 5.1 Inspector regression passed with
evidence under
`artifacts/tests/inspector-binary-63b82cefbf4b47318e23887957600827/`.
The accepted Inspector binary was reused unchanged and no game file changed.
This tool-development result does not retroactively raise R7's adoption score;
the next real consumer must demonstrate the lower-cost path.

## Fork binary GitHub release handoff

Deferred probe workflow gap (2026-09-04): DmlAssetRegistryProbe test5 still uses
manual UE build/generate/cook, staging and release-manifest assembly. Public
fingerprint, container checks, isolated inspection and manifest install/readback
cover the remaining steps. A probe-owned orchestrator is absent; defer tool
implementation until justified by continued use. Every UE invocation may launch
UBT (including commandlet platform validation), so apply the documented
LocalAppData trace permission requirement to those invocations too.

Test6 repeated this gap (group-query/forced-scan probe, 2026-09-04): 5/9 eligible
operations used public tool paths; generator build/generate/cook and package
assembly remained manual. Editor graph construction exposed two reusable
pitfalls: standard macro exec input names are not necessarily the K2 function
exec pin name; wildcard array functions need UK2Node_CallArrayFunction to
propagate element types. Corrected in the probe, with successful cook and
isolated cooked-bytecode checks; runtime remains pending. No new workflow tool
is authorized by this coverage report alone.

Test6 readback completed: 1719 stock-map control assets, zero mod-root assets
before and after forced scan. The probe's durable result lives in its README.
Additional deferred gap: compressed SaveGame marker readback has no documented
entry point. This turn used bounded in-memory Unreal-v2/zlib decoding and
validated string counts/terminators; do not infer marker contents from file size
or plain-text searches. Readback sample: installed-state verification covered,
marker decoding uncovered (1/2); this is not authorization for a general save
editor or unrelated save-format investigation.

RC3 releases are published and verified; identities and the compact tool report
live in [the checkpoint](voyage-json-save-checkpoint.md#github-rc3-publication).
Reusable gap: fork binary ZIP assembly and GitHub publication currently use
local staging plus gh; no repository release entry point covers them. Deferred
until recurring user demand warrants it; do not implement solely for coverage.
