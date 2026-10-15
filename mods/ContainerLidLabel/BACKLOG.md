# Container Lid Label: current state

## Restart

- Validated implementation checkpoint: `fb5eadc`.
- Installed, user-validated artifact: `v6-test-20261015-065220` on Steam
  `25929985`; exact executable/mapping identities are in
  [GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md).
- Authored and embedded default: `LargeContainerMaxWeightKg=200`. A nondefault
  value was confirmed in game; personal test settings are not release defaults.
- No required implementation or runtime gate remains for the requested feature.
- Contracts: [README](README.md), [architecture](ARCHITECTURE.md),
  [provenance](GAME_DERIVED_SOURCES.md).

## Recovery and evidence

Installation, exact predecessor backups, INI migration and retained build/release
validation records are under
`artifacts/installations/ContainerLidLabel/20261015-065622-v6-test-2dc3ea33/`.
The installed `ContainerLidLabel_v6-test.zip` retains the payload independently
of `Tmp/`. UToc SHA256:
`D4C8BC0F774F6701DD187A41B711F7E525DE4DC49A21653D0EA305B465F12527`.

The common restore manages containers/archive; restore the separate INI backup
as well when returning to the predecessor's old key. Preserve user values and
comments during upgrades. Never mutate installed files while the game runs.

## Validation boundary and deferred questions

Requested lid presentation and configurable capacity are user-validated.
Long localized names, extreme count widths, asynchronous icon refresh, each
save/stream lifecycle path, lowering below existing contents and cache retention
across class unload/reload were not individually enumerated by that report.
Investigate them only when a requested change or observed defect makes them relevant.

Other container types are not supported. Overriding the same stock Large package
in another mod can conflict with this mod.
