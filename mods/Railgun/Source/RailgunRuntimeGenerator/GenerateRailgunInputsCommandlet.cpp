#include "GenerateRailgunInputsCommandlet.h"
#include "VoyageInputContextAsset.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
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

    auto* Yaw = Asset<UInputAction>(LookYaw);
    auto* Pitch = Asset<UInputAction>(LookPitch);
    auto* Leave = Asset<UInputAction>(Exit);
    auto* Optics = Asset<UInputAction>(Zoom);
    auto* Shot = Asset<UInputAction>(Fire);
    Shot->ValueType = EInputActionValueType::Boolean;
    Optics->ValueType = EInputActionValueType::Boolean;
    Yaw->ValueType = EInputActionValueType::Axis1D;
    Pitch->ValueType = EInputActionValueType::Axis1D;
    Leave->ValueType = EInputActionValueType::Boolean;
    for (auto* Action : {Yaw, Pitch, Leave, Optics, Shot}) Action->bConsumeInput = true;

    auto* Mapping = Asset<UInputMappingContext>(Keyboard);
    Mapping->MapKey(Yaw, EKeys::MouseX);
    Mapping->MapKey(Pitch, EKeys::MouseY);
    Mapping->MapKey(Leave, EKeys::E);
    Mapping->MapKey(Optics, EKeys::RightMouseButton);
    Mapping->MapKey(Shot, EKeys::LeftMouseButton);
    check(Mapping->GetMappings().Num() == KeyboardMappingCount);

    auto* InputContext = Asset<UVoyageInputContextAsset>(Context);
    InputContext->InputMappingContext.Add(EInputDeviceType::MouseKeyboard, Mapping);
    InputContext->ContextType = EInputControlContextType::Default;
    InputContext->BindType = EInputControlBindType::Default;
    InputContext->InputPriorityOffset = ContextPriority;
    // No inherited Forklift bindings, modifier actions or driving inputs.
    // This gate authors assets only: it does not activate them or imply hints.
    const TArray<UObject*> Objects{Yaw, Pitch, Leave, Optics, Shot, Mapping, InputContext};
    for (UObject* Object : Objects)
        if (!SaveAsset(Object)) return 1;
    return 0;
}
