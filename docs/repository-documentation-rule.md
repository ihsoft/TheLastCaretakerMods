# Rule and knowledge ownership

- Substantive shared documents under `docs/` end in `-rule.md` or
  `-knowledge.md`. A rule constrains repository work: follow it, or report a
  conflict or blocker to the user and obtain an explicit decision before
  deviating. Knowledge records useful observations or reasoned inferences about
  the game, engine, or established repository methods;
  state uncertainty in the document heading and at the affected claim.
  Recheck or discard knowledge when new evidence conflicts with it. A report
  that cannot be independently reproduced or investigated is not established
  knowledge. Knowledge saves investigation effort but does not prevent a new
  investigation. `README.md`,
  source-provenance files, and other navigational or data files keep their own
  established names outside this shared-document naming pattern.
- Shared `docs/` content must have a concrete or reasonably expected use across
  mods. Keep mod-only contracts and open questions under the owning `mods/<mod>/`.
  Write reusable conclusions without tying them to one mod; link to a mod as an
  example when useful. Do not keep task histories or per-run measurements in the
  shared reading path. Explicitly requested diagnostics use the opt-in method
  linked from the subagent workflow.
- Repository-level README files are short folder labels. The shared tool index
  contains only verified, established capabilities, current interfaces and
  safety/compatibility limits.
  Keep candidate IDs, experimental hypotheses, trial instructions and progress
  history in an optional ignored owning backlog for a large investigation;
  keep raw evidence in ignored storage, not in shared guidance.
  Promote validated conclusions as general contracts without candidate labels;
  retain version/fingerprint restrictions when they determine applicability.
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
  of a README. The existing `tools/README.md` remains the tool-routing index
  until its separate review. Do not maintain a second `README.txt` with a
  competing summary.
  If a release ZIP includes a README, package that same `README.md`.
- Describe finished mods by their current behavior and necessary compatibility
  boundaries. The checked-in state is the accepted result; do not preserve who
  confirmed it, trial chronology, test dates, or per-run proof in normal mod
  documentation. Keep exact selected release packages with artifacts.
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
