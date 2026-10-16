#include "GenerateRailgunInputsCommandlet.h"
#include "VoyageInputContextAsset.h"
#include "VoyageItemHandEquipment.h"
#include "VoyageEquipmentActor.h"
#include "VoyageInputAction.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "RailgunInputNames.h"

namespace
{
template<class T> T* Asset(const TCHAR* Path)
{
    return NewObject<T>(CreatePackage(Path), *FPackageName::GetLongPackageAssetName(Path), RF_Public | RF_Standalone);
}

bool SaveAsset(UObject* Object)
{
    UPackage* Package = Object->GetOutermost();
    const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Object, *Filename, Args);
}

template<class T> T* Reference(const TCHAR* ObjectPath)
{
    FString PackageName;
    FString ObjectName;
    check(FString(ObjectPath).Split(RailgunInputNames::ObjectPathSeparator,
        &PackageName, &ObjectName,
        ESearchCase::CaseSensitive, ESearchDir::FromEnd));
    if (auto* Existing = FindObject<T>(nullptr, ObjectPath)) return Existing;
    return NewObject<T>(CreatePackage(*PackageName), *ObjectName,
        RF_Public | RF_Standalone);
}
}

UGenerateRailgunInputsCommandlet::UGenerateRailgunInputsCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

int32 UGenerateRailgunInputsCommandlet::Main(const FString& Params)
{
    using namespace RailgunInputNames;
    int32 PropertyCount = 0;
    for (TFieldIterator<FProperty> It(UVoyageInputContextAsset::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
        ++PropertyCount;
    checkf(PropertyCount == ContextPropertyCount, TEXT("Unexpected native input-context schema count: %d"), PropertyCount);

    auto* Character = Asset<UVoyageInputContextAsset>(CharacterContext);
    auto* VehicleExit = Asset<UVoyageInputContextAsset>(ExitContext);
    for (UVoyageInputContextAsset* Context : {Character, VehicleExit})
    {
        Context->ContextType = EInputControlContextType::Default;
        Context->BindType = EInputControlBindType::Default;
        Context->InputPriorityOffset = ContextPriority;
        check(Context->InputMappingContext.IsEmpty());
    }
    auto ReferenceAction = [](const TCHAR* Path, EInputActionValueType Type,
        const TCHAR* Description, bool VoyageAction)
    {
        UInputAction* Action = VoyageAction
            ? static_cast<UInputAction*>(Reference<UVoyageInputAction>(Path))
            : Reference<UInputAction>(Path);
        Action->ValueType = Type;
        Action->ActionDescription = FText::FromString(Description);
        return Action;
    };
    const TArray<UInputAction*> StockActionReferences {
        ReferenceAction(LookYaw, EInputActionValueType::Axis1D,
            LookYawDescription, false),
        ReferenceAction(LookPitch, EInputActionValueType::Axis1D,
            LookPitchDescription, false),
        ReferenceAction(Fire, EInputActionValueType::Boolean,
            FireDescription, false),
        ReferenceAction(Reload, EInputActionValueType::Boolean,
            ReloadDescription, false),
        ReferenceAction(Zoom, EInputActionValueType::Boolean,
            ZoomDescription, true),
        ReferenceAction(Exit, EInputActionValueType::Boolean,
            ExitDescription, true)};
    auto* EquipmentActorBlueprint = FKismetEditorUtilities::CreateBlueprint(
        AVoyageEquipmentActor::StaticClass(), CreatePackage(ServiceEquipmentActor),
        *FPackageName::GetLongPackageAssetName(ServiceEquipmentActor),
        BPTYPE_Normal, UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass());
    check(EquipmentActorBlueprint);
    FKismetEditorUtilities::CompileBlueprint(EquipmentActorBlueprint);
    check(EquipmentActorBlueprint->Status != BS_Error &&
        EquipmentActorBlueprint->GeneratedClass);
    auto* EquipmentActor = CastChecked<AVoyageEquipmentActor>(
        EquipmentActorBlueprint->GeneratedClass->GetDefaultObject());
    EquipmentActor->ADSAction = nullptr;
    FVoyageEquipmentInterpStateConfig AdsConfig;
    EquipmentActor->InterpStates.Add(EVoyageEquipmentInterpStateType::ADS,
        AdsConfig);
    check(EquipmentActor->InterpStates.Contains(
        EVoyageEquipmentInterpStateType::ADS));

    auto* Equipment = Asset<UVoyageItemHandEquipment>(ServiceEquipment);
    Equipment->Type = FPrimaryAssetType(ItemPrimaryAssetType);
    Equipment->EquipmentClass = EquipmentActorBlueprint->GeneratedClass;
    Equipment->DesiredSlot = EVoyageHolsterSlotType::Overlay;
    TArray<UObject*> Objects{Character, VehicleExit,
        EquipmentActorBlueprint, Equipment};
    for (UInputAction* Action : StockActionReferences) Objects.Add(Action);
    for (UObject* Object : Objects)
        if (!SaveAsset(Object)) return 1;
    return 0;
}
