# Repository PowerShell execution

Use the shared launcher for Windows PowerShell repository commands, from the
repository root. This also applies to older examples beginning with
`powershell.exe -File` or a direct `.ps1` invocation: keep the script and its
parameters, replace the host prefix.

```powershell
python -B tools/run_powershell.py mods/MyMod/Build-MyMod.ps1 -AllowDirtySource
```

Use the available Python 3 executable; when `python` is not on `PATH`, use its
absolute path. Do not rely on a tool's requested shell, the current `$PSHOME`,
or a bare `powershell.exe` to select the script environment.

The launcher selects the system's 64-bit Windows PowerShell by absolute path,
checks that it is Desktop 5.1 before starting the target, disables profiles,
and supplies only the Windows PowerShell system module directories. It excludes
inherited Core, custom and user module paths. Repository modules should be
imported by their own paths. It leaves the caller's environment unchanged and
preserves the working directory, other environment variables, standard streams
and target exit code. It prints the selected executable and version to stderr;
it creates no output files. A missing script or unsupported host fails before
the target runs. Fix the reported path or host; do not retry with another shell.

Arguments after the script path go to Windows PowerShell's `-File` interface.
Its existing limitations, including array parameters across a native command
boundary, still apply. This launcher does not serialize PowerShell objects.

An entry point that explicitly documents PowerShell 7 remains an exception;
follow that entry point's host requirement. The launcher does not attempt to
port its implementation to 5.1 or silently choose a host based on the caller.

## Unreal queue integration

The [Unreal queue](unreal-coordination.md) uses the same launcher implementation
for `powershell.exe` commands, including existing `Invoke-UnrealQueueEntry`
wrappers. A caller running in PowerShell 7 therefore cannot pass its module
paths into the queued Windows PowerShell producer. Other executable commands,
including explicitly selected `pwsh.exe` workers, retain their own contract.
The launcher itself does not reserve Unreal: use the mod's public producer or
the queue when required.
