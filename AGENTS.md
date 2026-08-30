# Repository rules

These rules apply to the entire `TheLastCaretakerMods` repository. A closer
`AGENTS.md` owns the exact contracts and workflow of its mod; do not duplicate
those details here.

## Rule and knowledge ownership

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

## Research and knowledge protocol

- Optimize research for understanding the producer, consumer, owner,
  lifecycle, identity, and data contract—not merely for one patch that appears
  to work.
- Promote useful findings immediately: stable conclusions go to architecture
  or shared-pattern docs; failed/deceptive approaches go to research pitfalls;
  active hypotheses, package hashes, and pending test results go to the owning
  backlog.
- After context compaction, reread the active backlog and the relevant durable
  architecture/pitfall documents before acting.
- Before a visibly heavy investigation (large binary analysis, broad asset
  extraction/cook, long brute-force scan, or uncertain reverse engineering),
  tell the user why it is costly and offer a lighter discriminating experiment.
- Experiments must discriminate between hypotheses. After one or two no-op
  results, strengthen the marker or revisit the architecture instead of
  changing more fields blindly.
- A controlled crash or visibly wrong result is acceptable when it proves the
  intended path is live, the game process is disposable, and a recoverable
  baseline exists.

## Game-derived data and version gates

- Do not commit anything reproducible from the installed game: extracted
  assets, `.usmap`/`.jmap`, JSON, pseudocode, raw reports, disassembly dumps,
  cooked packages, container inventories, or installed-file backups. Keep
  version-specific output below ignored `artifacts/` paths.
- Commit the method, not the snapshot. Reusable fingerprinting, extraction,
  inspection, patching, and disassembly logic belongs under `tools/`, with
  exact usage documented.
- Treat `tools/README.md` as the tool-routing index. Read it before surveying
  tool implementations, and update it whenever a reusable tool is added,
  renamed, or its interface, prerequisites, outputs, or safety contract change.
- Before reusing any game-derived input, obtain the installed game's Steam
  build ID and executable hash with `tools/Get-VoyageBuildFingerprint.ps1` and
  compare them with the owning mod's provenance registry.
- A changed fingerprint invalidates extracted snapshots and reconstructed
  contracts alike. Re-extract or revalidate every affected item before build,
  cook, packaging, or compatibility claims.
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
