# Smart Outlet index

Use the [repository index](../../AGENTS.md) for shared rules and knowledge.

- [Architecture](ARCHITECTURE.md): behavior, state ownership, input/HUD and
  native-contract boundaries when changing runtime code.
- [Build and authoring](BUILD.md): project copies, candidate creation, validation
  and installation commands, including complete INI replacement and recovery.
- [Public producer](Build-SmartOutlet.ps1) cooks maintained project assets
  and creates the additive DML candidate and release manifest.
- [Installer](Install-SmartOutlet.ps1) deploys a candidate and its complete
  packaged INI. [Commented INI](Assets/SmartOutlet.ini) owns rule keys and defaults.
- [Editor checks](Test-SmartOutlet.ps1) execute compiled Blueprint
  suites against editor mirrors.
- [Editor dependencies](ARCHITECTURE.md#editor-dependencies-and-provenance): required identities,
  reconstruction and revalidation conditions.
- [Changelog](CHANGELOG.md): player-facing notes for the next Nexus release.

- [Shared BPT ownership](../../docs/blueprint-text-maintenance-rule.md): decision
  boundaries for proposed tool, process or shared parser changes. Routine mod
  authoring uses the [BPT workflow](../../tools/blueprint-workspace.md).
