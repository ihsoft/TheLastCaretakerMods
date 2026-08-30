# Voyage asset patcher

Small, assertion-heavy patches for isolated cooked-asset experiments. Game
assets and generated outputs remain under ignored `artifacts/`; this directory
contains only the reproducible transformation.

The current UAssetAPI NuGet release does not support Unreal Engine 5.7. Clone
the current UAssetAPI source beside this repository as
`R:\Codex\ToolCache\UAssetAPI-source`, or override the project path when
building:

```powershell
dotnet build .\tools\VoyageAssetPatcher\VoyageAssetPatcher.csproj `
  -p:UAssetApiProject=D:\src\UAssetAPI\UAssetAPI\UAssetAPI.csproj
```

The `break-bottom-action-filter` operation locates the exact
`BP_DynamicPlayerInputHorizontalWidget_Bottom` export, asserts that its
`bFilterByActionType` property is `true`, changes it to `false`, writes to a
different asset, and reopens that output to verify the change. It deliberately
breaks the native E/H hint row so a real-game test can prove which serialized
widget owns the visible panel.

For Voyage 5.7.4, UAssetAPI cannot structurally deserialize this game-specific
Widget Blueprint export and exposes it as 17 raw bytes. The operation accepts
only the fully known `01044B039FFFFFFF010D00000000000000` layout and changes
byte 8 from `01` to `00`. This is the serialized boolean between the
unversioned-property header and the unchanged CanvasPanelSlot package index.
Any different game-version layout is rejected.

The `swap-forklift-horn-to-exit` operation is a reversible producer probe. It
locates the original forklift CDO, asserts that `HornInputAction` references
`IAV_VehicleHorn` and `ExitAction` references `IAV_VehicleExit`, then copies
the existing exit-action package index into the horn field. No import is added.
If the native standard HUD is built from these CDO fields, `H / Horn` should
become a duplicate `E / Exit Vehicle` hint. The output is reopened and checked
before packaging.

UAssetAPI's normal writer is used only to create two temporary, equally
reserialized files: an unchanged baseline and a Horn-to-Exit variant. Their
diff must contain exactly one four-byte package-index replacement. The tool
then applies that int at the same verified offset in the exact original
`.uexp`, copies the `.uasset` byte-for-byte, and reopens the surgical output.
This avoids shipping unrelated round-trip normalization changes.
