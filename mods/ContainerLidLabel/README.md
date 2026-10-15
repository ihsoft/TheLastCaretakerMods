# Container Lid Label

Container Lid Label adds a second label row to the black lid plate of the large
FortiHold container and lets one external INI set that container's mass
capacity. The stock side label, count, inventory contents, capacity bar, and lid
mesh behavior remain in place. The lid row duplicates the stock container name.
When the stock icon selection is visible, the lid also shows the same one or two
icon materials while keeping the name row visible.

The release contains exactly two cooked packages. The exact stock large-box
Blueprint is relocated to
`/Game/Mods/ContainerLidLabel/StockOrigin/BP_Inventory_PlayerBox_Large`; a
generated child at the original package path inherits that snapshot and adds
one `VoyageWidgetComponent` plus two icon planes under inherited
`PlayerBox_Lid`. The child overrides only `InitTextWidget`,
`UpdateTextWidget`, and `UpdateIconMaterial`, and each override calls the exact
stock parent function first. The same exact Large-only child calls its stock
parent first from `ReceiveBeginPlay` and `OnPersistentActorPostLoad`. It then
uses the exact generated Large class default object as a shared settings cache,
calls the stock inventory setter, and refreshes the existing capacity bar from
the inventory's current percent used.

The authored default is [ContainerLidLabel.ini](Assets/ContainerLidLabel.ini):

```ini
# Installed beside the mod containers in Voyage/Content/Paks/ContainerLidLabel.ini.
; Read once per generated-class load. Use a decimal point and no inline comments.
LargeContainerMaxWeightKg=200
```

At runtime the generated Large class reads
`Voyage/Content/Paks/ContainerLidLabel.ini` once while that generated class and
its class default object remain loaded. Every Large container and persistent
load callback reuses the cached result. If the class is unloaded and loaded
again, it may read the file again. The flat-file parser trims whitespace,
matches the key, ignores blank lines and full-line `;` or `#` comments, and
uses the last valid matching key. A missing file or key, or no
valid finite positive float value, uses the embedded `200` kg authored default.
Inline comments are not supported. Lowering the limit preserves existing
contents; the mod never clears, recreates, or trims inventory stacks.
The key applies only to the large container. When upgrading from `v5-test`,
rename `MaxWeightKg` to `LargeContainerMaxWeightKg` in the installed INI,
preserving its value and comments. The old key is not an alias.

Build with Windows PowerShell 5.1 from the repository root:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File `
  .\mods\ContainerLidLabel\Build-ContainerLidLabel.ps1 `
  -Version v6-test -AllowDirtySource
```

The producer requires Unreal Engine 5.8 at `K:\Epic Games\UE_5.8` and the
current Voyage installation at `P:\SteamLibrary\steamapps\common\Voyage` by
default. It writes a fresh release below `Tmp/ContainerLidLabel`, validates the
exact two-package inventory, creates the `_P` IoStore triplet, ships the INI
both beside the triplet and inside the ZIP, and publishes
`release-manifest.json` through the common validate-only installer gate. It
never installs the release. The common installer does not deploy this extra INI;
a later installation workflow must place it without overwriting an existing
user-edited copy.

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
were not individually exercised by that report. The capacity extension and the
rebuilt presentation on Steam build `25929985` was manually installed and the
user reported that the prior sectioned `v4-test` works. That report does not
independently establish installed hashes or detailed coverage of new/existing
boxes, persistent reload, a non-default value, cache behavior, or lowering the
limit below current contents. After the recorded installation, the user also
confirmed the flat `v5-test` works. The renamed `v6-test` key was subsequently
confirmed working with a temporary 1000 kg test value. The authored INI and
embedded fallback remain 200 kg, preserving stock capacity by default.

Use the [shared release workflow](../../tools/README.md#release-producers) for
build output and the [common installation and restore
workflow](../../tools/README.md#release-installation) for guarded deployment or
rollback. This mod overrides the stock Large container package and may conflict
with any other mod that provides the same virtual package.
