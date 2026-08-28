# DonkLift IoStore throttle experiment

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
- unique UberGraph block offset in the retoc legacy `.uexp`: `14128`
- double value offset in `.uexp`: `17680`

Packaging requires retoc with `UE5_7` support. The distributable/installable
result is a three-file set: `.pak`, `.ucas`, and `.utoc`.
