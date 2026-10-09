#include "AmmoPickup.h"

#include "GlbShell.h"
#include "RailgunModelGeneratorPrivate.h"
#include "VoyageDynamicStaticMeshActor.h"

using namespace Railgun::ModelGenerator;

namespace
{
const FName NativeMeshComponentName(TEXT("MeshComponent"));
const FName NoCollisionProfileName(TEXT("NoCollision"));
constexpr TCHAR RootNameKey[] = TEXT("root");
constexpr TCHAR CarrierNameKey[] = TEXT("carrier");
constexpr TCHAR PackageKey[] = TEXT("package");
constexpr TCHAR ClassObjectPathKey[] = TEXT("classObjectPath");
constexpr TCHAR CarrierMeshKey[] = TEXT("carrierMesh");
constexpr TCHAR CollisionCenterKey[] = TEXT("collisionCenterCm");
constexpr TCHAR CollisionSizeKey[] = TEXT("collisionSizeCm");
constexpr TCHAR PartsKey[] = TEXT("parts");
constexpr TCHAR MaterialsKey[] = TEXT("materials");
constexpr TCHAR CarrierMaterialsKey[] = TEXT("carrierMaterials");
constexpr TCHAR ComponentNameKey[] = TEXT("componentName");
constexpr TCHAR SourceNameKey[] = TEXT("sourceName");

bool IsInSubtree(AActor* Candidate, AActor* Root)
{
    for (AActor* Current = Candidate; Current;
        Current = Current->GetAttachParentActor())
    {
        if (Current == Root) return true;
    }
    return false;
}

bool IsFiniteTransform(const FTransform& Transform)
{
    return !Transform.ContainsNaN() &&
        FMath::IsFinite(Transform.GetScale3D().X) &&
        FMath::IsFinite(Transform.GetScale3D().Y) &&
        FMath::IsFinite(Transform.GetScale3D().Z);
}

TArray<TSharedPtr<FJsonValue>> MaterialJson(UStaticMeshComponent* Component)
{
    TArray<TSharedPtr<FJsonValue>> Values;
    for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
    {
        UMaterialInterface* Material = Component->GetMaterial(Slot);
        Values.Add(MakeShared<FJsonValueString>(
            Material ? Material->GetPathName() : FString()));
    }
    return Values;
}

void AddMeshCorners(FBox& Bounds, const FBox& MeshBounds,
    const FTransform& MeshToCarrier)
{
    for (int32 X = 0; X < 2; ++X)
    for (int32 Y = 0; Y < 2; ++Y)
    for (int32 Z = 0; Z < 2; ++Z)
    {
        const FVector Corner(
            X ? MeshBounds.Max.X : MeshBounds.Min.X,
            Y ? MeshBounds.Max.Y : MeshBounds.Min.Y,
            Z ? MeshBounds.Max.Z : MeshBounds.Min.Z);
        Bounds += MeshToCarrier.TransformPosition(Corner);
    }
}
}

namespace RailgunAmmoPickup
{
bool Generate(const TMap<FString, AActor*>& Actors, const FString& RootName,
    const FString& CarrierName, TSharedPtr<FJsonObject>& OutEvidence)
{
    AActor* Root = Actors.FindRef(RootName);
    AActor* Carrier = Actors.FindRef(CarrierName);
    auto* CarrierSource = Carrier
        ? Cast<UStaticMeshComponent>(Carrier->GetRootComponent()) : nullptr;
    UStaticMesh* CarrierMesh = CarrierSource
        ? CarrierSource->GetStaticMesh() : nullptr;
    if (!Root || !Carrier || !CarrierSource || !CarrierMesh ||
        !IsInSubtree(Carrier, Root))
    {
        UE_LOG(LogTemp, Error,
            TEXT("Ammo pickup roles must identify one render carrier inside the selected subtree"));
        return false;
    }
    if (!IsFiniteTransform(Carrier->GetActorTransform()) ||
        Carrier->GetActorScale3D().GetAbs().GetMin() <= SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Error, TEXT("Ammo pickup carrier transform is not finite"));
        return false;
    }

    for (int32 Slot = 0; Slot < CarrierSource->GetNumMaterials(); ++Slot)
    {
        if (CarrierSource->GetMaterial(Slot) != CarrierMesh->GetMaterial(Slot))
        {
            UE_LOG(LogTemp, Error,
                TEXT("Ammo pickup carrier has a source material override; shared RenderAsset would differ"));
            return false;
        }
    }

