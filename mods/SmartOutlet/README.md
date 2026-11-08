# Smart Outlet

Configurable input/output ports for resource storage, with upper/lower fill
thresholds, saved hysteresis state and a softly pulsing green/red indicator.
Supports several configured ports per storage actor. Requires DML.
See [Commented INI](Assets/SmartOutlet.ini) for settings and the supplied storage rules.

Build a candidate from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File mods/SmartOutlet/Build-SmartOutlet.ps1 -AllowDirtySource
```

Omit `-AllowDirtySource` for a clean checkout. Install the resulting manifest
with `mods/SmartOutlet/Install-SmartOutlet.ps1 -ReleaseManifest <path>` to preserve
user settings. For a candidate from a dirty checkout, also pass `-AllowDirtySource`.
See [Architecture](ARCHITECTURE.md) for behavior and boundaries.
