# Blueprint text prototype

Start with the [short guide](../blueprint-text-prototype.md). This is a shared
tool experiment, not a mod producer. Keep generated projects and results under
`Tmp/BlueprintTextPrototype/`; keep the catalogue cache local and ignored.

The agent edits desired source directly. Extend shared factories and reflection
when needed; do not introduce per-mod graph-building code. Unsupported state
must be diagnosed rather than silently discarded. Do not imply universal
Blueprint preservation from the bounded tests.
