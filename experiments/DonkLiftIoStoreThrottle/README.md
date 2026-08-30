# DonkLift IoStore throttle experiment

Validated source fingerprint: Steam build `23962331`, Unreal Engine `5.7.4`,
`VoyageSteam-Win64-Shipping.exe` SHA-256
`6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`.
Re-extract and revalidate every game-derived asset, offset, and hash after an
update.

This experiment tests whether Voyage accepts a pure IoStore override without
UE4SS. It intentionally changes only one existing Blueprint constant in
`BP_Forklift_Possesable`:

```text
FInterpTo_Constant(..., InterpSpeed = 1.0)
                              -> 0.2
```

Expected in-game result: both throttle buildup and automatic throttle release
are about five times slower than vanilla. This is only a loader/override proof,
not the final persistent-throttle design.

For an unambiguous loader test, pass `-Replacement 0.0`. The native input HUD
will still show `100%` while W/S is held, but the internal `Acceleration` cannot
move away from its current value, so a stationary forklift must not start.

The patch is pinned to the currently inspected Voyage 5.7.4 asset. The script
refuses to patch if the extracted `.uexp` SHA-256 or the target bytes differ.

Baseline asset SHA-256 values:

- `BP_Forklift_Possesable.uasset`:
  `B719449396A3057C5CD6C2967F106BBCDA7B1A8B0CAB3332258E4D696CA4EEBC`
- `BP_Forklift_Possesable.uexp`:
  `EF1B840F4751E63C0F537988934E56646A294640DEBBF7CE598D7EA18C016392`

The target was identified structurally from CUE4Parse Kismet output:

- function: `ExecuteUbergraph_BP_Forklift_Possesable`
- call: `KismetMathLibrary:FInterpTo_Constant`
- parameter: fourth (`InterpSpeed`)
- serialized Kismet offset: `3551`
- VM `iCode` index used by jump targets: `2385`
- unique UberGraph block offset in the retoc legacy `.uexp`: `14128`
- double value offset in `.uexp`: `17680`

Packaging requires retoc with `UE5_7` support. The distributable/installable
result is a three-file set: `.pak`, `.ucas`, and `.utoc`.

## Persistent-throttle prototype

`Build-PersistentThrottleAsset.ps1` uses UAssetGUI/UAssetAPI JSON to replace
the original interpolation-speed constant with:

```text
ThrottleInput * ThrottleInput * (1 / 3)
```

The existing `FInterpTo_Constant` still uses the game's original bounded
target. A digital `-1` or `1` produces the same positive ramp speed; releasing
the key produces exactly zero interpolation speed, so the current Acceleration
is retained. No `0.9999` sentinel is needed because this implementation does
not feed its own output back into the keyboard-command field.

Because the new nested expression enlarges the UberGraph bytecode, the script
performs a baseline and draft round-trip, measures the exact serialized bytecode delta, updates all
affected jump targets and every wrapper entrypoint into the UberGraph, then
performs and validates a final round-trip. The distributable asset preserves
the original game `.uasset` header and overlays only the export size/offset
bytes proven necessary by a normalized baseline-versus-patch comparison;
UAssetGUI's unrelated cooked-header normalization is deliberately discarded.

Do not use the CUE4Parse/display offset `3551` to relocate control-flow
operands. Unreal's jump addresses use the VM `iCode` index; for this exact
asset the insertion starts at `2385`.

Use a retoc build containing commit `d7b635039c3d` or later. Release v0.1.5 can
list UE 5.7 as an option, but its legacy header lacks the UE 5.7 import type
hierarchies needed for UAssetGUI to parse this asset.
