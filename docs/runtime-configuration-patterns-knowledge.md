# Runtime configuration patterns (inferred reuse patterns)

These patterns apply to runtime settings across mods. They do not establish
that a particular game class exposes the required events or native APIs;
verify those contracts against the supported game fingerprint.

## Separate reading, storage and application

Choose when a setting must change before choosing its owner. A restart-oriented
global setting can be read once and applied to many instances. An instance
creation/load callback is an application point, not necessarily a file-read point.
An interaction-based reload is a different contract and must cover other routes
that consume the same setting. Avoid periodic discovery when owned references
and lifecycle events suffice.

A mod-owned class default object can hold a shared transient cache when the
engine/runtime permits the necessary calls without a world. Use the exact owned
class, cache successful reads and fallback outcomes, and keep parsing free of
latent operations and instance/world side effects. Never use a stock CDO as
mutable mod storage. This cache lasts as long as the class/CDO stays loaded;
class reload may reread the file. Do not promise process-wide lifetime without
evidence. Apply the cached value separately to each live instance.

## Preserve runtime contracts

Call the native setter when effective state depends on derived values,
modifiers or notifications; changing a serialized field alone may not update
runtime state. Refresh dependent presentation through its established contract.
For lifecycle overrides, verify the exact declaring owner and cooked parent-call
target. Successful compilation or a visible parent-call node does not prove
that intermediate game initialization remains in the call chain.

Validate numeric settings in their final storage type. A positive finite double
can become zero when narrowed to float; range checking before conversion alone
does not preserve a positive-value contract. Define missing/invalid input
behavior and keep authored defaults consistent with the embedded fallback.

Document accepted keys, units, comment syntax and cache lifetime. Name keys by
their semantic scope when multiple object types may share a file. Renaming a
key requires an explicit migration/alias policy and preservation of user values
and comments during installation; do not silently replace them with defaults.

For a concrete implementation and its version-bound limits, see
[ContainerLidLabel architecture](../mods/ContainerLidLabel/ARCHITECTURE.md).
