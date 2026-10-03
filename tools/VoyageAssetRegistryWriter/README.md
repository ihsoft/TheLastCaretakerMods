# Voyage AssetRegistry writer

This is an independent UE 5.8 editor host used by
`../New-VoyageAssetRegistry.ps1`. Its commandlet accepts already normalized
metadata, writes a premade registry through Unreal's
`FAssetRegistryState.Save`, reopens it, and compares every record before it
reports success. It does not load Voyage classes or cooked mod assets.

`voyage-policy.json` is the shared game profile that converts UAssetAPI/
UAssetGUI JSON into registry tags. The current profile supports one top-level
primary export per input package for `/Script/Voyage.VoyageItem`,
`VoyageItemAmmo`, or `VoyageSkill`; auxiliary exports are ignored. It writes
format 24 with `filterEditorOnly=true`, empty bundles/dependencies/package data,
and default chunk `0`. The profile records the game fingerprint that established
its restored native-class defaults and its revalidation condition. Explicit
serialized false, zero, and empty values override defaults.

The native writer itself is class-agnostic and accepts any positive record
count, multiple assets in one package, and arbitrary chunk arrays. The normal
path consumes a manifest-validated publication below `.tools/bin`; run
`Publish-VoyageAssetRegistryWriter.ps1` explicitly after compiled source or
the selected UE 5.8 build changes.
