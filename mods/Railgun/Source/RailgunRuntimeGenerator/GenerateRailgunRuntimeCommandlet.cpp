#include "GenerateRailgunRuntimeCommandlet.h"
#include "RailgunRuntimeNames.h"
#include "RailgunInputNames.h"
#include "DedicatedStationNames.h"
#include "Kismet/BlueprintPathsLibrary.h"
#include "VoyageEditorBlueprintFunctionLibrary.h"
#include "TextSettingsGraphNames.h"
#include "ContextEntryNames.h"
#include "ActorScanGraphNames.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "VoyageProjectileMovementComponent.h"
#include "VoyageCombatSubsystem.h"
#include "Subsystems/SubsystemBlueprintLibrary.h"
#include "TimerGraphNames.h"
#include "Components/Image.h"
#include "Components/RadialSlider.h"
#include "Components/ScaleBox.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundWave.h"
#include "AssetImportTask.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "PluginBlueprintLibrary.h"
#include "Factories/SoundFactory.h"
#include "Factories/TextureFactory.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_RemoveDelegate.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MacroInstance.h"
#include "VoyageDynamicPlayerInputWidget.h"
#include "VoyageModuleComponent.h"
#include "VoyageModuleActor.h"
#include "VoyageBaseDataAsset.h"
#include "VoyageItem.h"
#include "VoyageSkill.h"
#include "VoyageFabricatorComponent.h"
#include "VoyageBaseInventoryComponent.h"
#include "PersistentInterface.h"
#include "VoyageInventoryWeightLimitedComponent.h"
#include "VoyageInventoryItemValidatorInterface.h"
#include "VoyageDynamicMeshActor.h"
#include "InteractiveDetectorPointerComponent.h"
#include "VoyageActorWidgetInterface.h"
#include "VoyageVehiclePawn.h"
#include "InteractiveObjectComponent.h"
#include "VoyageVehicleForkliftPawn.h"
#include "BlueprintGraphNames.h"
#include "ActorLifecycleGraphNames.h"
#include "CharacterObservationGraphNames.h"
#include "CharacterStationGraphNames.h"
#include "OpticalCameraGraphNames.h"
#include "Modules/ModuleManager.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_EnhancedInputAction.h"
#include "Misc/Parse.h"
#include "UObject/PackageFileSummary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "WidgetBlueprint.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/PrimitiveComponent.h"
#include "SlateFontInfoBlueprintLibrary.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "EdGraphSchema_K2.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_ClassDynamicCast.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Self.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/BlueprintMapLibrary.h"
#include "Kismet/BlueprintSetLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, RailgunRuntimeGenerator)
namespace P = BlueprintGraphNames::Pins;
namespace E = ActorLifecycleGraphNames;
namespace N = RailgunRuntimeNames;

