# Container Lid Label backlog

## Restart state

- Current independently recorded installation is `v5-test-20261015-054306`,
  installed by the parent directly at 2026-10-15 06:09 UTC on Steam `25929985`.
  Triplet/archive hashes matched; the existing INI retained `MaxWeightKg=200`
  while its obsolete section header was removed. Exact predecessor backups,
  install/readback evidence and separate INI migration/restore record are under
  `artifacts/installations/ContainerLidLabel/20261015-060929-v5-test-0f93978d/`.
  The common restore handles containers/archive; restore the separately backed-up
  INI too when returning to the sectioned predecessor. The user subsequently
  confirmed that this installed version works; individual edge cases were not enumerated.
- Built candidate `v6-test-20261015-065220` renames the external INI key to
  `LargeContainerMaxWeightKg`, reserving distinct names for future container
  types. Default, parser, cache, lifecycle and visuals stay unchanged. No legacy
  alias. At installation, rename the existing key while preserving its value
  and comments, with backup and readback. Build/container validation and 19
  cooked assertions passed. UToc SHA256:
  `D4C8BC0F774F6701DD187A41B711F7E525DE4DC49A21653D0EA305B465F12527`.
  Release is under `Tmp/ContainerLidLabel/releases/ContainerLidLabel-v6-test-20261015-065220/`.
  Not installed. Do not alter the running game.
- The earlier `v3-test` presentation was game-validated
  on Steam build `25880952`. It duplicates the container name on
  the black lid plate, keeps that name visible with icons, uses fixed 5 cm icon
  slots beside the count, and keeps a sole icon on its stock left or right side.
- Candidate `v5-test` extends the same exact Large-container child with flat
  `Content/Paks/ContainerLidLabel.ini` capacity configuration. Default is 200 kg;
  missing or invalid configuration falls back to that embedded authored value.
  It applies after the stock parent from BeginPlay and persistent post-load,
  caches the parsed result on the exact generated Large class default object,
  preserves inventory contents, and refreshes the existing capacity bar. The
  file may be read again only if that generated class is unloaded and reloaded.
- The prior sectioned `v4-test` was manually installed and the user reported it
  works. Installed hashes and detailed edge-case coverage were not independently
  established. The flat `v5-test` candidate is built and statically validated for Steam build `25929985`,
  executable SHA-256
  `608A4BCD1A0A392477970F455234571946D061E61C1D27F82DE9589EBBA3D69F`.
  It is installed and user-confirmed working as recorded above.
- The exact-large loader-free design contains two runtime packages: a relocated
  fresh stock Large parent and its child at the original stock package path.
- Durable build, semantic, installation, rollback and user-validation evidence
  is under
  `artifacts/installations/ContainerLidLabel/20261013-070846-v3-test-6b391a6f/`.
  Its predecessor backup restores the exact installed `v2-test` triplet; older
  provenance ZIPs remain installed.
- Next gate is authorized installation of `v6-test` with the INI rename and
  runtime confirmation. Unenumerated
  edge cases remain existing/new boxes, persistent reload, lowering below
  contents and cache retention until class reload/restart.

## Validation boundary

The user confirmed the icon size and positions, native-side placement of a sole
icon, and simultaneous lower name plus icons for `v3-test` on Steam build
`25880952`. The user reported the manually installed sectioned `v4-test` works,
without independently verified installed hashes or detailed edge-case coverage.
The user confirmed that the installed flat `v5-test` works. The renamed-key
`v6-test` is not yet runtime-validated. Long localized names, extreme count widths, lid animation,
asynchronous icon refresh, and save/load presentation were not individually
exercised. A changed game fingerprint invalidates the reconstructed contracts
and requires fresh extraction, build and validation.

The mod overrides
`/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large` and may
conflict with another container that provides the same virtual package.
