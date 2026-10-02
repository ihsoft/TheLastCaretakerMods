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

## Reviewed entries

The registry currently retains reviewed entries for Steam builds `25056839`
and `25191271`. Each sibling `mapping-manifest.json` is the authority for its
game hash, engine/parser profile, generator identity, mapping hash, validation
and revalidation condition; do not duplicate those values into consumer code.

Normal callers use `tools/Get-VoyageMappings.ps1`, which selects and validates
the entry matching the installed game. After a genuinely new fingerprint,
`tools/New-VoyageMappings.ps1 -InstallForUAssetGUI` can install a newly generated
and validated mapping into UAssetGUI. Portable or headless tests should use the
exact path returned by the resolver rather than choosing a registry directory
manually.