namespace
{
constexpr TCHAR WritePluginRegistrySwitch[] = TEXT("WritePluginRegistry");
constexpr TCHAR RegistryMetadataArgument[] = TEXT("RegistryMetadata=");
constexpr TCHAR OutputRegistryArgument[] = TEXT("OutputRegistry=");
constexpr int32 ExpectedRegistryVersion = 24;
constexpr int32 RegistryVersionOffset = 16;
constexpr int32 RegistryFilterOffset = 20;
namespace RegistryJson
{
constexpr TCHAR SchemaVersion[] = TEXT("schemaVersion");
constexpr TCHAR FormatVersion[] = TEXT("formatVersion");
constexpr TCHAR FilterEditorOnly[] = TEXT("filterEditorOnly");
constexpr TCHAR Assets[] = TEXT("assets");
constexpr TCHAR PackageName[] = TEXT("packageName");
constexpr TCHAR PackagePath[] = TEXT("packagePath");
constexpr TCHAR AssetName[] = TEXT("assetName");
constexpr TCHAR ObjectPath[] = TEXT("objectPath");
constexpr TCHAR OptionalOuterPath[] = TEXT("optionalOuterPath");
constexpr TCHAR AssetClassPath[] = TEXT("assetClassPath");
constexpr TCHAR Tags[] = TEXT("tags");
constexpr TCHAR ChunkIds[] = TEXT("chunkIds");
constexpr TCHAR PackageFlags[] = TEXT("packageFlags");
constexpr TCHAR AssetBundles[] = TEXT("assetBundles");
constexpr TCHAR VoyageClassPackage[] = TEXT("/Script/Voyage");
constexpr TCHAR ObjectSeparator[] = TEXT(".");
}

struct FRegistryAssetDefinition
{
    FName PackageName;
    FName PackagePath;
    FName AssetName;
    FName ObjectPath;
    FTopLevelAssetPath AssetClassPath;
    FName OptionalOuterPath;
    FAssetDataTagMap Tags;
    TArray<int32> ChunkIds;
    uint32 PackageFlags = 0;
};

bool ReadRegistryMetadata(const FString& FileName,
    TArray<FRegistryAssetDefinition>& Definitions)
{
    FString Text;
    TSharedPtr<FJsonObject> Root;
    if (!FFileHelper::LoadFileToString(Text, *FileName) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) ||
        !Root.IsValid() || Root->GetIntegerField(RegistryJson::SchemaVersion) != 1 ||
        Root->GetIntegerField(RegistryJson::FormatVersion) != ExpectedRegistryVersion ||
        !Root->GetBoolField(RegistryJson::FilterEditorOnly))
    {
        return false;
    }
    const TArray<TSharedPtr<FJsonValue>>& Assets =
        Root->GetArrayField(RegistryJson::Assets);
    if (Assets.Num() != 3)
    {
        return false;
    }
    TSet<FName> ObjectPaths;
    for (const TSharedPtr<FJsonValue>& Value : Assets)
    {
        const TSharedPtr<FJsonObject> Asset = Value->AsObject();
        if (!Asset.IsValid())
        {
            return false;
        }
        FRegistryAssetDefinition Definition;
        Definition.PackageName = FName(Asset->GetStringField(RegistryJson::PackageName));
        Definition.PackagePath = FName(Asset->GetStringField(RegistryJson::PackagePath));
        Definition.AssetName = FName(Asset->GetStringField(RegistryJson::AssetName));
        Definition.ObjectPath = FName(Asset->GetStringField(RegistryJson::ObjectPath));
        Definition.OptionalOuterPath =
            FName(Asset->GetStringField(RegistryJson::OptionalOuterPath));
        const FString ClassPath = Asset->GetStringField(RegistryJson::AssetClassPath);
        FString ClassPackage;
        FString ClassName;
        if (Definition.PackageName.IsNone() || Definition.PackagePath.IsNone() ||
            Definition.AssetName.IsNone() || Definition.ObjectPath.IsNone() ||
            !Definition.OptionalOuterPath.IsNone() ||
            !ClassPath.Split(RegistryJson::ObjectSeparator, &ClassPackage, &ClassName,
                ESearchCase::CaseSensitive, ESearchDir::FromEnd) ||
            ClassPackage != RegistryJson::VoyageClassPackage || ClassName.IsEmpty() ||
            Definition.ObjectPath != FName(Definition.PackageName.ToString() +
                RegistryJson::ObjectSeparator + Definition.AssetName.ToString()) ||
            Definition.PackagePath != FName(FPackageName::GetLongPackagePath(
                Definition.PackageName.ToString())) || ObjectPaths.Contains(Definition.ObjectPath))
        {
            return false;
        }
        ObjectPaths.Add(Definition.ObjectPath);
        Definition.AssetClassPath = FTopLevelAssetPath(FName(ClassPackage), FName(ClassName));
        const TSharedPtr<FJsonObject> Tags = Asset->GetObjectField(RegistryJson::Tags);
        if (!Tags.IsValid())
        {
            return false;
        }
        for (const TPair<FString, TSharedPtr<FJsonValue>>& Tag : Tags->Values)
        {
            FString TagValue;
            if (!Tag.Value.IsValid() || !Tag.Value->TryGetString(TagValue))
            {
                return false;
            }
            Definition.Tags.Add(FName(Tag.Key), MoveTemp(TagValue));
        }
        const TArray<TSharedPtr<FJsonValue>>& ChunkIds =
            Asset->GetArrayField(RegistryJson::ChunkIds);
        if (ChunkIds.Num() != 1 || ChunkIds[0]->AsNumber() != 0.0)
        {
            return false;
        }
        Definition.ChunkIds.Add(0);
        const double PackageFlags = Asset->GetNumberField(RegistryJson::PackageFlags);
        if (PackageFlags < 0.0 || PackageFlags > MAX_uint32 ||
            PackageFlags != FMath::FloorToDouble(PackageFlags))
        {
            return false;
        }
        Definition.PackageFlags = static_cast<uint32>(PackageFlags);
        if (Asset->GetArrayField(RegistryJson::AssetBundles).Num() != 0 ||
            Definition.Tags.FindRef(FPrimaryAssetId::PrimaryAssetNameTag) !=
                Definition.AssetName.ToString() ||
            Definition.Tags.FindRef(FPrimaryAssetId::PrimaryAssetTypeTag).IsEmpty())
        {
            return false;
        }
        Definitions.Add(MoveTemp(Definition));
    }
    return Definitions.Num() == 3;
}

