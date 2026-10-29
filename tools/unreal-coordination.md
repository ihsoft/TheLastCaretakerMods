# Unreal execution queue

Use the queue for engine builds, commandlets, cooks and editor sessions. It
serializes cooperating tools across repository copies and projects in the same
Windows user account. It does not coordinate other Windows accounts.

```powershell
.\tools\Invoke-UnrealQueued.ps1 -Operation 'Build Example' `
  -FilePath powershell.exe `
  -ArgumentList @('-NoProfile','-File','mods/Example/Build.ps1')
```

Wrap the entire producer, including its verification and rollback. BPT workspace
engine actions and both lower-level BPT engine scripts already join the queue.
Offline previews and JSON editing do not reserve it. A synchronous nested tool
inherits the current reservation; do not copy its environment token into an
independent task or start concurrent engine work inside one reservation.

The default wait is unlimited. Optional `-TimeoutSeconds` cancels only this
waiting request without starting its command. Queue state is printed when it
changes: requests ahead, current owner, or editor processes preventing launch.
Wait in the running tool session rather than repeatedly restarting it. The
wrapped command's failure is still a failure; contention itself is not.

Tickets are ordered by admission under an OS-held scheduler lock. Each request
holds its own OS lease, so a dead owner/waiter is removed without age guesses
or PID-based ownership assumptions. State lives in `%TEMP%/Codex-Unreal-Queue`.
Do not manually delete a live queue or its locks. Editor processes left after
a producer crash continue to block the next engine launch. Recovery of the
producer's asset transaction remains its own responsibility.

An existing manual editor causes waiting and is never closed by the tool.
External launches do not acquire the reservation, so all agents must follow the
[shared execution rule](../docs/unreal-project-rule.md#shared-unreal-execution).
File ownership, project backups, asset conflict checks and Git locks remain
separate. Offline process tests: `python -B tools/test_unreal_coordination.py`.
