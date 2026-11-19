# ElectrifiedBuildings

Adds electrical sockets to building controls with one identifiable power inlet.
Requires DML. See [BUILD.md](BUILD.md) for authoring, validation and installation.

Choose one download:

- **Standard — Original power rules.** Additional power outlets, with the original building inlet behavior and no added battery.
- **Expanded — Flexible power connections.** A 200 Wh reserve and bidirectional building sockets.

Install only one variant. Both contain the same mod; their INI defaults differ.
Settings live in `Voyage/Content/Paks/LogicMods/ElectrifiedBuildings.ini`.
Reload your save after editing them. Installing either package replaces that INI.

From the repository root:

```powershell
python -B tools/run_powershell.py mods/ElectrifiedBuildings/Build-ElectrifiedBuildings.ps1 -AllowDirtySource
```
