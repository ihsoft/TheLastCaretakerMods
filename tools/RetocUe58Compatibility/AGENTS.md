# retoc UE 5.8 compatibility index

This component owns the narrow retoc compatibility build required by Voyage UE
5.8 legacy packages. Normal asset and release tools resolve the canonical binary
themselves; do not build this component as a routine preflight.

- [Toolchain maintenance](../toolchain-maintenance.md): publish or repair the
  canonical retoc binary and locate the explicit development builder.
- [Cooked-asset knowledge](../../docs/voyage-cooked-asset-toolchain-knowledge.md):
  the legacy import-layout boundary that requires this compatibility path.

For explicit compatibility development, use
`../Build-RetocUe58Compatibility.ps1`; its checks and generated manifest own the
required interface and compatibility assertions.
