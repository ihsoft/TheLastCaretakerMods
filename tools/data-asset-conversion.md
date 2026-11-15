# Cooked DataAsset serialization conversion

Use `Convert-VoyageDataAsset.ps1` when a maintained native DataAsset needs a
documented cooked serialization contract different from its editor cook. This
changes serialization, not authored values. It is separate from editing a
project DataAsset or reconstructing one from game data.

The consumer supplies the package/class identity, reviewed mapping, native
readback and serialization contract. It owns the reason for conversion and
game-build applicability; the tool has no mod names or asset catalogue.
Do not convert unrelated classes merely because a different mod needs it.

```powershell
python -B tools/run_powershell.py tools/Convert-VoyageDataAsset.ps1 `
  -CookedAsset Tmp/Example/build/stage/Voyage/Content/Mods/Example/DA_Item.uasset `
  -Package /Game/Mods/Example/DA_Item -ExpectedClass /Script/Voyage.VoyageItem `
  -NativeReadback Tmp/Example/build/item-project.json `
  -SerializationContract mods/Example/Build/item-serialization.json `
  -MappingsPath '<reviewed usmap>' -EngineVersion '<mapping engineVersion>' `
  -OutputRoot Tmp/Example/build/item-conversion
```

Run outside the restricted sandbox, as for the other native toolchain operations.
The published `.tools/bin/UAssetGUI.exe` performs parsing/writing through the
[bounded runner](toolchain-validation.md). The command does not build dependencies.

`CookedAsset` must be an existing disposable `.uasset/.uexp` pair below repository
`Tmp/`; linked working paths and additional package companions are rejected.
`OutputRoot` must be fresh and below `Tmp/`. The command reads the pair, validates
and converts in its output directory, then replaces that same pair in the
caller's stage only after all comparisons pass. Source project assets are never
the conversion input. Failure retains diagnostics; if final copying fails, the
stage is incomplete and must not be packaged. Rebuild a fresh stage to retry.

## Native readback

Use `Read-VoyageDataAsset.ps1` to obtain the conversion input from the saved
working project. It reads one native DataAsset and its owned objects through
Unreal reflection, with an exact expected class. It never saves asset packages.
Both the project and a fresh output directory must be below repository `Tmp/`;
linked paths are rejected. The operation owns the Unreal queue, prepares declared
native dependencies and attaches its shared reader plugin. Other plugins enabled
by the project must also be available: use BPT `Prepare` first for a project
that enables `BlueprintTextWorkspace`.

```powershell
$native = python -B tools/run_powershell.py tools/Read-VoyageDataAsset.ps1 `
  -ProjectFile Tmp/Example/build/Project/Voyage.uproject `
  -Package /Game/Mods/Example/DA_Item -ExpectedClass /Script/Voyage.VoyageItem `
  -EngineRoot '<selected engine root>' `
  -OutputRoot Tmp/Example/build/native-item | ConvertFrom-Json
```

Pass the returned `readbackPath` as the converter's `NativeReadback`; successful
reading returns `status: passed`. Read and cook the same saved working state.
The tool retains `native.json` and preparation/editor diagnostics in the output.
A missing asset, mismatched class, unsupported property or duplicate owned-object
name fails the operation. Inspect the reported log; supply missing project
dependencies or extend the shared reader's supported representation rather than
adding a mod-specific reader or weakening the comparison. Native API mirrors
and serialization contracts remain consumer-owned.

This is conversion evidence, not an authoring format. Use the shared
[BPT DataAsset workflow](blueprint-workspace.md#edit-an-existing-dataasset) to
edit saved properties and keep their review text synchronized.

## Contract and comparison

The input must be a complete tagged cook with one top-level native object,
whose name equals the package basename and whose class equals `ExpectedClass`
(a `/Script/Module.Class` path). Owned exports are allowed; duplicate export
names are rejected because this comparison identifies them by name. This is a
bounded DataAsset path, not an arbitrary Blueprint or bulk-data converter.

`SerializationContract` is a JSON object containing exactly these UAssetAPI
metadata fields: `IsUnversioned`, `LegacyFileVersion`, `ObjectVersion`,
`ObjectVersionUE5`, `FileVersionLicenseeUE`, `CustomVersionContainer`,
`PackageFlags`, `HasUnversionedProperties`, `CustomSerializationFlags`.
The requested output may use tagged or unversioned properties. Contract values
come from the consumer's reviewed game-format evidence, not a shared fixed
game version. The writer rebuilds the property stream using the mapping;
this is not a binary header patch. Import `PackageName` fields use `None`, and
unversioned enum spelling is normalized for the writer.

The operation compares fresh cook with a native read of the same working project,
then compares the rewritten package with that fresh cook. It preserves represented
properties, export class/outer/template/super identities, object flags, references,
owned objects and supported localized text histories. It also verifies every
requested serialization metadata value after reopening the result.

The converter accepts either `{class, fields}` for a root without owned
objects, or `{objects: {name: {class, outer, fields}}}` for the complete owned
object graph. The shared reader produces the graph form. Fields represent
serialized differences from native defaults:
scalars as `{type, value}`, references as `{object: path-or-null}`, structs as
`{struct, fields}`, arrays as `{array: [...]}`, maps as `{map: [[key, value], ...]}`,
enums as `{enum, value}`, and localized text as `{text: history}`. Text supports
Base and StringTableEntry histories, including flags and identity; optional
`nativeHistory` must agree with that identity. Map order and import-table indices
are normalized; semantic changes are not. Unsupported reflected types and fixed
arrays are rejected by the reader. The fresh-cook comparison also covers whether
the reflected state represents the class's cooked serialization. BPT authoring
output is not interchangeable with this readback format.

Success returns `status: passed`, `readbackPath` (UAssetAPI JSON), and `write` /
`readback` bounded-process results. The output retains `fresh-cook.json`, writer
input, converted packages, `native-cook.checked.json`, `conversion.checked.json`
and process logs. A caller can pass `readbackPath` to the
[primary-asset registry producer](asset-registry.md). Neither comparison nor a
successful build establishes behavior in the game.

For maintenance, run `python -B tools/AssetPackaging/test_packaging.py`. Validate
affected tagged/unversioned modes through a consumer's fresh build; do not add
its entire item payload as a frozen regression fixture.
