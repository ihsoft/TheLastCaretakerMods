#include "GenerateVoyageAutoLoaderCommandlet.h"

#if WITH_EDITOR

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Factories/WorldFactory.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/SaveGame.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_Event.h"
#include "K2Node_ClassDynamicCast.h"
#include "K2Node_GetArrayItem.h"
#include "Kismet/KismetMathLibrary.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "RegistryDiscoveryGraphNames.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/SavePackage.h"
#include "VoyageWorldSettings.h"
#include "Kismet/BlueprintPathsLibrary.h"
#include "VoyageEditorBlueprintFunctionLibrary.h"
#include "VoyageGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "UObject/Script.h"

namespace
{
constexpr TCHAR ControllerPackage[] = TEXT("/Game/VoyageAutoLoader/WBP_AutoLoaderControl");
constexpr TCHAR ControllerAsset[] = TEXT("WBP_AutoLoaderControl");
constexpr TCHAR BootstrapPackage[] = TEXT("/Game/VoyageAutoLoader/BP_AutoLoaderMenuGameMode");
constexpr TCHAR BootstrapAsset[] = TEXT("BP_AutoLoaderMenuGameMode");
const FName CachedWorkerField(TEXT("CachedWorker"));
constexpr TCHAR GenerateCanaryFlag[] = TEXT("GenerateCanary");
constexpr TCHAR DmlClassPath[] = TEXT("/Game/DmgModLoader/WBP_DML.WBP_DML_C");
constexpr TCHAR WidgetRootName[] = TEXT("NonInteractiveRoot");
constexpr TCHAR PakExtension[] = TEXT(".pak");
constexpr TCHAR UtocExtension[] = TEXT(".utoc");
constexpr TCHAR UcasExtension[] = TEXT(".ucas");
const FName WidgetClassPin(TEXT("WidgetClass"));
const FName FoundWidgetsPin(TEXT("FoundWidgets"));
const FName OwningPlayerPin(TEXT("OwningPlayer"));
const FName TickEventName(TEXT("Tick"));
const FName ConstructEventName(TEXT("Construct"));
const FName DestructEventName(TEXT("Destruct"));
const FName WidgetTypePin(TEXT("WidgetType"));
const FName WorldNameField(TEXT("WorldName"));
const FName ActivationPhaseField(TEXT("ActivationPhase"));
constexpr TCHAR EmptyMapPackageName[] = TEXT("/Game/Maps/Empty");
constexpr TCHAR EmptyMapAssetName[] = TEXT("Empty");
constexpr TCHAR ModPackageName[] = TEXT("/Game/Mods/AutoLoaderCanary/ModActor");
constexpr TCHAR ModAssetName[] = TEXT("ModActor");
constexpr TCHAR MenuGameModePackageName[] = TEXT("/Game/Game/BP_VoyageGameModeMenu");
constexpr TCHAR MenuGameModeAssetName[] = TEXT("BP_VoyageGameModeMenu");
constexpr TCHAR LoaderGameModePackageName[] = TEXT("/Game/VoyageAutoLoader/BP_AutoLoaderWorker");
constexpr TCHAR LoaderGameModeAssetName[] = TEXT("BP_AutoLoaderWorker");
constexpr TCHAR ActorMarkerPackage[] = TEXT("/Game/Mods/AutoLoaderCanary/BP_ActivationMarker");
constexpr TCHAR ActorMarkerAsset[] = TEXT("BP_ActivationMarker");
constexpr TCHAR ActorBeginPlaySlot[] = TEXT("VoyageAutoLoader_C1_ACTOR_BEGINPLAY");
constexpr TCHAR UnitScale[] = TEXT("1,1,1");
const FName ScalePin(TEXT("Scale"));
constexpr TCHAR AlwaysSpawnValue[] = TEXT("AlwaysSpawn");
const FName ActorClassPin(TEXT("ActorClass"));
const FName OutActorsPin(TEXT("OutActors"));
const FName CollisionHandlingPin(TEXT("CollisionHandlingOverride"));
constexpr TCHAR PaksSubdirectory[] = TEXT("Paks/");
constexpr TCHAR CandidateFilter[] = TEXT("*.autoload");
constexpr TCHAR CandidateSuffix[] = TEXT(".autoload");
constexpr TCHAR EntrySuffix[] = TEXT("/ModActor.ModActor_C");
const FName CountPin(TEXT("Count"));
const FName JoinSourceArrayPin(TEXT("SourceArray"));
const FName JoinSeparatorPin(TEXT("Separator"));
const FName StringLengthInputPin(TEXT("S"));
constexpr TCHAR DescriptorLineSeparator[] = TEXT("\n");
const FName InPathPin(TEXT("InPath"));
const FName FileTypesPin(TEXT("FileTypes"));
constexpr TCHAR ModObjectPrefix[] = TEXT("/Game/Mods/");
const FName ReceiveBeginPlayName(TEXT("ReceiveBeginPlay"));
const FName ExecutePinName(UEdGraphSchema_K2::PN_Execute);
const FName ThenPinName(UEdGraphSchema_K2::PN_Then);
const FName SelfPinName(UEdGraphSchema_K2::PN_Self);
const FName ReturnValuePinName(UEdGraphSchema_K2::PN_ReturnValue);
const FName SaveGameObjectPinName(TEXT("SaveGameObject"));
const FName SaveGameClassPinName(TEXT("SaveGameClass"));
const FName SlotNamePinName(TEXT("SlotName"));

template <typename NodeType>
NodeType* FinishNode(NodeType* Node, UEdGraph* Graph, int32 X, int32 Y)
{
    Node->CreateNewGuid();
    Node->PostPlacedNewNode();
    Node->SetFlags(RF_Transactional);
    Node->AllocateDefaultPins();
    Node->NodePosX = X;
    Node->NodePosY = Y;
    Graph->AddNode(Node, true, false);
    return Node;
}

UEdGraphPin* RequirePin(UEdGraphNode* Node, const FName Name)
{
    UEdGraphPin* Pin = Node->FindPin(Name);
    if (!Pin)
    {
        for (const UEdGraphPin* Available : Node->Pins)
        {
            UE_LOG(LogTemp, Error, TEXT("Available pin: %s"), *Available->PinName.ToString());
        }
    }
    checkf(Pin, TEXT("Missing pin '%s' on node '%s'"), *Name.ToString(), *Node->GetName());
    return Pin;
}

UK2Node_CallFunction* AddCall(UEdGraph* Graph, UFunction* Function, int32 X, int32 Y)
{
    check(Function);
    UK2Node_CallFunction* Node = Function->GetOuterUClass() == UKismetArrayLibrary::StaticClass()
        ? NewObject<UK2Node_CallArrayFunction>(Graph)
        : NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(Function);
    return FinishNode(Node, Graph, X, Y);
}

bool Connect(const UEdGraphSchema_K2* Schema, UEdGraphPin* A, UEdGraphPin* B)
{
    if (Schema->TryCreateConnection(A, B))
    {
        return true;
    }
    UE_LOG(LogTemp, Error, TEXT("Could not connect %s.%s to %s.%s"),
        *A->GetOwningNode()->GetName(), *A->PinName.ToString(),
        *B->GetOwningNode()->GetName(), *B->PinName.ToString());
    return false;
}

void SetDefault(UEdGraphNode* Node, const FName PinName, const FString& Value)
{
    RequirePin(Node, PinName)->DefaultValue = Value;
}

void SetDefaultObject(UEdGraphNode* Node, const FName PinName, UObject* Value)
{
    UEdGraphPin* Pin = RequirePin(Node, PinName);
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    Schema->TrySetDefaultObject(*Pin, Value);
    check(Pin->DefaultObject == Value);
}

bool SaveAsset(UPackage* Package, UObject* Asset, const FString& Extension)
{
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), Extension);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
}

