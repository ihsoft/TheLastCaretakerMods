# Voyage AssetRegistry writer index

This Unreal editor host writes and reopens premade Voyage asset registries. The
supported black-box entry point is the repository-level producer:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\New-VoyageAssetRegistry.ps1 -AssetJsonPath "ammo.json;item.json" -OutputPath "Tmp\Example\AssetRegistry.bin"
```

- [Asset registry guide](../asset-registry.md): accepted inputs, result, and
  normal failure boundary.
- [Toolchain maintenance](../toolchain-maintenance.md): source development and
  publication boundaries.

Run the producer before reading this project's implementation. Its profile,
manifest, and reopen comparison own format and compatibility validation. The
producer launches Unreal and must start outside the restricted sandbox.
