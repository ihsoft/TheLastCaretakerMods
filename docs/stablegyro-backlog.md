# StableGyro active backlog

## Restart

- Current committed baseline: `1de443f`.
- Current target fingerprint: Steam build `25880952`, UE `5.8.2` editor /
  `5.8` parser, executable SHA-256
  `16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44`.
- Current source change: `PitchRampSeconds=1.5` is the new canonical default.
  The user tested that value through the installed INI, prefers its response,
  and explicitly requested the default change. The installed INI already has
  `1.5`; this source-only change does not rewrite it.
- Installed candidate: `v1-test2`. Durable installation and rollback evidence
  lives at
  `artifacts/installations/StableGyro/20261012-231553-v1-test2-736debce/install-manifest.json`
  with sibling `settings-installation.json`. The settings hash in that evidence
  describes installation time and was superseded by the user's later edit.
- Runtime boundary: the user validated settings reload on Gyro entry, the
  `EnablePitchCorrection` enabled/disabled A/B behavior, and the preferred
  `PitchRampSeconds=1.5` tuning. This does not validate every altitude,
  horizontal-damping, or pitch-physics case.
- Pitch timing has no active investigation. The user measured the previous
  `PitchRampSeconds=3.0` configuration at approximately three seconds from
  neutral to maximum. The older build's perceived 1.5-second duration was not
  measured, so its historical cause remains unproven and does not block the
  selected tuning.
- No new build or installation is required for the canonical-default edit.
  Future builds will consume `1.5`; the current installed user value already
  matches it.
- No pending implementation or diagnostic work; further runtime validation is
  required only for additional compatibility claims.
- Owning references: `mods/StableGyro/AGENTS.md`,
  `mods/StableGyro/GAME_DERIVED_SOURCES.md`,
  `docs/research-pitfalls.md#управление-gyro`, and
  `docs/vehicle-and-hud-modding-patterns.md`.