UBlueprint* CreateBlueprint(const TCHAR* PackageName, const TCHAR* AssetName, UClass* ParentClass)
{
    UPackage* Package = CreatePackage(PackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentClass, Package, FName(AssetName), BPTYPE_Normal,
        UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    check(Blueprint);
    return Blueprint;
}


#include "DescriptorGraph.inl"

bool BuildActivationActor(UBlueprint* Blueprint)
{
    UBlueprint* Marker = CreateBlueprint(ActorMarkerPackage, ActorMarkerAsset, USaveGame::StaticClass());
    FEdGraphPinType WorldNameType;
    WorldNameType.PinCategory = UEdGraphSchema_K2::PC_String;
    if (!FBlueprintEditorUtils::AddMemberVariable(Marker, WorldNameField, WorldNameType)) return false;
    FBlueprintEditorUtils::SetVariableSaveGameFlag(Marker, WorldNameField, true);
    FKismetEditorUtilities::CompileBlueprint(Marker);
    if (Marker->Status == BS_Error || !SaveAsset(Marker->GetOutermost(), Marker, FPackageName::GetAssetPackageExtension()))
        return false;
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    auto* Begin = NewObject<UK2Node_Event>(Graph);
    Begin->EventReference.SetExternalMember(ReceiveBeginPlayName, AActor::StaticClass());
    Begin->bOverrideFunction = true;
    FinishNode(Begin, Graph, 0, 0);
    auto* Create = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, CreateSaveGameObject)), 300, 0);
    SetDefaultObject(Create, SaveGameClassPinName, Marker->GeneratedClass);
    auto* Save = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, SaveGameToSlot)), 600, 0);
    SetDefault(Save, SlotNamePinName, ActorBeginPlaySlot);
    bool Ok = Connect(Schema, RequirePin(Begin, ThenPinName), RequirePin(Create, ExecutePinName));
    auto* WorldName = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetCurrentLevelName)), 300, -200);
    auto* StoreWorld = NewObject<UK2Node_VariableSet>(Graph);
    StoreWorld->VariableReference.SetExternalMember(WorldNameField, Marker->GeneratedClass);
    FinishNode(StoreWorld, Graph, 450, 0);
    Ok &= Connect(Schema, RequirePin(Create, ThenPinName), RequirePin(WorldName, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(WorldName, ThenPinName), RequirePin(StoreWorld, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Create, ReturnValuePinName), RequirePin(StoreWorld, SelfPinName));
    Ok &= Connect(Schema, RequirePin(WorldName, ReturnValuePinName), RequirePin(StoreWorld, WorldNameField));
    Ok &= Connect(Schema, RequirePin(StoreWorld, ThenPinName), RequirePin(Save, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Create, ReturnValuePinName), RequirePin(Save, SaveGameObjectPinName));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return Ok && Blueprint->Status != BS_Error;
}

