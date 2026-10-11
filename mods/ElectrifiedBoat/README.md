# ElectrifiedBoat

ElectrifiedBoat lets you connect to your boat's electrical network anywhere
you can build an **Electric Wall Socket**. On a boat, the socket connects to
the boat's power supply instead of looking for a matching socket through the
wall. Existing sockets are connected when you load your save, too.

Away from a boat, wall sockets work normally. Other resource sockets are not
changed. No new building item, controls, or configuration are required.

## Distribution

Requires **DML v0.6**.

1. Close the game and extract `ElectrifiedBoat.pak`, `ElectrifiedBoat.ucas`, and
   `ElectrifiedBoat.utoc` together into the game's `Voyage/Content/Paks` folder
   or a mod subfolder inside it. A `LogicMods` folder is not required.
2. Start the game. If the mod is not already registered in DML, run
   `DML add ElectrifiedBoat` in the console.
3. Build an Electric Wall Socket on your boat and connect a cable to it.

Keep only one version of this mod installed. Back up your save before removing
the mod; loading a save without it is not a guaranteed compatibility feature.

## Build and validation

Building requires Windows, Unreal Engine **5.8.2** with its C++ build tools,
the installed game, and the repository's prepared
[build tools](../../tools/README.md). The build checks the supported game
version; its exact fingerprint is in
[GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md).

Run this from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File .\mods\ElectrifiedBoat\Build-ElectrifiedBoat.ps1
```

The defaults are `K:\Epic Games\UE_5.8` for the engine and
`P:\SteamLibrary\steamapps\common\Voyage` for the game. To use other locations,
append `-EngineRoot 'C:\path\to\UE_5.8'` and
`-GameRoot 'D:\path\to\Voyage'` to the command.

The script builds the mod and checks the resulting package. Output goes to
`Tmp/ElectrifiedBoat/build-<timestamp>/package/`. Install only the three
`ElectrifiedBoat.pak/.ucas/.utoc` files as described above; the other files are
build metadata. The script does not install the mod or produce a release ZIP.

For development contracts, see [AGENTS.md](AGENTS.md).
