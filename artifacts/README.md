# Local retained artifacts

Generated data below this directory stays ignored; only this registry and the
local `AGENTS.md` are tracked. The shared
[generated-output lifecycle](../docs/artifact-lifecycle.md) determines when a
result is worth keeping beyond its current work cycle. Disposable logs, build
output, one-off checks and ordinary readbacks belong in repository `Tmp/`.
Retained game-derived inputs keep their relevant Steam build and asset provenance.

| Family | Owner and creator | Consumer and retention value | Owning contract |
| --- | --- | --- | --- |
| `asset-cache/**` | Shared game-asset retrieval; populated by `Get-VoyageAssetJson.ps1` through the inspector | `Get-VoyageAssetJson.ps1`, `Get-VoyageAssetSummary.ps1` and research reuse the real stock cache | [Stable JSON retrieval](../tools/README.md#stable-json-retrieval-for-individual-assets) |
| `asset-cache/<version>/_summaries/**` | Shared compact-summary subsystem; created by `Get-VoyageAssetSummary.ps1` for validated stock JSON | Research calls reuse content-addressed derived summaries; removing an obsolete game-cache version also removes its summaries | [Compact Blueprint structure](../tools/README.md#compact-blueprint-structure) |
| `installations/<Mod>-V<N>/**` | Selected release candidate and the named mod | One exact ZIP and release manifest for publication, identification, and later reinstallation; a same-version collision stops the release | [Release workflow](../docs/release-and-installation.md), [release installation](../tools/README.md#release-installation) |

Earlier timestamped installation directories are historical recovery inputs.
Do not rename or remove them while a restore or mod-specific migration still
refers to their exact path or hash.

Documenting a present family records its owner and contract; it does not make
every existing run permanently valuable. Consult the owner before cleanup.