bool BuildLoaderGraph(UBlueprint* Blueprint)
{
    using namespace RegistryDiscoveryGraphNames;
    using namespace BlueprintGraphNames::Pins;
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    Descriptor::FGraph G { Graph };
    auto* Begin = NewObject<UK2Node_Event>(Graph);
    Begin->EventReference.SetExternalMember(ReceiveBeginPlayName, AActor::StaticClass());
    Begin->bOverrideFunction = true;
    FinishNode(Begin, Graph, 0, 0);
    G.Tail = RequirePin(Begin, ThenPinName);

    auto* World = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetCurrentLevelName));
    G.Link(G.Tail, G.Pin(World, ExecutePinName));
    G.Tail = G.Pin(World, ThenPinName);
    auto* Phase = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectString));
    SetDefault(Phase, Select::WhenTrue, Descriptor::MenuValue);
    SetDefault(Phase, Select::WhenFalse, Descriptor::GameplayValue);
    G.Link(G.Equal(G.Pin(World, ReturnValuePinName), EmptyMapAssetName), G.Pin(Phase, Select::Condition));
    G.Write(ActivationPhaseField, G.Pin(Phase, ReturnValuePinName));

    auto Join = [&](UEdGraphPin* Left, UEdGraphPin* Right, const TCHAR* Literal = nullptr)
    {
        auto* Node = G.String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Concat_StrStr));
        G.Link(Left, G.Pin(Node, Binary::LeftOperand));
        if (Right) G.Link(Right, G.Pin(Node, Binary::RightOperand));
        else SetDefault(Node, Binary::RightOperand, Literal);
        return G.Pin(Node, ReturnValuePinName);
    };
    auto* ContentPath = G.Call(UBlueprintPathsLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UBlueprintPathsLibrary, ProjectContentDir));
    auto* Full = G.Call(UBlueprintPathsLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UBlueprintPathsLibrary, ConvertRelativePathToFull));
    G.Link(Join(G.Pin(ContentPath, ReturnValuePinName), nullptr, PaksSubdirectory), G.Pin(Full, InPathPin));
    auto* PaksPath = G.Pin(Full, ReturnValuePinName);
    auto* Files = G.Call(UVoyageEditorBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageEditorBlueprintFunctionLibrary, GetAllFilesInDirectory));
    SetDefault(Files, FileTypesPin, CandidateFilter);
    G.Link(PaksPath, G.Pin(Files, InPathPin));
    G.Link(G.Tail, G.Pin(Files, ExecutePinName));
    G.Tail = G.Pin(Files, ThenPinName);
    auto* Loop = G.ForEach(G.Pin(Files, ReturnValuePinName));
    auto* File = G.Pin(Loop, ArrayElement);
    auto* Stem = G.String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, LeftChop));
    G.Link(File, G.Pin(Stem, SourceString));
    SetDefault(Stem, CountPin, LexToString(FCString::Strlen(CandidateSuffix)));
    auto* Root = G.String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Concat_StrStr));
    SetDefault(Root, Binary::LeftOperand, ModObjectPrefix);
    G.Link(G.Pin(Stem, ReturnValuePinName), G.Pin(Root, Binary::RightOperand));

    auto* Read = G.Call(UVoyageEditorBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageEditorBlueprintFunctionLibrary, LoadFileToArray));
    G.Link(Join(PaksPath, File), G.Pin(Read, InPathPin));
    G.Link(G.Tail, G.Pin(Read, ExecutePinName));
    G.Tail = G.Pin(Read, ThenPinName);
    auto* Text = G.String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, JoinStringArray));
    G.Link(G.Pin(Read, ReturnValuePinName), G.Pin(Text, JoinSourceArrayPin));
    SetDefault(Text, JoinSeparatorPin, DescriptorLineSeparator);
    G.Write(Descriptor::Text, G.Pin(Text, ReturnValuePinName));
    G.Write(Descriptor::DefaultEntry, Join(G.Pin(Root, ReturnValuePinName), nullptr, EntrySuffix));
    auto* Parse = G.Call(Blueprint->GeneratedClass, Descriptor::Parse);
    G.Link(G.Tail, G.Pin(Parse, ExecutePinName));
    G.Tail = G.Pin(Parse, ThenPinName);
    G.Branch(G.Read(Descriptor::Valid));

    auto PhaseEnabled = [&](const TCHAR* Name, FName Enabled)
    {
        auto* Node = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND));
        G.Link(G.Equal(G.Read(ActivationPhaseField), Name), G.Pin(Node, Binary::LeftOperand));
        G.Link(G.Read(Enabled), G.Pin(Node, Binary::RightOperand));
        return G.Pin(Node, ReturnValuePinName);
    };
    auto* Either = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR));
    G.Link(PhaseEnabled(Descriptor::MenuValue, Descriptor::Menu), G.Pin(Either, Binary::LeftOperand));
    G.Link(PhaseEnabled(Descriptor::GameplayValue, Descriptor::Gameplay), G.Pin(Either, Binary::RightOperand));
    G.Branch(G.Pin(Either, ReturnValuePinName));
    auto* FullStem = Join(PaksPath, G.Pin(Stem, ReturnValuePinName));
    for (const TCHAR* Extension : { PakExtension, UtocExtension, UcasExtension })
    {
        auto* Exists = G.Call(UBlueprintPathsLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UBlueprintPathsLibrary, FileExists));
        G.Link(Join(FullStem, nullptr, Extension), G.Pin(Exists, InPathPin));
        G.Branch(G.Pin(Exists, ReturnValuePinName));
    }

    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftClassPath));
    G.Link(G.Read(Descriptor::Entry), G.Pin(Path, PathString));
    auto* Ref = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftClassPathToSoftClassRef));
    G.Link(G.Pin(Path, ReturnValuePinName), G.Pin(Ref, SoftClassPath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadClassAsset_Blocking));
    G.Link(G.Pin(Ref, ReturnValuePinName), G.Pin(Load, AssetClass));
    G.Link(G.Tail, G.Pin(Load, ExecutePinName));
    auto* Cast = NewObject<UK2Node_ClassDynamicCast>(Graph);
    Cast->TargetType = AActor::StaticClass(); Cast->SetPurity(false);
    FinishNode(Cast, Graph, G.X += 200, 0);
    G.Link(G.Pin(Load, ThenPinName), G.Pin(Cast, ExecutePinName));
    G.Link(G.Pin(Load, ReturnValuePinName), Cast->GetCastSourcePin());

    // One existing-instance check and at most one spawn per entry. Every failed
    // gate simply ends this loop body; later candidate files still get processed.
    auto* Existing = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetAllActorsOfClass));
    G.Link(Cast->GetValidCastPin(), G.Pin(Existing, ExecutePinName));
    G.Link(Cast->GetCastResultPin(), G.Pin(Existing, ActorClassPin));
    auto* Count = G.Call(UKismetArrayLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length));
    G.Link(G.Pin(Existing, OutActorsPin), G.Pin(Count, TargetArray));
    G.Tail = G.Pin(Existing, ThenPinName);
    G.Branch(G.Limit(G.Pin(Count, ReturnValuePinName), 0));
    auto* Transform = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeTransform));
    SetDefault(Transform, ScalePin, UnitScale);
    auto* Spawn = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginDeferredActorSpawnFromClass));
    G.Link(G.Tail, G.Pin(Spawn, ExecutePinName));
    G.Link(Cast->GetCastResultPin(), G.Pin(Spawn, ActorClassPin));
    G.Link(G.Pin(Transform, ReturnValuePinName), G.Pin(Spawn, SpawnTransform));
    SetDefault(Spawn, CollisionHandlingPin, AlwaysSpawnValue);
    auto* Finish = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor));
    G.Link(G.Pin(Spawn, ThenPinName), G.Pin(Finish, ExecutePinName));
    G.Link(G.Pin(Spawn, ReturnValuePinName), G.Pin(Finish, Actor));
    G.Link(G.Pin(Transform, ReturnValuePinName), G.Pin(Finish, SpawnTransform));

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return G.Ok && Blueprint->Status != BS_Error;
}

