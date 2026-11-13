#include "InspectRailgunProjectCommandlet.h"

#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "PhysicsEngine/BodySetup.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr TCHAR ModulePackage[] = TEXT("/Game/Mods/Railgun/Module/BP_Module_Railgun");
constexpr TCHAR PickupPackage[] = TEXT("/Game/Mods/Railgun/Fabricator/AmmoCassette/BP_RailgunAmmoCassette");

TArray<TSharedPtr<FJsonValue>> VectorJson(const FVector& Value)
{
    return {MakeShared<FJsonValueNumber>(Value.X), MakeShared<FJsonValueNumber>(Value.Y), MakeShared<FJsonValueNumber>(Value.Z)};
}

TArray<TSharedPtr<FJsonValue>> Materials(UStaticMeshComponent* Component)
{
    TArray<TSharedPtr<FJsonValue>> Result;
    for (int32 Index = 0; Index < Component->GetNumMaterials(); ++Index)
    {
        const UMaterialInterface* Material = Component->GetMaterial(Index);
        Result.Add(MakeShared<FJsonValueString>(
            Material ? Material->GetPathName() : FString()));
    }
    return Result;
}

TSharedPtr<FJsonObject> ComponentState(USceneComponent* Component)
{
    auto Result = MakeShared<FJsonObject>();
    auto* Mesh = Cast<UStaticMeshComponent>(Component);
    Result->SetStringField(TEXT("mesh"), Mesh && Mesh->GetStaticMesh()
        ? Mesh->GetStaticMesh()->GetPathName() : FString());
    Result->SetBoolField(TEXT("hiddenInGame"), Component->bHiddenInGame);
    Result->SetArrayField(TEXT("location"), VectorJson(Component->GetRelativeLocation()));
    const FRotator Rotation = Component->GetRelativeRotation();
    Result->SetArrayField(TEXT("rotation"), VectorJson(
        FVector(Rotation.Pitch, Rotation.Yaw, Rotation.Roll)));
    Result->SetArrayField(TEXT("scale"), VectorJson(Component->GetRelativeScale3D()));
    if (Mesh) Result->SetArrayField(TEXT("materials"), Materials(Mesh));
    return Result;
}

int32 Fail(const TCHAR* Message)
{
    UE_LOG(LogTemp, Error, TEXT("Railgun project inspection: %s"), Message);
    return 1;
}
}

