# Repository hygiene

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
- In C++ generator code, replace every repeated `TEXT("...")` literal with a
  named constant. Also name a single-use literal when it represents a logical
  contract, tuning value, asset identity, or naturally belongs to an existing
  constant group. Keep engine-level names reusable by multiple mods in a
  shared header under `tools/`; keep game- and mod-specific values with their
  owning mod. Keep diagnostic and assertion format strings inline even when
  the same wording appears in multiple files; they are messages, not shared
  identities or contracts.
- Do not expose opaque reflected pin names such as `A`, `B`, or `bPickA` at
  generator call sites. Shared helpers may store those raw names internally,
  but consuming code must use the node's semantic role, such as binary left
  and right operands or select condition/true/false values.
- Keep toolchain-specific ignore rules in the owning mod. The repository root
  should contain only genuinely repository-wide ignore patterns.
- Temporary probe mods/containers are allowed while isolating risk, but remove
  them from user-facing mod folders and the installed game after the result is
  understood. Keep reusable standalone inspectors under `tools/`.
- A container whose purpose includes overriding an existing stock virtual
  package must use a basename ending in `_P`. Additive-only containers are not
  subject to a blanket rename. The suffix establishes Unreal patch-order
  intent; it is not runtime proof that a particular duplicate provider won.
