# Smart Outlet

Select smart input, smart output or normal on each supported storage socket
with V (Interact 3). Smart modes use fill thresholds, saved state and a pulsing
green/red indicator. Requires DML. The [commented INI](Assets/SmartOutlet.ini)
defines available modes and their settings for each storage class.

Build a candidate from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/SmartOutlet/Build-SmartOutlet.ps1 -AllowDirtySource
```

Omit `-AllowDirtySource` for a clean checkout. Install the resulting manifest
with `mods/SmartOutlet/Install-SmartOutlet.ps1 -ReleaseManifest <path>` to preserve
user settings. For a candidate from a dirty checkout, also pass `-AllowDirtySource`.
See [Architecture](ARCHITECTURE.md) for behavior and boundaries.
