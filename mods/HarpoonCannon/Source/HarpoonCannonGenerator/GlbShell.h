#pragma once

// Native Interchange owns geometry/material conversion. This adapter owns only
// Voyage shell contracts and stable role tags; source hierarchy stays intact.
namespace HarpoonGlb
{
inline constexpr TCHAR RegistryPath[] = TEXT("../../models/HarpoonCannon/model-source.json");
inline constexpr TCHAR ImportRoot[] = TEXT("/Game/Mods/HarpoonCannon/Visual");
inline constexpr TCHAR InventoryFile[] = TEXT("Saved/HarpoonGlbInventory.json");
inline constexpr TCHAR SourceKey[] = TEXT("source");
inline constexpr TCHAR PathKey[] = TEXT("path");
inline constexpr TCHAR NodesKey[] = TEXT("nodes");
inline constexpr TCHAR RootKey[] = TEXT("root");
inline constexpr TCHAR BaseKey[] = TEXT("base");
inline constexpr TCHAR YawKey[] = TEXT("yaw");
inline constexpr TCHAR PitchKey[] = TEXT("pitch");
inline constexpr TCHAR SightKey[] = TEXT("sight");
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
    const auto Roles = Registry->GetObjectField(NodesKey);
    const FString Source = FPaths::Combine(FPaths::GetPath(RegistryFile), Registry->GetObjectField(SourceKey)->GetStringField(PathKey));
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
    for (const TCHAR* Key : {RootKey, BaseKey, YawKey, PitchKey, SightKey})
        if (!Actors.Contains(Roles->GetStringField(Key))) { UE_LOG(LogTemp, Error, TEXT("Missing GLB role %s"), Key); return 1; }
    for (const auto& Value : Roles->GetArrayField(AmmoKey)) if (!Actors.Contains(Value->AsString())) return 1;
    AActor* ModelRoot = Actors.FindChecked(Roles->GetStringField(RootKey));
    AActor* Base = Actors.FindChecked(Roles->GetStringField(BaseKey));
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
    // Reuse one stationary, origin-aligned render mesh for stock unfinished-Q
    // acquisition. Never derive gameplay collision size from visual bounds.
    AActor* CollisionActor = nullptr;
    for (const FString& Name : Names)
    {
        AActor* Actor = Actors[Name];
        auto* MeshComponent = Cast<UStaticMeshComponent>(Actor->GetRootComponent());
        if (Actor->GetAttachParentActor() == Base && MeshComponent && MeshComponent->GetStaticMesh() && Actor->GetActorTransform().Equals(FTransform::Identity, 0.0001))
        { CollisionActor = Actor; break; }
    }
    if (!CollisionActor) { UE_LOG(LogTemp, Error, TEXT("No stationary origin-aligned mesh for fabricator collision; model contract needs review")); return 1; }
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
            const auto Size = HarpoonModelContract::FabricatorBoxSize;
            FKBoxElem Box(Size.X, Size.Y, Size.Z); Box.Center = HarpoonModelContract::FabricatorBoxCenter;
            Body->AggGeom.BoxElems.Add(Box); Body->CollisionTraceFlag = CTF_UseSimpleAsComplex;
        }
        Body->InvalidatePhysicsData();
    }
    UPackage* Package = CreatePackage(CannonAssetNames::LeafProbePackageName);
    UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(AVoyageModuleActor::StaticClass(), Package,
        FName(CannonAssetNames::LeafProbeAssetName), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), CannonAssetNames::GeneratorLeafProbeName);
    USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
    auto* Mount = AddRootNode(SCS, UBoxComponent::StaticClass(), CannonAssetNames::ModuleMountRootName);
    auto* MountTemplate = CastChecked<UBoxComponent>(Mount->ComponentTemplate);
    MountTemplate->SetBoxExtent(FVector(CannonAssetNames::CollisionHalfWidthCentimeters, CannonAssetNames::CollisionHalfWidthCentimeters, CannonAssetNames::CollisionHalfHeightCentimeters));
    MountTemplate->SetRelativeLocation(CannonAssetNames::CollisionRelativeLocation);
    MountTemplate->SetCollisionProfileName(CannonAssetNames::BlockAllDynamicCollisionProfileName);
    MountTemplate->SetGenerateOverlapEvents(CannonAssetNames::ModuleMountGeneratesInteractionOverlaps);
    MountTemplate->SetSimulatePhysics(false);
    auto* Dynamic = AddRootNode(SCS, UVoyageDynamicCollisionComponent::StaticClass(), CannonAssetNames::DynamicCollisionName);
    CastChecked<UVoyageDynamicCollisionComponent>(Dynamic->ComponentTemplate)->bAutoWeld = true;
    BP->ComponentClassOverrides.Emplace(FBPComponentClassOverride(CannonAssetNames::ModuleComponentName, UVoyageCustomModuleComponent::StaticClass()));
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
                Target->SetCollisionProfileName(Actor == CollisionActor ? CannonAssetNames::BlockAllDynamicCollisionProfileName : CannonAssetNames::NoCollisionProfileName);
                Target->SetGenerateOverlapEvents(false); Target->SetSimulatePhysics(false);
            }
            if (Name == Roles->GetStringField(YawKey)) Component->ComponentTags.Add(HarpoonModelContract::YawTag);
            if (Name == Roles->GetStringField(PitchKey)) Component->ComponentTags.Add(HarpoonModelContract::PitchTag);
            if (Name == Roles->GetStringField(SightKey)) Component->ComponentTags.Add(HarpoonModelContract::SightTag);
            Nodes.Add(Actor, Node);
            auto Entry = MakeShared<FJsonObject>(); Entry->SetStringField(NameKey, Name);
            Entry->SetStringField(ParentKey, Actor == ModelRoot ? CannonAssetNames::ModuleMountRootName.ToString() : Parent->GetActorLabel());
            Entry->SetStringField(MeshKey, MeshPath);
            Entry->SetArrayField(LocationKey, VectorJson(Component->GetRelativeLocation()));
            const auto R = Component->GetRelativeRotation(); Entry->SetArrayField(RotationKey, VectorJson(FVector(R.Pitch, R.Yaw, R.Roll)));
            Entry->SetArrayField(ScaleKey, VectorJson(Component->GetRelativeScale3D()));
            ComponentEvidence.Add(MakeShared<FJsonValueObject>(Entry));
        }
        if (Nodes.Num() == Previous) return 1;
    }
    if (!CompileGeneratedBlueprint(BP)) return 1;
    UVoyageModuleComponent* Module = CastChecked<AVoyageModuleActor>(BP->GeneratedClass->GetDefaultObject())->ModuleComponent;
    if (!Module || !Module->IsA<UVoyageCustomModuleComponent>()) return 1;
    Module->ItemAsset = CreateLeafItemReferenceStub(); if (!Module->ItemAsset) return 1;
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
    Inventory->SetStringField(CollisionKey, CollisionMesh->GetOutermost()->GetName()); Inventory->SetObjectField(RolesKey, Roles);
    FString Json; FJsonSerializer::Serialize(Inventory, TJsonWriterFactory<>::Create(&Json));
    if (!FFileHelper::SaveStringToFile(Json, *FPaths::Combine(FPaths::ProjectDir(), InventoryFile))) return 1;
    UE_LOG(LogTemp, Display, TEXT("GLB shell generated: %d components, %d owned packages"), Nodes.Num(), PackageNames.Num());
    return 0;
}
}