// Invisible viewport controller follows the same widget re-creation mechanism
// observed in the independent DML lifecycle audit, without DML class imports.
// Runtime persistence across Voyage travel is a candidate gate, not yet proven.
bool BuildController(UWidgetBlueprint* Blueprint, UClass* WorkerClass)
{
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    bool Ok = true;
    auto Event = [&](FName Name, int32 Y)
    {
        auto* Node = NewObject<UK2Node_Event>(Graph);
        Node->EventReference.SetExternalMember(Name, UUserWidget::StaticClass());
        Node->bOverrideFunction = true;
        FinishNode(Node, Graph, 0, Y);
        return RequirePin(Node, ThenPinName);
    };
    // Optional menu compatibility is independent of gameplay autoload.
    UEdGraphPin* BootstrapExec = Event(ConstructEventName, -500);
    auto* DmlPath = AddCall(Graph, UKismetSystemLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftClassPath)), 800, -600);
    SetDefault(DmlPath, RegistryDiscoveryGraphNames::PathString, DmlClassPath);
    auto* DmlRef = AddCall(Graph, UKismetSystemLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftClassPathToSoftClassRef)), 1000, -600);
    Ok &= Connect(Schema, RequirePin(DmlPath, ReturnValuePinName), RequirePin(DmlRef, RegistryDiscoveryGraphNames::SoftClassPath));
    auto* DmlLoad = AddCall(Graph, UKismetSystemLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadClassAsset_Blocking)), 1200, -500);
    Ok &= Connect(Schema, BootstrapExec, RequirePin(DmlLoad, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(DmlRef, ReturnValuePinName), RequirePin(DmlLoad, RegistryDiscoveryGraphNames::AssetClass));
    auto* DmlCast = NewObject<UK2Node_ClassDynamicCast>(Graph);
    DmlCast->TargetType = UUserWidget::StaticClass();
    DmlCast->SetPurity(false);
    FinishNode(DmlCast, Graph, 1450, -500);
    Ok &= Connect(Schema, RequirePin(DmlLoad, ThenPinName), RequirePin(DmlCast, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(DmlLoad, ReturnValuePinName), DmlCast->GetCastSourcePin());
    auto* ExistingDml = AddCall(Graph, UWidgetBlueprintLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, GetAllWidgetsOfClass)), 1700, -500);
    Ok &= Connect(Schema, DmlCast->GetValidCastPin(), RequirePin(ExistingDml, ExecutePinName));
    Ok &= Connect(Schema, DmlCast->GetCastResultPin(), RequirePin(ExistingDml, WidgetClassPin));
    auto* DmlCount = AddCall(Graph, UKismetArrayLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length)), 1900, -700);
    Ok &= Connect(Schema, RequirePin(ExistingDml, FoundWidgetsPin), RequirePin(DmlCount, BlueprintGraphNames::Pins::TargetArray));
    auto* DmlZero = AddCall(Graph, UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt)), 2100, -700);
    Ok &= Connect(Schema, RequirePin(DmlCount, ReturnValuePinName), RequirePin(DmlZero, BlueprintGraphNames::Pins::Binary::LeftOperand));
    auto* DmlGate = FinishNode(NewObject<UK2Node_IfThenElse>(Graph), Graph, 2300, -500);
    Ok &= Connect(Schema, RequirePin(ExistingDml, ThenPinName), RequirePin(DmlGate, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(DmlZero, ReturnValuePinName), RequirePin(DmlGate, BlueprintGraphNames::Pins::Condition));
    auto* DmlCreate = AddCall(Graph, UWidgetBlueprintLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create)), 2500, -500);
    Ok &= Connect(Schema, RequirePin(DmlGate, ThenPinName), RequirePin(DmlCreate, ExecutePinName));
    Ok &= Connect(Schema, DmlCast->GetCastResultPin(), RequirePin(DmlCreate, WidgetTypePin));
    auto* DmlPlayer = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController)), 2500, -800);
    Ok &= Connect(Schema, RequirePin(DmlPlayer, ReturnValuePinName), RequirePin(DmlCreate, OwningPlayerPin));
    auto* DmlValid = AddCall(Graph, UKismetSystemLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid)), 2700, -700);
    Ok &= Connect(Schema, RequirePin(DmlCreate, ReturnValuePinName), RequirePin(DmlValid, BlueprintGraphNames::Pins::Object));
    auto* CreatedGate = FinishNode(NewObject<UK2Node_IfThenElse>(Graph), Graph, 2900, -500);
    Ok &= Connect(Schema, RequirePin(DmlCreate, ThenPinName), RequirePin(CreatedGate, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(DmlValid, ReturnValuePinName), RequirePin(CreatedGate, BlueprintGraphNames::Pins::Condition));
    auto* DmlViewport = AddCall(Graph, UUserWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UUserWidget, AddToViewport)), 3100, -500);
    Ok &= Connect(Schema, RequirePin(CreatedGate, ThenPinName), RequirePin(DmlViewport, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(DmlCreate, ReturnValuePinName), RequirePin(DmlViewport, SelfPinName));
    UEdGraphPin* RecreateExec = Event(DestructEventName, -1000);
    auto* Recreate = AddCall(Graph, UWidgetBlueprintLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create)), 800, -1000);
    // Create() uses WidgetType; GetAllWidgetsOfClass uses WidgetClass.
    SetDefaultObject(Recreate, WidgetTypePin, Blueprint->GeneratedClass);
    auto* Controller = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController)), 600, -1300);
    Ok &= Connect(Schema, RequirePin(Controller, ReturnValuePinName), RequirePin(Recreate, OwningPlayerPin));
    auto* AddViewport = AddCall(Graph, UUserWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UUserWidget, AddToViewport)), 1100, -1000);
    Ok &= Connect(Schema, RecreateExec, RequirePin(Recreate, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Recreate, ThenPinName), RequirePin(AddViewport, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Recreate, ReturnValuePinName), RequirePin(AddViewport, SelfPinName));

    // Positive menu identity or gameplay readiness; Pawn readiness belongs to mods.
    Descriptor::FGraph PhaseGraph { Graph };
    auto* World = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetCurrentLevelName)), 250, 0);
    PhaseGraph.Tail = Event(TickEventName, 0);
    auto* CachedValid = AddCall(Graph, UKismetSystemLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid)), 0, -200);
    PhaseGraph.Link(PhaseGraph.Read(CachedWorkerField), RequirePin(CachedValid, BlueprintGraphNames::Pins::Object));
    auto* CachedGate = PhaseGraph.Branch(RequirePin(CachedValid, ReturnValuePinName));
    Ok &= Connect(Schema, RequirePin(CachedGate, BlueprintGraphNames::Pins::Else), RequirePin(World, ExecutePinName));
    auto* Instance = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetGameInstance)), 450, -200);
    auto* InstanceCast = NewObject<UK2Node_DynamicCast>(Graph);
    InstanceCast->TargetType = UVoyageGameInstance::StaticClass();
    InstanceCast->SetPurity(false);
    FinishNode(InstanceCast, Graph, 500, 0);
    Ok &= Connect(Schema, RequirePin(Instance, ReturnValuePinName), InstanceCast->GetCastSourcePin());
    Ok &= Connect(Schema, RequirePin(World, ThenPinName), RequirePin(InstanceCast, ExecutePinName));
    auto* Playing = AddCall(Graph, UVoyageGameInstance::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UVoyageGameInstance, IsPlaying)), 750, -200);
    Ok &= Connect(Schema, InstanceCast->GetCastResultPin(), RequirePin(Playing, SelfPinName));
    auto* Gate = FinishNode(NewObject<UK2Node_IfThenElse>(Graph), Graph, 1000, 0);
    Ok &= Connect(Schema, InstanceCast->GetValidCastPin(), RequirePin(Gate, ExecutePinName));
    auto* Eligible = AddCall(Graph, UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR)), 900, -200);
    Ok &= Connect(Schema, RequirePin(Playing, ReturnValuePinName), RequirePin(Eligible, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(Schema, PhaseGraph.Equal(RequirePin(World, ReturnValuePinName), EmptyMapAssetName), RequirePin(Eligible, BlueprintGraphNames::Pins::Binary::RightOperand));
    Ok &= Connect(Schema, RequirePin(Eligible, ReturnValuePinName), RequirePin(Gate, BlueprintGraphNames::Pins::Condition));
    Ok &= PhaseGraph.Ok;
    auto* Query = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetAllActorsOfClass)), 1800, 0);
    SetDefaultObject(Query, ActorClassPin, WorkerClass);
    Ok &= Connect(Schema, RequirePin(Gate, ThenPinName), RequirePin(Query, ExecutePinName));
    auto* Length = AddCall(Graph, UKismetArrayLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length)), 2000, -200);
    Ok &= Connect(Schema, RequirePin(Query, OutActorsPin), RequirePin(Length, BlueprintGraphNames::Pins::TargetArray));
    auto* Zero = AddCall(Graph, UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt)), 2200, -200);
    Ok &= Connect(Schema, RequirePin(Length, ReturnValuePinName), RequirePin(Zero, BlueprintGraphNames::Pins::Binary::LeftOperand));
    auto* SpawnGate = FinishNode(NewObject<UK2Node_IfThenElse>(Graph), Graph, 2400, 0);
    Ok &= Connect(Schema, RequirePin(Query, ThenPinName), RequirePin(SpawnGate, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Zero, ReturnValuePinName), RequirePin(SpawnGate, BlueprintGraphNames::Pins::Condition));
    auto* Transform = AddCall(Graph, UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeTransform)), 2400, -400);
    SetDefault(Transform, ScalePin, UnitScale);
    auto* Spawn = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginDeferredActorSpawnFromClass)), 2700, 0);
    SetDefaultObject(Spawn, ActorClassPin, WorkerClass);
    SetDefault(Spawn, CollisionHandlingPin, AlwaysSpawnValue);
    Ok &= Connect(Schema, RequirePin(SpawnGate, ThenPinName), RequirePin(Spawn, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Transform, ReturnValuePinName), RequirePin(Spawn, BlueprintGraphNames::Pins::SpawnTransform));
    auto* Finish = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor)), 3000, 0);
    Ok &= Connect(Schema, RequirePin(Spawn, ThenPinName), RequirePin(Finish, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Spawn, ReturnValuePinName), RequirePin(Finish, BlueprintGraphNames::Pins::Actor));
    Ok &= Connect(Schema, RequirePin(Transform, ReturnValuePinName), RequirePin(Finish, BlueprintGraphNames::Pins::SpawnTransform));
    // Cache the existing worker as well as a newly created worker. Invalidated
    // actor references re-enable discovery after world teardown; names are not
    // used as world-instance identity.
    auto* FirstWorker = FinishNode(NewObject<UK2Node_GetArrayItem>(Graph), Graph, 2400, 400);
    PhaseGraph.Link(RequirePin(Query, OutActorsPin), FirstWorker->GetTargetArrayPin());
    PhaseGraph.Tail = RequirePin(SpawnGate, BlueprintGraphNames::Pins::Else);
    PhaseGraph.Write(CachedWorkerField, FirstWorker->GetResultPin());
    PhaseGraph.Tail = RequirePin(Finish, ThenPinName);
    PhaseGraph.Write(CachedWorkerField, RequirePin(Finish, ReturnValuePinName));
    Ok &= PhaseGraph.Ok;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (auto* Defaults = Cast<UUserWidget>(Blueprint->GeneratedClass->GetDefaultObject()))
        Defaults->SetVisibility(ESlateVisibility::HitTestInvisible);
    return Ok && Blueprint->Status != BS_Error;
}

