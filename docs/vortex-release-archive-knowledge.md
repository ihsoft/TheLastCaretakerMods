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
| Game-root bundle, including new content with `.uplugin` | Only payload entries below `Voyage/...` | Paths represented by the ZIP entries | None |
| Plain DML actor | Explicit `LogicMods/` directory entry and `LogicMods/<container triplet>` | `Voyage/Content/Paks/LogicMods/` | None |

Vortex extension 1.1.0 and newer recognizes an archive whose payload paths
start with `Voyage/` and deploys those files unchanged below the game root. A
content-plugin bundle therefore places its container at
`Voyage/Content/Paks/...` and its descriptor at
`Voyage/Mods/<Plugin>/<Plugin>.uplugin`. It contains neither
`vortex_override_instructions.json` nor FOMOD metadata.

A LogicMod with an INI or another file needing a separate destination also uses
the game-root layout: put its triplet under
`Voyage/Content/Paks/LogicMods/` and every other payload at its exact
`Voyage/...` destination. For another mixed destination without a plugin or
LogicMods, the archive producer's `-RootLayout` and `-AdditionalDestination`
parameters select the same layout. Every entry in that layout must start with
`Voyage/`.

The flat `pakalt` layout remains useful when files must go directly under
`Voyage/Content/Paks/` rather than `~mods/`. Its override is a JSON array with
one `copy` instruction per payload file followed by a `setmodtype` instruction:

```json
[
  { "type": "copy", "source": "Example.ini", "destination": "Example.ini" },
  { "type": "copy", "source": "Example_P.pak", "destination": "Example_P.pak" },
  { "type": "copy", "source": "Example_P.ucas", "destination": "Example_P.ucas" },
  { "type": "copy", "source": "Example_P.utoc", "destination": "Example_P.utoc" },
  { "type": "setmodtype", "value": "thelastcaretaker-pakalt" }
]
```

An INI placed beside a PAK is distinct from the game's user configuration under
`%LOCALAPPDATA%/Voyage/Saved/Config/Windows/`; choose its destination from the
mod's runtime contract. If a plain patch needs Paks root instead of `~mods`, use
the flat `pakalt` override even without an extra file.

The `LogicMods/` directory entry matters for the plain DML layout: the extension
detects that entry, not merely file paths beneath it. Vortex places DML actors;
it does not itself provide DML. A release containing both a content plugin and a
PAK uses the game-root `Voyage/...` layout so the descriptor is deployed with
the container.

For any release, install, enable, and deploy the exact ZIP in Vortex. Check the
expected destinations, the game root for unexpected files, and Vortex's
deployment manifest before claiming Vortex compatibility. A successful ZIP or
release-manifest check proves package structure, not Vortex deployment or
runtime behavior. File-based override instructions do not apply to collection
installs; those need separate compatibility evidence.

References: [TLC Vortex extension](https://github.com/ChemGuy1611/ChemBoy1-Vortex-Games/blob/main/game-thelastcaretaker/index.js),
[Nexus override instructions](https://github.com/Nexus-Mods/Vortex/wiki/File-Based-Override-Instructions).
