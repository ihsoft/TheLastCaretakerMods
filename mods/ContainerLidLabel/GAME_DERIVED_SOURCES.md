# ContainerLidLabel game-derived contracts

The supported native and Blueprint contracts were established for Steam build
25982825, Update 6 beta (`Voyage.exe` 5.8.2.0), with UE 5.8.2 authoring tools.
The accepted source-build pin is owned by [game-build.json](game-build.json).
The builder fingerprints the game and enforces that pin. `-IgnorePinnedGameVersion`
bypasses only the ID comparison for investigation; it does not establish support
for changed layouts, native APIs or persistence behavior.

## Authored assets and editor dependencies

The four mod packages in [architecture](ARCHITECTURE.md) are authored through
BPT and saved in the maintained [project](Project/Voyage.uproject). Their
synchronized JSON is tracked beside the project. No extracted stock package,
relocated parent, custom native runtime DLL, or patched descendant ships.

| Editor input / runtime reference | Source and reconstruction boundary |
| --- | --- |
| BP_Inventory_PlayerBox, BP_Inventory_PlayerBox_Large, BP_Inventory_PlayerBox_Small under `/Game/Blueprints/Interactives/Containers/` | Authored compilation/test stand-ins from stock summaries and component contracts. Stock Small and Large derive from PlayerBox; Small's editor stand-in preserves that relation. Base/Large stand-ins use Actor rather than reconstructing the complete game hierarchy. |
| `/Game/UI/Game/Inventory/BP_PlayerBox_Text` | Authored UserWidget stand-in exposing stock SetText/SetFont/SetSize and fields needed for editor assertions. The shipped reference resolves to the game's widget. |
| `/Game/UI/VoyageRoboto` | Authored font placeholder for the existing stock font identity; excluded from release. |
| `/Engine/BasicShapes/Plane` | Engine asset referenced by the added icon components. |
| Voyage native types | Minimal reflected declarations from reviewed mappings, stock Blueprint bytecode and native registration, implemented only for editor tests. |

The native contract includes inventory Name, icon indices/palette and zero-argument
name/icon delegates; SetMaxWeightLimit and GetPercentWeightUsed; file reader
LoadFileToArray(FString) returning TArray<FString>; VoyageIconPaletteData.IconPalette;
and VoyagePersistentSubsystem.OnActorRestored with an Actor parameter.
The editor-only OnActorSpawned mirror supports the test that the public event
pair produces one display. Test-prefixed APIs have no shipping counterpart.

The stock Small BeginPlay destroys RightIconPlane after its parent initializes
materials. The base UpdateIconMaterial checks that plane before secondary
updates. Component names PlayerBox_Lid, LeftIconPlane, RightIconPlane and
ResourceMeter are runtime lookup contracts, not names inferred from appearance.

## Lifecycle evidence and revalidation

Native registration identifies OnActorRestored's multicast signature. Inspection
of SpawnRuntimeActorFromClass and SpawnRuntimeActorFromData establishes that
their common creation core emits Restored and their wrappers then emit Spawned
for the returned actor. Persistence restoration also calls that common core
without the Spawned wrapper. The manager consequently consumes only Restored.
This evidence does not establish global exactly-once notification for every
possible game path, or replace real DML bootstrap/save-loading validation.

Game validation covers supported container presentation, lid motion and saved-game
loading; editor tests exercise snapshot settings, exact type filtering and cleanup.
Editor fixtures deliberately simplify the stock hierarchy and cannot prove
shipping timing or rendering. In particular, their initial snapshot uses the
Medium/Small hierarchy; Large is also exercised through events.

Recheck stock identities, function/property owners, component names, palette
shape, parent relationships and native event ordering when a game update affects
these contracts or observed behavior disagrees. Derive new stand-ins from the
current stock definitions, not from old names alone. Layout changes require
in-game visual acceptance. Fresh extraction/readback belongs to temporary tool
output; keep only required maintained editor inputs in this source tree.
