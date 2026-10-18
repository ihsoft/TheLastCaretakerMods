# Voyage scanning and recipe unlocks (static findings; runtime untested)

## Evidence and version boundary

This is shared game-mechanism research, independent of any particular mod.
The contracts below are bound to stock Blueprint bytecode, reflected property
metadata and native behavior for:

- Executable: `VoyageSteam-Win64-Shipping.exe`.
- Asset parser profile: `UE5_8`.

These are static findings, not validation of a mod calling the same functions.
The Update label for this earlier snapshot was not recorded. Revalidate the
affected Blueprint and native
contracts when a current query disagrees or a new Update changes the behavior.
Extracted assets, disassembly and raw reports remain ignored artifacts.

## Stock Replica Scanner completion path

The equipment item is
`/Game/Data/Assets/Equipment/Hands/DA_Equipment_ScanTool`; its equipment class
is `/Game/Blueprints/Equipment/BP_Equipment_ScanTool`.

`BP_Equipment_ScanTool.OnScanCompleted` enters its event graph, which obtains
the equipment's owner controller, casts it to `VoyagePlayerController`, and
calls this native function exposed to Blueprint:

```cpp
UVoyageGameplayBlueprintFunctionLibrary::MarkItemScanned(
    PlayerController, ScanItem, true);
```

The meaning of the third boolean has not been established; preserve the stock
call value when studying this path rather than inventing a parameter meaning.

The native call chain checks the item and `IsScannable`, obtains an item ID
and rejects empty ID components before updating the scan record. Its grant
branch is conditional on the previous scan count being zero, an additional
virtual predicate whose meaning is not yet identified, and
`VoyageItem.bUnlockAsBuildable` being true. It then obtains the inventory of
type `EVoyageInventoryType::PlayerVault` (enum value `2`), checks that the item
is absent, and calls `AddItem` with an item count of one and both trailing
boolean arguments true.

The relationship between the native reads and `IsScannable` /
`bUnlockAsBuildable` was checked against their reflected boolean setters;
it is not inferred solely from similar names or nearby fields. The inventory
selection and `AddItem` call were also correlated with their native exec
wrappers and the inventory enum.

The receiver is the player's **vault**, not the physical backpack. This is
the stock scan-based route for granting a known item/pattern without learning
a research-tree skill; it is not a per-fabricator recipe-list patch.

## Item flags are not initial availability

| Property | Established role in this path |
| --- | --- |
| `VoyageItem.IsScannable` | Required by the native scan-registration path |
| `VoyageItem.bUnlockAsBuildable` | Required by the conditional first-scan vault grant |
| `VoyageItem.ScanLogics` | Additional per-item scan actions, separate from the native grant |

Neither boolean alone means "available in every fabricator from game start."
Scanning changes player state; merely shipping an item with those flags does
not perform that state transition. Conversely, scan-based unlocking does not
replace primary-asset registration: the inspected native path still needs a
usable item ID.

The grant branch is first-scan-sensitive. Do not assume that changing
`bUnlockAsBuildable` and repeating `MarkItemScanned` repairs a previously
recorded scan that did not grant the item.

## Additional scan logic: explicit vault grant

`/Game/Blueprints/Equipment/ScanLogic/ScanLogic_AddToVault` derives from
`VoyageScanLogic`. Its `OnRun` obtains the player controller's pawn, casts it
to `VoyageCharacter`, then iterates its own configured `Items` array and calls
`VaultInventory.AddItem` for each entry. The supplied item data has count `1`,
timestamp `0` and flags `0`; both trailing boolean arguments are true.

`/Game/Blueprints/Equipment/ToolAbilities/BP_ToolAbility_Scan` has a separate
`BP_ToolAbility_Scan_OnHoldCompleted` path that obtains scan data and iterates
`ScanTargetItem.ScanLogics`, calling `VoyageScanLogic.Run`. This branch is gated
by `ForceAllowScan`; it must not be described as an unconditional step of
every scan or confused with `MarkItemScanned`'s native grant.

## Boundaries for reuse

The stock game exposes both a scan-registration entry point and an explicit
vault-add example. This does **not** yet establish that arbitrary mod calls
will update every existing and newly created fabricator, survive save/load,
or propagate correctly in multiplayer. Those lifecycle and notification
contracts need runtime validation before either path is advertised as a
general mod unlock API. The unidentified native predicate also means the
listed item flags are necessary checks, not a complete sufficient condition.

## Reproducing the inspection

Use the public interfaces in [asset inspection](../tools/asset-inspection.md): fingerprint
with `Get-VoyageBuildFingerprint.ps1`, then inspect the exact stock assets above
with `Get-VoyageAssetSummary.ps1` (`Calls` focus). Use
`Inspect-VoyageAsset.ps1` for pseudocode and mapping queries
`mappings:VoyageItem` and `mappings-enum:EVoyageInventoryType`.

The scan ability's pseudocode emitter rejected `WeakObjectProperty` in this
inspection; its serialized bytecode still parsed. Inspect the specific
function through `Get-VoyageAssetJson.ps1` in that case, rather than treating
missing pseudocode as missing behavior. The equipment and vault-add logic
pseudocode were available.

For native confirmation, `Invoke-VoyageExecutableInspector.ps1` locates
`MarkItemScanned` and the two property registration records. Follow the
resolved exec call with bounded `Inspect-VoyageNativeMemberAccess.py` windows
as documented in [executable inspection](../tools/executable-inspection.md).
Do not reuse raw addresses on another build
or treat string references alone as proof of behavior.