bool BuildBootstrap(UBlueprint* Blueprint, UClass* ControllerClass)
{
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    const auto* Schema = GetDefault<UEdGraphSchema_K2>();
    auto* Begin = NewObject<UK2Node_Event>(Graph);
    Begin->EventReference.SetExternalMember(ReceiveBeginPlayName, AActor::StaticClass());
    Begin->bOverrideFunction = true;
    FinishNode(Begin, Graph, 0, 0);
    auto* Create = AddCall(Graph, UWidgetBlueprintLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create)), 300, 0);
    SetDefaultObject(Create, WidgetTypePin, ControllerClass);
    auto* Player = AddCall(Graph, UGameplayStatics::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController)), 100, -200);
    auto* AddViewport = AddCall(Graph, UUserWidget::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UUserWidget, AddToViewport)), 600, 0);
    auto* Existing = AddCall(Graph, UWidgetBlueprintLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, GetAllWidgetsOfClass)), 100, 200);
    SetDefaultObject(Existing, WidgetClassPin, ControllerClass);
    auto* Count = AddCall(Graph, UKismetArrayLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length)), 350, 200);
    auto* Zero = AddCall(Graph, UKismetMathLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt)), 600, 200);
    auto* Gate = FinishNode(NewObject<UK2Node_IfThenElse>(Graph), Graph, 850, 200);
    bool Ok = Connect(Schema, RequirePin(Begin, ThenPinName), RequirePin(Existing, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Existing, FoundWidgetsPin), RequirePin(Count, BlueprintGraphNames::Pins::TargetArray));
    Ok &= Connect(Schema, RequirePin(Count, ReturnValuePinName), RequirePin(Zero, BlueprintGraphNames::Pins::Binary::LeftOperand));
    Ok &= Connect(Schema, RequirePin(Zero, ReturnValuePinName), RequirePin(Gate, BlueprintGraphNames::Pins::Condition));
    Ok &= Connect(Schema, RequirePin(Existing, ThenPinName), RequirePin(Gate, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Gate, ThenPinName), RequirePin(Create, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Player, ReturnValuePinName), RequirePin(Create, OwningPlayerPin));
    Ok &= Connect(Schema, RequirePin(Create, ThenPinName), RequirePin(AddViewport, ExecutePinName));
    Ok &= Connect(Schema, RequirePin(Create, ReturnValuePinName), RequirePin(AddViewport, SelfPinName));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return Ok && Blueprint->Status != BS_Error;
}

