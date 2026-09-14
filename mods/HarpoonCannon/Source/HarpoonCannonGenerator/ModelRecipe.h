#pragma once
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Authored model data only. No game class, collision, input or HUD contracts.
namespace ModelRecipe
{
inline constexpr TCHAR DescriptorPath[] = TEXT("../../models/HarpoonCannon/runtime-model.json");
inline constexpr TCHAR Schema[] = TEXT("schemaVersion");
inline constexpr TCHAR Sources[] = TEXT("sources");
inline constexpr TCHAR Obj[] = TEXT("obj");
inline constexpr TCHAR Path[] = TEXT("path");
inline constexpr TCHAR Meshes[] = TEXT("meshes");
inline constexpr TCHAR Hierarchy[] = TEXT("hierarchy");
inline constexpr TCHAR Role[] = TEXT("role");
inline constexpr TCHAR Origin[] = TEXT("meshOriginInSourceCm");
inline constexpr TCHAR Location[] = TEXT("locationRelativeToParentCm");
inline constexpr TCHAR ObjectNames[] = TEXT("objectNames");
inline constexpr TCHAR Name[] = TEXT("name");
inline constexpr TCHAR Ammo[] = TEXT("ammo");
inline constexpr TCHAR Prototype[] = TEXT("prototype");
inline constexpr TCHAR Instances[] = TEXT("instances");
inline constexpr TCHAR Base[] = TEXT("base");
inline constexpr TCHAR Yaw[] = TEXT("yaw");
inline constexpr TCHAR Pitch[] = TEXT("pitch");
inline constexpr TCHAR Sight[] = TEXT("sight");
inline constexpr TCHAR ParentRole[] = TEXT("parentRole");

struct FMesh
{
    TArray<FString> Objects;
    FVector MeshOrigin = FVector::ZeroVector;
    FVector ComponentLocation = FVector::ZeroVector;
};
struct FInstance { FName ComponentName; FVector Location; };
struct FRecipe
{
    FString ObjPath;
    TMap<FString, FMesh> Parts;
    FMesh AmmoMesh;
    TArray<FInstance> AmmoInstances;
    FVector SightLocation = FVector::ZeroVector;
};

inline FVector ReadVector(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
{
    const auto& Values = Object->GetArrayField(Field);
    checkf(Values.Num() == 3, TEXT("Model vector %s must have three coordinates"), Field);
    return FVector(Values[0]->AsNumber(), Values[1]->AsNumber(), Values[2]->AsNumber());
}
inline FMesh ReadMesh(const TSharedPtr<FJsonObject>& Object)
{
    FMesh Mesh; Mesh.MeshOrigin = ReadVector(Object, Origin);
    for (const auto& Value : Object->GetArrayField(ObjectNames)) Mesh.Objects.Add(Value->AsString());
    checkf(!Mesh.Objects.IsEmpty(), TEXT("Empty model mesh selection"));
    return Mesh;
}
inline bool Load(FRecipe& Recipe)
{
    const FString File = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), DescriptorPath));
    FString Json;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Json, *File) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Root) || !Root.IsValid())
    { UE_LOG(LogTemp, Error, TEXT("Cannot read model descriptor: %s"), *File); return false; }
    if (Root->GetIntegerField(Schema) != 1) return false;
    const TSharedPtr<FJsonObject>* SightObject = nullptr;
    if (!Root->TryGetObjectField(Sight, SightObject) || (*SightObject)->GetStringField(ParentRole) != Pitch)
    { UE_LOG(LogTemp, Error, TEXT("This optical shell requires a pitch-owned sight in the model descriptor")); return false; }
    Recipe.SightLocation = ReadVector(*SightObject, Location);
    Recipe.ObjPath = FPaths::Combine(FPaths::GetPath(File), Root->GetObjectField(Sources)->GetObjectField(Obj)->GetStringField(Path));
    for (const auto& Value : Root->GetArrayField(Meshes))
    {
        const auto Object = Value->AsObject();
        Recipe.Parts.Add(Object->GetStringField(Role), ReadMesh(Object));
    }
    if (Recipe.Parts.Num() != 3 || !Recipe.Parts.Contains(Base) || !Recipe.Parts.Contains(Yaw) || !Recipe.Parts.Contains(Pitch)) return false;
    for (const auto& Value : Root->GetArrayField(Hierarchy))
    {
        const auto Object = Value->AsObject();
        Recipe.Parts.FindChecked(Object->GetStringField(Role)).ComponentLocation = ReadVector(Object, Location);
    }
    const auto AmmoObject = Root->GetObjectField(Ammo);
    Recipe.AmmoMesh = ReadMesh(AmmoObject->GetObjectField(Prototype));
    for (const auto& Value : AmmoObject->GetArrayField(Instances))
    {
        const auto Object = Value->AsObject();
        Recipe.AmmoInstances.Add({FName(*Object->GetStringField(Name)), ReadVector(Object, Location)});
    }
    return true;
}
}