    struct FRenderPart
    {
        AActor* Actor = nullptr;
        UStaticMeshComponent* Source = nullptr;
        FTransform ToCarrier;
    };
    TArray<FRenderPart> Parts;
    FBox CompositeBounds(ForceInit);
    const FTransform CarrierWorld = Carrier->GetActorTransform();
    for (const auto& Pair : Actors)
    {
        AActor* Actor = Pair.Value;
        auto* Source = Actor
            ? Cast<UStaticMeshComponent>(Actor->GetRootComponent()) : nullptr;
        if (!Source || !Source->GetStaticMesh() || !IsInSubtree(Actor, Root))
            continue;
        const FTransform ToCarrier = Actor->GetActorTransform().GetRelativeTransform(
            CarrierWorld);
        if (!IsFiniteTransform(ToCarrier))
        {
            UE_LOG(LogTemp, Error,
                TEXT("Ammo pickup part transform is not finite: %s"), *Pair.Key);
            return false;
        }
        AddMeshCorners(CompositeBounds, Source->GetStaticMesh()->GetBoundingBox(),
            ToCarrier);
        Parts.Add(FRenderPart{Actor, Source, ToCarrier});
    }
    if (Parts.Num() == 0 || !CompositeBounds.IsValid)
    {
        UE_LOG(LogTemp, Error, TEXT("Ammo pickup subtree has no render geometry"));
        return false;
    }
    Parts.Sort([](const FRenderPart& Left, const FRenderPart& Right)
    {
        return Left.Actor->GetActorLabel() < Right.Actor->GetActorLabel();
    });
    const FVector CollisionSize = CompositeBounds.GetSize();
    const FVector CollisionCenter = CompositeBounds.GetCenter();
    if (CollisionSize.GetMin() <= 0.0 || CollisionSize.ContainsNaN() ||
        CollisionCenter.ContainsNaN())
    {
        UE_LOG(LogTemp, Error, TEXT("Ammo pickup composite bounds are invalid"));
        return false;
    }

    if (!CarrierMesh->GetBodySetup()) CarrierMesh->CreateBodySetup();
    UBodySetup* CarrierBody = CarrierMesh->GetBodySetup();
    CarrierBody->RemoveSimpleCollision();
    FKBoxElem CollisionBox(CollisionSize.X, CollisionSize.Y, CollisionSize.Z);
    CollisionBox.Center = CollisionCenter;
    CarrierBody->AggGeom.BoxElems.Add(CollisionBox);
    CarrierBody->CollisionTraceFlag = CTF_UseSimpleAsComplex;
    CarrierBody->InvalidatePhysicsData();

    UPackage* ParentPackage = CreatePackage(StockParentPackageName);
    UBlueprint* ParentBlueprint = FKismetEditorUtilities::CreateBlueprint(
        AVoyageDynamicStaticMeshActor::StaticClass(), ParentPackage,
        FName(StockParentAssetName), BPTYPE_Normal, UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass());
    if (!ParentBlueprint || !CompileGeneratedBlueprint(ParentBlueprint) ||
        !ParentBlueprint->GeneratedClass ||
        ParentBlueprint->GeneratedClass->GetName() != StockParentClassName)
    {
        UE_LOG(LogTemp, Error,
            TEXT("Cannot create exact editor-only BP_DynamicMeshActor stand-in"));
        return false;
    }
    AActor* ParentCDO = ParentBlueprint->GeneratedClass->GetDefaultObject<AActor>();
    auto* NativeMesh = ParentCDO
        ? Cast<UVoyageFastSceneComponent>(
            ParentCDO->GetDefaultSubobjectByName(NativeMeshComponentName))
        : nullptr;
    if (!NativeMesh || ParentCDO->GetRootComponent() != NativeMesh ||
        NativeMesh->CreationMethod != EComponentCreationMethod::Native ||
        !SaveGeneratedAsset(ParentPackage, ParentBlueprint))
    {
        UE_LOG(LogTemp, Error,
            TEXT("Editor-only BP_DynamicMeshActor stand-in lost its native carrier"));
        return false;
    }

    UPackage* Package = CreatePackage(PackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentBlueprint->GeneratedClass, Package, FName(AssetName), BPTYPE_Normal,
        UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    if (!Blueprint || !Blueprint->SimpleConstructionScript)
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot create ammo pickup Blueprint"));
        return false;
    }

