# Game-derived data and version gates

- Do not commit anything reproducible from the installed game: extracted
  assets, `.jmap`, JSON, pseudocode, raw reports, disassembly dumps, cooked
  packages, container inventories, or installed-file backups. Keep generated
  output in ignored local storage selected by the lifecycle policy below; the
  fact that a file is reproducible does not determine whether it is temporary
  or worth retaining. Exceptions to the commit rule are the
  explicitly reviewed `.usmap` registry under `mappings/Voyage/`. User-authored
  mod assets, including `mods/Railgun/Assets/Model/Railgun.glb`, are ordinary
  source inputs; this does not authorize extracted game meshes or materials.
- Apply the shared [generated-output lifecycle](../docs/artifact-lifecycle-rule.md):
  `Tmp/` holds one-cycle output by default, while ignored `artifacts/` is for
  generated output with a concrete use beyond the current research, feature,
  or release cycle. Preserve owner, consumer, revalidation, rollback, and
  cleanup boundaries there. Consult `artifacts/ARTIFACT-FAMILIES.md` and
  `artifacts/AGENTS.md` before creating or changing retained content.
- Every mod writes temporary output below `Tmp/<Mod>/`, using its exact mod
  directory name. Give runs meaningful names: identify active research and its
  subject, while ordinary test builds remain disposable. A producer removes
  run-only scratch after success; after failure, keep the logs and evidence
  needed for diagnosis or recovery and report their location. The detailed
  naming and cleanup contract is in the shared lifecycle document.
- Every tracked `.usmap` must live in its own Steam-build/engine-version
  directory and have a sibling manifest containing the Steam build ID,
  exact engine/parser version, generator commit, file hash, validation evidence,
  and revalidation condition. Never overwrite an older mapping for a new game
  build. Keep a selected candidate and the evidence required to review it as a
  durable artifact; raw dumper output and logs are temporary unless retained
  explicitly for an unresolved investigation.
- Never generate mappings merely because a task needs them. Call
  `tools/Get-VoyageMappings.ps1`; it fingerprints the installed game, selects
  the matching reviewed entry from `mappings/Voyage/`, validates it, and returns
  the path. Run `New-VoyageMappings.ps1` only after that resolver reports that
  no reviewed mapping matches a genuinely new Steam build ID, and ask the user to
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
  or tracing third-party internals. Do not demand a separate tool version,
  commit, or binary hash check before a normal call unless the documented
  interface requires it for an actual format or safety contract. Inspect
  implementation only when the tool
  fails, hangs, rejects valid-looking inputs, produces an unexpected result,
  or the documented contract is insufficient for the task.
- Toolchain coverage must reach at least 80% of recurring mechanical operations
  specifically in game-asset work and release preparation. Measure game
  fingerprinting, mapping resolution/generation, asset retrieval/inspection/
  modification/validation, and release build, cook, packaging, verification,
  installation and restoration. Source-only modeling, image/video production
  and unrelated development are outside this metric. Do not inflate or dilute the
  metric with reasoning, source editing, documentation, or genuinely novel
  diagnosis after a black-box failure. Report uncovered operations and their
  impact in the task's tool-use report instead of normalizing ad-hoc commands.
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
  coverage: defer lower-priority gaps rather than inventing new work to improve
  the score. New workflow families require a
  clear user need; an old one-off report alone is insufficient.
- Do not interrupt active work, change a tool or rule, or create a standalone
  commit for every low-impact observation. Keep a single occurrence in the
  owning agent report and review such observations in a batch
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
- Durable cooked-asset contracts and boundaries live in
  [toolchain knowledge](../docs/voyage-cooked-asset-toolchain-knowledge.md); the
  workflow and validation ladder live in
  [toolchain rules](../docs/voyage-cooked-asset-toolchain-rule.md).
- Durable findings about the Boat's Diesel socket hover card live in
  `docs/boat-resource-socket-architecture-knowledge.md`; active feature work belongs with
  the owning mod, using an ignored local backlog only when the task is large
  enough to need one.
- Before reusing a saved game-derived input, compare the installed game's Steam
  build ID with its provenance. A producer that identifies
  the game and re-extracts current stock inputs during the run satisfies this
  gate; do not add a separate manual fingerprint call before that producer.
- For a requested candidate, run the owning public producer first (or the
  documented build path where no public producer exists). Do not
  manually audit every game contract, tool version, or mapping before that run.
  Each game-bound mod keeps its pinned Steam build ID in `mods/<Mod>/game-build.json`;
  `version.txt` remains the published Nexus version. Every public producer that
  uses this pin accepts `-IgnorePinnedGameVersion`. On a pin mismatch, retry with
  that flag. It bypasses only comparison with the previous accepted build: the
  producer must record the installed game's actual build ID, use current stock
  inputs, and retain structural, provenance, and package checks. Do not relabel
  an old extracted input as current. No game-build pin is needed for a contract
  governed only by the engine or serialization format.
- If that retry cannot resolve reviewed mappings for the installed game, use
  `Get-VoyageMappings.ps1` to confirm the gap, then ask the user whether to
  update mappings or delegate that work; obtain the user's model choice before
  spawning a mapping agent. Retry the producer after the mapping is ready.
  Investigate source and game contracts only when the normal build path still
  fails or the game test reveals a problem. A successful build provides a
  structurally checked candidate; offer a real-game test next. A successful
  game test accepts the tested behavior on that build. Then update the mod's
  pinned ID without changing the tested package bytes. Keep the actual build
  ID, mapping identity, and the exact tested package in release evidence.
  General engine behavior follows its engine/serialization checks rather than
  acquiring a game-build gate solely because it was observed on one build.
- Keep mod version, artifact version, user-facing game version, Steam build ID,
  and Unreal Engine version as separate identities. A game
  update neither proves incompatibility nor requires a mod-version bump; it
  invalidates version-bound build inputs and requires new evidence before a new
  compatibility claim. Record whether a display/marketing version came from a
  machine-readable source or from the user, and never infer it from an
  executable version such as `UE5-CL-0`.
- Compatibility metadata may say `game-validated` only after the requested
  runtime behavior is checked in the real game. Compile, cook, container verify,
  semantic comparison, clean load, and save load remain lower gates.
- An installer that replaces a Voyage mod container must check the game process
  immediately before mutation, preserve an exact temporary recovery copy during
  mutation, install from a validated release manifest, and read back every
  installed hash. Keep transaction records and recovery copies in `Tmp/` only
  while they are needed for that operation. Keep ZIP archives outside the game;
  install only files the game consumes. This also applies to temporary builds.
- Any generated or reconstructed game-derived file that must remain in Git as
  a source input needs a provenance header naming its Steam build ID when the
  contract is game-specific, engine version, reconstruction method/tools, and
  revalidation condition. If the
  format cannot contain comments, record the same metadata in its owning
  README or registry.
