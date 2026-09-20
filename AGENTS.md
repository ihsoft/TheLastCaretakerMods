# Repository rules

These rules apply to the entire `TheLastCaretakerMods` repository. A closer
`AGENTS.md` owns the exact contracts and workflow of its mod; do not duplicate
those details here.

## Rule and knowledge ownership

- Repository-level README files and the shared tool index contain only verified,
  established capabilities, current interfaces and safety/compatibility limits.
  Keep candidate IDs, experimental hypotheses, trial instructions and progress
  history in the owning backlog or ignored artifacts, not in shared guidance.
  Promote validated conclusions as general contracts without candidate labels;
  retain version/fingerprint restrictions when they determine applicability.
- Autonomous DonkLift rules live in
  `mods/DonkLiftKeyboardControl/AGENTS.md`.
- The preserved UE4SS/Lua DonkLift implementation is a separate alternative
  under `mods/DonkLiftKeyboardControlUE4SS`; it is not a dependency of the
  autonomous mod.
- Shared Voyage vehicle/HUD patterns belong in
  `docs/vehicle-and-hud-modding-patterns.md`. Evidence and object-specific
  findings remain in the architecture and research documents.
- Current source, the closest rules, durable documentation, and the latest
  game-validated Git checkpoint are sources of truth. Chat history and local
  extracted artifacts are not.
- Keep rules minimal and owned: remove obsolete or counterproductive rules,
  merge duplicates, and put command syntax and implementation conventions in
  the owning tool documentation rather than repeating them here. Replace a
  rule only after preserving its still-valid safety/data contract. Record the
  reason and verification in the active backlog; do not retain a chronological
  history of superseded rules in the normal agent reading path.

## Research and knowledge protocol

- Optimize research for understanding the producer, consumer, owner,
  lifecycle, identity, and data contract—not merely for one patch that appears
  to work.
- Promote useful findings immediately: stable conclusions go to architecture
  or shared-pattern docs; failed/deceptive approaches go to research pitfalls;
  active hypotheses, package hashes, and pending test results go to the owning
  backlog.
- After context compaction, recover from repository state rather than chat, but
  do not automatically read every large document in full. First read the active
  backlog's restart/current-state/next-action sections, then use `rg` and bounded
  ranges for the relevant architecture or pitfall evidence. Read a complete
  large backlog only when those sections and targeted references are
  insufficient. Active backlogs must keep a compact restart section containing
  the current checkpoint, pending gates, next action, and owning references so
  this staged recovery does not discard compaction-critical context.
- Before a visibly heavy investigation (large binary analysis, broad asset
  extraction/cook, long brute-force scan, or uncertain reverse engineering),
  tell the user why it is costly and offer a lighter discriminating experiment.
- Experiments must justify their information value against the user's time to
  launch, check and close the game. Before requesting a test, state the hypothesis,
  distinguishable outcomes and the next decision each outcome supports. Many
  tests without useful evidence are expensive, not progress. After one or two
  no-op results, or earlier when confidence in the discriminator is low, return
  to research or strengthen observation before requesting another gameplay test;
  do not keep changing fields blindly.
- A controlled crash or visibly wrong result is acceptable when it proves the
  intended path is live, the game process is disposable, and a recoverable
  baseline exists.

## Game-derived data and version gates

- Do not commit anything reproducible from the installed game: extracted
  assets, `.jmap`, JSON, pseudocode, raw reports, disassembly dumps, cooked
  packages, container inventories, or installed-file backups. Keep
  version-specific output below ignored `artifacts/` paths. Exceptions are the
  explicitly reviewed `.usmap` registry under `mappings/Voyage/`. User-authored
  mod assets, including `mods/Railgun/Assets/Model/Railgun.glb`, are ordinary
  source inputs; this does not authorize extracted game meshes or materials.
- Every tracked `.usmap` must live in its own Steam-build/engine-version
  directory and have a sibling manifest containing the executable fingerprint,
  exact engine/parser version, generator commit, file hash, validation evidence,
  and revalidation condition. Never overwrite an older mapping for a new game
  build, and keep raw dumper output/logs under ignored `artifacts/`.
