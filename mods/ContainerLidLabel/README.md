# Container Lid Label

Container Lid Label adds a second label row to the black lid plate of the large
FortiHold container while leaving the stock side label, count, capacity bar,
inventory component, and lid mesh behavior in place. The row duplicates the
stock container name. When the stock icon selection is visible, the lid also
shows the same one or two icon materials while keeping the name row visible.

The release contains exactly two cooked packages. The exact stock large-box
Blueprint is relocated to
`/Game/Mods/ContainerLidLabel/StockOrigin/BP_Inventory_PlayerBox_Large`; a
generated child at the original package path inherits that snapshot and adds
one `VoyageWidgetComponent` plus two icon planes under inherited
`PlayerBox_Lid`. The child overrides only `InitTextWidget`,
`UpdateTextWidget`, and `UpdateIconMaterial`, and each override calls the exact
stock parent function first.

Build with Windows PowerShell 5.1 from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File `
  .\mods\ContainerLidLabel\Build-ContainerLidLabel.ps1 `
  -Version v3-test -AllowDirtySource
```

The producer requires Unreal Engine 5.8 at `K:\Epic Games\UE_5.8` and the
current Voyage installation at `P:\SteamLibrary\steamapps\common\Voyage` by
default. It writes a fresh release below `Tmp/ContainerLidLabel`, validates the
exact two-package inventory, creates the `_P` IoStore triplet and ZIP, and
publishes `release-manifest.json` through the common validate-only installer
gate. It never installs the release.

To recreate only `build-manifest.json` for an existing validated release, pass
its exact `-OutputRoot`, matching `-Version`, and `-FinalizeExisting`. This mode
checks the saved container report, release identity, fingerprint, payload and
archive hashes, and relocated stock export before writing the report. It does
not build, cook, package, or install.

The installed `v3-test` layout uses label center `(0, 66.4, 13.73)`,
widget draw size `200x110`, scale `0.12`, and stock Voyage Roboto Bold at size
12. Text wrapping is disabled on both stock text blocks. The larger icon slots
flank the stock count at fixed lid-local centers `(-17, 62.27709, 13.73)` and
`(17, 62.27709, 13.73)` with scale `0.05`. A sole icon remains in its native
left or right slot. The lower name widget stays visible with either icon.

The user validated the requested presentation in the real game on Steam build
`25880952`, executable SHA-256
`16D0726AF6545CDFCCC2722444DB4DAD7236B93514368E8FE4758171E758EF44`:
the icon size and positions look correct, a sole icon stays on its native side,
and the lower name remains visible together with icons. Long localized names,
extreme count widths, lid animation, asynchronous icon refresh, and save/load
were not individually exercised by that report.

Use the [shared release workflow](../../tools/README.md#release-producers) for
build output and the [common installation and restore
workflow](../../tools/README.md#release-installation) for guarded deployment or
rollback. This mod overrides the stock Large container package and may conflict
with any other mod that provides the same virtual package.
