# BoatHUDTotalResources

BoatHUDTotalResources is an asset-only mod for The Last Caretaker. It changes the
lower-right fuel and electricity values shown while possessing the Boat from
the built-in modules' amounts to the corresponding totals stored by every
module in the same ship grid.

Petrol is a separate resource and is never included. The mod ships no native
DLL. Its `ModActor` can be spawned by DML or by the optional `.autoload` loader.

## Runtime architecture

The stock Boat HUD remains the lifecycle and layout owner. We do not pack any
copy of the original HUD package. A generated child blueprint at
`/Game/Mods/BoatHUDTotalResources/BP_BoatHUDTotalResources` inherits the stock
HUD dynamically and collapses only the native-updated fuel and battery text,
inserting separate leaves in their existing parent panels.

Runtime replacement is executed by a generated actor blueprint
`/Game/Mods/BoatHUDTotalResources/ModActor` whose
`AActor::BeginPlay` graph obtains the game's `VoyageHUD` and changes only the
`VoyageBoatPawn` entry in its `CharacterWidgetClasses` map to the generated
child. The game remains responsible for creating, owning, hiding, and removing
the Boat HUD through its normal lifecycle.

The actor follows DML's conventional `/Game/Mods/<mod>/ModActor.ModActor_C`
identity. Another gameplay mod can also activate the replacement by spawning
this actor from its own `BeginPlay`.

Each generated leaf obtains the possessed Boat's `VoyageModuleComponent`, enumerates
modules in the same grid through `VoyageModuleSubsystem`, and sums its explicit
resource: Diesel or Electricity. The electricity amount is displayed as one
decimal kWh. Each generated leaf copies the corresponding stock text's font,
color, and text-transform policy before it is inserted.

This composition avoids adding Tick state to the unversioned inherited Voyage
HUD child. The remaining feature gates are recorded in
[`../../docs/boat-hud-total-resources-backlog.md`](../../docs/boat-hud-total-resources-backlog.md);
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
game UE `5.8.1`. On 2026-09-02 the user confirmed that the two-resource HUD
candidate works in the real game. Its graphs were then rebuilt without
behavior changes under the final `BoatHUDTotalResources` identities; the exact
renamed container passed build, cook, four-asset inventory, and `retoc verify`,
then loaded and worked in the real game. This is the current release checkpoint.

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
graphs and CDOs, exact four-asset inventory, and final container hashes. It also
writes `BoatHUDTotalResources.autoload`, whose entry class is the generated
swap actor.

## Install and remove

Install only while Voyage is closed, preserving exact removal evidence for the
container and `.autoload` sidecar. If the legacy `BoatHUDTotalResources_P`
family is installed, the installer backs it up by hash and retires it so two
containers cannot export the same runtime packages:

```powershell
.\Install-BoatHUDTotalResources.ps1 `
  -PackageRoot '..\..\artifacts\builds\boat-hud-total-resources\package' `
  -EvidenceRoot '..\..\artifacts\tests\boat-hud-total-resources-install'
```

Remove only while Voyage is closed:

```powershell
.\Remove-BoatHUDTotalResources.ps1 `
  -InstallManifest '..\..\artifacts\tests\boat-hud-total-resources-install\install-manifest.json'
```