    TArray<TSharedPtr<FJsonValue>> PartEvidence;
    for (const FRenderPart& Part : Parts)
    {
        if (Part.Actor == Carrier) continue;
        const FName ComponentName(*Part.Actor->GetActorLabel());
        USCS_Node* Node = Blueprint->SimpleConstructionScript->CreateNode(
            UStaticMeshComponent::StaticClass(), ComponentName);
        Blueprint->SimpleConstructionScript->AddNode(Node);
        Node->SetParent(NativeMesh);
        auto* Target = CastChecked<UStaticMeshComponent>(Node->ComponentTemplate);
        Target->SetStaticMesh(Part.Source->GetStaticMesh());
        Target->SetRelativeTransform(Part.ToCarrier);
        Target->SetMobility(EComponentMobility::Movable);
        Target->SetHiddenInGame(Part.Source->bHiddenInGame);
        for (int32 Slot = 0; Slot < Part.Source->GetNumMaterials(); ++Slot)
            Target->SetMaterial(Slot, Part.Source->GetMaterial(Slot));
        Target->SetCollisionProfileName(NoCollisionProfileName);
        Target->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Target->SetGenerateOverlapEvents(false);
        Target->SetSimulatePhysics(false);
        Target->SetCanEverAffectNavigation(false);
        Target->PrimaryComponentTick.bCanEverTick = false;
        Target->PrimaryComponentTick.bStartWithTickEnabled = false;

        auto Evidence = MakeShared<FJsonObject>();
        Evidence->SetStringField(SourceNameKey, Part.Actor->GetActorLabel());
        Evidence->SetStringField(ComponentNameKey, ComponentName.ToString());
        Evidence->SetStringField(RailgunGlb::MeshKey,
            Part.Source->GetStaticMesh()->GetPathName());
        Evidence->SetArrayField(MaterialsKey, MaterialJson(Part.Source));
        Evidence->SetBoolField(RailgunGlb::HiddenInGameKey,
            Part.Source->bHiddenInGame);
        Evidence->SetArrayField(RailgunGlb::LocationKey,
            RailgunGlb::VectorJson(Part.ToCarrier.GetLocation()));
        const FRotator Rotation = Part.ToCarrier.Rotator();
        Evidence->SetArrayField(RailgunGlb::RotationKey,
            RailgunGlb::VectorJson(FVector(
                Rotation.Pitch, Rotation.Yaw, Rotation.Roll)));
        Evidence->SetArrayField(RailgunGlb::ScaleKey,
            RailgunGlb::VectorJson(Part.ToCarrier.GetScale3D()));
        PartEvidence.Add(MakeShared<FJsonValueObject>(Evidence));
    }

    if (!CompileGeneratedBlueprint(Blueprint) ||
        Blueprint->ParentClass != ParentBlueprint->GeneratedClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Ammo pickup Blueprint failed compilation"));
        return false;
    }
    for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
    {
        auto* Template = Cast<UStaticMeshComponent>(Node->ComponentTemplate);
        if (!Template || !Node->bIsParentComponentNative ||
            Node->ParentComponentOrVariableName != NativeMeshComponentName ||
            Template->IsSimulatingPhysics() ||
            Template->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Ammo pickup child lost native-parent, shared-mesh, or physics contract"));
            return false;
        }
    }
    if (!SaveGeneratedAsset(Package, Blueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot save ammo pickup Blueprint"));
        return false;
    }

    OutEvidence = MakeShared<FJsonObject>();
    OutEvidence->SetStringField(RootNameKey, RootName);
    OutEvidence->SetStringField(CarrierNameKey, CarrierName);
    OutEvidence->SetStringField(PackageKey, PackageName);
    OutEvidence->SetStringField(ClassObjectPathKey, ClassObjectPath);
    OutEvidence->SetStringField(CarrierMeshKey, CarrierMesh->GetPathName());
    OutEvidence->SetArrayField(CarrierMaterialsKey,
        MaterialJson(CarrierSource));
    OutEvidence->SetArrayField(CollisionCenterKey,
        RailgunGlb::VectorJson(CollisionCenter));
    OutEvidence->SetArrayField(CollisionSizeKey,
        RailgunGlb::VectorJson(CollisionSize));
    OutEvidence->SetArrayField(PartsKey, PartEvidence);
    UE_LOG(LogTemp, Display,
        TEXT("Ammo pickup generated: %s carrier=%s renderChildren=%d"),
        *Blueprint->GetPathName(), *CarrierMesh->GetPathName(),
        PartEvidence.Num());
    return true;
}
}