bool MatchesRegistryDefinition(const FAssetData& Asset,
    const FRegistryAssetDefinition& Definition)
{
    const FAssetData::FChunkArrayView ActualChunkIds = Asset.GetChunkIDs();
    if (Asset.PackageName != Definition.PackageName ||
        Asset.PackagePath != Definition.PackagePath ||
        Asset.AssetName != Definition.AssetName ||
        Asset.GetObjectPathString() != Definition.ObjectPath.ToString() ||
        Asset.AssetClassPath != Definition.AssetClassPath ||
        Asset.GetOptionalOuterPathName() != Definition.OptionalOuterPath ||
        Asset.PackageFlags != Definition.PackageFlags ||
        ActualChunkIds.Num() != Definition.ChunkIds.Num() ||
        Asset.TaggedAssetBundles.IsValid())
    {
        return false;
    }
    for (int32 Index = 0; Index < ActualChunkIds.Num(); ++Index)
    {
        if (ActualChunkIds[Index] != Definition.ChunkIds[Index])
        {
            return false;
        }
    }
    const FAssetDataTagMap ActualTags = Asset.TagsAndValues.CopyMap();
    if (ActualTags.Num() != Definition.Tags.Num())
    {
        return false;
    }
    for (const TPair<FName, FString>& Expected : Definition.Tags)
    {
        if (ActualTags.FindRef(Expected.Key) != Expected.Value)
        {
            return false;
        }
    }
    return true;
}

