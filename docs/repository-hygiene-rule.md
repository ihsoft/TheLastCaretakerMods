# Repository hygiene

- Create a commit only after the user explicitly authorizes that commit. A
  request to edit, research, build, validate, install, prepare a release, or
  continue work does not imply commit permission. Earlier permission applies
  only to the reviewed scope it covered and is not standing authorization for
  later commits. Prepare and validate the change first; if the user has not
  already requested its commit, present the concrete result and wait. This gate
  also applies to temporary research checkpoints. Any later revert, squash,
  merge, amend, or replacement of such a checkpoint requires its own explicit
  authorization when it creates or rewrites repository history.
- Serialize every Git-state mutation through the repository `git-transaction`
  semaphore documented in `tools/repository-git-coordination.md`. Normal scoped
  commits must use `tools/commit-repository-changes.ps1`, which owns exact-path
  staging, commit, and post-commit verification inside one lock. Do not run
  direct `git add`, `git commit`, or another index/HEAD/ref mutation, and never
  split staging and commit across lock acquisitions. Read-only Git commands and
  edits/builds on explicitly disjoint paths may remain concurrent. If the lock
  is busy, wait or report its recorded owner; never delete an active lock.
- Preserve unrelated worktree changes and stage only the exact files belonging
  to the validated change.
- For Blueprints maintained through BPT, commit changed project assets together
  with their synchronized tracked BPT representations. Follow the
  [BPT synchronization contract](../tools/blueprint-workspace.md#source-of-truth-and-git);
  stale exports and unapplied JSON changes do not satisfy this requirement.
- Before committing a change under `mods/<Mod>/`, assess its net player-visible
  effect relative to the latest published release. Update that mod's
  `CHANGELOG.md` in the same proposed commit when the cumulative result changes;
  rewrite or remove stale `Next` entries when later work supersedes or reverts
  them. Follow the shared release rule rather than treating the changelog as a
  list of commits.
- When changing C++ editor tooling, follow the
  [shared helper conventions](../tools/toolchain-maintenance.md#shared-generator-helpers)
  for named constants and semantic reflected-pin roles.
- Keep toolchain-specific ignore rules in the owning mod. The repository root
  should contain only genuinely repository-wide ignore patterns.
- Temporary probe mods/containers are allowed while isolating risk, but remove
  them from user-facing mod folders and the installed game after the result is
  understood. Keep reusable standalone inspectors under `tools/`.
- A container whose purpose includes overriding an existing stock virtual
  package must use a basename ending in `_P`. Additive-only containers are not
  subject to a blanket rename. The suffix establishes Unreal patch-order
  intent; it is not runtime proof that a particular duplicate provider won.
