# ScopeFix

ScopeFix adjusts aiming alignment for the electric rifle and the RedDot sight
in The Last Caretaker. Install its container triplet directly; no additional
mod, UE4SS or mod loader is required.

## Behavior

The mod adjusts the camera position used while aiming. The accepted settings
prioritize long-range alignment: the user observed improved aiming at long
range and shots below the aim point at close range, and accepted that tradeoff.
Weapon animation makes precise impact assessment difficult, so this is a
qualitative game-tested result rather than a measured accuracy guarantee.

Two settings are involved: the electric rifle's aiming-camera offset and the
shared RedDot sight's own offset. Other sight assets are unchanged. RedDot's
adjustment also applies when fitted to other compatible weapons, although
those combinations were not separately reported as tested.

See [Aiming configuration](docs/aiming-configuration.md) for the reasoning,
exact values and supported game fingerprint.

## Installation

Close the game and place the matching `ScopeFix_P.pak`, `ScopeFix_P.ucas`
and `ScopeFix_P.utoc` files in the game's `Voyage/Content/Paks` directory.
Use all three files from the same archive. Avoid combining overrides of the
same electric-rifle or RedDot data assets.

To remove a manual installation, close the game and remove those three files.
Repository-managed installation and rollback use `Install-VoyageRelease.ps1`
and `Restore-VoyageReleaseInstallation.ps1` with their respective manifests.

## Build

From the repository root, with Windows PowerShell 5.1 or later:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/ScopeFix/Build-ScopeFix.ps1 -OutputRoot artifacts/scope-fix/build-new
```

The output directory must be new. The builder reads the installed game's
stock assets through the repository tools, checks their fingerprint and
camera-offset values, changes only two values, and produces a verified
container triplet plus `build-evidence.json`. It requires the published
repository tools and reviewed mappings. It does not install the mod.

For release packaging, archive the exact triplet and call
`tools/New-VoyageReleaseManifest.ps1` with `-SourcePath mods/ScopeFix`.
Follow the parameter contract in the [tool index](../../tools/README.md).
Each new artifact still needs appropriate validation; a successful container
check alone does not establish runtime compatibility.