class FGraph
{
public:
    UEdGraph* Graph;
    UClass* HudClass;
    UEdGraphPin* Tail = nullptr;
    int32 X = 0;
    FGraph(UEdGraph* InGraph, UClass* InHudClass) : Graph(InGraph), HudClass(InHudClass) {}
    template<class T> T* Node(T* In)
    {
        In->CreateNewGuid(); In->PostPlacedNewNode(); In->AllocateDefaultPins();
        In->NodePosX = X; X += 180; Graph->AddNode(In, true, false); return In;
    }
    UEdGraphPin* Pin(UEdGraphNode* In, FName Name)
    {
        auto* Out = In->FindPin(Name);
        checkf(Out, TEXT("Missing pin %s on %s"), *Name.ToString(), *In->GetName());
        return Out;
    }
    void Link(UEdGraphPin* From, UEdGraphPin* To)
    {
        checkf(GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(From, To),
            TEXT("Cannot link %s -> %s"), *From->PinName.ToString(), *To->PinName.ToString());
    }
    void Default(UEdGraphNode* In, FName Name, const TCHAR* Value)
    {
        GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Pin(In, Name), Value);
    }
    UK2Node_CallFunction* Call(UClass* Owner, FName Function)
    {
        auto* Fn = Owner->FindFunctionByName(Function);
        checkf(Fn, TEXT("Missing reflected function %s.%s"), *Owner->GetPathName(), *Function.ToString());
        checkf(Fn->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure),
            TEXT("Function is not Blueprint-callable: %s"), *Function.ToString());
        auto* Out = NewObject<UK2Node_CallFunction>(Graph); Out->SetFromFunction(Fn);
        return Node(Out);
    }
    UK2Node_CallArrayFunction* ArrayCall(FName Function)
    {
        auto* Fn = UKismetArrayLibrary::StaticClass()->FindFunctionByName(Function);
        checkf(Fn, TEXT("Missing reflected array function %s"), *Function.ToString());
        auto* Out = NewObject<UK2Node_CallArrayFunction>(Graph); Out->SetFromFunction(Fn);
        return Node(Out);
    }
    void Exec(UEdGraphNode* In)
    {
        Link(Tail, Pin(In, P::Execute)); Tail = Pin(In, P::Then);
    }
    UEdGraphPin* Read(FName Name)
    {
        auto* Get = NewObject<UK2Node_VariableGet>(Graph);
        Get->VariableReference.SetSelfMember(Name); Node(Get); return Pin(Get, Name);
    }
    void Write(FName Name, UEdGraphPin* Value, const TCHAR* Literal = nullptr)
    {
        auto* Set = NewObject<UK2Node_VariableSet>(Graph);
        Set->VariableReference.SetSelfMember(Name); Node(Set);
        if (Value) Link(Value, Pin(Set, Name)); else Default(Set, Name, Literal);
        Exec(Set);
    }
    UK2Node_IfThenElse* Branch(UEdGraphPin* Condition)
    {
        auto* Out = Node(NewObject<UK2Node_IfThenElse>(Graph));
        Link(Tail, Pin(Out, P::Execute)); Link(Condition, Pin(Out, P::Condition));
        Tail = Pin(Out, P::Then); return Out;
    }
    void Require(UEdGraphPin* Condition, const TCHAR* FailureText)
    {
        auto* Test = Branch(Condition);
        auto* Success = Tail; Tail = Pin(Test, P::Else);
        Text(N::FreezeStatus, FailureText); Tail = Success;
    }
    UEdGraphPin* ActorArray(UEdGraphPin* FirstActor, UEdGraphPin* SecondActor)
    {
        auto* Array = Node(NewObject<UK2Node_MakeArray>(Graph)); Array->AddInputPin();
        Link(FirstActor, Pin(Array, Array->GetPinName(0)));
        Link(SecondActor, Pin(Array, Array->GetPinName(1)));
        return Array->GetOutputPin();
    }
    UEdGraphPin* Valid(UEdGraphPin* Object)
    {
        auto* Fn = Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid));
        Link(Object, Pin(Fn, P::Object)); return Pin(Fn, P::ReturnValue);
    }
    UEdGraphPin* Compare(FName Function, UEdGraphPin* Value, const TCHAR* Other)
    {
        auto* Fn = Call(UKismetMathLibrary::StaticClass(), Function);
        Link(Value, Pin(Fn, P::Binary::LeftOperand)); Default(Fn, P::Binary::RightOperand, Other);
        return Pin(Fn, P::ReturnValue);
    }
    UEdGraphPin* Binary(FName Function, UEdGraphPin* Left, UEdGraphPin* Right)
    {
        auto* Fn = Call(UKismetMathLibrary::StaticClass(), Function);
        Link(Left, Pin(Fn, P::Binary::LeftOperand)); Link(Right, Pin(Fn, P::Binary::RightOperand));
        return Pin(Fn, P::ReturnValue);
    }
    void Text(FName Component, const TCHAR* Value, UEdGraphPin* DynamicText = nullptr)
    {
        if (!HudClass) return; // Non-UI station guards have no observer widget.
        if (Component == N::Marker) return; // No world-space debug geometry in HC06.
        auto* Widget = NewObject<UK2Node_VariableGet>(Graph);
        Widget->VariableReference.SetExternalMember(Component, HudClass); Node(Widget);
        Link(Read(N::HudInstance), Pin(Widget, P::FunctionTarget));
        auto* Set = Call(UTextBlock::StaticClass(), GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText));
        Link(Pin(Widget, Component), Pin(Set, P::FunctionTarget));
        if (DynamicText) Link(DynamicText, Pin(Set, E::WidgetText));
        else
        {
            auto* Literal = Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
            Default(Literal, E::StringValue, Value);
            Link(Pin(Literal, P::ReturnValue), Pin(Set, E::WidgetText));
        }
        Exec(Set);
    }
    void Number(FName Component, const TCHAR* Prefix, UEdGraphPin* Value)
    {
        auto* String = Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, BuildString_Double));
        Default(String, E::StringPrefix, Prefix); Link(Value, Pin(String, E::DoubleValue));
        auto* TextValue = Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
        Link(Pin(String, P::ReturnValue), Pin(TextValue, E::StringValue));
        Text(Component, nullptr, Pin(TextValue, P::ReturnValue));
    }
    void BooleanText(FName Component, UEdGraphPin* Condition, const TCHAR* WhenTrue, const TCHAR* WhenFalse)
    {
        auto* Select = Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectString));
        Link(Condition, Pin(Select, P::Select::Condition));
        Default(Select, P::Select::WhenTrue, WhenTrue); Default(Select, P::Select::WhenFalse, WhenFalse);
        auto* Value = Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
        Link(Pin(Select, P::ReturnValue), Pin(Value, E::StringValue));
        Text(Component, nullptr, Pin(Value, P::ReturnValue));
    }
    UEdGraphPin* Transform(UEdGraphPin* Location, UEdGraphPin* Rotation)
    {
        auto* Make = Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeTransform));
        Link(Location, Pin(Make, E::Location)); Link(Rotation, Pin(Make, E::Rotation));
        Default(Make, E::Scale, N::UnitScale); return Pin(Make, P::ReturnValue);
    }
    UEdGraphPin* Offset(UEdGraphPin* TransformValue, const TCHAR* Value)
    {
        auto* Fn = Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, TransformLocation));
        Link(TransformValue, Pin(Fn, E::Transform)); Default(Fn, E::LocalPosition, Value);
        return Pin(Fn, P::ReturnValue);
    }
};

