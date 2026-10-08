#include "GlbShell.h"

#include "RailgunModelGeneratorPrivate.h"

using namespace Railgun::ModelGenerator;

namespace RailgunGlb
{
TArray<TSharedPtr<FJsonValue>> VectorJson(const FVector& V)
{
    return {MakeShared<FJsonValueNumber>(V.X), MakeShared<FJsonValueNumber>(V.Y), MakeShared<FJsonValueNumber>(V.Z)};
}

int32 Generate()
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
    if (!Roles->TryGetArrayField(AmmoKey, AmmoInstances) ||
        AmmoInstances->Num() != RailgunModelContract::AmmoCassetteRoots.Num())
    {
        UE_LOG(LogTemp, Error, TEXT("nodes.ammoInstances must bind exactly six ordered cassette roots"));
        return 1;
    }
    TSet<AActor*> AmmoRoots;
    TSet<AActor*> AmmoRenderDescendants;
    const auto IsDescendantOf = [](AActor* Candidate, AActor* Ancestor)
    {
        for (AActor* Parent = Candidate ? Candidate->GetAttachParentActor() : nullptr;
            Parent; Parent = Parent->GetAttachParentActor())
        {
            if (Parent == Ancestor) return true;
        }
        return false;
    };
    for (int32 Index = 0; Index < AmmoInstances->Num(); ++Index)
    {
        FString RootName;
        if (!(*AmmoInstances)[Index]->TryGetString(RootName) ||
            RootName != RailgunModelContract::AmmoCassetteRoots[Index].ToString())
        {
            UE_LOG(LogTemp, Error, TEXT("Ammo cassette order differs from the shared model contract"));
            return 1;
        }
        AActor* AmmoRoot = Actors.FindRef(RootName);
        if (!AmmoRoot || AmmoRoots.Contains(AmmoRoot)) return 1;
        for (AActor* ExistingRoot : AmmoRoots)
        {
            if (IsDescendantOf(AmmoRoot, ExistingRoot) ||
                IsDescendantOf(ExistingRoot, AmmoRoot))
            {
                UE_LOG(LogTemp, Error,
                    TEXT("Ammo cassette roots overlap: %s"), *RootName);
                return 1;
            }
        }
        AmmoRoots.Add(AmmoRoot);
        const FString SlotName = RootName.LeftChop(FCString::Strlen(AmmoCassetteSuffix));
        AActor* Slot = Actors.FindRef(SlotName);
        AActor* Bin = Actors.FindRef(SlotName + AmmoBinSuffix);
        if (!Slot || AmmoRoot->GetAttachParentActor() != Slot || !Bin ||
            Bin->GetAttachParentActor() != Slot ||
            !Cast<UStaticMeshComponent>(Bin->GetRootComponent()))
        {
            UE_LOG(LogTemp, Error, TEXT("Ammo cassette/bin sibling contract failed for %s"), *SlotName);
            return 1;
        }
        if (IsDescendantOf(Slot, AmmoRoot) || IsDescendantOf(Bin, AmmoRoot))
        {
            UE_LOG(LogTemp, Error,
                TEXT("Ammo holder entered hidden cassette subtree: %s"), *SlotName);
            return 1;
        }
        int32 RenderDescendantCount = 0;
        for (const auto& Pair : Actors)
        {
            AActor* Descendant = Pair.Value;
            AActor* Parent = Descendant->GetAttachParentActor();
            while (Parent && Parent != AmmoRoot) Parent = Parent->GetAttachParentActor();
            if (Parent != AmmoRoot ||
                !Cast<UStaticMeshComponent>(Descendant->GetRootComponent())) continue;
            if (AmmoRenderDescendants.Contains(Descendant))
            {
                UE_LOG(LogTemp, Error,
                    TEXT("Ammo render descendant belongs to multiple cassette roots: %s"),
                    *Pair.Key);
                return 1;
            }
            ++RenderDescendantCount;
            AmmoRenderDescendants.Add(Descendant);
        }
        if (RenderDescendantCount == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Ammo cassette has no render subtree: %s"), *RootName);
            return 1;
        }
    }
    AActor* ModelRoot = Actors.FindChecked(Roles->GetStringField(RootKey));
    AActor* Base = Actors.FindChecked(Roles->GetStringField(BaseKey));
    FString ChargeIndicatorName;
    if (!Roles->TryGetStringField(ChargeIndicatorMeshKey, ChargeIndicatorName))
    {
        UE_LOG(LogTemp, Error,
            TEXT("Set nodes.chargeIndicatorMesh to the Railgun charge-indicator render mesh"));
        return 1;
    }
    AActor* ChargeIndicatorActor = Actors.FindRef(ChargeIndicatorName);
    auto* ChargeIndicatorSource = ChargeIndicatorActor
        ? Cast<UStaticMeshComponent>(ChargeIndicatorActor->GetRootComponent())
        : nullptr;
    UStaticMesh* ChargeIndicatorMesh = ChargeIndicatorSource
        ? ChargeIndicatorSource->GetStaticMesh() : nullptr;
    if (!ChargeIndicatorMesh)
    {
        UE_LOG(LogTemp, Error,
            TEXT("nodes.chargeIndicatorMesh must identify a GLB render mesh, not an empty anchor: %s"),
            *ChargeIndicatorName);
        return 1;
    }
    if (ChargeIndicatorMesh->GetNumUVChannels(0) < 1)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Charge-indicator mesh requires UV channel 0: %s"),
            *ChargeIndicatorName);
        return 1;
    }
    const FMeshDescription* ChargeIndicatorDescription =
        ChargeIndicatorMesh->GetMeshDescription(0);
    if (!ChargeIndicatorDescription)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Charge-indicator mesh has no source mesh description: %s"),
            *ChargeIndicatorName);
        return 1;
    }
    FStaticMeshConstAttributes ChargeIndicatorAttributes(
        *ChargeIndicatorDescription);
    const auto ChargeIndicatorUVs =
        ChargeIndicatorAttributes.GetVertexInstanceUVs();
    const auto ChargeIndicatorNormals =
        ChargeIndicatorAttributes.GetVertexInstanceNormals();
    if (!ChargeIndicatorUVs.IsValid() ||
        ChargeIndicatorUVs.GetNumChannels() < 1 ||
        !ChargeIndicatorNormals.IsValid())
    {
        UE_LOG(LogTemp, Error,
            TEXT("Charge-indicator mesh requires UV0 and vertex normals: %s"),
            *ChargeIndicatorName);
        return 1;
    }
    FVector2f MinimumUV(TNumericLimits<float>::Max());
    FVector2f MaximumUV(TNumericLimits<float>::Lowest());
    for (const FVertexInstanceID VertexInstanceId :
        ChargeIndicatorDescription->VertexInstances().GetElementIDs())
    {
        const FVector2f UV = ChargeIndicatorUVs.Get(VertexInstanceId, 0);
        MinimumUV.X = FMath::Min(MinimumUV.X, UV.X);
        MinimumUV.Y = FMath::Min(MinimumUV.Y, UV.Y);
        MaximumUV.X = FMath::Max(MaximumUV.X, UV.X);
        MaximumUV.Y = FMath::Max(MaximumUV.Y, UV.Y);
        if (ChargeIndicatorNormals.Get(VertexInstanceId).IsNearlyZero())
        {
            UE_LOG(LogTemp, Error,
                TEXT("Charge-indicator mesh has an invalid vertex normal: %s"),
                *ChargeIndicatorName);
            return 1;
        }
    }
    if (!MinimumUV.Equals(FVector2f::ZeroVector, 0.001f) ||
        !MaximumUV.Equals(FVector2f::UnitVector, 0.001f))
    {
        UE_LOG(LogTemp, Error,
            TEXT("Charge-indicator UV0 must span the complete 0..1 range: %s"),
            *ChargeIndicatorName);
        return 1;
    }
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
    const auto InventoryInteractionConfig = Registry->GetObjectField(InventoryInteractionKey);
    AActor* InventoryParent = Actors.FindRef(
        InventoryInteractionConfig->GetStringField(CollisionNodeKey));
    FVector InventoryInteractionSize, InventoryInteractionCenter;
    if (!InventoryParent ||
        !ReadVector(InventoryInteractionConfig, SizeKey, InventoryInteractionSize) ||
        !ReadVector(InventoryInteractionConfig, CenterKey, InventoryInteractionCenter) ||
        InventoryInteractionSize.GetMin() <= 0)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Inventory interaction requires a model node, finite center and positive full size in Unreal cm"));
        return 1;
    }
    AActor* InventoryAncestor = InventoryParent;
    while (InventoryAncestor && InventoryAncestor != Base)
    {
        if (InventoryAncestor == Actors.FindChecked(Roles->GetStringField(YawKey)) ||
            InventoryAncestor == Actors.FindChecked(Roles->GetStringField(PitchKey))) break;
        InventoryAncestor = InventoryAncestor->GetAttachParentActor();
    }
    if (InventoryAncestor != Base ||
        !InventoryParent->GetActorScale3D().Equals(FVector::OneVector, 0.0001))
    {
        UE_LOG(LogTemp, Error,
            TEXT("Inventory interaction must follow a stationary unit-scale node under BASE"));
        return 1;
    }
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
            UClass* ComponentClass = SourceMesh
                ? UStaticMeshComponent::StaticClass()
                : USceneComponent::StaticClass();
            auto* Node = Actor == ModelRoot
                ? AddRootNode(SCS, ComponentClass, FName(*Name))
                : AddChildNode(SCS, Nodes.FindChecked(Parent), ComponentClass,
                    FName(*Name));
            auto* Component = CastChecked<USceneComponent>(Node->ComponentTemplate);
            Component->SetRelativeTransform(SourceComponent->GetRelativeTransform());
            Component->SetMobility(EComponentMobility::Movable);
            FString MeshPath;
            if (SourceMesh)
            {
                auto* Target = CastChecked<UStaticMeshComponent>(Component);
                Target->SetStaticMesh(SourceMesh->GetStaticMesh());
                if (AmmoRenderDescendants.Contains(Actor))
                    Target->SetHiddenInGame(true);
                MeshPath = SourceMesh->GetStaticMesh()->GetPathName();
                for (int32 Slot = 0; Slot < SourceMesh->GetNumMaterials(); ++Slot) Target->SetMaterial(Slot, SourceMesh->GetMaterial(Slot));
                if (Actor == ChargeIndicatorActor)
                {
                    Target->SetCastShadow(false);
                }
                Target->SetCollisionProfileName(Actor == CollisionActor ? RailgunAssetNames::BlockAllDynamicCollisionProfileName : RailgunAssetNames::NoCollisionProfileName);
                Target->SetGenerateOverlapEvents(false); Target->SetSimulatePhysics(false);
            }
            if (Name == Roles->GetStringField(YawKey)) Component->ComponentTags.Add(RailgunModelContract::YawTag);
            if (Name == Roles->GetStringField(PitchKey)) Component->ComponentTags.Add(RailgunModelContract::PitchTag);
            if (Name == Roles->GetStringField(MuzzleKey)) Component->ComponentTags.Add(RailgunModelContract::MuzzleTag);
            if (Name == Roles->GetStringField(SightKey)) Component->ComponentTags.Add(RailgunModelContract::SightTag);
            if (Actor == ModelRoot)
                Component->ComponentTags.Add(RailgunModelContract::RootTag);
            if (Actor == ChargeIndicatorActor)
                Component->ComponentTags.Add(RailgunModelContract::ChargeIndicatorTag);
            Nodes.Add(Actor, Node);
            auto Entry = MakeShared<FJsonObject>(); Entry->SetStringField(NameKey, Name);
            Entry->SetStringField(ParentKey,
                Actor == ModelRoot ? FString() : Parent->GetActorLabel());
            Entry->SetStringField(MeshKey, MeshPath);
            Entry->SetBoolField(HiddenInGameKey, Component->bHiddenInGame);
            Entry->SetArrayField(LocationKey, VectorJson(Component->GetRelativeLocation()));
            const auto R = Component->GetRelativeRotation(); Entry->SetArrayField(RotationKey, VectorJson(FVector(R.Pitch, R.Yaw, R.Roll)));
            Entry->SetArrayField(ScaleKey, VectorJson(Component->GetRelativeScale3D()));
            ComponentEvidence.Add(MakeShared<FJsonValueObject>(Entry));
        }
        if (Nodes.Num() == Previous) return 1;
    }
    auto* Dynamic = AddRootNode(SCS,
        UVoyageDynamicCollisionComponent::StaticClass(),
        RailgunAssetNames::DynamicCollisionName);
    CastChecked<UVoyageDynamicCollisionComponent>(
        Dynamic->ComponentTemplate)->bAutoWeld = true;
    Nodes.FindChecked(PowerAnchor)->AddChildNode(Electric);
    auto* Entry = AddChildNode(SCS, Nodes.FindChecked(EntryParent), UBoxComponent::StaticClass(), RailgunModelContract::EntryComponent);
    auto* EntryTemplate = CastChecked<UBoxComponent>(Entry->ComponentTemplate);
    EntryTemplate->SetRelativeLocation(EntryCenter);
    EntryTemplate->SetBoxExtent(EntrySize * 0.5);
    EntryTemplate->SetCollisionProfileName(RailgunAssetNames::NoCollisionProfileName);
    EntryTemplate->SetGenerateOverlapEvents(false);
    EntryTemplate->ComponentTags.Add(RailgunModelContract::EntryTag);
    auto* InventoryReference = AddChildNode(SCS, Nodes.FindChecked(InventoryParent),
        UBoxComponent::StaticClass(), RailgunModelContract::InventoryComponent);
    auto* InventoryReferenceTemplate = CastChecked<UBoxComponent>(
        InventoryReference->ComponentTemplate);
    InventoryReferenceTemplate->SetRelativeLocation(InventoryInteractionCenter);
    InventoryReferenceTemplate->SetBoxExtent(InventoryInteractionSize * 0.5);
    InventoryReferenceTemplate->SetCollisionProfileName(
        RailgunAssetNames::NoCollisionProfileName);
    InventoryReferenceTemplate->SetGenerateOverlapEvents(false);
    InventoryReferenceTemplate->ComponentTags.Add(
        RailgunModelContract::InventoryTag);
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
    Module->ConfigData.MaxResourceAmount = RailgunAssetNames::RailgunIdleBufferKJ;
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
    Inventory->SetObjectField(InventoryInteractionKey, InventoryInteractionConfig);
    Inventory->SetStringField(CollisionKey, CollisionMesh->GetOutermost()->GetName()); Inventory->SetObjectField(RolesKey, Roles);
    FString Json; FJsonSerializer::Serialize(Inventory, TJsonWriterFactory<>::Create(&Json));
    if (!FFileHelper::SaveStringToFile(Json, *FPaths::Combine(FPaths::ProjectDir(), InventoryFile))) return 1;
    UE_LOG(LogTemp, Display, TEXT("GLB shell generated: %d components, %d owned packages"), Nodes.Num(), PackageNames.Num());
    return 0;
}
}