- Never generate mappings merely because a task needs them. Call
  `tools/Get-VoyageMappings.ps1`; it fingerprints the installed game, selects
  the matching reviewed entry from `mappings/Voyage/`, validates it, and returns
  the path. Run `New-VoyageMappings.ps1` only after that resolver reports that
  no reviewed mapping matches a genuinely new fingerprint, and ask the user to
  start the game before generation.
- Except for the reviewed mapping registry, commit the method, not the snapshot.
  Reusable fingerprinting, extraction, inspection, patching, and disassembly
  logic belongs under `tools/`, with exact usage documented.
- Treat `tools/README.md` as the tool-routing index. Read it before surveying
  tool implementations, and update it whenever a reusable tool is added,
  renamed, or its interface, prerequisites, outputs, or safety contract change.
- Treat the documented scripts as black boxes on their supported path. Select
  the tool by intent in `tools/README.md`, run its documented interface, and
  evaluate its exit code, manifest, summary, and output before reading source
  or tracing third-party internals. Inspect implementation only when the tool
  fails, hangs, rejects valid-looking inputs, produces an unexpected result,
  or the documented contract is insufficient for the task.
- Toolchain coverage must reach at least 80% of recurring mechanical operations
  specifically in game-asset work and release preparation. Measure game
  fingerprinting, mapping resolution/generation, asset retrieval/inspection/
  modification/validation, and release build, cook, packaging, verification,
  installation and restoration. Source-only modeling, image/video production
  and unrelated development are outside this metric. Do not inflate or dilute the
  metric with reasoning, source editing, documentation, or genuinely novel
  diagnosis after a black-box failure. Record uncovered operations in
  `docs/voyage-toolchain-backlog.md` instead of normalizing ad-hoc commands.
- The pipeline's primary outcome is low agent effort and low context/token use
  for ordinary predictable work, while preserving correctness, provenance and
  safety gates. A black-box wrapper is only one means to that outcome: it is not
  successful if callers still have to study its implementation, reconstruct its
  dependency chain, sift through large raw output, or repeat predictable steps.
  Prefer one intent-level entry point with a compact structured result and paths
  to detailed ignored evidence. Keep verbose logs and full JSON out of normal
  conversation/context unless a reported failure requires them.
- A reported gap is evidence, not automatic authorization or a requirement to
  build another tool. Prioritize defects and recurring friction in actual
  user-requested workflows. The 80% target is not a mandate for 100% feature
  coverage: keep lower-priority gaps deferred rather than inventing new work
  to fill the queue or improve the score. New workflow families require a
  clear user need; an old one-off report alone is insufficient.
- Do not interrupt active work, change a tool or rule, or create a standalone
  commit for every low-impact observation. Keep a single occurrence in the
  owning agent report or active backlog and review such observations in a batch
  at the next natural checkpoint. Act sooner only when the current workflow is
  blocked, correctness/safety/provenance is at risk, or the same friction recurs
  in multiple independent real-task iterations. Repetition is the normal trigger
  for small usability improvements; one minor occurrence is not.
- Pipeline work must have a substantial, measurable expected return. Before
  changing a tool or rule, name the observed recurring cost and the intended
  before/after delta in public calls, retries, implementation files opened,
  output volume, failure rate, or coverage of a real workflow. Include the
  expected implementation, debugging, maintenance, and caller-context/token
  cost: the cumulative savings over the realistically expected uses must exceed
  that cost by a clear margin. Re-evaluate this break-even estimate when a test
  or repair expands unexpectedly, and stop or defer the work when it no longer
  pays back. Afterward record the measured result. Defer marginal score polishing
  or speculative convenience work when it offers only a small improvement
  without a repeated failure, material risk reduction, or clear user need;
  work performed merely to keep improving the pipeline is itself a failure mode.
- Every coding agent working in this repository must finish with a compact
  tool-use report: intended operations, documented entry points invoked,
  successes and evidence, failures or unexpected results, any fallback/source
  inspection and its trigger, validation performed, and uncovered reusable
  operation. A delegating agent must request this report explicitly and promote
  reusable gaps or findings into the toolchain backlog or durable rules.
