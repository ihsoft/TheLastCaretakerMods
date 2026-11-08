# Voyage scanning, research and recipe unlocks

## Evidence and version boundary

This document separates stock scanner inspection from two game-validated runtime
routes: granting an individual recipe through Vault, and making an existing
technology available for normal research. The inspected game uses:

- Executable: `VoyageSteam-Win64-Shipping.exe`.
- Asset parser profile: `UE5_8`.

The runtime routes below are validated on Steam build **25982825**, UE 5.8.
The scanner analysis remains static evidence; its earlier snapshot did not record
an Update label. Revalidate affected contracts when current evidence disagrees
or a game update changes the behavior.
Extracted assets, disassembly and raw reports remain ignored artifacts.

## Choosing a runtime unlock route

| Goal | Route | Component recipes |
| --- | --- | --- |
| Make one existing item recipe known directly | Add the item to the player's Vault | Does not itself unlock the required ingredient recipes in the validated case |
| Make an existing hidden technology researchable | Change its live `VoyageSkill.Unlock` fields, then let the player learn it | Uses the game's research grants, which supplied the missing component recipe in the validated case |

Both routes work. Direct Vault addition suits simple recipes or cases where all
required ingredients are already available. Recipe visibility alone does not
establish that the player can manufacture every prerequisite.

## Existing technologies can be enabled at runtime

An already registered stock `VoyageSkill` can be made researchable during mod
startup without shipping a replacement Skill asset, research-tree asset or
`AssetRegistry.bin`. The established sequence is:

1. Load the existing Skill DataAsset by its exact object path.
2. Modify named members of its live `Unlock` struct by reference.
3. Preserve the other unlock conditions and the Skill's item/component grant data.
4. Let the player learn the technology through the normal research UI.

The concrete validated case is
`/Game/Data/Assets/Skill/Modules/DA_Skill_Engineering_Large_Salt_Water_Tank`:

| Member | Stock value | Runtime value |
| --- | --- | --- |
| `Unlock.UnlockMethod` | `Never` | `Tier` |
| `Unlock.Requirement` | `30` | `1` |
| `Unlock.Cost` | `2` | `2` (preserved) |

With these changes at DML startup, the technology appears in research and can
be learned. Learning it makes the required **Plastic Casing** recipe available,
and the **Salt Water Tank** can then be built. This establishes runtime eligibility
changes before research interaction; it does not mean that changing the fields
automatically learns the technology.

`VoyageSkill` also owns `Items`, `bAddComponentsToVault`,
`bAddPartComponentsToVault`, `UnlockComponentCategoryFilter` and
`UnlockComponentPrimaryAssetFilter`. Preserve this native grant configuration
when enabling a technology. The successful component grant establishes the
normal research route's behavior for this skill; it does not separately establish
every flag's defaults, filtering rules or recursive depth for other skills.

Write only the intended members, rather than replacing `Unlock` with a struct
built from an incomplete editor mirror. Keep the loaded object referenced while
the mod owns the change. If restoring on shutdown, preserve later modifications
by another owner; restoration of conditions is separate from revoking learned
recipes.

This route covers an existing registered skill. It does not establish discovery
of new unregistered skills, refresh of an already-open research UI, multiplayer
behavior, or save/reload and mod-removal persistence. It needs no permanent tick
or polling for the validated startup case.

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

Calling this route from a mod is game-validated for making the existing Salt
Water Tank recipe appear. Use `VoyageBaseInventoryComponent.HasItem` before
`AddItem` to avoid repeated grants. It grants the item recipe, not the associated
research skill. In the validated case, Plastic Casing remained unavailable and
blocked fabrication until the technology was learned through normal research.
Treat ingredient availability as a separate requirement when choosing Vault
addition for simple items; do not describe this route as broken or as a complete
replacement for research grants.

`/Game/Blueprints/Equipment/ToolAbilities/BP_ToolAbility_Scan` has a separate
`BP_ToolAbility_Scan_OnHoldCompleted` path that obtains scan data and iterates
`ScanTargetItem.ScanLogics`, calling `VoyageScanLogic.Run`. This branch is gated
by `ForceAllowScan`; it must not be described as an unconditional step of
every scan or confused with `MarkItemScanned`'s native grant.

## Boundaries for reuse

The stock game exposes both a scan-registration entry point and an explicit
vault-add example. The bounded runtime results above do **not** establish that
arbitrary mod calls
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
