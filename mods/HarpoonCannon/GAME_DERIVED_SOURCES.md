# Game-derived source registry

## Retained source gate: historical, build blocked pending revalidation

Restart on 2026-09-05 found installed Steam build `25056839`, executable hash
`CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933`.
The public resolver returns reviewed mappings successfully. `RESEARCH.md`
records partial current-stock inspection; this does NOT renew the serialized
native definitions below. Keep their old fingerprint explicit until each
affected contract is revalidated, then separately migrate and verify authoring.
An update invalidates evidence; it is not by itself proof of incompatibility.

## Original source fingerprint

The separate `AutoloadProbe/` HC01/HC02 experiments do not consume the retained
native definitions below. Its engine-only generator targets editor UE5.8.2
and current game build 25056839 with the executable hash above. The only game
class reference is the current-stock inspected soft class
`/Game/Blueprints/Vehicles/BP_CameraDrone.BP_CameraDrone_C`; no game Blueprint
snapshot is shipped. Extraction supplies current `scriptobjects.bin` only to
retoc. Renew this reference, script-object input and autoload contract after
fingerprint changes. Build manifests record tool/mapping/source hashes; static
success cannot establish native spawn/persistence or interaction safety.
Subsequent HC01/HC02 user screenshots validate bounded lifetime and natural
stock action hints only; see RESEARCH.md. Entry/exit and cleanup remain pending.

- Steam build: `23962331`
- Unreal Engine mapping version: `5.7.4`
- `VoyageSteam-Win64-Shipping.exe` SHA-256:
  `6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D`
- Fingerprint tool: `../../tools/Get-VoyageBuildFingerprint.ps1`

Any fingerprint change invalidates every entry below until it is re-inspected
and the backlog records the new evidence.

## Reconstructed editor-only identities

- `Source/Voyage/VoyageVehiclePawn.h`
  - identity: `/Script/Voyage.VoyageVehiclePawn : /Script/Engine.Pawn`
  - evidence: current mappings and the inspected forklift, JetSki, and
    GyroCopter Blueprint parent chains
- `Source/Voyage/VoyageBaseUserWidget.h`
  - identity: `/Script/Voyage.VoyageBaseUserWidget : /Script/UMG.UserWidget`
  - evidence: `GetHUDOverrideWidget` return metadata and current widget class
    mappings
- `Source/Voyage/VoyageDynamicCollisionComponent.h`
  - identity: `/Script/Voyage.VoyageDynamicCollisionComponent :
    /Script/Engine.ActorComponent`
  - current mapping exposes exactly `bAutoWeld` followed by
    `bAutoweldIgnoreNormal`; Cyclone's SCS root component sets only
    `bAutoWeld=true`
- `Source/Voyage/VoyageModuleActor.h`
  - identity: `/Script/Voyage.VoyageModuleActor : /Script/Engine.Actor`
  - the current mapping exposes four own properties (`bCollectable`,
    `ModuleComponent`, `PersistentComponent`, and
    `DestructibleObjectComponent`)
  - targeted disassembly of the gated executable confirms that the native
    constructor creates the three named component fields as default subobjects;
    the base turbine generated class then overrides the inherited
    `ModuleComponent` template with `VoyageCustomModuleComponent`
  - the current editor-only mirror reproduces that actor-level field order,
    false `bCollectable` default, named default-subobject construction, and
    Blueprint component-class override
- `Source/Voyage/VoyageModuleComponent.h` and
  `Source/Voyage/VoyageCustomModuleComponent.h`
  - identities:
    `/Script/Voyage.VoyageModuleComponent : /Script/Engine.ActorComponent`
    and `/Script/Voyage.VoyageCustomModuleComponent :
    /Script/Voyage.VoyageModuleComponent`
  - current mappings provide the exact eight-property base prefix through
    `VoyageModuleComponent.ItemAsset` and the exact 20-property derived prefix
    of `VoyageCustomModuleComponent`
  - the mirror reproduces those property counts and order so the generated
    unversioned `ItemAsset` value lands at the runtime index; derived container
    inner types are editor-only alignment placeholders and are never serialized
  - cooked Cyclone CDO inspection resolved the value at runtime identity
    `/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium.0` and confirmed
    that no neighboring derived field is accidentally overridden
- `Source/Voyage/VoyageItem.h`
  - identities:
    `/Script/Voyage.VoyageBaseDataAsset : /Script/Engine.PrimaryDataAsset`,
    `/Script/Voyage.VoyageItemBase : /Script/Voyage.VoyageBaseDataAsset`, and
    `/Script/Voyage.VoyageItem : /Script/Voyage.VoyageItemBase`
  - current mappings and the extracted turbine items establish this inheritance
    chain; the class is used only to author a project-local reference stub at
    the exact item path, and that stub is excluded from cook and packaging
- `Source/Voyage/VoyagePersistentActorComponent.h` and
  `Source/Voyage/VoyageDestructibleObjectComponent.h`
  - class identities and native default-subobject names are confirmed by the
    current mappings, original Cyclone CDO export, and module-actor constructor
    disassembly; their internal runtime schemas remain intentionally absent
- `Source/HarpoonCannonGenerator/CannonAssetNames.h`
  - current shark data identities from current package inventory and inspected
    `PlayerModuleComponent.ItemAsset` values
  - current leaf probe identity:
    `/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Medium_New`,
    confirmed by `DA_Item_Module_WindTurbineMedium.DroppedActor` and the
    inspected Blueprint package; package inventory found no child Blueprint
    depending on this leaf path
  - current leaf item identity:
    `/Game/Data/Assets/Modules/DA_Item_Module_WindTurbineMedium`, confirmed by
    `DA_Skill_Engineering_Cyclone_Wind_Turbine`, the item export, and the
    original medium-turbine component CDO
  - the rejected Whisper experiment used the parent
    `/Game/Blueprints/Modules/Generators/BP_Module_WindTurbine_Small`; replacing
    that package broke its untouched medium-turbine child with `Bad export
    index 4607/19`, so it must not be a packaged identity again
  - future loaded connector identity:
    `/Game/AssetSets/Sockets/SM_Mooring_CableSocket_Out`, freshly confirmed by
    exact package extraction under the current fingerprint; the first restored
    leaf visual checkpoint deliberately keeps this hard reference disabled and
    includes only the mod-authored connector proxy in the pitch mesh
  - the generated `/Game/Mods/HarpoonCannon/SM_HarpoonCannonPlaceholder` cube
    was a historical mod-authored visual marker and contained no game-derived
    data
  - the current `SourceAssets/harpoon_cannon_blockout.obj` and its generated
    base/yaw/pitch meshes are mod-authored visual data, not game-derived data;
    source hashes and coordinate contract are recorded in
    `SourceAssets/README.md`
  - first-person FOV is a mod tuning contract, not extracted game data

These native definitions exist only to compile generated Blueprint assets.
Their binaries must never be included in a mod container; the running game
supplies the real `/Script/Voyage` classes.
