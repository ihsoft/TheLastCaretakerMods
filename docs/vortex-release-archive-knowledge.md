# Vortex release archive layouts

This knowledge describes ZIP layouts for The Last Caretaker's Vortex extension.
Use the [release rule](release-and-installation-rule.md) for candidate identity,
retention, and acceptance. The ZIP contains files Vortex must deploy, with no
enclosing version directory. `tools/New-VoyageVortexArchive.ps1` selects and
checks these layouts for a standard three-file IoStore container.

| Payload | ZIP entries | Vortex destination | Override |
| --- | --- | --- | --- |
| Plain asset patch | `.pak`, `.ucas`, `.utoc` at root | `Voyage/Content/Paks/~mods/` | None |
| PAK plus adjacent INI or another Paks file | Same flat entries plus `vortex_override_instructions.json` | `Voyage/Content/Paks/` | `thelastcaretaker-pakalt` |
| New content with `.uplugin` | `Voyage/Content/Paks/...` and `Voyage/Mods/<Plugin>/<Plugin>.uplugin` | The corresponding paths below game root | `thelastcaretaker-root` |
| Plain DML actor | Explicit `LogicMods/` directory entry and `LogicMods/<container triplet>` | `Voyage/Content/Paks/LogicMods/` | None |

An override is a JSON array with one `copy` instruction per payload file,
followed by one `setmodtype` instruction. Each `source` is an existing ZIP
entry. Each `destination` is relative to the chosen mod type's target:

```json
[
  { "type": "copy", "source": "Example.ini", "destination": "Example.ini" },
  { "type": "copy", "source": "Example_P.pak", "destination": "Example_P.pak" },
  { "type": "copy", "source": "Example_P.ucas", "destination": "Example_P.ucas" },
  { "type": "copy", "source": "Example_P.utoc", "destination": "Example_P.utoc" },
  { "type": "setmodtype", "value": "thelastcaretaker-pakalt" }
]
```

For a bundle with `.uplugin`, use full `Voyage/...` paths for both `source`
and `destination` and select `thelastcaretaker-root`. A LogicMod with an INI
or other file needing a separate destination also uses this game-root layout:
put its triplet under `Voyage/Content/Paks/LogicMods/` and map every file.
For another mixed destination without a plugin or LogicMods, the archive
producer's `-RootLayout` and `-AdditionalDestination` select the same explicit
game-root mapping.
An INI placed beside a PAK is distinct from the game's user configuration
under `%LOCALAPPDATA%/Voyage/Saved/Config/Windows/`; choose its destination
from the mod's runtime contract. If a plain patch needs Paks root instead of
`~mods`, use the flat `pakalt` override even without an extra file.

The `LogicMods/` directory entry matters: the extension detects that entry,
not merely file paths beneath it. Vortex places DML actors; it does not itself
provide DML. A release containing both a content plugin and a PAK should not
rely on generic PAK detection to deploy the descriptor.

The flat PAK+INI layout works with StableGyro in Vortex. The game-root
`.uplugin` bundle works with Railgun in Vortex. The plain PAK and LogicMods
layouts follow the extension's installer code and need their own deployment
checks before being called tested. For any release, install, enable,
and deploy in Vortex, then inspect the resulting game files before claiming
Vortex compatibility. A successful ZIP or manifest check proves package
structure, not Vortex deployment or runtime behavior.

References: [TLC Vortex extension](https://github.com/ChemGuy1611/ChemBoy1-Vortex-Games/blob/main/game-thelastcaretaker/index.js),
[Nexus override instructions](https://github.com/Nexus-Mods/Vortex/wiki/File-Based-Override-Instructions).
