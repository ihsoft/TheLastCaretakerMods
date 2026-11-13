# Smart Outlet

Select smart input, smart output or normal on each supported storage socket
with V (Interact 3). Smart modes use fill thresholds, saved state and a pulsing
green/red indicator. Requires DML.

Build a candidate from the repository root:

```powershell
$shell = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
./tools/Invoke-UnrealQueued.ps1 -Operation 'Build Smart Outlet' `
  -FilePath (Join-Path $PSHOME $shell) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/SmartOutlet/Build-SmartOutlet.ps1','-AllowDirtySource')
```

Omit `-AllowDirtySource` for a clean checkout.
