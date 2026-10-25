# Experiment and validation workflow

- Install all new DML mods' container files together directly in
  `Voyage/Content/Paks/LogicMods` so DML can discover them. A virtual actor path
  or a previously registered mod working elsewhere is not proof of fresh
  discovery. See the [DML installation contract](../docs/voyage-cooked-asset-toolchain-knowledge.md#dml-actor-identity-and-physical-installation).
- Never replace installed mod or game files while the game is running. The
  user may play while repository-only work continues; do not interrupt or
  alter his current game session.
- Do not ask the user in advance whether the game is closed. Once installation
  is authorized and the candidate has passed its required review/validation,
  the installing agent checks the actual game process immediately before
  mutation. If absent, install without another user question. If running, leave
  installed files untouched, report the blocker and wait for the user; never
  close the game yourself. Unknown process state is not proof that it is closed.
  On resumption, check again. This rule does not grant installation authority
  to research-only work or waive the manifest, backup and readback gates.
- Before a risky test, preserve an exact known-good package, change one
  architectural variable, state the expected outcomes, and record the actual
  game result immediately. Published packages in `artifacts/installations/`
  already have known-good status under the
  [release workflow](release-and-installation-rule.md#output-lifetime);
  do not search for separate gameplay-validation evidence for them.
- A successful compile, cook, container verification, clean log, or load is
  not gameplay/UI validation. Require a real-game test proportional to the
  changed behavior.
- Until that real-game test confirms the intended behavior, keep the candidate's
  implementation, configuration, and build changes uncommitted. Keep proposed
  architecture, rules, knowledge, and other current-state documentation in an
  ignored owning backlog. Lower validation gates authorize installation and
  testing only. After the game test passes, prepare the exact tested change and
  its durable conclusions for review without rebuilding the candidate merely
  to obtain a clean-source package. Commit them only after explicit user
  authorization. An explicit user request for an unvalidated checkpoint is the
  only exception and must preserve the missing validation boundary.
- If an experiment removes control, causes autonomous behavior, corrupts UI,
  or crashes, restore the last checkpoint instead of layering speculative
  fixes on the unknown state.
