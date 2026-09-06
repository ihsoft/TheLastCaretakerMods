# Voyage executable inspector

Read-only helper for locating reflected Voyage class/function names in the
current shipping executable. It records nearby ASCII strings, absolute image
pointers, common RIP-relative code references, and direct relative calls or
jumps without modifying or injecting into the running game.

Queries match both ASCII and UTF-16LE strings. The latter are important for
generated Unreal class-registration records, whose class names are commonly
stored as wide literals next to their native function tables.

```powershell
.\tools\Invoke-VoyageExecutableInspector.ps1 `
  -Query VoyageInputControlsComponent,VoyageDynamicPlayerInputComponent,GetProvidedActions `
  -WindowBytes 8192
```

Treat offsets as version-specific. Always record the executable hash beside
any stable conclusion derived from this output.

Use `--target-va` to find absolute pointers and common RIP-relative code reads
and direct calls or jumps to an already-known image address without copying the
executable or loading it into the game process:

```powershell
.\tools\Invoke-VoyageExecutableInspector.ps1 `
  -TargetVirtualAddress 0x14AD1B8D8
```

`--member-offsets` searches executable sections for little-endian member
displacements and reports pages containing at least three distinct requested
offsets. This is a correlation aid, not a full x86 decoder; inspect the
reported instructions before drawing conclusions.

Normal work must use the wrapper above. It fingerprints the installed game,
resolves `.tools/bin/VoyageExecutableInspector.exe` through its hash and input
manifest, preserves the full report and log under ignored `artifacts/`, and
returns a compact result. Use `Get-VoyageExecutableInspectorBinary.ps1` only to
resolve or audit the accepted binary. After an intentional committed change to
this source project, publish with `Publish-VoyageExecutableInspectorBinary.ps1`
and run `Test-VoyageExecutableInspectorBinary.ps1` under Windows PowerShell 5.1.
Direct `dotnet run` is reserved for deliberate development of this utility, not
routine game research.
