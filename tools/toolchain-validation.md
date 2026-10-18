# Toolchain validation and bounded diagnostics

## retoc import compatibility

Run `Test-RetocImportCompatibility.ps1` only when changing or diagnosing the
retoc reader/writer. Supply the candidate and the preserved control binaries
required by its interface. Consume its compact result and open `reportPath`
only for a failed case. The test does not install a mod or prove game behavior.

```powershell
.\tools\Test-RetocImportCompatibility.ps1 `
  -CandidateRetoc '<candidate-retoc.exe>' `
  -UpstreamRetoc '<upstream-control.exe>' `
  -LegacyRetoc '<legacy-control.exe>'
```

## UAssetGUI JSON round trip

`Test-UAssetGuiJsonRoundtrip.ps1 -CandidateGui <exe> -InputAsset <uasset>
-Mappings <usmap> -EngineVersion 5.8` checks a candidate GUI's JSON read/write
path. Use a provenance-matched asset that parses fully. `-TestCanPlace` adds a
case specific to `BP_FabricationPlacementComponent.GenerateAndSetLocation`;
omit it for other assets. Results and the detailed report stay below `Tmp/`.
Successful serialization is not a game test.

## Bounded native diagnostic

After a native tool hangs, crashes, or grows unexpectedly, use the bounded
runner instead of repeating an unbounded call:

```powershell
.\tools\Invoke-VoyageBoundedTool.ps1 `
  -Executable '.\.tools\bin\retoc.exe' `
  -Arguments @('--version') `
  -MemoryLimitMB 256 -TimeoutSeconds 10
```

It accepts a literal executable path and argument array, limits the process
tree, and writes stdout, stderr, and a compact report below `Tmp/tool-runs/` by
default. Check `status`, `exitCode`, `stdoutPath`, `stderrPath`, and `reportPath`.
`-AllowFailure` is for an expected diagnostic failure; inspect the returned
status explicitly. The report records arguments, so do not pass secrets. This
runner is resource containment, not authorization for filesystem mutation.
