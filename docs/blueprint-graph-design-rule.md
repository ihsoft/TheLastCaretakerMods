# Blueprint graph design

Apply these rules when authoring or simplifying Blueprint graphs through BPT.
The objective is human understanding of the behavior authored by the agent.
Node count is a review signal, not a quota or evidence of faster execution.
The [workspace workflow](../tools/blueprint-workspace.md) owns editing,
synchronization and preview; the [format guide](../tools/blueprint-text.md)
owns supported graph representations.

## BPT is an AI-to-Human interface

BPT lets an agent author behavior that a human can understand, evaluate and
discuss through the preview. Human readability is a primary acceptance criterion
for both the authored graph and its presentation. The agent's ability to process
a large or noisy representation does not make that representation suitable for
the person reviewing it.

Apply Occam's razor: include only what the graph needs. Every node, branch,
check, cast, state variable or abstraction must serve required behavior, an
established contract or a concrete human-understanding need. The burden of
justification is on adding complexity. Possibility, habit, speculative future
reuse and "just in case" are insufficient reasons. If an element can be omitted
without losing those requirements, omit it. Use the preservation rules below
to establish that an existing element is unnecessary before removing it.

Minimize nonessential noise: redundant checks and casts, repeated plumbing,
technical identifiers and irrelevant displayed details compete with meaningful
operations for the reader's attention. Remove semantically redundant operations
from the authored graph under the preservation rules below. For information
needed by serialization or detailed inspection but not by the main reading task,
improve the preview's presentation and keep the detail accessible through Details
or by turning off grouping and expressions. Hiding a necessary decision or failure
path is not simplification. Keep explicit Self nodes and disabled nodes visible:
they are part of the authored topology, even when they should later be removed.

