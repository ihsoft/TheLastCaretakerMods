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
  each fact one owning source: do not duplicate artifact identities, build labels,
  test dates or results already owned by manifests or local evidence. Release
  compatibility constraints may distinguish versions; they are not a work log.
- Shared `docs/` content must have a concrete or reasonably expected use across
  mods. Keep mod-only contracts under the owning `mods/<mod>/`.
  Shared rules and knowledge describe the recommended approach themselves.
  A concrete mod may appear only as an optional context link, never as the
  canonical implementation, a required reading step, or an example of how work
  must be done. Keep such a link only when it adds a useful detail, such as a
  rare consumer-specific boundary, and explain that value at the link. The
  shared contract must remain understandable without opening it. Do not keep
  task histories or per-run measurements in the shared reading path.
  For explicitly requested resource diagnostics, use the
  [agent work measurement method](agent-work-measurement-rule.md).
- Root and mod `AGENTS.md` files are compact indexes, not collections of full
  rules or architecture descriptions. Keep substantive guidance in its owning
  document. Explain each link by the task or decision it helps with, so an agent
  can choose whether to read it before loading the target. Do not require reading
  every indexed document or following every link unconditionally. The same
  selective-reading principle applies to links within shared and mod documents;
  identify required dependencies by the operation that makes them relevant.
- Shared guidance remains applicable until there are reasonable grounds to doubt
  it on the current game version. An update alone does not invalidate a document
  or require repeating its research. A verified game version may qualify the
  evidence for a specific claim; it must not become a blanket prerequisite for
  using the document. Record demonstrated compatibility limits on the affected
  contract, mapping, or artifact rather than pinning the whole document to a
  game version. Recheck affected claims when observed behavior, changed source
  contracts, or tool diagnostics give reason to doubt them.
- Repository-level README files are short folder labels. The tool index routes
  tasks to focused guides; the tool catalog records public entry points. Those
  guides contain only verified, established capabilities, current interfaces,
  and safety/compatibility limits.
  Describe the public call, its useful result, and the next action on failure.
  Do not copy a tool's implementation steps, pinned dependency commits, binary
  per-file digests, or historical test counts into normal agent guidance. Run the selected
  tool first; inspect its diagnostics and implementation only when it fails or
  gives an unexpected result. Keep exact provenance in the tool's own checks
  and generated manifests only when those identities matter under the shared
  [hash rule](hash-usage-rule.md).
  Promote validated conclusions as general contracts without candidate labels;
  retain demonstrated version/fingerprint restrictions on the specific contracts
  or artifacts whose applicability they determine.
  Durable documentation describes accepted repository state. Until the requested
  end state and its required acceptance checks pass, keep findings and proposed
  contracts in the ignored backlog, not in documentation edits, including
  uncommitted drafts. Follow the
  [research workflow](research-workflow-rule.md#working-backlogs-for-large-tasks)
  for final documentation derived from that backlog and the boundaries for an
  initial mod skeleton or an explicitly requested process rule. Describe
  capabilities only at the level supported by their
  evidence; build, structural, installation and clean-load success do not
  establish gameplay behavior.
- Before committing documentation, make every link to a repository-local file
  resolve in the committed repository. Committed guidance must not depend on
  ignored or otherwise local-only files, including working backlogs and output
  under `Tmp/` or `artifacts/`. Plain paths that specify where a tool writes
  output are not links to existing resources. During an uncommitted
  investigation, local working files may link to each other; resolve such links
  before committing the documents that contain them.
- Mod documentation uses four core names with distinct responsibilities:
  `AGENTS.md` indexes task-specific entry points; `ARCHITECTURE.md` owns behavior,
  state, implementation constraints and dependency contracts; `BUILD.md` owns
  source layout, authoring, build, validation and installation entry points;
  `README.md` supplies the short public description and candidate build command.
  Create `BUILD.md` when supported authoring or build operations exist, not as
  an empty promise in a new skeleton. Put editor-input provenance in a focused
  architecture section; a separate provenance document is justified when its
  size or independent use warrants it. Other focused documents are allowed for
  the same reason, but must not compete with these owners. `CHANGELOG.md`
  remains separate publication input under the rule below.
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
- Each mod's `CHANGELOG.md` is concise player-facing publication input. Keep
  the cumulative result since the latest published release under `Next`, index
  it from the mod's `AGENTS.md`, and follow the shared release rule when
  revising or finalizing it. It records neither commit history nor discarded
  intermediate behavior. It is not an architecture, knowledge, rule, work log,
  or release payload file.
- Describe finished mods by their current behavior and necessary compatibility
  boundaries. Keep exact published release packages with artifacts; selected
  unpublished candidates follow the release workflow below.
- The repository-wide [release and installation workflow](../docs/release-and-installation-rule.md)
  owns published-version identity, candidate retention, installation evidence,
  and installed-file layout for every mod. Mod-local rules may add necessary
  payload or migration contracts, but may not redefine those shared identities.
- Current source, the closest rules, durable documentation, and the latest
  game-validated Git checkpoint are sources of truth. Chat history and local
  extracted artifacts are not.
- Keep rules minimal and owned: remove obsolete or counterproductive rules,
  merge duplicates, and put command syntax and implementation conventions in
  the owning tool documentation rather than repeating them here. A short context
  summary with a link is sufficient where another document owns the detail;
  do not copy substantial explanations without a concrete need. Use enough
  detail to make scope, obligations, and exceptions clear, without padding or
  shortening away necessary meaning. Check changed guidance against its owning
  shared and local rules, and resolve contradictions instead of leaving competing
  instructions in place. Replace a rule only after preserving its still-valid
  safety/data contract. Record the
  reason and verification with the change; do not retain a chronological
  history of superseded rules in the normal agent reading path.