void AddVariable(UBlueprint* BP, FName Name, FName Category, UObject* Type = nullptr)
{
    FEdGraphPinType PinType; PinType.PinCategory = Category; PinType.PinSubCategoryObject = Type;
    if (Category == UEdGraphSchema_K2::PC_Real) PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    check(FBlueprintEditorUtils::AddMemberVariable(BP, Name, PinType));
}

}

namespace
{
#include "GraphCallHelpers.h"
#include "StationAttachmentGraph.h"
#include "StationHudInterfaceGraph.h"
#include "NativeVehicleGraphHelpers.h"
#include "StationActionHints.h"
#include "StationEntryGraph.h"
#include "RailgunShot.h"
#include "DedicatedStationGenerator.h"
#include "RailgunAmmo.h"
#include "RailgunInventory.h"
#include "ContextStationCoordinator.h"
}

UGenerateRailgunRuntimeCommandlet::UGenerateRailgunRuntimeCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

int32 UGenerateRailgunRuntimeCommandlet::Main(const FString& Params)
{
    const bool bWritePluginRegistry = FParse::Param(*Params, WritePluginRegistrySwitch);
    if (bWritePluginRegistry)
    {
        FString MetadataFile;
        FString OutputFile;
        if (!FParse::Value(*Params, RegistryMetadataArgument, MetadataFile) ||
            !FParse::Value(*Params, OutputRegistryArgument, OutputFile) ||
            !FPaths::FileExists(MetadataFile) || MetadataFile == OutputFile)
        {
            UE_LOG(LogTemp, Error,
                TEXT("Registry writing requires distinct RegistryMetadata and OutputRegistry paths"));
            return 1;
        }
        TArray<FRegistryAssetDefinition> Definitions;
        if (!ReadRegistryMetadata(MetadataFile, Definitions))
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot read exact Railgun registry metadata"));
            return 1;
        }
        FAssetRegistrySerializationOptions Options(UE::AssetRegistry::ESerializationTarget::ForDevelopment);
        FAssetRegistryState PluginRegistry;
        for (const FRegistryAssetDefinition& Definition : Definitions)
        {
            FAssetData* Record = new FAssetData(Definition.PackageName,
                Definition.PackagePath, Definition.AssetName, Definition.AssetClassPath,
                Definition.Tags, Definition.ChunkIds, Definition.PackageFlags);
            if (!MatchesRegistryDefinition(*Record, Definition) ||
                !Record->GetPrimaryAssetId().IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Generated registry record differs from metadata: %s"),
                    *Definition.ObjectPath.ToString());
                delete Record;
                return 1;
            }
            PluginRegistry.AddAssetData(Record);
        }
        if (PluginRegistry.GetNumAssets() != Definitions.Num() ||
            PluginRegistry.GetNumPackages() != Definitions.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("Generated registry record count is invalid"));
            return 1;
        }
        {
            TUniquePtr<FArchive> Output(IFileManager::Get().CreateFileWriter(*OutputFile));
            if (!Output)
            {
                UE_LOG(LogTemp, Error, TEXT("Cannot create output AssetRegistry.bin"));
                return 1;
            }
            Output->SetFilterEditorOnly(true);
            if (!PluginRegistry.Save(*Output, Options) || Output->IsError())
            {
                UE_LOG(LogTemp, Error, TEXT("Cannot serialize output AssetRegistry.bin"));
                return 1;
            }
        }
        TArray<uint8> OutputBytes;
        int32 OutputVersion = -1;
        int32 OutputFilter = -1;
        if (!FFileHelper::LoadFileToArray(OutputBytes, *OutputFile) ||
            OutputBytes.Num() < RegistryFilterOffset + static_cast<int32>(sizeof(int32)))
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot read plugin registry header"));
            return 1;
        }
        FMemory::Memcpy(&OutputVersion, OutputBytes.GetData() + RegistryVersionOffset, sizeof(int32));
        FMemory::Memcpy(&OutputFilter, OutputBytes.GetData() + RegistryFilterOffset, sizeof(int32));
        if (OutputVersion != ExpectedRegistryVersion || OutputFilter != 1)
        {
            UE_LOG(LogTemp, Error, TEXT("Plugin registry writer profile differs: version=%d filter=%d"),
                OutputVersion, OutputFilter);
            return 1;
        }
        FAssetRegistryState Reopened;
        if (!FAssetRegistryState::LoadFromDisk(
                *OutputFile, FAssetRegistryLoadOptions(), Reopened))
        {
            UE_LOG(LogTemp, Error, TEXT("Output registry cannot be reopened"));
            return 1;
        }
        if (Reopened.GetNumAssets() != Definitions.Num() ||
            Reopened.GetNumPackages() != Definitions.Num())
        {
            UE_LOG(LogTemp, Error,
                TEXT("Plugin registry reopen count failed: assets=%d packages=%d"),
                Reopened.GetNumAssets(), Reopened.GetNumPackages());
            return 1;
        }
        for (const FRegistryAssetDefinition& Definition : Definitions)
        {
            const FAssetData* ReopenedAsset = Reopened.GetAssetByObjectPath(
                FSoftObjectPath(Definition.ObjectPath.ToString()));
            if (!ReopenedAsset || !MatchesRegistryDefinition(*ReopenedAsset, Definition))
            {
                UE_LOG(LogTemp, Error,
                    TEXT("Plugin registry record changed on reopening: %s"),
                    *Definition.ObjectPath.ToString());
                return 1;
            }
        }
        UE_LOG(LogTemp, Display,
            TEXT("PLUGIN REGISTRY VERIFIED donorFree=1 output=%d metadata=%s"),
            Reopened.GetNumAssets(), *MetadataFile);
        return 0;
    }
    if (FParse::Param(*Params, DedicatedStationNames::VerifySwitch))
    {
        TArray<const TCHAR*> VerifyPackages {N::Package, DedicatedStationNames::OperatorPackage, DedicatedStationNames::HudPackage,
            RailgunInputNames::LookYaw, RailgunInputNames::LookPitch, RailgunInputNames::Exit, RailgunInputNames::Zoom, RailgunInputNames::Fire, Shot::Package,
            ShotAudio::Package, ZoomTest::MaskPackage, EnergyHud::ChargingPackage, EnergyHud::OfflinePackage, EnergyHud::ReadyPackage,
            EnergyHud::AmmoIndicatorPackage,
            RailgunInputNames::Keyboard, RailgunInputNames::Context,
            RailgunAmmo::AmmoIconPackage, RailgunAmmo::GunIconPackage,
            RailgunAmmo::SkillIconPackage, RailgunAmmo::FullClonePackage,
            RailgunAmmo::SkillPackage};
        for (const TCHAR* Package : VerifyPackages)
        {
            FString Relative(Package); check(Relative.RemoveFromStart(DedicatedStationNames::GamePrefix));
            FString File = FPaths::Combine(FPaths::ProjectDir(), DedicatedStationNames::CookPrefix, Relative) + FPackageName::GetAssetPackageExtension();
            TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*File)); check(Reader);
            FPackageFileSummary Summary; *Reader << Summary;
            checkf(!Reader->IsError() && !(Summary.GetPackageFlags() & PKG_UnversionedProperties), TEXT("Tagged property gate failed: %s"), *File);
            UE_LOG(LogTemp, Display, TEXT("TAGGED VERIFIED %s flags=%u"), Package, Summary.GetPackageFlags());
        }
        return 0;
    }
    const bool Dedicated = FParse::Param(*Params, DedicatedStationNames::DedicatedSwitch);
    checkf(Dedicated, TEXT("HC24 runtime emission is rejected; use DedicatedStation only"));
    float AmmoWeightKg = 0.0f;
    checkf(FParse::Value(*Params, RailgunAmmo::AmmoWeightSourceArgument,
        AmmoWeightKg) && AmmoWeightKg > 0.0f,
        TEXT("DedicatedStation requires a positive ammo weight from the owned JSON"));
    FString ShotSoundFile;
    checkf(FParse::Value(*Params, ShotAudio::SourceArgument, ShotSoundFile) && FPaths::FileExists(ShotSoundFile),
        TEXT("Missing shot sound source: %s"), *ShotSoundFile);
    ShotAudio::Wave = ImportShotSound(ShotSoundFile);
    auto ImportRequiredTexture = [&](const TCHAR* Argument, const TCHAR* PackageName,
        const TCHAR* AssetName, bool RequireSquare)
    {
        FString SourceFile;
        checkf(FParse::Value(*Params, Argument, SourceFile) && FPaths::FileExists(SourceFile),
            TEXT("Missing UI texture source for %s: %s"), AssetName, *SourceFile);
        return ImportUiTexture(SourceFile, PackageName, AssetName, RequireSquare);
    };
    ZoomTest::OverlayTexture = ImportRequiredTexture(ZoomTest::OverlaySourceArgument,
        ZoomTest::MaskPackage, ZoomTest::MaskAsset, true);
    EnergyHud::ChargingTexture = ImportRequiredTexture(EnergyHud::ChargingSourceArgument,
        EnergyHud::ChargingPackage, EnergyHud::ChargingAsset, false);
    EnergyHud::OfflineTexture = ImportRequiredTexture(EnergyHud::OfflineSourceArgument,
        EnergyHud::OfflinePackage, EnergyHud::OfflineAsset, false);
    EnergyHud::ReadyTexture = ImportRequiredTexture(EnergyHud::ReadySourceArgument,
        EnergyHud::ReadyPackage, EnergyHud::ReadyAsset, false);
    EnergyHud::AmmoIndicatorTexture = ImportRequiredTexture(
        EnergyHud::AmmoIndicatorSourceArgument, EnergyHud::AmmoIndicatorPackage,
        EnergyHud::AmmoIndicatorAsset, true);
    ImportRequiredTexture(RailgunAmmo::AmmoIconSourceArgument,
        RailgunAmmo::AmmoIconPackage, RailgunAmmo::AmmoIconAsset, true);
    ImportRequiredTexture(RailgunAmmo::GunIconSourceArgument,
        RailgunAmmo::GunIconPackage, RailgunAmmo::GunIconAsset, true);
    ImportRequiredTexture(RailgunAmmo::SkillIconSourceArgument,
        RailgunAmmo::SkillIconPackage, RailgunAmmo::SkillIconAsset, true);
    UVoyageItemAmmo* Ammo = CreateRailgunAmmoReference();
    CreateRailgunSkillReference();
    UVoyageItemCategoryAsset* AmmoCategory =
        CreateRailgunReference<UVoyageItemCategoryAsset>(
            RailgunAmmo::AmmoCategoryPackage, RailgunAmmo::AmmoCategoryAsset);
    ConfigureRailgunInventory(Ammo, AmmoCategory, AmmoWeightKg);
    Shot::Class=CreateRailgunShot();
    UClass* StationClass = CreateDedicatedStation();
    UPackage* Package = CreatePackage(N::Package);
    UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(APawn::StaticClass(), Package, FName(N::Asset),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    check(BP);
    auto* Root = BP->SimpleConstructionScript->CreateNode(USceneComponent::StaticClass(), N::Root);
    BP->SimpleConstructionScript->AddNode(Root);
    auto* CameraNode = BP->SimpleConstructionScript->CreateNode(UCameraComponent::StaticClass(), O::Camera);
    Root->AddChildNode(CameraNode);
    auto* CameraTemplate = CastChecked<UCameraComponent>(CameraNode->ComponentTemplate);
    CameraTemplate->bUsePawnControlRotation = false; CameraTemplate->bConstrainAspectRatio = false;
    CameraTemplate->SetAutoActivate(true);
    AddVariable(BP, O::Active, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, N::NativeHudRequested, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, Control::Owned, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, Control::ReturnPending, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, O::PreviousView, UEdGraphSchema_K2::PC_Object, AActor::StaticClass());
    AddVariable(BP, O::BaselineFov, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, O::RequestedFov, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, Aim::Yaw, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, Aim::Pitch, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, O::CharacterHidden, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, N::Age, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, N::OriginalPawn, UEdGraphSchema_K2::PC_Object, ACharacter::StaticClass());
    AddVariable(BP, S::Held, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, S::Controller, UEdGraphSchema_K2::PC_Object, APlayerController::StaticClass());
    AddVariable(BP, S::Movement, UEdGraphSchema_K2::PC_Object, UCharacterMovementComponent::StaticClass());
    AddVariable(BP, S::Root, UEdGraphSchema_K2::PC_Object, UPrimitiveComponent::StaticClass());
    AddVariable(BP, S::Anchor, UEdGraphSchema_K2::PC_Object, USceneComponent::StaticClass());
    AddVariable(BP, S::CollisionBefore, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, S::RotationBefore, UEdGraphSchema_K2::PC_Struct, TBaseStructure<FRotator>::Get());
    AddVariable(BP, S::EntryLocal, UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
    AddVariable(BP, S::AnchorStart, UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
    AddVariable(BP, V::Vehicle, UEdGraphSchema_K2::PC_Object, AVoyageVehiclePawn::StaticClass());
    AddVariable(BP, NativeInputNames::Context, UEdGraphSchema_K2::PC_Object, UVoyageInputContextAsset::StaticClass());
    AddVariable(BP, V::Body, UEdGraphSchema_K2::PC_Object, UPrimitiveComponent::StaticClass());
    FKismetEditorUtilities::CompileBlueprint(BP);
    BuildContextCoordinator(BP, StationClass);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    if (BP->Status == BS_Error) return 1;
    auto* CDO = CastChecked<AActor>(BP->GeneratedClass->GetDefaultObject());
    CDO->PrimaryActorTick.bCanEverTick = true; CDO->PrimaryActorTick.bStartWithTickEnabled = true;
    CDO->PrimaryActorTick.TickGroup = TG_PostPhysics;
    CDO->PrimaryActorTick.TickInterval = CE::CoordinatorTickInterval; CDO->SetActorEnableCollision(false);
    auto* PawnCDO = CastChecked<APawn>(CDO);
    PawnCDO->AutoPossessPlayer = EAutoReceiveInput::Disabled;
    PawnCDO->AutoPossessAI = EAutoPossessAI::Disabled;
    PawnCDO->bUseControllerRotationPitch = false;
    PawnCDO->bUseControllerRotationYaw = false;
    PawnCDO->bUseControllerRotationRoll = false;
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(N::Package, FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Save; Save.TopLevelFlags = RF_Public | RF_Standalone; Save.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, BP, *Filename, Save) ? 0 : 1;
}