bool CreateBootstrapMap(UClass* LoaderGameModeClass)
{
    UPackage* Package = CreatePackage(EmptyMapPackageName);
    UWorldFactory* Factory = NewObject<UWorldFactory>();
    const TSubclassOf<AWorldSettings> PreviousWorldSettingsClass = GEngine->WorldSettingsClass;
    GEngine->WorldSettingsClass = AVoyageWorldSettings::StaticClass();
    UWorld* World = Cast<UWorld>(Factory->FactoryCreateNew(
        UWorld::StaticClass(), Package, FName(EmptyMapAssetName),
        RF_Public | RF_Standalone, nullptr, GWarn));
    GEngine->WorldSettingsClass = PreviousWorldSettingsClass;
    if (!World || !World->PersistentLevel)
    {
        return false;
    }

    AVoyageWorldSettings* WorldSettings = Cast<AVoyageWorldSettings>(
        World->PersistentLevel->GetWorldSettings(false));
    if (!WorldSettings)
    {
        return false;
    }
    WorldSettings->bUseSaveGame = false;
    WorldSettings->bSpawnGameplayActors = false;
    WorldSettings->DefaultGameMode = LoaderGameModeClass;

    World->PersistentLevel->MarkPackageDirty();
    return SaveAsset(Package, World, FPackageName::GetMapPackageExtension());
}

}

