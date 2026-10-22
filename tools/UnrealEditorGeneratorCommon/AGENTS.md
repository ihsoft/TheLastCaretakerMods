# Unreal editor generator common index

This directory contains header-only, engine-level Blueprint graph helpers for
editor commandlets. It is build-time source, never a runtime mod payload.

- [Toolchain maintenance](../toolchain-maintenance.md#shared-generator-helpers):
  ownership and extension guidance for the shared helpers.
- [Repository hygiene](../../docs/repository-hygiene-rule.md): reflected-name
  constants and semantic pin-role conventions.

Keep Voyage identities, mod paths, and mod tuning in their owning mod. Inspect
the relevant header before extending an existing semantic group.
