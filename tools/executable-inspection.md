# Executable inspection

Use the read-only public inspector for native strings, references, and member
offset correlations:

```powershell
.\tools\Invoke-VoyageExecutableInspector.ps1 `
  -Query VoyageInputControlsComponent,GetProvidedActions `
  -WindowBytes 8192
```

The wrapper identifies the installed game and returns a compact match count,
analysis boundary, and `reportPath`. Read the full report only for the relevant
matches. `-TargetVirtualAddress` and `-MemberOffset` select narrower queries;
`-RequireMatch` makes a zero-result query fail. Matches alone do not prove
class ownership, calls, or runtime lifecycle.

Only when the compact result cannot answer a specific instruction question,
use `Inspect-VoyageNativeMemberAccess.py <exe> <new-output.json> --offset <hex>`
for a bounded disassembly window. It needs Python, `pefile`, and `capstone` and
writes below `Tmp/`. Its offsets and addresses belong to the inspected build;
do not reuse them after a game update. The [inspector guide](VoyageExecutableInspector/README.md)
has the full query interface. If the canonical binary fails validation, use
the publisher listed in [toolchain maintenance](toolchain-maintenance.md).
