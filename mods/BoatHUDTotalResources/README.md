# BoatHUDTotalResources

BoatHUDTotalResources is a loader-free mod for The Last Caretaker. It changes the
lower-right fuel and electricity values shown while possessing the Boat from
the built-in modules' amounts to the corresponding totals stored by every
module in the same ship grid.

Petrol is a separate resource and is never included. The mod ships no native
DLL and requires neither UE4SS nor DML.

## Runtime architecture

The stock Boat HUD remains the lifecycle and layout owner. A narrow child at
the stock package path collapses only the native-updated fuel and battery text
and inserts separate text leaves in their existing parent panels. Repeating
aggregation runs only on those leaves, whose direct parent is engine
`UUserWidget`.

Each leaf obtains the possessed Boat's `VoyageModuleComponent`, enumerates
modules in the same grid through `VoyageModuleSubsystem`, and sums its explicit
resource: Diesel or Electricity. The electricity amount is displayed as one
decimal kWh. Each generated leaf copies the corresponding stock text's font,
color, and text-transform policy before it is inserted.

This composition avoids adding Tick state to the unversioned inherited Voyage
HUD child. Shared cooked-asset contracts are documented separately in
[`../../docs/voyage-cooked-asset-toolchain.md`](../../docs/voyage-cooked-asset-toolchain.md).

## Compatibility

The two-resource HUD supports Steam build `25056839`, game UE `5.8.1`.
It displays the Boat's grid-wide Diesel and Electricity totals without adding
Petrol or disturbing the neighboring HUD elements. The builder uses the
available UE 5.8.2 editor and a fresh stock HUD package for the target build.
Its game-derived contracts are recorded in
[GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md).

## Build

Publish the reviewed canonical UE 5.8 retoc compatibility binary once from the
repository root:

```powershell
.\tools\Publish-RetocBinary.ps1
```

Then prepare a clean current-build original while Voyage is closed:

```powershell
.\Prepare-BoatHUDTotalResourcesOriginal.ps1 `
  -OutputRoot '..\..\artifacts\extracted\boat-hud-total-resources-original'
```

Build the container:

```powershell
.\Build-BoatHUDTotalResources.ps1 `
  -OriginalsRoot '..\..\artifacts\extracted\boat-hud-total-resources-original' `
  -OutputRoot '..\..\artifacts\builds\boat-hud-total-resources'
```

The build gates the game fingerprint, clean extraction provenance, cooked
graphs and CDOs, exact four-asset inventory, and final container hashes.

## Install and remove

Install only while Voyage is closed, preserving exact removal evidence:

```powershell
$evidenceRoot = Join-Path '..\..\artifacts\installations\BoatHUDTotalResources' `
  ([Guid]::NewGuid().ToString('N'))
.\Install-BoatHUDTotalResources.ps1 `
  -PackageRoot '..\..\artifacts\builds\boat-hud-total-resources\package' `
  -EvidenceRoot $evidenceRoot
```

Remove only while Voyage is closed:

```powershell
.\Remove-BoatHUDTotalResources.ps1 `
  -InstallManifest (Join-Path $evidenceRoot 'install-manifest.json')
```

For removal in a later session, use the manifest path printed by the installer.
