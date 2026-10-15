# Local retained artifacts

Generated data below this directory stays ignored; only this registry and the
local `AGENTS.md` are tracked. Disposable logs, build output, one-off checks and
ordinary readbacks belong in repository `Tmp/`. Game-derived data remains bound
to its recorded game fingerprint.

| Family | Owner and creator | Consumer and retention value | Owning contract |
| --- | --- | --- | --- |
| `asset-cache/**` | Shared game-asset retrieval; populated by `Get-VoyageAssetJson.ps1` through the inspector | `Get-VoyageAssetJson.ps1`, `Get-VoyageAssetSummary.ps1` and research reuse the real stock cache | [Stable JSON retrieval](../tools/README.md#stable-json-retrieval-for-individual-assets) |
| `asset-summaries/game/**` | Shared compact-summary subsystem; created by `Get-VoyageAssetSummary.ps1` | Research calls reuse content-addressed derived summaries; they are not Railgun build inputs | [Compact Blueprint structure](../tools/README.md#compact-blueprint-structure) |
| `installations/<mod>/<installation>/**` | Shared installer/restore subsystem and the named mod; created by `Install-VoyageRelease.ps1` and `Restore-VoyageReleaseInstallation.ps1` | Rollback and installation provenance consume the manifest and its inseparable predecessor backups | [Release installation](../tools/README.md#release-installation) |
| `railgun/**` | Railgun; current contents are reviewed one-off migration evidence under `migrations/**` | Manual reviewed recovery uses migration records and backups. The current producer writes builds to `Tmp/Railgun` and retains release ZIPs in Voyage Paks, not here | [Railgun build and recovery](../mods/Railgun/README.md), [Railgun rules](../mods/Railgun/AGENTS.md) |

Documenting a present family records its owner and contract; it does not make
every existing run permanently valuable. Consult the owner before cleanup.
