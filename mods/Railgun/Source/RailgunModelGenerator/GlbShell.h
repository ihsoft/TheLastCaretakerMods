#pragma once

// Native Interchange owns geometry/material conversion. This adapter owns only
// Voyage shell contracts and stable role tags; source hierarchy stays intact.
namespace RailgunGlb
{
inline constexpr TCHAR RegistryPath[] = TEXT("Assets/Model/model-source.json");
inline constexpr TCHAR ModelPath[] = TEXT("Assets/Model/Railgun.glb");
inline constexpr TCHAR ImportRoot[] = TEXT("/Game/Mods/Railgun/Visual");
inline constexpr TCHAR InventoryFile[] = TEXT("Saved/RailgunGlbInventory.json");
inline constexpr TCHAR SchemaVersionKey[] = TEXT("schemaVersion");
inline constexpr TCHAR NodesKey[] = TEXT("nodes");
inline constexpr TCHAR RootKey[] = TEXT("root");
inline constexpr TCHAR BaseKey[] = TEXT("base");
inline constexpr TCHAR YawKey[] = TEXT("yaw");
inline constexpr TCHAR PitchKey[] = TEXT("pitch");
inline constexpr TCHAR SightKey[] = TEXT("sight");
inline constexpr TCHAR MuzzleKey[] = TEXT("muzzle");
inline constexpr TCHAR PowerSocketAnchorKey[] = TEXT("powerSocketAnchor");
inline constexpr TCHAR FabricatorKey[] = TEXT("fabricatorCollision");
inline constexpr TCHAR EntryKey[] = TEXT("entryInteraction");
inline constexpr TCHAR CollisionNodeKey[] = TEXT("node");
inline constexpr TCHAR SizeKey[] = TEXT("sizeCm");
inline constexpr TCHAR CenterKey[] = TEXT("centerCm");
inline constexpr TCHAR AmmoKey[] = TEXT("ammoInstances");
inline constexpr TCHAR NameKey[] = TEXT("name");
inline constexpr TCHAR ParentKey[] = TEXT("parent");
inline constexpr TCHAR MeshKey[] = TEXT("mesh");
inline constexpr TCHAR LocationKey[] = TEXT("location");
inline constexpr TCHAR RotationKey[] = TEXT("rotation");
inline constexpr TCHAR ScaleKey[] = TEXT("scale");
inline constexpr TCHAR ComponentsKey[] = TEXT("components");
inline constexpr TCHAR PackagesKey[] = TEXT("packages");
inline constexpr TCHAR CollisionKey[] = TEXT("collisionMesh");
inline constexpr TCHAR RolesKey[] = TEXT("roles");

inline TArray<TSharedPtr<FJsonValue>> VectorJson(const FVector& V)
{
    return {MakeShared<FJsonValueNumber>(V.X), MakeShared<FJsonValueNumber>(V.Y), MakeShared<FJsonValueNumber>(V.Z)};
}

inline int32 Generate()
{
    const FString RegistryFile = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), RegistryPath));
    FString Text;
    TSharedPtr<FJsonObject> Registry;
    if (!FFileHelper::LoadFileToString(Text, *RegistryFile) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Registry)) return 1;
    if (Registry->GetIntegerField(SchemaVersionKey) != 1) return 1;
    const auto Roles = Registry->GetObjectField(NodesKey);
    const FString Source = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), ModelPath));
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) { UE_LOG(LogTemp, Error, TEXT("GLB import requires editor world")); return 1; }
    TArray<UObject*> Imported;
    FImportAssetParameters Parameters;
    Parameters.bIsAutomated = true;
    Parameters.bReplaceExisting = false;
    Parameters.ImportLevel = World->PersistentLevel;
    Parameters.OnAssetDoneNative.BindLambda([&Imported](UObject* Asset) { Imported.Add(Asset); });
    auto& Manager = UInterchangeManager::GetInterchangeManager();
    if (!Manager.ImportScene(ImportRoot, UInterchangeManager::CreateSourceData(Source), Parameters)) return 1;
    TMap<FString, AActor*> Actors;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        AActor* Ancestor = Actor;
        while (Ancestor && Ancestor->GetActorLabel() != Roles->GetStringField(RootKey)) Ancestor = Ancestor->GetAttachParentActor();
        if (!Ancestor) continue;
        if (Actors.Contains(Actor->GetActorLabel())) { UE_LOG(LogTemp, Error, TEXT("Duplicate GLB node name")); return 1; }
        Actors.Add(Actor->GetActorLabel(), Actor);
    }
    for (const TCHAR* Key : {RootKey, BaseKey, YawKey, PitchKey, SightKey, MuzzleKey})
        if (!Actors.Contains(Roles->GetStringField(Key))) { UE_LOG(LogTemp, Error, TEXT("Missing GLB role %s"), Key); return 1; }
    const TArray<TSharedPtr<FJsonValue>>* AmmoInstances = nullptr;
    if (Roles->TryGetArrayField(AmmoKey, AmmoInstances))
        for (const auto& Value : *AmmoInstances) if (!Actors.Contains(Value->AsString())) return 1;
    AActor* ModelRoot = Actors.FindChecked(Roles->GetStringField(RootKey));
    AActor* Base = Actors.FindChecked(Roles->GetStringField(BaseKey));
    FString PowerAnchorName;
    if (!Roles->TryGetStringField(PowerSocketAnchorKey, PowerAnchorName) || !Actors.Contains(PowerAnchorName))
    {
        UE_LOG(LogTemp, Error, TEXT("Set nodes.powerSocketAnchor to an existing empty GLB node under BASE"));
        return 1;
    }
    AActor* PowerAnchor = Actors.FindChecked(PowerAnchorName);
    AActor* PowerAncestor = PowerAnchor;
    while (PowerAncestor && PowerAncestor != Base)
    {
        if (PowerAncestor == Actors.FindChecked(Roles->GetStringField(YawKey)) ||
            PowerAncestor == Actors.FindChecked(Roles->GetStringField(PitchKey))) break;
        PowerAncestor = PowerAncestor->GetAttachParentActor();
    }
    if (PowerAncestor != Base || PowerAnchor == Base ||
        Cast<UStaticMeshComponent>(PowerAnchor->GetRootComponent()) ||
        !PowerAnchor->GetActorScale3D().Equals(FVector::OneVector, 0.0001))
    {
        UE_LOG(LogTemp, Error, TEXT("Power socket anchor must be an empty stationary descendant of BASE with unit world scale: %s"), *PowerAnchorName);
        return 1;
    }
    // Existing controls write neutral-relative yaw/pitch; reject changed axis
    // contracts rather than silently applying rotations in the wrong frame.
    for (const TCHAR* Key : {RootKey, BaseKey, YawKey, PitchKey})
    {
        const auto T = Actors.FindChecked(Roles->GetStringField(Key))->GetRootComponent()->GetRelativeTransform();
        if (!T.GetRotation().Equals(FQuat::Identity, 0.0001) || !T.GetScale3D().Equals(FVector::OneVector, 0.0001))
        { UE_LOG(LogTemp, Error, TEXT("GLB role axis/scale contract changed: %s"), Key); return 1; }
    }
    if (!ModelRoot->GetActorLocation().IsNearlyZero() || !Base->GetActorLocation().IsNearlyZero()) return 1;
    TArray<FString> Names; Actors.GetKeys(Names); Names.Sort();
    // Explicit carrier; nested stationary nodes are allowed, moving ancestry is not.
    const auto CollisionConfig = Registry->GetObjectField(FabricatorKey);
    const FString CarrierName = CollisionConfig->GetStringField(CollisionNodeKey);
    AActor* CollisionActor = Actors.FindRef(CarrierName);
    auto* Carrier = CollisionActor ? Cast<UStaticMeshComponent>(CollisionActor->GetRootComponent()) : nullptr;
    if (!Carrier || !Carrier->GetStaticMesh()) { UE_LOG(LogTemp, Error, TEXT("Collision carrier must be a mesh: %s"), *CarrierName); return 1; }
    AActor* Ancestor = CollisionActor;
    while (Ancestor && Ancestor != Base)
    {
        if (Ancestor == Actors.FindChecked(Roles->GetStringField(YawKey)) || Ancestor == Actors.FindChecked(Roles->GetStringField(PitchKey))) return 1;
        Ancestor = Ancestor->GetAttachParentActor();
    }
    if (Ancestor != Base || !CollisionActor->GetActorTransform().Equals(FTransform::Identity, 0.0001))
    { UE_LOG(LogTemp, Error, TEXT("Collision carrier requires stationary base ancestry and world identity: %s"), *CarrierName); return 1; }
    auto ReadVector = [&](const TSharedPtr<FJsonObject>& Config, const TCHAR* Key, FVector& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Config->TryGetArrayField(Key, Values) || Values->Num() != 3) return false;
        double XYZ[3];
        for (int32 I = 0; I < 3; ++I) if (!(*Values)[I]->TryGetNumber(XYZ[I]) || !FMath::IsFinite(XYZ[I])) return false;
        Out = FVector(XYZ[0], XYZ[1], XYZ[2]); return true;
    };
    FVector BoxSize, BoxCenter;
    if (!ReadVector(CollisionConfig, SizeKey, BoxSize) || !ReadVector(CollisionConfig, CenterKey, BoxCenter) || BoxSize.GetMin() <= 0)
    { UE_LOG(LogTemp, Error, TEXT("Collision box requires finite center and positive size in cm")); return 1; }
    const auto EntryConfig = Registry->GetObjectField(EntryKey);
    AActor* EntryParent = Actors.FindRef(EntryConfig->GetStringField(CollisionNodeKey));
    FVector EntrySize, EntryCenter;
    if (!EntryParent || !ReadVector(EntryConfig, SizeKey, EntrySize) ||
        !ReadVector(EntryConfig, CenterKey, EntryCenter) || EntrySize.GetMin() <= 0)
    { UE_LOG(LogTemp, Error, TEXT("Entry interaction requires a model node, finite center and positive full size in Unreal cm")); return 1; }
    AActor* EntryAncestor = EntryParent;
    while (EntryAncestor && EntryAncestor != Base)
    {
        if (EntryAncestor == Actors.FindChecked(Roles->GetStringField(YawKey)) || EntryAncestor == Actors.FindChecked(Roles->GetStringField(PitchKey))) break;
        EntryAncestor = EntryAncestor->GetAttachParentActor();
    }
    if (EntryAncestor != Base || !EntryParent->GetActorScale3D().Equals(FVector::OneVector, 0.0001))
    { UE_LOG(LogTemp, Error, TEXT("Entry interaction must follow a stationary unit-scale node under BASE")); return 1; }
    UStaticMesh* CollisionMesh = CastChecked<UStaticMeshComponent>(CollisionActor->GetRootComponent())->GetStaticMesh();
    int32 CollisionUses = 0;
    for (const auto& Pair : Actors) if (auto* C = Cast<UStaticMeshComponent>(Pair.Value->GetRootComponent())) if (C->GetStaticMesh() == CollisionMesh) ++CollisionUses;
    if (CollisionUses != 1) return 1;
    TSet<UStaticMesh*> Meshes;
    for (const auto& Pair : Actors) if (auto* C = Cast<UStaticMeshComponent>(Pair.Value->GetRootComponent())) if (C->GetStaticMesh()) Meshes.Add(C->GetStaticMesh());
    for (UStaticMesh* Mesh : Meshes)
    {
        if (!Mesh->GetBodySetup()) Mesh->CreateBodySetup();
        auto* Body = Mesh->GetBodySetup(); Body->RemoveSimpleCollision();
        if (Mesh == CollisionMesh)
        {
            const auto Size = BoxSize;
            FKBoxElem Box(Size.X, Size.Y, Size.Z); Box.Center = BoxCenter;
            Body->AggGeom.BoxElems.Add(Box); Body->CollisionTraceFlag = CTF_UseSimpleAsComplex;
        }
        Body->InvalidatePhysicsData();
    }
    UPackage* Package = CreatePackage(RailgunAssetNames::LeafProbePackageName);
    UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(AVoyageModuleActor::StaticClass(), Package,
        FName(RailgunAssetNames::LeafProbeAssetName), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), RailgunAssetNames::GeneratorLeafProbeName);
    USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
    auto* Mount = AddRootNode(SCS, UBoxComponent::StaticClass(), RailgunAssetNames::ModuleMountRootName);
    auto* MountTemplate = CastChecked<UBoxComponent>(Mount->ComponentTemplate);
    MountTemplate->SetBoxExtent(FVector(RailgunAssetNames::CollisionHalfWidthCentimeters, RailgunAssetNames::CollisionHalfWidthCentimeters, RailgunAssetNames::CollisionHalfHeightCentimeters));
    MountTemplate->SetRelativeLocation(RailgunAssetNames::CollisionRelativeLocation);
    MountTemplate->SetCollisionProfileName(RailgunAssetNames::BlockAllDynamicCollisionProfileName);
    MountTemplate->SetGenerateOverlapEvents(RailgunAssetNames::ModuleMountGeneratesInteractionOverlaps);
    MountTemplate->SetSimulatePhysics(false);
    auto* Dynamic = AddRootNode(SCS, UVoyageDynamicCollisionComponent::StaticClass(), RailgunAssetNames::DynamicCollisionName);
    CastChecked<UVoyageDynamicCollisionComponent>(Dynamic->ComponentTemplate)->bAutoWeld = true;
    // Attach after importing the hierarchy; no fixed offset or second axis conversion.
    auto* Electric = SCS->CreateNode(USceneComponent::StaticClass(), RailgunAssetNames::ElectricComponentName);
    CastChecked<USceneComponent>(Electric->ComponentTemplate)->SetRelativeTransform(FTransform::Identity);
    auto* ElectricSocketClass = UVoyageModuleSocketViewComponent::StaticClass();
    if (!ElectricSocketClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Voyage electric socket class is unavailable"));
        return 1;
    }
    auto* ElectricSocket = AddChildNode(SCS, Electric, ElectricSocketClass, RailgunAssetNames::ElectricSocketName);
    auto* ElectricSocketTemplate = Cast<UVoyageModuleSocketViewComponent>(ElectricSocket->ComponentTemplate);
    if (!ElectricSocketTemplate) return 1;
    ElectricSocketTemplate->SetRelativeLocation(FVector::ZeroVector);
    ElectricSocketTemplate->SocketID = RailgunAssetNames::ElectricSocketId;
    ElectricSocketTemplate->bAutoInitialize = false;
    ElectricSocketTemplate->Port.DefaultDirection = EModuleSocketType::ST_Input;
    ElectricSocketTemplate->DataAsset = TSoftObjectPtr<UObject>(FSoftObjectPath(RailgunAssetNames::ElectricSocketDataObjectPath));
    // Native base module owns the bounded electric buffer. CustomModule has
    // separate MaxResources/consumption maps that shadow base configuration.
    TMap<AActor*, USCS_Node*> Nodes;
    TArray<TSharedPtr<FJsonValue>> ComponentEvidence;
    while (Nodes.Num() < Actors.Num())
    {
        const int32 Previous = Nodes.Num();
        for (const FString& Name : Names)
        {
            AActor* Actor = Actors[Name]; if (Nodes.Contains(Actor)) continue;
            AActor* Parent = Actor->GetAttachParentActor();
            if (Actor != ModelRoot && !Nodes.Contains(Parent)) continue;
            auto* SourceComponent = Actor->GetRootComponent();
            auto* SourceMesh = Cast<UStaticMeshComponent>(SourceComponent);
            auto* Node = AddChildNode(SCS, Actor == ModelRoot ? Mount : Nodes[Parent],
                SourceMesh ? UStaticMeshComponent::StaticClass() : USceneComponent::StaticClass(), FName(*Name));
            auto* Component = CastChecked<USceneComponent>(Node->ComponentTemplate);
            Component->SetRelativeTransform(SourceComponent->GetRelativeTransform());
            Component->SetMobility(EComponentMobility::Movable);
            FString MeshPath;
            if (SourceMesh)
            {
                auto* Target = CastChecked<UStaticMeshComponent>(Component);
                Target->SetStaticMesh(SourceMesh->GetStaticMesh());
                MeshPath = SourceMesh->GetStaticMesh()->GetPathName();
                for (int32 Slot = 0; Slot < SourceMesh->GetNumMaterials(); ++Slot) Target->SetMaterial(Slot, SourceMesh->GetMaterial(Slot));
                Target->SetCollisionProfileName(Actor == CollisionActor ? RailgunAssetNames::BlockAllDynamicCollisionProfileName : RailgunAssetNames::NoCollisionProfileName);
                Target->SetGenerateOverlapEvents(false); Target->SetSimulatePhysics(false);
            }
            if (Name == Roles->GetStringField(YawKey)) Component->ComponentTags.Add(RailgunModelContract::YawTag);
            if (Name == Roles->GetStringField(PitchKey)) Component->ComponentTags.Add(RailgunModelContract::PitchTag);
            if (Name == Roles->GetStringField(MuzzleKey)) Component->ComponentTags.Add(RailgunModelContract::MuzzleTag);
            if (Name == Roles->GetStringField(SightKey)) Component->ComponentTags.Add(RailgunModelContract::SightTag);
            Nodes.Add(Actor, Node);
            auto Entry = MakeShared<FJsonObject>(); Entry->SetStringField(NameKey, Name);
            Entry->SetStringField(ParentKey, Actor == ModelRoot ? RailgunAssetNames::ModuleMountRootName.ToString() : Parent->GetActorLabel());
            Entry->SetStringField(MeshKey, MeshPath);
            Entry->SetArrayField(LocationKey, VectorJson(Component->GetRelativeLocation()));
            const auto R = Component->GetRelativeRotation(); Entry->SetArrayField(RotationKey, VectorJson(FVector(R.Pitch, R.Yaw, R.Roll)));
            Entry->SetArrayField(ScaleKey, VectorJson(Component->GetRelativeScale3D()));
            ComponentEvidence.Add(MakeShared<FJsonValueObject>(Entry));
        }
        if (Nodes.Num() == Previous) return 1;
    }
    Nodes.FindChecked(PowerAnchor)->AddChildNode(Electric);
    auto* Entry = AddChildNode(SCS, Nodes.FindChecked(EntryParent), UBoxComponent::StaticClass(), RailgunModelContract::EntryComponent);
    auto* EntryTemplate = CastChecked<UBoxComponent>(Entry->ComponentTemplate);
    EntryTemplate->SetRelativeLocation(EntryCenter);
    EntryTemplate->SetBoxExtent(EntrySize * 0.5);
    EntryTemplate->SetCollisionProfileName(RailgunAssetNames::NoCollisionProfileName);
    EntryTemplate->SetGenerateOverlapEvents(false);
    EntryTemplate->ComponentTags.Add(RailgunModelContract::EntryTag);
    if (!CompileGeneratedBlueprint(BP)) return 1;
    UVoyageModuleComponent* Module = CastChecked<AVoyageModuleActor>(BP->GeneratedClass->GetDefaultObject())->ModuleComponent;
    if (!Module || Module->GetClass() != UVoyageModuleComponent::StaticClass()) return 1;
    Module->ItemAsset = CreateLeafItemReferenceStub(); if (!Module->ItemAsset) return 1;
    Module->ConfigData.bAutoStartModule = true;
    Module->ConfigData.ModuleType = EVoyageModuleType::Active;
    Module->ConfigData.bAcceptResourceOffer = true;
    Module->ConfigData.bAcceptResourceOfferOff = true; // an empty receiver must be able to recover power
    Module->ConfigData.bAcceptResourceOfferProduction = true;
    Module->ConfigData.ResourceBandwidthInput = RailgunAssetNames::RailgunEnergyConsumptionOn;
    Module->ConfigData.MaxResourceAmount = RailgunAssetNames::RailgunIdleBufferWh;
    Module->ConfigData.ResourceConsumptionOn = RailgunAssetNames::RailgunEnergyConsumptionOn;
    Module->ConfigData.ResourceConsumptionStandby = RailgunAssetNames::RailgunEnergyConsumptionStandby;
    Module->SocketCustomTarget.ComponentProperty = RailgunAssetNames::ElectricSocketName;
    Module->bUseSocketCustomTarget = true;
    TSet<FString> PackageNames;
    for (UObject* Asset : Imported)
    {
        if (!Asset->IsA<UStaticMesh>() && !Asset->IsA<UMaterialInterface>() && !Asset->IsA<UTexture>()) continue;
        if (!Asset->GetOutermost()->GetName().StartsWith(ImportRoot)) return 1;
        if (!SaveGeneratedAsset(Asset->GetOutermost(), Asset)) return 1;
        PackageNames.Add(Asset->GetOutermost()->GetName());
    }
    if (!SaveGeneratedAsset(Package, BP)) return 1;
    PackageNames.Add(Package->GetName());
    auto Inventory = MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Packages;
    TArray<FString> Sorted = PackageNames.Array(); Sorted.Sort();
    for (const FString& Name : Sorted) Packages.Add(MakeShared<FJsonValueString>(Name));
    Inventory->SetArrayField(PackagesKey, Packages); Inventory->SetArrayField(ComponentsKey, ComponentEvidence);
    Inventory->SetObjectField(FabricatorKey, CollisionConfig);
    Inventory->SetObjectField(EntryKey, EntryConfig);
    Inventory->SetStringField(CollisionKey, CollisionMesh->GetOutermost()->GetName()); Inventory->SetObjectField(RolesKey, Roles);
    FString Json; FJsonSerializer::Serialize(Inventory, TJsonWriterFactory<>::Create(&Json));
    if (!FFileHelper::SaveStringToFile(Json, *FPaths::Combine(FPaths::ProjectDir(), InventoryFile))) return 1;
    UE_LOG(LogTemp, Display, TEXT("GLB shell generated: %d components, %d owned packages"), Nodes.Num(), PackageNames.Num());
    return 0;
}
}
