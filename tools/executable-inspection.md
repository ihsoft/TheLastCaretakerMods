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
matches. Queries match ASCII and UTF-16LE strings. Use
`-TargetVirtualAddress` to find absolute pointers, common RIP-relative reads,
and direct relative calls or jumps to a known image address. Use
`-MemberOffset` to correlate several known little-endian member displacements;
the result is a search aid, not a complete x86 decode. `-RequireMatch` makes a
zero-result query fail. A nearby name or match count alone does not prove class
ownership, a reference, a call, or runtime lifecycle.

Only when the compact result cannot answer a specific instruction question,
use `Inspect-VoyageNativeMemberAccess.py <exe> <new-output.json> --offset <hex>`
for a bounded disassembly window. It needs Python, `pefile`, and `capstone` and
writes below `Tmp/`. Its offsets and addresses belong to the inspected build;
do not reuse them after a game update. If the canonical binary fails validation,
use the publisher listed in [toolchain maintenance](toolchain-maintenance.md).
