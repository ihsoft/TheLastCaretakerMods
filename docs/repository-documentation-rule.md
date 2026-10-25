# Rule and knowledge ownership

- Substantive shared documents under `docs/` end in `-rule.md` or
  `-knowledge.md`. A rule constrains repository work: follow it, or report a
  conflict or blocker to the user and obtain an explicit decision before
  deviating. Knowledge records verified facts about the game, engine, or
  established repository methods.
  Recheck or discard knowledge when new evidence conflicts with it. A report
  that cannot be independently reproduced or investigated is not established
  knowledge. Knowledge saves investigation effort but does not prevent a new
  investigation. `README.md`,
  source-provenance files, and other navigational or data files keep their own
  established names outside this shared-document naming pattern.
- Do not commit guesses, assumptions or unverified hypotheses anywhere in the
  repository, including documentation, comments and metadata such as
  `VERSION.json`, unless the user explicitly requests committing those claims.
  Labelling a claim uncertain or obtaining general commit approval does not
  grant that exception. A reported symptom does not establish a defect, its
  cause or a known limitation. Committed claims must be supported by evidence
  at the level they assert and useful to a future reader.
- Work history consumes time and context. Never commit session narratives,
  investigation diaries, trial chronology, or statements that merely say what
  was previously done or tested, including disguised history in JSON metadata
  or comments. Keep such history only in a local Git-ignored file when a large
  investigation or prolonged fix needs it to survive compaction or resume work.
  Keep only the minimum restart context and discard it when the work ends.
- Keep a durable fact only when it changes a concrete implementation,
  compatibility, release-selection or recovery decision. State the current
  contract and its applicability, without the story of discovering it. Give
  each fact one owning source: do not duplicate artifact hashes, build labels,
  test dates or results already owned by manifests or local evidence. Release
  compatibility constraints may distinguish versions; they are not a work log.
- Shared `docs/` content must have a concrete or reasonably expected use across
  mods. Keep mod-only contracts under the owning `mods/<mod>/`.
  Write reusable conclusions without tying them to one mod; link to a mod as an
  example when useful. Do not keep task histories or per-run measurements in the
  shared reading path. Explicitly requested diagnostics use the opt-in method
  linked from the subagent workflow.
- Repository-level README files are short folder labels. The tool index routes
  tasks to focused guides; the tool catalog records public entry points. Those
  guides contain only verified, established capabilities, current interfaces,
  and safety/compatibility limits.
  Describe the public call, its useful result, and the next action on failure.
  Do not copy a tool's implementation steps, pinned dependency commits, binary
  hashes, or historical test counts into normal agent guidance. Run the selected
  tool first; inspect its diagnostics and implementation only when it fails or
  gives an unexpected result. Keep exact provenance in the tool's own checks
  and generated manifests when those identities matter.
  Promote validated conclusions as general contracts without candidate labels;
  retain version/fingerprint restrictions when they determine applicability.
  Durable documentation describes accepted repository state. Do not update it
  ahead of a pending runtime gate: when a behavior or presentation claim needs
  a real-game test, keep the proposed documentation change uncommitted until
  that test passes. Describe capabilities only at the level supported by their
  evidence; build, structural, installation and clean-load success do not
  establish gameplay behavior.
- Before committing documentation, make every link to a repository-local file
  resolve in the committed repository. Committed guidance must not depend on
  ignored or otherwise local-only files, including working backlogs and output
  under `Tmp/` or `artifacts/`. Plain paths that specify where a tool writes
  output are not links to existing resources. During an uncommitted
  investigation, local working files may link to each other; resolve such links
  before committing the documents that contain them.
- A README is a short label for its directory, not an agent knowledge source.
  Each mod has one `README.md` with a brief description and the command to
  build a candidate from the command line. If no supported build command
  exists, say so rather than inventing one. The shared release rule owns the
  ordinary build and installation process; the mod index points directly to
  its public producer. Put only mod-specific behavior, settings, exceptional
  build boundaries, and compatibility contracts in their owning documents.
  AGENTS indexes and agent-facing documents link to those documents instead
  of a README. The tools directory uses `tools/tool-index.md` for routing and
  `tools/tool-catalog.md` for entry-point registration. Its README is a short
  folder description. Do not maintain a second `README.txt` with a
  competing summary.
  If a release ZIP includes a README, package that same `README.md`.
- Describe finished mods by their current behavior and necessary compatibility
  boundaries. Keep exact selected release packages with artifacts.
- The repository-wide [release and installation workflow](../docs/release-and-installation-rule.md)
  owns published-version identity, candidate retention, installation evidence,
  and installed-file layout for every mod. Mod-local rules may add necessary
  payload or migration contracts, but may not redefine those shared identities.
- Autonomous DonkLift rules live in
  `mods/DonkLiftKeyboardControl/MOD-RULES.md`; its `AGENTS.md` indexes them.
- The preserved UE4SS/Lua DonkLift implementation is a separate alternative
  under `mods/DonkLiftKeyboardControlUE4SS`; it is not a dependency of the
  autonomous mod.
- Shared Voyage vehicle/HUD patterns belong in
  `docs/vehicle-and-hud-modding-patterns-knowledge.md`. Evidence and object-specific
  findings remain in the architecture and research documents.
- Current source, the closest rules, durable documentation, and the latest
  game-validated Git checkpoint are sources of truth. Chat history and local
  extracted artifacts are not.
- Keep rules minimal and owned: remove obsolete or counterproductive rules,
  merge duplicates, and put command syntax and implementation conventions in
  the owning tool documentation rather than repeating them here. Replace a
  rule only after preserving its still-valid safety/data contract. Record the
  reason and verification with the change; do not retain a chronological
  history of superseded rules in the normal agent reading path.
