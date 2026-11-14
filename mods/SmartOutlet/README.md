# Smart Outlet

Select smart input, smart output or normal on each supported storage socket
with V (Interact 3). Smart modes use fill thresholds, saved state and a pulsing
green/red indicator. Requires DML.

Settings: [SmartOutlet.ini](Assets/SmartOutlet.ini), with comments describing
available modes and thresholds. Installed at
`<game directory>/Voyage/Content/Paks/LogicMods/SmartOutlet.ini`.

Build a candidate from the repository root (see [build details](BUILD.md)):

```powershell
python -B tools/run_powershell.py mods/SmartOutlet/Build-SmartOutlet.ps1 -AllowDirtySource
```

Omit `-AllowDirtySource` for a clean checkout.