- Triage cross-agent questions and pause/release requests before continuing a
  long local pipeline investigation. Give the owning feature agent the smallest
  decision, evidence, or explicit resume signal needed to keep independent work
  moving, then continue pipeline work in parallel. Do not make another agent
  wait merely because the pipeline owner is documenting or developing an
  unrelated tool; use the Git semaphore only for the short shared mutation.
- Public build, release, extraction, and packaging scripts must support Windows
  PowerShell 5.1 unless their documented interface explicitly says otherwise.
  Parse and smoke-test the public `-File` entry point with `powershell.exe`;
  follow the implementation conventions in `tools/README.md`.
- Never launch an Unreal Engine build producer, `Build.bat`, UnrealBuildTool, or
  an `UnrealEditor-Cmd.exe` build/generate/cook step in the restricted sandbox,
  not even as a first probe. Request outside-sandbox execution for the initial
  invocation of the owning producer. UBT must write and rotate
  `%LOCALAPPDATA%\UnrealBuildTool\Trace*.uba`; sandbox denial causes a
  `dotnet.exe` dialog and exit `-532462766` / `0xE0434352` before the project is
  read. Treat that signature as a launch-permission failure, not a project,
  Blueprint, engine, SDK, or .NET defect; inspect the captured output and retry
  once outside the sandbox with a fresh output identity.
- A tracked wrapper's normal path must not restore/build a .NET project through
  `dotnet run`. Consume a manifest-validated published binary from
  `.tools/bin/`; reserve source-project execution for explicit tool development.
- Normal Blueprint structure, function, call, member-reference, soft-object,
  and SCS-component questions call `Get-VoyageAssetSummary.ps1` with the asset
  identity and the narrowest applicable `-Focus`; use its compact result and
  returned `summaryPath` instead of searching the full export JSON. Call
  `Get-VoyageAssetJson.ps1` when the complete serialized export is actually
  required and consume only its returned `jsonPath`. Their default `Game` source mounts
  and caches only stock game containers; use explicit `Mod` source with one
  exact mod container only for exceptional debugging, and never cache or
  promote that output. Request the complete inventory through `-ListPackages`
  and consume only its returned `packageListPath`; do not discover `_catalog`
  manually. The central game store, its layout, indexing, provenance sidecars,
  reuse, and invalidation are private to the tool: do not inspect them to locate
  an asset, choose or override a store root, or manually copy, move, merge,
  promote, rewrite, or delete entries. If the tool fails or returns an unexpected
  result, stop normal research and diagnose the tool before touching its store.
- Durable cooked-asset toolchain contracts, accepted fork checkpoints, and the
  validation ladder live in `docs/voyage-cooked-asset-toolchain.md`.
- Active cross-cutting tool and pipeline work lives in
  `docs/voyage-toolchain-backlog.md`; mod backlogs must not absorb general
  toolchain investigations.
- Durable findings about the Boat's Diesel socket hover card live in
  `docs/boat-resource-socket-architecture.md`; active feature work alone belongs
  in `docs/boat-hud-total-resources-backlog.md`.
- Before reusing any game-derived input, obtain the installed game's Steam
  build ID and executable hash with `tools/Get-VoyageBuildFingerprint.ps1` and
  compare them with the owning mod's provenance registry.
- A changed fingerprint invalidates extracted snapshots and reconstructed
  contracts alike. Re-extract or revalidate every affected item before build,
  cook, packaging, or compatibility claims.
- Keep mod version, artifact version, user-facing game version, Steam build ID,
  executable hash, and Unreal Engine version as separate identities. A game
  update neither proves incompatibility nor requires a mod-version bump; it
  invalidates version-bound build inputs and requires new evidence before a new
  compatibility claim. Record whether a display/marketing version came from a
  machine-readable source or from the user, and never infer it from an
  executable version such as `UE5-CL-0`.
- Compatibility metadata may say `game-validated` only after the user tests the
  requested runtime behavior in the real game. Compile, cook, container verify,
  semantic comparison, clean load, and save load remain lower gates.
