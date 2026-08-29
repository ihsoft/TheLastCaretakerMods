#include "GenerateDonkLiftInheritanceCommandlet.h"

#if WITH_EDITOR

#include "Engine/Blueprint.h"
#include "Components/ChildActorComponent.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "VoyageVehicleForkliftPawn.h"

namespace
{
constexpr TCHAR BasePackageName[] = TEXT("/Game/Mods/DonkLiftKeyboard/BP_Forklift_Original");
constexpr TCHAR ChildPackageName[] = TEXT("/Game/Blueprints/Vehicles/BP_Forklift_Possesable");
constexpr TCHAR ForkliftAssetName[] = TEXT("BP_Forklift_Possesable");
constexpr TCHAR HelperObjectPath[] = TEXT("/Game/Mods/DonkLiftKeyboardControl/ModActor.ModActor");

bool SaveBlueprint(UPackage* Package, UBlueprint* Blueprint)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error)
    {
        UE_LOG(LogTemp, Error, TEXT("Compilation failed for %s"), *Blueprint->GetPathName());
        return false;
    }

    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(),
        FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs);
}
}

UGenerateDonkLiftInheritanceCommandlet::UGenerateDonkLiftInheritanceCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateDonkLiftInheritanceCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(BasePackageName) ||
        FPackageName::DoesPackageExist(ChildPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("Inheritance experiment assets already exist"));
        return 1;
    }

    UBlueprint* Helper = LoadObject<UBlueprint>(nullptr, HelperObjectPath);
    if (!Helper || !Helper->GeneratedClass)
    {
        UE_LOG(LogTemp, Error, TEXT("Helper Blueprint was not found: %s"), HelperObjectPath);
        return 1;
    }

    UPackage* BasePackage = CreatePackage(BasePackageName);
    UBlueprint* BaseBlueprint = FKismetEditorUtilities::CreateBlueprint(
        AVoyageVehicleForkliftPawn::StaticClass(),
        BasePackage,
        FName(ForkliftAssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        FName(TEXT("GenerateDonkLiftInheritanceBase")));
    if (!BaseBlueprint || !SaveBlueprint(BasePackage, BaseBlueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the temporary parent Blueprint"));
        return 1;
    }

    UPackage* ChildPackage = CreatePackage(ChildPackageName);
    UBlueprint* ChildBlueprint = FKismetEditorUtilities::CreateBlueprint(
        BaseBlueprint->GeneratedClass,
        ChildPackage,
        FName(ForkliftAssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        FName(TEXT("GenerateDonkLiftInheritanceChild")));
    if (!ChildBlueprint || !ChildBlueprint->SimpleConstructionScript)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the child Blueprint"));
        return 1;
    }

    USCS_Node* HelperNode = ChildBlueprint->SimpleConstructionScript->CreateNode(
        UChildActorComponent::StaticClass(),
        FName(TEXT("DonkLiftKeyboardHelper")));
    UChildActorComponent* HelperTemplate = Cast<UChildActorComponent>(HelperNode->ComponentTemplate);
    if (!HelperTemplate)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create the helper component template"));
        return 1;
    }
    UClass* HelperClass = Helper->GeneratedClass.Get();
    if (!HelperClass->IsChildOf(AActor::StaticClass()))
    {
        UE_LOG(LogTemp, Error, TEXT("Helper Blueprint is not an Actor class: %s"), *HelperClass->GetPathName());
        return 1;
    }
    HelperTemplate->SetChildActorClass(TSubclassOf<AActor>(HelperClass));
    ChildBlueprint->SimpleConstructionScript->AddNode(HelperNode);

    if (!SaveBlueprint(ChildPackage, ChildBlueprint))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save the child Blueprint"));
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Generated temporary parent: %s.%s"),
        BasePackageName, ForkliftAssetName);
    UE_LOG(LogTemp, Display, TEXT("Generated child override: %s.%s"),
        ChildPackageName, ForkliftAssetName);
    UE_LOG(LogTemp, Display, TEXT("Child helper class: %s"), *Helper->GeneratedClass->GetPathName());
    return 0;
}

#endif
