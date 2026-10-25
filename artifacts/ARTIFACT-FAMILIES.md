# Retained artifact families

Generated data below this directory stays ignored. The shared
[generated-output lifecycle](../docs/artifact-lifecycle-rule.md) determines
when a result is worth keeping beyond the current work cycle. Disposable
logs, build output, one-off checks and ordinary readbacks belong in `Tmp/`.
Retained game-derived inputs keep relevant Steam build and asset provenance.

| Family | Owner and creator | Consumer and retention value | Owning contract |
| --- | --- | --- | --- |
| `asset-cache/**` | Shared game-asset retrieval; populated by `Get-VoyageAssetJson.ps1` through the inspector | `Get-VoyageAssetJson.ps1`, `Get-VoyageAssetSummary.ps1` and research reuse the real stock cache | [Complete asset JSON](../tools/asset-inspection.md#complete-asset-json-or-package-inventory) |
| `asset-cache/<version>/_summaries/**` | Shared compact-summary subsystem; created by `Get-VoyageAssetSummary.ps1` for validated stock JSON | Research calls reuse content-addressed derived summaries; removing an obsolete game-cache version also removes its summaries | [Compact Blueprint structure](../tools/asset-inspection.md#compact-blueprint-structure) |
| `installations/<Mod>-V<N>/**` | Published release and the named mod | One exact published ZIP and its original release manifest for identification and reliable rollback; a same-version collision stops retention | [Release workflow](../docs/release-and-installation-rule.md), [release installation](../tools/release-installation.md#release-installation) |

Canonical `<Mod>-V<N>` directories are trusted, published rollback versions
under the current release workflow. Earlier noncanonical directories predate
this contract; do not infer publication from their location, and do not rename
or remove them while a restore or mod-specific migration still refers to their
exact path or hash.

Documenting a present family records its owner and contract; it does not make
every existing run permanently valuable. Consult the owner before cleanup.
