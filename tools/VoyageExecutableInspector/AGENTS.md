# Voyage executable inspector index

This project implements a read-only inspector for the current shipping
executable. Use the public wrapper as a black box:

```powershell
.\tools\Invoke-VoyageExecutableInspector.ps1 -Query VoyageInputControlsComponent -RequireMatch
```

- [Executable inspection](../executable-inspection.md): query modes, compact
  result, evidence boundary, and escalation path.
- [Toolchain maintenance](../toolchain-maintenance.md): publish or repair the
  manifest-validated binary.

Read source only for explicit tool development or after the wrapper reports an
unexpected result that its evidence cannot explain.
