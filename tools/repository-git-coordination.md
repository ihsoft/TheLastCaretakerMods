# Repository Git coordination

All agents share one working tree and one Git index. Exact path ownership alone
does not prevent another process from staging files between an index check and
`git commit`. On 2026-09-05 that race created a local mixed commit containing
20 paths from another task.

## Normal commit

Do not stage first. Give the wrapper exact files; it acquires the repository
`git-transaction` mutex, rejects a non-empty shared index, stages only those
files, checks the staged and committed path sets, commits, and releases the
mutex.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools\commit-repository-changes.ps1 `
  -Path AGENTS.md,tools\repository-coordination.psm1 `
  -Message "Serialize repository Git mutations" `
  -Owner "pipeline-task"
```

Use `-ExpectedHead <commit>` when the commit depends on a reviewed HEAD. A busy
lock waits for up to five minutes by default and records its owner under the
ignored `.tools/repository-locks/` directory.

`-UseExistingIndex` is only for a coordinated recovery of an already staged
exact path set. The wrapper requires the supplied paths to equal the entire
staged set. It is not the normal workflow.

## Other Git mutations

Read-only commands need no lock. An exceptional operation that changes the
index, HEAD, refs, branches, tags, stash, or worktree through Git must import
`tools/repository-coordination.psm1` and execute the complete transaction in
one `Invoke-WithRepositoryLock` call using resource `git-transaction`. Do not
acquire the lock around only one step of a multi-step mutation.

The semaphore intentionally does not serialize builds, tests, read-only
research, or edits to explicitly disjoint owned paths. This keeps the fix
focused on the demonstrated shared-index/ref race instead of disabling useful
parallel work.