UGenerateVoyageAutoLoaderCommandlet::UGenerateVoyageAutoLoaderCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UGenerateVoyageAutoLoaderCommandlet::Main(const FString& Params)
{
    // Test fixture generation is opt-in and never part of a normal loader build.
    if (FParse::Param(*Params, GenerateCanaryFlag))
    {
        UBlueprint* Canary = CreateBlueprint(ModPackageName, ModAssetName, AActor::StaticClass());
        return BuildActivationActor(Canary) &&
            SaveAsset(Canary->GetOutermost(), Canary, FPackageName::GetAssetPackageExtension()) ? 0 : 1;
    }

    UBlueprint* MenuGameMode = CreateBlueprint(
        MenuGameModePackageName, MenuGameModeAssetName, AGameModeBase::StaticClass());
    FKismetEditorUtilities::CompileBlueprint(MenuGameMode);
    if (MenuGameMode->Status == BS_Error ||
        !SaveAsset(MenuGameMode->GetOutermost(), MenuGameMode,
            FPackageName::GetAssetPackageExtension()))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate menu GameMode identity mirror"));
        return 1;
    }

    UBlueprint* LoaderGameMode = CreateBlueprint(
        LoaderGameModePackageName, LoaderGameModeAssetName, AActor::StaticClass());
    FEdGraphPinType PhaseType;
    PhaseType.PinCategory = UEdGraphSchema_K2::PC_String;
    if (!FBlueprintEditorUtils::AddMemberVariable(LoaderGameMode, ActivationPhaseField, PhaseType) ||
        !BuildDescriptorParser(LoaderGameMode)) return 1;

    if (!BuildLoaderGraph(LoaderGameMode) || !TestDescriptorParser(LoaderGameMode) ||
        !SaveAsset(LoaderGameMode->GetOutermost(), LoaderGameMode,
            FPackageName::GetAssetPackageExtension()))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate autoload worker"));
        return 1;
    }

    UPackage* ControllerOwner = CreatePackage(ControllerPackage);
    auto* ControllerBlueprint = CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
        UUserWidget::StaticClass(), ControllerOwner, FName(ControllerAsset), BPTYPE_Normal,
        UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
    FEdGraphPinType CachedWorkerType;
    CachedWorkerType.PinCategory = UEdGraphSchema_K2::PC_Object;
    CachedWorkerType.PinSubCategoryObject = AActor::StaticClass();
    if (!FBlueprintEditorUtils::AddMemberVariable(ControllerBlueprint, CachedWorkerField, CachedWorkerType)) return 1;
    ControllerBlueprint->WidgetTree->RootWidget = ControllerBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), FName(WidgetRootName));
    FKismetEditorUtilities::CompileBlueprint(ControllerBlueprint);
    if (!BuildController(ControllerBlueprint, LoaderGameMode->GeneratedClass) ||
        !SaveAsset(ControllerOwner, ControllerBlueprint, FPackageName::GetAssetPackageExtension())) return 1;
    auto* Bootstrap = CreateBlueprint(BootstrapPackage, BootstrapAsset, MenuGameMode->GeneratedClass);
    if (!BuildBootstrap(Bootstrap, ControllerBlueprint->GeneratedClass) ||
        !SaveAsset(Bootstrap->GetOutermost(), Bootstrap, FPackageName::GetAssetPackageExtension())) return 1;

    if (!CreateBootstrapMap(Bootstrap->GeneratedClass))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to generate self-contained /Game/Maps/Empty loader"));
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Generated four-package loader; no registry probes or diagnostic SaveGame payload"));
    return 0;
}

#else

UGenerateVoyageAutoLoaderCommandlet::UGenerateVoyageAutoLoaderCommandlet() = default;
int32 UGenerateVoyageAutoLoaderCommandlet::Main(const FString&) { return 1; }

#endif
