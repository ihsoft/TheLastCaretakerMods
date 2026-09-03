# BoatTotalDiesel

BoatTotalDiesel is a loader-free mod for The Last Caretaker. It changes the
lower-right fuel value shown while possessing the Boat from the main tank's
amount to the total Diesel stored by every module in the same ship grid.

Petrol is a separate resource and is never included. The mod ships no native
DLL and requires neither UE4SS nor DML.

## Runtime architecture

The stock Boat HUD remains the lifecycle and layout owner. A narrow child at
the stock package path collapses only the native-updated fuel text and inserts
a separate text leaf in the same parent panel. Repeating aggregation runs only
on that leaf, whose direct parent is engine `UUserWidget`.

The leaf obtains the possessed Boat's `VoyageModuleComponent`, enumerates
modules in the same grid through `VoyageModuleSubsystem`, and sums only
`GetResourceAmount(EModuleResourceType::Diesel)`.

This composition avoids adding Tick state to the unversioned inherited Voyage
HUD child. The remaining feature gates are recorded in
[`../../docs/boat-total-diesel-backlog.md`](../../docs/boat-total-diesel-backlog.md);
completed cooked-asset work is documented separately in
[`../../docs/voyage-cooked-asset-toolchain.md`](../../docs/voyage-cooked-asset-toolchain.md).

## Validation status

The aggregation path was validated in game on the previous Steam build
`23962331`: the
lower-right value changed from the main-tank-only `1911 L` to the confirmed
total `18966 L` without disturbing neighboring HUD elements.

Steam build `24990438` moved Voyage to Unreal Engine `5.8.1` and invalidated
that cooked package. Fresh runtime reflection confirms that the HUD field,
Diesel enum value, and both aggregation functions are unchanged. The available
editor is UE 5.8.2. Its output for the UE 5.8.1 game passed the static
container gates and loaded successfully in the game.

On 2026-08-31 the user confirmed that the installed build `24990438` works in
the real game and shows the intended total-Diesel HUD value. Petrol stored in
tanks on the ship was also confirmed not to enter the displayed sum.

The repository's current game fingerprint is Steam build `25056839`, still on
game UE `5.8.1`. Toolchain and changed-save canaries are validated for that
fingerprint, but the mod's total-Diesel behavior and Boat leave/re-enter
lifecycle still require an explicit current-build game check before a new
release claim.

## Build

Build the reviewed UE 5.8 retoc compatibility binary once from the repository
root:

```powershell
.\tools\Build-RetocUe58Compatibility.ps1 `
  -OutputRoot '.\artifacts\tools\retoc-ue58-compatibility'
```

Then prepare a clean current-build original while Voyage is closed:

```powershell
.\Prepare-BoatTotalDieselOriginal.ps1 `
  -OutputRoot '..\..\artifacts\extracted\boat-total-diesel-original'
```

Build the container:

```powershell
.\Build-BoatTotalDiesel.ps1 `
  -OriginalsRoot '..\..\artifacts\extracted\boat-total-diesel-original' `
  -OutputRoot '..\..\artifacts\builds\boat-total-diesel'
```

The build gates the game fingerprint, clean extraction provenance, cooked
graphs and CDOs, exact three-asset inventory, and final container hashes.

## Install and remove

Install only while Voyage is closed, preserving exact removal evidence:

```powershell
.\Install-BoatTotalDiesel.ps1 `
  -PackageRoot '..\..\artifacts\builds\boat-total-diesel\package' `
  -EvidenceRoot '..\..\artifacts\tests\boat-total-diesel-install'
```

Remove only while Voyage is closed:

```powershell
.\Remove-BoatTotalDiesel.ps1 `
  -InstallManifest '..\..\artifacts\tests\boat-total-diesel-install\install-manifest.json'
```
