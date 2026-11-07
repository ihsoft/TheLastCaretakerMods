# Blueprint graph design

Apply these rules when authoring or simplifying Blueprint graphs through BPT.
The objective is understandable control flow and locally understandable operations.
Node count is a review signal, not a quota or evidence of faster execution.
The [workspace workflow](../tools/blueprint-workspace.md) owns editing,
synchronization and preview; the [format guide](../tools/blueprint-text.md)
owns supported graph representations.

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

## Separate flow from implementation

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
| Function | The same complete operation is reused, with participant or value differences expressed as parameters. |
| Separate Blueprint object | A responsibility has its own state, lifecycle and small useful interface. Define that ownership before moving it. |

A collapsed graph represents a local implementation, not a reusable definition
shared by several calls. A function supplies one implementation for its callers.
Neither form removes that implementation from the owning Blueprint's complete
BPT document. Moving a responsibility to another object is a larger ownership
decision, not an automatic next step after reducing a graph's node count.

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

Use a setter's `Output_Get` in place of a nearby redundant getter only when the
variable, target and evaluation context match. Do not treat `Output_Get` as an
immutable snapshot of the assignment or remove a deliberate captured value.

Equivalent calls can share one node when their receivers, arguments and
continuations match and routing execution to that node preserves invocation
count and order. Check the paths, not just the node signatures.

Use a straight execution chain for ordered calls. Use Sequence when the graph
actually needs separately dispatched branches; Sequence is not a generic way to
preserve the termination behavior of an extracted operation.

## Make failure handling useful

Preserve meaningful runtime recovery and termination behavior. Do not add status
outputs and caller branches merely to reproduce defensive plumbing without a
useful caller response. Distinguish invalid authored configuration from an
expected runtime condition. Removing an existing failure gate can change behavior
even when successful execution is identical; identify and obtain agreement on
that change instead of presenting it as a mechanical extraction. The shared
[error-handling rule](error-handling-rule.md) still applies.

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

Choose gameplay checks from the affected states and transitions, with expected
outcomes: entry and exit, enable and disable, loss and recovery of eligibility,
press and release, and interactions between affected controls. Graph names and
node counts do not define adequate test coverage. One combined scenario may
exercise several extractions; retain distinct checks where behavior differs.

Use the ordinary Apply and one-call Preview workflow. Compilation and export
comparison establish editor consistency, not gameplay equivalence or a performance
improvement. If new failures or unresolved dependencies appear, narrow the next
step around that uncertainty rather than continuing to enlarge the batch.
