# Smart Outlet index

Start with the [repository index](../../AGENTS.md) for shared rules and
knowledge. For a new mod, follow its
[starting workflow](../../docs/new-mod-rule.md).

- **Read first for every mod task:** [Architecture](ARCHITECTURE.md) owns
  behavior, lifecycle, material authoring, native mirrors and validation limits.
- [Public producer](Build-SmartOutlet.ps1) cooks maintained project assets
  and creates the additive DML candidate and release manifest.
- [Installer](Install-SmartOutlet.ps1) deploys a candidate and preserves the
  player's INI. [Commented INI](Assets/SmartOutlet.ini) owns rule keys and defaults.
- [Editor checks](Validate-SmartOutlet.ps1) execute all six compiled Blueprint
  suites against editor mirrors; wrap the script in the shared Unreal queue.
- [Changelog](CHANGELOG.md): player-facing notes for the next Nexus release.

Smart Outlet is a maintained mod using optional section-based input/output
profiles and per-port mode selection. The shared parser is owned by the BPT
curator; this project imports a separate runtime copy. Required editor-only
game identities are documented in [source provenance](SOURCE-PROVENANCE.md).
Candidates remain under `Tmp/SmartOutlet/` until publication;
`version.txt` advances only after a numbered Nexus release.

The user routes BPT tool/process observations to
[the BPT curator](codex://threads/01a1ba44-22d4-7ac3-9707-d7ae9c210e4e).
Send findings there; do not modify shared BPT tooling or process from this task.
Ordinary mod-specific Blueprint authoring still uses the existing BPT workflow.
