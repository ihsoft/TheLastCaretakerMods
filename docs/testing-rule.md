# Test scope and value

A retained automated test must cheaply detect a concrete, plausible error that
would otherwise be easy to miss. Its expected benefit must justify writing,
running and maintaining it. Code existence, a parameter, a fixed bug or a coverage
percentage alone does not justify a test. Apply this rule when adding, reviewing
or retiring tests; it does not require a repository-wide rewrite during routine
work.

## Separate three kinds of checks

- **Operation validation** belongs to the operation producing or consuming the
  data. Extraction checks its current result; installation checks its inputs and
  writes. These checks remain part of normal execution where needed to uphold
  the operation's contract.
- **Change acceptance** establishes that a particular implementation, asset or
  configuration change works. A focused inspection, temporary experiment or
  game scenario may be sufficient. It need not become a permanent test.
- **Regression tests** are retained to catch plausible future mistakes. Before
  adding one, identify the error it catches, why future changes could cause it,
  and why existing checks are insufficient. A short explanation in the change
  report is enough; do not create another tracking framework.

Do not rerun extraction or import checks on every mod build when maintained
inputs have not changed. Their source-readiness and game-version gates remain
owned by the [project workflow](unreal-project-rule.md). Likewise, validate an
INI against its schema when either changes, not whenever an unchanged file is
copied. Acceptance and regression checks do not replace necessary input or
output validation inside a tool.

## Choose meaningful coverage

Good candidates include non-obvious interactions between components, state
transitions, event ordering, ownership and cleanup, parameter-dependent modes,
and meaningful failure boundaries. Complexity is not required: a small operation
with serious consequences, such as selecting files for deletion, can justify
focused tests. Exercise realistic valid cases, relevant boundaries and likely
invalid inputs; do not enumerate every theoretical combination.

Expected behavior must be independently understandable even when tracing the
whole execution chain is expensive. Check successful outcomes and, for rejected
operations, the required absence of damaging side effects. Exercise recovery
only where it is an actual supported contract with a credible failure risk;
do not invent recovery machinery to create something to test.

Assert the promised behavior rather than a copy of the implementation. Do not
freeze incidental function sets, graph shape, call counts, layout coordinates or
asset statistics merely because they describe the current source. Such details
are valid assertions only when they express the contract being protected: for
example, one notification per real change can prevent a feedback loop. A known
vertex count in a particular game mesh usually describes that input, not extractor
correctness. A small synthetic fixture with a known transformation can provide
a useful independent expectation. Exact bytes are appropriate when byte identity
is itself the contract, subject to the [hash rule](hash-usage-rule.md).

A structurally valid output can still be wrong. Prefer semantic checks relevant
to the operation over assuming that every error will crash. Do not compute the
expected result using the same algorithm under test. Use the smallest fixture
that exercises the risk; a large population or stress run needs an actual
scale-dependent claim. A fixed bug warrants a retained test only if recurrence
is plausible, not as a permanent record of the fix.

## Keep responsibility with its owner

Shared tools and libraries own tests of their contracts. A mod checks its own
decisions and integration, trusting the supported shared interface. Do not repeat
a shared parser, installer or project-copy suite in every consumer. A thin
forwarding wrapper does not automatically need a separate test.

Test a boundary between components when their interaction creates a concrete
risk, such as incompatible path interpretation, units or field bindings. Keep
that check focused on the interaction. An imported and locally modified shared
component may need additional coverage for its changed behavior; this does not
justify copying the entire upstream suite.

## Run and maintain checks by affected scope

Run relevant existing checks for the changed contracts. Broaden only for affected
dependencies, new failures or unresolved concerns. Ordinary builds validate
their candidate; they need not run every regression suite or re-prove the toolchain.
Expensive engine or game work needs a claim that cheaper checks cannot establish.
The [game experiment rule](game-experiment-rule.md) owns gameplay acceptance and
live-session safety; passing editor fixtures does not prove native game behavior.

When a test fails, distinguish a defect, an obsolete expectation and an invalid
fixture. Do not mechanically update expected values to the new output or change
valid production behavior merely to satisfy an incidental assertion. Narrow or
remove tests whose risk has disappeared or is already covered by their owning
operation. Report what was checked and the remaining evidence boundary; neither
assertion count nor a passing suite is a claim of comprehensive correctness.