UInspectRailgunProjectCommandlet::UInspectRailgunProjectCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UInspectRailgunProjectCommandlet::Main(const FString& Params)
{
    FString Output;
    if (!FParse::Value(*Params, TEXT("Output="), Output))
        return Fail(TEXT("-Output is required"));
    FString RoleRegistry;
    if (!FParse::Value(*Params, TEXT("ModelRoles="), RoleRegistry))
        return Fail(TEXT("-ModelRoles is required"));
    FString Text;
    TSharedPtr<FJsonObject> Registry;
    if (!FFileHelper::LoadFileToString(Text, *RoleRegistry) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Registry))
        return Fail(TEXT("model role registry is unreadable"));
    const auto Roles = Registry->GetObjectField(TEXT("nodes"));
    auto* Shell = LoadObject<UBlueprint>(nullptr, ModulePackage);
    auto* Pickup = LoadObject<UBlueprint>(nullptr, PickupPackage);
    if (!Shell || !Shell->SimpleConstructionScript || !Pickup ||
        !Pickup->SimpleConstructionScript || !Pickup->GeneratedClass)
        return Fail(TEXT("maintained shell or pickup Blueprint is missing"));

    USimpleConstructionScript* SCS = Shell->SimpleConstructionScript;
    const FString RootName = Roles->GetStringField(TEXT("root"));
    TMap<FString, USCS_Node*> Nodes;
    TArray<TSharedPtr<FJsonValue>> Components;
    for (USCS_Node* Node : SCS->GetAllNodes())
    {
        auto* Scene = Cast<USceneComponent>(Node->ComponentTemplate);
        if (!Scene) continue;
        bool InModel = false;
        for (USCS_Node* Ancestor = Node; Ancestor; Ancestor = SCS->FindParentNode(Ancestor))
            if (Ancestor->GetVariableName().ToString() == RootName) { InModel = true; break; }
        if (!InModel) continue;
        const FString Name = Node->GetVariableName().ToString();
        Nodes.Add(Name, Node);
        auto Entry = ComponentState(Scene);
        Entry->SetStringField(TEXT("name"), Name);
        const USCS_Node* Parent = SCS->FindParentNode(Node);
        Entry->SetStringField(TEXT("parent"), Parent ? Parent->GetVariableName().ToString() : FString());
        Components.Add(MakeShared<FJsonValueObject>(Entry));
    }
    if (!Nodes.Contains(RootName)) return Fail(TEXT("model root is missing"));
    const auto Collision = Registry->GetObjectField(TEXT("fabricatorCollision"));
    USCS_Node* CollisionNode = Nodes.FindRef(Collision->GetStringField(TEXT("node")));
    USCS_Node* CarrierNode = Nodes.FindRef(Roles->GetStringField(TEXT("ammoPickupCarrier")));
    auto* CollisionComponent = CollisionNode ? Cast<UStaticMeshComponent>(CollisionNode->ComponentTemplate) : nullptr;
    auto* CarrierComponent = CarrierNode ? Cast<UStaticMeshComponent>(CarrierNode->ComponentTemplate) : nullptr;
    if (!CollisionComponent || !CollisionComponent->GetStaticMesh() ||
        !CarrierComponent || !CarrierComponent->GetStaticMesh())
        return Fail(TEXT("collision or ammunition carrier role has no mesh"));
    UStaticMesh* CarrierMesh = CarrierComponent->GetStaticMesh();
    UBodySetup* Body = CarrierMesh->GetBodySetup();
    if (!Body || Body->AggGeom.BoxElems.Num() != 1)
        return Fail(TEXT("ammunition carrier must have one collision box"));
    const FKBoxElem& Box = Body->AggGeom.BoxElems[0];
    auto Ammo = MakeShared<FJsonObject>();
    Ammo->SetStringField(TEXT("root"), Roles->GetStringField(TEXT("ammoPickupRoot")));
    Ammo->SetStringField(TEXT("carrier"), Roles->GetStringField(TEXT("ammoPickupCarrier")));
    Ammo->SetStringField(TEXT("package"), PickupPackage);
    Ammo->SetStringField(TEXT("classObjectPath"), Pickup->GeneratedClass->GetPathName());
    Ammo->SetStringField(TEXT("carrierMesh"), CarrierMesh->GetPathName());
    Ammo->SetArrayField(TEXT("carrierMaterials"), Materials(CarrierComponent));
    Ammo->SetArrayField(TEXT("collisionCenterCm"), VectorJson(Box.Center));
    Ammo->SetArrayField(TEXT("collisionSizeCm"), VectorJson(FVector(Box.X, Box.Y, Box.Z)));
    TArray<TSharedPtr<FJsonValue>> Parts;
    for (USCS_Node* Node : Pickup->SimpleConstructionScript->GetAllNodes())
    {
        auto* Mesh = Cast<UStaticMeshComponent>(Node->ComponentTemplate);
        if (!Mesh || !Mesh->GetStaticMesh() || !Node->bIsParentComponentNative ||
            Node->ParentComponentOrVariableName != TEXT("MeshComponent"))
            return Fail(TEXT("pickup child must retain a mesh and native carrier parent"));
        auto Part = ComponentState(Mesh);
        Part->SetStringField(TEXT("sourceName"), Node->GetVariableName().ToString());
        Part->SetStringField(TEXT("componentName"), Node->GetVariableName().ToString());
        Parts.Add(MakeShared<FJsonValueObject>(Part));
    }
    if (Parts.IsEmpty()) return Fail(TEXT("pickup has no render children"));
    Ammo->SetArrayField(TEXT("parts"), Parts);

    // The maintained visual directory owns its release resources. Never infer
    // the cook list from a previous generator run or a Saved inventory.
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files,
        *FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Mods/Railgun/Visual")),
        TEXT("*.uasset"), true, false);
    TArray<FString> PackageNames;
    for (const FString& File : Files)
    {
        FString PackageName;
        if (!FPackageName::TryConvertFilenameToLongPackageName(File, PackageName) ||
            !PackageName.StartsWith(TEXT("/Game/Mods/Railgun/Visual/")))
            return Fail(TEXT("visual package escaped the owned directory"));
        PackageNames.Add(PackageName);
    }
    if (PackageNames.IsEmpty()) return Fail(TEXT("maintained visual assets are absent"));
    PackageNames.Add(ModulePackage);
    PackageNames.Add(PickupPackage);
    PackageNames.Sort();
    TArray<TSharedPtr<FJsonValue>> Packages;
    for (const FString& Name : PackageNames) Packages.Add(MakeShared<FJsonValueString>(Name));
    auto Inventory = MakeShared<FJsonObject>();
    Inventory->SetArrayField(TEXT("packages"), Packages);
    Inventory->SetArrayField(TEXT("components"), Components);
    for (const TCHAR* Key : {TEXT("fabricatorCollision"), TEXT("entryInteraction"), TEXT("inventoryInteraction")})
        Inventory->SetObjectField(Key, Registry->GetObjectField(Key));
    Inventory->SetObjectField(TEXT("roles"), Roles);
    Inventory->SetObjectField(TEXT("ammoPickup"), Ammo);
    Inventory->SetStringField(TEXT("collisionMesh"), CollisionComponent->GetStaticMesh()->GetOutermost()->GetName());
    if (!FJsonSerializer::Serialize(Inventory, TJsonWriterFactory<>::Create(&Text)) ||
        !FFileHelper::SaveStringToFile(Text, *Output))
        return Fail(TEXT("cannot write inventory"));
    UE_LOG(LogTemp, Display, TEXT("Maintained Railgun project inspected: %d packages, %d components"),
        Packages.Num(), Components.Num());
    return 0;
}
