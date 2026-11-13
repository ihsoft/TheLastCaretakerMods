# Smart Outlet index

Use the [repository index](../../AGENTS.md) for shared rules and knowledge.

- [Architecture](ARCHITECTURE.md): behavior, state ownership, input/HUD and
  native-contract boundaries when changing runtime code.
- [Development](DEVELOPMENT.md): project copies, authoring and validation commands.
- [Public producer](Build-SmartOutlet.ps1) cooks maintained project assets
  and creates the additive DML candidate and release manifest.
- [Installer](Install-SmartOutlet.ps1) deploys a candidate and preserves the
  player's INI. [Commented INI](Assets/SmartOutlet.ini) owns rule keys and defaults.
- [Editor checks](Validate-SmartOutlet.ps1) execute all six compiled Blueprint
  suites against editor mirrors and offline settings/installation tests.
- [Source provenance](SOURCE-PROVENANCE.md): required editor identities,
  reconstruction and revalidation conditions.
- [Changelog](CHANGELOG.md): player-facing notes for the next Nexus release.

The user routes BPT tool/process observations to
[the BPT curator](codex://threads/01a1ba44-22d4-7ac3-9707-d7ae9c210e4e).
Send findings there; do not modify shared BPT tooling or process from this task.
The shared INI parser is also curator-owned; the mod maintains its imported copy.
