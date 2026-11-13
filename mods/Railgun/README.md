# Railgun

Adds a buildable, powered ship-mounted railgun.

Settings: [Railgun.ini](Assets/Railgun.ini), installed at
`<game directory>/Voyage/Content/Paks/Railgun.ini`.

Build from the repository root (see [build details](BUILD.md)):

```powershell
$hostExe = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
& ./tools/Invoke-UnrealQueued.ps1 -Operation 'Build Railgun' `
  -FilePath (Join-Path $PSHOME $hostExe) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/Railgun/Build-Railgun.ps1')
```
