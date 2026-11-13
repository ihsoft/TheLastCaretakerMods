# StableGyro index

Start with the [repository index](../../AGENTS.md) for shared rules and knowledge.

- [Architecture](ARCHITECTURE.md): binding implementation contracts, helper
  lifecycle, input, compass, fuel, cargo, saved identities and serialization; read
  the affected sections before changing behavior or dependencies.
- [Build and authoring](BUILD.md): source layout, working copies, BPT, explicit
  source updates, automated checks and candidate installation.
- [Candidate producer](Build-StableGyro.ps1): cook and validate a disposable copy;
  `-Install` delegates to the same candidate installer.
- [Candidate installer](Install-StableGyro.ps1): install a supplied release
  manifest with user INI preservation, without rebuilding.
- [Source recovery](Restore-StableGyroSourceUpdate.ps1): recover an interrupted
  source acceptance from its persisted manifest; usage and conflict handling
  are in [source updates](BUILD.md#source-updates).
- [Altitude control](ALTITUDE-CONTROL.md): native hover, vertical-control and fuel
  formulas that bound the mod.
- [Editor-input provenance](ARCHITECTURE.md#editor-input-provenance): reconstructed
  inputs, source-state owners and contract-specific revalidation.
- [Gameplay validation](BUILD.md#gameplay-validation): required scenarios for
  affected behavior; distinguish them from established compatibility.
- [Changelog](CHANGELOG.md): cumulative player-facing notes for publication.
