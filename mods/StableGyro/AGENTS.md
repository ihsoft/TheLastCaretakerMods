# StableGyro index

Start with the [repository index](../../AGENTS.md) for shared rules and knowledge.

- [Architecture](ARCHITECTURE.md): binding implementation contracts, helper
  lifecycle, input, compass, fuel, cargo, saved identities and serialization; read
  the affected sections before changing behavior or dependencies.
- [Build and authoring](BUILD.md): source layout, working copies, BPT, explicit
  source updates and candidate installation.
- [Item authoring](BUILD.md#item-authoring): edit the saved
  DataAsset through BPT and preserve the explicit stock-refresh recipe.
- [Candidate producer](Build-StableGyro.ps1): cook and validate a disposable copy;
  `-Install` delegates to the same candidate installer.
- [Candidate installer](Install-StableGyro.ps1): install a supplied release
  manifest with the complete packaged INI, without rebuilding.
- [Source update commands](BUILD.md#source-updates): why and when to refresh the
  stock-derived item or mooring plate, public calls and acceptance boundaries.
- [Altitude control](ALTITUDE-CONTROL.md): native hover, vertical-control and fuel
  formulas that bound the mod.
- [Editor-input provenance](ARCHITECTURE.md#editor-input-provenance): reconstructed
  inputs, source-state owners and contract-specific revalidation.
- [Gameplay validation](BUILD.md#gameplay-validation): required scenarios for
  affected behavior; distinguish them from established compatibility.
- [Map UI](ARCHITECTURE.md#minimap-and-interactive-map): player centering,
  coverage, opacity composition and the native interactive overlay.
- [Changelog](CHANGELOG.md): cumulative player-facing notes for publication.
