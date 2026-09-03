# Voyage mappings registry

This is the repository's narrow exception to the general rule against tracking
game-derived data. It contains reviewed Voyage `.usmap` files needed to make the
validated UAssetGUI workflow usable on a clean machine.

Each mapping is immutable and scoped by:

- Steam build ID;
- `VoyageSteam-Win64-Shipping.exe` SHA-256;
- game Unreal Engine version and UAssetAPI parser mode;
- generator repository/commit and mapping SHA-256.

Never replace an existing file after a game update. Generate and validate the
new mapping below ignored `artifacts/mappings/`, then add a new versioned
directory with its own manifest. Dumper logs, reflection scans, process
addresses, `.jmap`, and other reproducible diagnostics remain ignored.

Normal consumers must not browse this registry or regenerate mappings. Run
`tools/Get-VoyageMappings.ps1`; it fingerprints the installed game, selects and
validates the matching reviewed entry, and returns its exact path. Generate a
candidate only when that resolver proves no reviewed entry matches a confirmed
new game fingerprint.

## Current mapping

The current reviewed entry is:

`Voyage/steam-25056839-ue5.8.1/Voyage-25056839.usmap`

- Game: The Last Caretaker / Voyage
- Steam App ID: `1783560`
- Steam build: `25056839`
- Game engine: Unreal Engine `5.8.1`
- UAssetAPI/UAssetGUI parser mode: `VER_UE5_8`

Validate it from the repository root:

```powershell
.\tools\Test-VoyageMappings.ps1 `
  -MappingsPath '.\mappings\Voyage\steam-25056839-ue5.8.1\Voyage-25056839.usmap' `
  -ManifestPath '.\mappings\Voyage\steam-25056839-ue5.8.1\mapping-manifest.json' `
  -ExpectedSteamBuildId '25056839' `
  -ExpectedExecutableSha256 'CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933'
```

For normal UAssetGUI use, copy it to:

`%LOCALAPPDATA%\UAssetGUI\Mappings\Voyage-25056839.usmap`

For `--portable` use, copy it to `Data\Mappings` beside the executable. Headless
tests should pass the tracked path explicitly to the canonical
`.tools\bin\UAssetGUI.exe`.