Callers state their own requirements. If a maximum must be 1000, pass 1000 even
when the callee happens to default to 1000. If the caller has no maximum
requirement, omit that argument and let the callee supply its own default;
BPT rejects the omission if that parameter has no default. Do not ask an agent
to enumerate or compare callee defaults before deciding what its caller needs.
The [BPT argument contract](../tools/blueprint-text.md#optional-call-arguments)
preserves this difference between a fixed requirement and reliance on the callee.

For simple Unreal arithmetic and logic, write every operand explicitly as a
value or connection. Express `1 + 1` with both operands, even if an engine
function happens to default one of them to 1. This is graph-writing style,
separate from BPT's ability to represent optional arguments.

Low noise is one part of readability. Meaningful names, clear responsibility
boundaries, visible execution order, readable expressions and useful navigation
must let the reader follow intent, conditions and effects without repeatedly
decoding implementation details. Explain a readability improvement by the
irrelevant decisions or indirections it removes from that reading task, and the
meaning it makes easier to see. Runtime speedup is not required to justify this
benefit; fewer nodes or a smaller JSON file alone do not establish it.

## Identify the source of complexity

Before choosing a transformation, identify what makes the graph difficult to
understand. Use its responsibilities, control flow and data dependencies:

| Problem | Appropriate response |
| --- | --- |
| Different responsibilities or levels of detail are mixed together. | Separate complete operations from the coordinating flow. |
| The same algorithm is implemented repeatedly. | Check semantic equivalence, then share its implementation and parameterize genuine differences. |
| A coherent operation is hard to read because of labels, layout, wiring or expression display. | Use the existing preview views or address the presentation problem. Graph decomposition is not automatically required. |

These problems can coexist. Choose edits for the identified cause rather than
starting with a target node count or a preferred abstraction. A presentation
defect alone is not a reason to change the represented algorithm.

Start a cleanup with the graph hierarchy, not a full-document rewrite. Look for
empty or disabled-only implementations, redundant value plumbing, duplicated
operations, and graphs that mix responsibilities. Inspect the selected regions
and their callers before transforming them. A large graph is a reason to review
its structure, not permission to change its algorithm.

## Remove inert structure and redundant plumbing

Remove an empty or entirely disabled implementation when it has no required
declaration, caller or inherited-behavior contract. Do not retain it because
"BPT might require it": BPT can represent absent graphs and a Blueprint with
`graphs: []`. Delete the graph record, rather than leaving an empty record and
calling that removal. Remove or update its owning collapsed call when applicable.

Distinguish implementation from contract before deletion. A function entry,
interface implementation or intentional no-op override may still define required
behavior. Check direct callers, delegate/timer bindings and relevant external or
inherited contracts; absence of a local wire is not proof of disuse. In particular,
removing an override can expose parent behavior. Preserve the Blueprint itself
when its class identity, defaults, components or inherited behavior are needed.
If ordinary Apply rejects a justified removal, investigate its concrete diagnostic;
do not invent dummy nodes or retain a placeholder to satisfy an assumed tool need.

Prefer a setter's `Output_Get` to a separate nearby Get of the same variable on
the same target, used in the same evaluation context. For example, assigning an
aim angle and immediately applying it should not need another getter node. Inspect
all consumers before removing a shared getter. Keep a separate read when it serves
another execution path or makes a different reading point clear.

`Output_Get` reads the variable; it is not an immutable snapshot of the assignment.
Unreal expands it into a getter during compilation. Do not substitute the setter's
input value, remove an intentional captured value, or claim a runtime speedup
from this visual simplification. Setter callbacks and changing target expressions
can make superficially similar reads different.

Prefer implicit Self when accessing the current object's members and the node
supports that context. Do not add an explicit Self node merely to wire a target
that can use its own context. When cleaning up an existing explicit Self, convert
the consumer's member reference and target declaration to self context as needed;
disconnecting a target still declared as external can cause an invalid-target
compiler error. Preserve the exact member, including inherited-member identity.
Remove the Self node only after checking all of its consumers. Keep explicit Self
when it supplies an ordinary argument value or a connection the consumer requires;
do not replace references to other objects with Self. Validate an authored cleanup
through Apply and fresh export. A preview must not silently perform this source edit.

A branch with a literal condition and no meaningful alternative is routing noise,
not a decision. Connect its incoming paths directly to the selected continuation,
or leave them terminal when that continuation is absent. Preserve execution count,
order and termination; do not turn several incoming paths into a Sequence.

## Separate flow from implementation

Decompose from the overall behavior toward its implementation. Each graph is an
implementation detail of its caller and may coordinate the operations beneath it.
Keep one level of detail in a graph. A coordinating graph should show meaningful
stages, their order, and the decisions that select them. Put the mechanics of a
stage in an operation named for its purpose, such as `AcquireAltitudeLock` or
`RefreshCargoAvailability`. Avoid generic names such as `ProcessPart2`.

Extract a complete calculation, decision or side effect with a small, explicit
boundary. A single caller is sufficient when the operation has a useful meaning.
Keep guards at the level whose policy they express: a stage's eligibility should
remain visible to its coordinator; checks internal to component processing can
stay with that processing. Do not hide an unchanged large graph behind one call
and treat the resulting small caller as finished decomposition.

Stop when the remaining nodes express one understandable operation. Adding levels
to meet a node limit can make navigation and reasoning harder. Judge the caller
and implementation together, including parameters, state access and failure paths.

About 15–30 nodes is a useful working size, not a lower bound or hard limit.
Beyond that range, look for a complete operation at a different level of detail.
Review the extracted body as well as the smaller caller: hiding hundreds of nodes
one click deeper does not finish the work. A small dispatcher or a short meaningful
operation need not be padded. A cohesive larger operation, such as a coupled loop
or a set of required assignments, can be clearer than several fragments with many
crossing values. Keep such an exception for its concrete boundary constraint,
not merely because its nodes fit on a wide canvas. Preview folding does not make
the underlying operation smaller.

Check whether the operation's name, boundary and surrounding flow let a reader
understand its caller without repeatedly opening the implementation:

- What inputs, owner state and preconditions does it require?
- What result does it produce, and what state or external objects does it change?
- When does execution continue, and which paths terminate?

Pin count alone does not measure the boundary's complexity. A function with no
arguments can still depend on many owner variables and side effects. Keep those
dependencies coherent and apparent from the operation's purpose and context.
When the answers remain unclear, reconsider the name, boundary or placement of
the operation. Do not add documentation to every node as a substitute for a
coherent design.

## Choose the boundary

| Form | Use when |
| --- | --- |
| Collapsed graph | A local stage or expression needs a name and a separate view while retaining its surrounding graph context. |
| Function | A complete callable operation has a useful contract; reuse shares one implementation, with genuine participant or value differences expressed as parameters. A single caller can also justify that contract. |
| Separate Blueprint object | A responsibility has its own state, lifecycle and small useful interface. Define that ownership before moving it. |
| Blueprint Function Library | Stateless pure calculations or predicates are shared across Blueprint owners. Follow the supported [library contract](../tools/blueprint-text.md#blueprint-function-libraries). |

A collapsed graph represents a local implementation, not a reusable definition
shared by several calls. A function supplies one implementation for its callers.
Neither form removes that implementation from the owning Blueprint's complete
BPT document. Moving a responsibility to another object is a larger ownership
decision, not an automatic next step after reducing a graph's node count.

Choose Blueprint ownership before growing a large collection of unrelated local
subgraphs. An entry actor can discover and connect participants while a widget
owns presentation, a possessed pawn owns its input/possession contract, and a
separately lived effect owns its own completion. These are responsibility examples,
not a mandatory object per category. Separate only when the new owner can keep its
state and lifecycle coherent behind a small interface. Repeated cross-object
property access, bidirectional dependencies and new readiness/teardown plumbing
are costs to weigh against the separation. Do not move input away from its engine
owner just to make a Blueprint smaller, or introduce a library for one owner's
stateful operation. Share a local function first when the callers have one owner.

Parameterize genuine differences in repeated algorithms, such as the socket to
clean up or the mesh and transform to instantiate. Prefer separate named policies
to a universal operation with several mode booleans. Similar predicates may
deliberately differ in eligibility or tick phase; shared appearance is insufficient
reason to merge them or cache one result for both.

Reuse a component template for common configuration when instance differences
can be supplied during creation. Preserve any separate identities required by
saves, registration or external references. Instance count alone does not require
the same number of templates.

## Preserve execution semantics

For a behavior-preserving edit, account for:

- Execution order, including native parent calls, tick phases and callbacks.
- When live values are read, which reads are explicitly captured, and which may
  be reevaluated for a downstream consumer. A pure node is not necessarily a
  constant. Keep related reads, writes and edge-detection updates together.
- Boundary pin types, reference semantics, local-variable scope and target
  identity. In particular, passing an array by value is not interchangeable with
  passing the array being mutated by reference.
- Every outgoing execution path, including failed casts and branches that stop.
  An unused collapsed-graph exit does not imply missing work. Connecting it may
  cause downstream operations to execute on a path that previously terminated.
- State ownership and reset scope. A vertical-only reset and a complete flight
  reset are different operations even when several assignments match.

Pass an already-produced value across a boundary when that preserves its original
use. Do not add caches, replace live reads with snapshots, or change pure/impure
execution as an incidental part of a readability edit.

Equivalent calls can share one node when their receivers, arguments and
continuations match and routing execution to that node preserves invocation
count and order. Check the paths, not just the node signatures.

Use a straight execution chain for ordered calls. Use Sequence when the graph
actually needs separately dispatched branches; Sequence is not a generic way to
preserve the termination behavior of an extracted operation.

Distinguish [empty and absent graphs](../tools/blueprint-text.md#empty-and-absent-graphs)
from an unused Blueprint. A Construction Script containing only a parent call
is not proven empty by its node count: establish the actual parent's behavior.
An empty editor stub does not establish empty behavior in the shipping parent.

## Justify casts and validity checks

Unnecessary checks and casts add apparent decisions that the human must interpret.
Removing them when their guarantees are already established improves the
AI-to-Human interface even without a measurable runtime performance change.

For each check, identify the input contract, a concrete way it can fail before
use, and the purpose of its failure path. Similar-looking chains can require
different checks. Separate type narrowing, object lifetime and required authored
structure; one check does not establish all three.

- Use a cast when the source type does not guarantee the required class, for
  example when filtering heterogeneous objects. Preserve concrete types through
  variables, arrays and ForEach pins when no generalization is needed. Do not
  widen a typed array merely to cast each element back to its original type.
- A cast to an already guaranteed type is redundant for typing, but its execution
  pins may still reject a null reference. Inspect the success and failure paths
  and other consumers before removing it. A successful cast is not a substitute
  for checking object lifetime where that matters.
- Keep IsValid when a reference can legitimately be absent or invalid at use,
  and skipping, resetting or restoring state is the appropriate response.
  Cached references, deferred work, EndPlay and calls that can trigger callbacks
  are reasons to examine lifetime, not automatic reasons to add a check everywhere.
- Remove a validity check when the producer or an earlier guard already provides
  the required guarantee through the point of use. A fresh typed array alone
  does not prove this: consider the producer's filtering and mutations during
  traversal. A synchronous read-only traversal differs from a loop that changes
  game state. An empty array needs no per-element guard to handle its emptiness.
- Consecutive operations can share a caller-owned guard when all call paths meet
  it and intervening work cannot invalidate its guarantee. Preserve the separate
  failure responses, operation order and continuation. Make the callee precondition
  apparent; do not cache validity across ticks or asynchronous boundaries.

Required authored components and names are project contracts. A guard that merely
skips required work when such a contract is broken can hide a defect rather than
provide useful recovery. Establish those contracts during development and build
validation; do not treat a proposed check as already provided by the tooling.
Apply the [error-handling rule](error-handling-rule.md) to contract failures.
If the necessary lifetime or producer contract is unknown, state that limit;
neither removing the check nor retaining it proves the missing guarantee.

## Make failure handling useful

Preserve meaningful runtime recovery and termination behavior. Do not add status
outputs and caller branches merely to reproduce defensive plumbing without a
useful caller response. Distinguish invalid authored configuration from an
expected runtime condition. Removing an existing failure gate can change behavior
even when successful execution is identical; identify and obtain agreement on
that change instead of presenting it as a mechanical extraction. The shared
[error-handling rule](error-handling-rule.md) still applies.

For the limits of development diagnostics and early HUD notifications, see
[Shipping feedback](vehicle-and-hud-modding-patterns-knowledge.md#feedback-before-the-hud-is-available).

## Keep the change reviewable

Use BPT for mod-specific structure and values. If required state is absent, report
the format gap rather than recovering hidden mod data from native assets or old
generators. General engine documentation can explain engine semantics.

Preserve identities of moved nodes and graphs. Give new replacement calls and
their pins new identities; do not repurpose a removed implementation node's ID
just because its signature fits. This keeps moves, additions and removals
distinguishable in review.

For an extraction, reason about the graph after expanding the new boundary:
original operations, links, side effects and terminal paths should be preserved.
For reuse or deduplication, also check argument/target equivalence and execution
order. Treat deliberate behavior changes separately. This semantic review does
not replace or require repeating the public tool's mechanical validation.

When validation fails after an authorized structural change, distinguish a
violated contract from an expectation tied to the previous representation.
If that expectation is obsolete, update the exact expectations for the new
structure while preserving the required identity, content and behavior checks.
For example, component-template count and runtime instance count are different
contracts. Do not weaken a check merely to make it pass, or treat a successful
representation check as proof of runtime behavior. Diagnose import/export
mismatches rather than dismissing them as harmless metadata differences.

Decomposition should reduce the context needed for the next task. Use the BPT
hierarchy summary to locate relevant operations, then read the selected graph
with its boundary context and required definitions through the public source
inspection interface. Expand that scope when dependencies require it; do not
routinely reread the complete Blueprint merely because its JSON is one file.
Continue editing the complete proposed source as required by the workspace
workflow.

For human review, keep the implementation and calling context reachable through
the ordinary preview navigation, search and preserved view state. Extra nesting
has a navigation cost; assess that cost alongside smaller local graphs. Return
the standard preview rather than manually assembling or inspecting generated
pages. The agent owns semantic decisions; successful public-tool results own
the mechanical acceptance checks described by the workspace workflow.
Preview folding and expression rendering change the view, not BPT or runtime
behavior; do not rewrite graphs solely to imitate those presentation features.

## Size iterations by uncertainty

Use a small, focused first step when introducing an unfamiliar transformation or
testing a new format/tool capability. Resolve uncertain semantics before applying
the transformation broadly. Once the operation and its boundaries are established,
group related behavior-preserving edits into a coherent candidate. Do not require
a separate game launch for every extracted subgraph. Keep deliberate behavior
changes explicit and separately assessable.

Apply the shared [test scope rule](testing-rule.md) when choosing retained
coverage. Choose gameplay acceptance from the affected states and transitions,
with expected outcomes: entry and exit, enable and disable, loss and recovery of
eligibility, press and release, and interactions between affected controls.
One combined scenario may exercise several extractions; retain distinct checks
where behavior differs.

Use the ordinary Apply and one-call Preview workflow. Compilation and export
comparison establish editor consistency, not gameplay equivalence or a performance
improvement. If new failures or unresolved dependencies appear, narrow the next
step around that uncertainty rather than continuing to enlarge the batch.
