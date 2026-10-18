# Native asset registry generation

## Premade AssetRegistry generation

Use `New-VoyageAssetRegistry.ps1 -AssetJsonPath <array> -OutputPath <bin>` to
derive primary-asset records directly from UAssetAPI/UAssetGUI JSON and write
them with Unreal's native `FAssetRegistryState.Save`. From another PowerShell
script, pass a normal string array. At a Windows PowerShell 5.1 `-File`
boundary, pass the paths as one semicolon-delimited argument. Example:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass `
  -File tools\New-VoyageAssetRegistry.ps1 `
  -AssetJsonPath "ammo.json;item.json;skill.json" `
  -OutputPath "Tmp\Example\registry\AssetRegistry.bin"
```

The Voyage profile accepts one top-level `VoyageItem`, `VoyageItemAmmo`, or
`VoyageSkill` primary export per input package. The producer checks its
game/format compatibility, validates each record, and writes a sibling
manifest. It rejects unsupported or ambiguous inputs before replacing output.
Use the reported failure to decide whether the asset data or the tool needs
investigation; do not precheck its native writer or dependency versions.
