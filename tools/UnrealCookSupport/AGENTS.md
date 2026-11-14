# Shared cook support index

Private editor-only provider for `VoyageModPipeline.psm1`; mod producers use the
[shared pipeline](../mod-setup.md#shared-mod-pipeline), not this commandlet directly.

- [Toolchain maintenance](../toolchain-maintenance.md): when changing native code.
- [Editor support](../unreal-editor-support.md): provider preparation and reuse.

Keep package selection and serialization policy with the caller. This module
only passes an inventory to the stock cooker and checks the requested output
format. It has no game classes or runtime payload.
