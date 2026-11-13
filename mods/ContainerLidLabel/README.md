# ContainerLidLabel

DML mod adding names and icons to the lids of large, medium and small containers,
with independently configurable capacities.

Settings are in `Voyage/Content/Paks/LogicMods/ContainerLidLabel.ini`.

Build from the repository root (see [build details](BUILD.md)):

```powershell
$hostExe = if ($PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' }
& ./tools/Invoke-UnrealQueued.ps1 -Operation 'Build ContainerLidLabel' `
  -FilePath (Join-Path $PSHOME $hostExe) `
  -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    'mods/ContainerLidLabel/Build-ContainerLidLabel.ps1')
```
