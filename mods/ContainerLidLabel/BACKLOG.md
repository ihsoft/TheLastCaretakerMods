# Container Lid Label backlog

## Restart state

- Requested scope is complete and game-validated on Voyage Steam build
  `25880952`, executable SHA-256
  `16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44`.
- Current installed artifact is `v3-test`. It duplicates the container name on
  the black lid plate, keeps that name visible with icons, uses fixed 5 cm icon
  slots beside the count, and keeps a sole icon on its stock left or right side.
- The exact-large loader-free design contains two runtime packages: a relocated
  fresh stock Large parent and its child at the original stock package path.
- Durable build, semantic, installation, rollback and user-validation evidence
  is under
  `artifacts/installations/ContainerLidLabel/20261013-070846-v3-test-6b391a6f/`.
  Its predecessor backup restores the exact installed `v2-test` triplet; older
  provenance ZIPs remain installed.
- No current feature work is pending. Treat the user's later idea as a separate
  scope rather than extending this completed implementation implicitly.

## Validation boundary

The user confirmed the icon size and positions, native-side placement of a sole
icon, and simultaneous lower name plus icons. Long localized names, extreme
count widths, lid animation, asynchronous icon refresh, and save/load were not
individually exercised. A changed game fingerprint invalidates the reconstructed
contracts and requires fresh extraction, build and validation.

The mod overrides
`/Game/Blueprints/Interactives/Containers/BP_Inventory_PlayerBox_Large` and may
conflict with another container that provides the same virtual package.