- An installer that replaces a Voyage mod container must check the game process
  immediately before mutation, preserve an exact recoverable backup, install
  from a validated release manifest, read back every installed hash, and record
  installation time and source-artifact identity. When a release ZIP is kept
  beside standalone IoStore containers as installed provenance, its filename
  and manifest identity must use the artifact version, so distinct candidates
  such as `v2` and `v2-test` cannot masquerade as the same source.
- Any generated or reconstructed game-derived file that must remain in Git as
  a source input needs a provenance header naming its game fingerprint, engine
  version, reconstruction method/tools, and revalidation condition. If the
  format cannot contain comments, record the same metadata in its owning
  README or registry.

## Shared Voyage vehicle and HUD principles

- Treat input mapping, vehicle state, physical movement, provided actions, and
  HUD rendering as separate layers until evidence proves a direct connection.
- Enhanced Input mappings or action events alone do not imply that Voyage's
  standard hint UI will display an action. Locate the live action provider,
  registration path, filter, and rendered widget identity.
- Blueprint overrides bind to the exact declaring UFunction owner. A same-name
  function compiled against the wrong stand-in class is a different function,
  even when the graph compiles successfully.
- Editor-only mirrors must reproduce exact `/Script` module/class/function,
  property, struct, and enum identities needed by cooked assets. Convenient
  invented members are unsafe.
- A relocated original Blueprint is a snapshot of one game version. Never keep
  it in Git or assume a previously built replacement inherits later game
  changes automatically.
- Identify UI elements by stable UObject/action identity where possible, not
  child index, rendered key text, or localized label.
- Treat the shared-pattern document as a starting model for another vehicle,
  not proof that every Voyage vehicle uses the same fields or lifecycle.

## Experiment and validation workflow

- Never replace installed mod or game files while the game is running. The
  user may play while repository-only work continues; do not interrupt or
  alter his current game session.
- Before a risky test, preserve an exact known-good package, change one
  architectural variable, state the expected outcomes, and record the actual
  game result immediately.
- A successful compile, cook, container verification, clean log, or load is
  not gameplay/UI validation. Require a real-game test proportional to the
  changed behavior.
- If an experiment removes control, causes autonomous behavior, corrupts UI,
  or crashes, restore the last checkpoint instead of layering speculative
  fixes on the unknown state.

## Repository hygiene

- Serialize every Git-state mutation through the repository `git-transaction`
  semaphore documented in `tools/repository-git-coordination.md`. Normal scoped
  commits must use `tools/commit-repository-changes.ps1`, which owns exact-path
  staging, commit, and post-commit verification inside one lock. Do not run
  direct `git add`, `git commit`, or another index/HEAD/ref mutation, and never
  split staging and commit across lock acquisitions. Read-only Git commands and
  edits/builds on explicitly disjoint paths may remain concurrent. If the lock
  is busy, wait or report its recorded owner; never delete an active lock.
- Preserve unrelated worktree changes and stage only the exact files belonging
  to the validated change.
- In C++ generator code, replace every repeated `TEXT("...")` literal with a
  named constant. Also name a single-use literal when it represents a logical
  contract, tuning value, asset identity, or naturally belongs to an existing
  constant group. Keep engine-level names reusable by multiple mods in a
  shared header under `tools/`; keep game- and mod-specific values with their
  owning mod. Keep diagnostic and assertion format strings inline even when
  the same wording appears in multiple files; they are messages, not shared
  identities or contracts.
- Do not expose opaque reflected pin names such as `A`, `B`, or `bPickA` at
  generator call sites. Shared helpers may store those raw names internally,
  but consuming code must use the node's semantic role, such as binary left
  and right operands or select condition/true/false values.
- Keep toolchain-specific ignore rules in the owning mod. The repository root
  should contain only genuinely repository-wide ignore patterns.
- Temporary probe mods/containers are allowed while isolating risk, but remove
  them from user-facing mod folders and the installed game after the result is
  understood. Keep reusable standalone inspectors under `tools/`.
