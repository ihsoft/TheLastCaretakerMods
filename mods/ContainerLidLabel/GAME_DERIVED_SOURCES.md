# Game-derived source contract

This generator is bound to Voyage Steam build `25880952`, Unreal Engine 5.8,
and `VoyageSteam-Win64-Shipping.exe` SHA-256
`16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44`.

The release producer resolves the installed fingerprint before building and
freshly extracts
`/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large` with
the public stock-only extractor. It validates exactly two embedded occurrences
of the 69-byte source package path, replaces them with the equal-length
relocated path, preserves file length, and requires the `.uexp` payload to be
byte-identical. The extracted source, relocated bytes, cooked assets, reports,
and release outputs remain ignored under `Tmp/` or generated mod directories.

Revalidate every native mirror, parent/function owner, field owner, relocation
count, and serialized layout after any fingerprint change. Compile, cook,
container verification, and CUE4Parse readback are static gates.

The exact `v3-test` bytes built from this contract were user-validated for the
requested presentation: accepted icon size/positions, native-side placement of
a sole icon, and the lower name remaining visible with icons. Durable evidence,
including the reviewed build manifest, semantic report, exact code-source
snapshot and installed-file hashes, is retained under
`artifacts/installations/ContainerLidLabel/20261013-070846-v3-test-6b391a6f/`.
That evidence does not claim separate coverage of long localized names,
extreme count widths, lid animation, asynchronous icon refresh, or save/load.
