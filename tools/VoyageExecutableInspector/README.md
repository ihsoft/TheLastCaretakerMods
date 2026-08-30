# Voyage executable inspector

Read-only helper for locating reflected Voyage class/function names in the
current shipping executable. It records nearby ASCII strings, absolute image
pointers, and common RIP-relative code references without modifying or
injecting into the running game.

Queries match both ASCII and UTF-16LE strings. The latter are important for
generated Unreal class-registration records, whose class names are commonly
stored as wide literals next to their native function tables.

```powershell
dotnet run --project tools\VoyageExecutableInspector -c Release -- `
  P:\SteamLibrary\steamapps\common\Voyage\Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe `
  artifacts\inspection\native-input-chain.txt `
  --window=8192 `
  VoyageInputControlsComponent VoyageDynamicPlayerInputComponent GetProvidedActions
```

Treat offsets as version-specific. Always record the executable hash beside
any stable conclusion derived from this output.

Use `--target-va` to find absolute pointers and common RIP-relative code reads
of an already-known image address without copying the executable or loading it
into the game process:

```powershell
dotnet run --project tools\VoyageExecutableInspector -c Release -- `
  P:\SteamLibrary\steamapps\common\Voyage\Voyage\Binaries\Win64\VoyageSteam-Win64-Shipping.exe `
  artifacts\inspection\get-provided-actions-global.txt `
  --target-va=0x14AD1B8D8
```

`--member-offsets` searches executable sections for little-endian member
displacements and reports pages containing at least three distinct requested
offsets. This is a correlation aid, not a full x86 decoder; inspect the
reported instructions before drawing conclusions.
