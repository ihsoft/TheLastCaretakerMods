# Container Lid Label

Adds the container name below the count on the large FortiHold lid plate, with
left/right icons beside the count. A sole icon keeps its native side, and the
name remains visible with icons. The stock side label, count, capacity bar,
inventory contents and lid mesh behavior remain in place.

The mod also makes the large container's mass capacity configurable. Default
capacity remains the stock 200 kg. Other container types are unaffected.

## Configuration

Runtime file: `Voyage/Content/Paks/ContainerLidLabel.ini`.
Authored default: [ContainerLidLabel.ini](Assets/ContainerLidLabel.ini).

```ini
# Large-container mass limit in kilograms.
LargeContainerMaxWeightKg=200
```

The flat parser trims whitespace, accepts standalone `;` and `#` comments, and
uses the last valid matching key. Inline comments are unsupported. Missing or
invalid input falls back to the embedded 200 kg default. Values must be finite,
positive and representable as a positive float.

The generated Large class caches the result once while its class default object
remains loaded. Containers and persistent-load callbacks reuse that value;
class unload/reload may reread. Restart the game to apply INI changes. Lowering
the limit never clears, recreates or trims stored stacks.

For an older installation using `MaxWeightKg`, rename the key to
`LargeContainerMaxWeightKg`, preserving the value and comments. The old key is
not an alias. The common container installer does not manage this external INI;
back it up, migrate it separately and verify the result.

## Build and compatibility

Build with Windows PowerShell 5.1 from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File `
  .\mods\ContainerLidLabel\Build-ContainerLidLabel.ps1 `
  -Version v6-test -AllowDirtySource
```

The producer uses Unreal Engine 5.8 at `K:\Epic Games\UE_5.8` and Voyage at
`P:\SteamLibrary\steamapps\common\Voyage` by default. It writes a fresh release
below `Tmp/ContainerLidLabel`, validates the two-package `_P` container, includes
the INI beside the container and in the ZIP, and produces a release manifest.
It never installs. For manifest-only refresh of an existing validated release,
use `-FinalizeExisting` with its exact `-OutputRoot` and matching `-Version`;
this does not rebuild or modify the payload.

The mod is user-validated on Steam build 25929985. Exact source and compatibility
identities belong to [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md).
[Current state](BACKLOG.md) identifies the validated checkpoint, rollback evidence
and limits. [Architecture](ARCHITECTURE.md) describes package/component ownership,
lifecycle calls and cache behavior. Another mod overriding the same stock Large
Blueprint can conflict with this one.

Use the [shared release workflow](../../tools/README.md#release-producers) and
[guarded installation/restore](../../tools/README.md#release-installation).