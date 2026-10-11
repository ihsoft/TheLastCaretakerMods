// HAND-WRITTEN BUILD TOOL SOURCE: additive Electrified Boat generator.
// Contracts were reconstructed for Steam build 25191271 / executable SHA-256
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// The generator and Voyage mirrors are editor-only and never ship.

#include "GenerateElectrifiedBoatCommandlet.h"

#if WITH_EDITOR

#include "SocketGraph.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Actor.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Subsystems/SubsystemBlueprintLibrary.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "VoyageBoatPawn.h"
#include "ModuleSocketComponent.h"
#include "VoyageLevelInstanceComponent.h"
#include "VoyageMiscBlueprintFunctionLibrary.h"
#include "VoyageModuleActor.h"
#include "VoyageModuleComponent.h"
#include "VoyageModuleSocketViewComponent.h"
#include "VoyagePersistentSubsystem.h"
#include "VoyageVirtualActorSocket.h"
#include "WeakActorArrayGetNode.h"

namespace ElectrifiedBoat
{
constexpr TCHAR StockSocketPackage[] =
    TEXT("/Game/Blueprints/Modules/Utility/Wireless/BP_WallSocket_Electric");
constexpr TCHAR StockSocketAsset[] = TEXT("BP_WallSocket_Electric");
constexpr TCHAR StockExternalSocketPackage[] =
    TEXT("/Game/Blueprints/Modules/BP_AttachmentVirtualSocketActor_Electricity");
constexpr TCHAR StockExternalSocketAsset[] =
    TEXT("BP_AttachmentVirtualSocketActor_Electricity");
constexpr TCHAR ModActorPackage[] =
    TEXT("/Game/Mods/ElectrifiedBoat/ModActor");
constexpr TCHAR ModActorAsset[] = TEXT("ModActor");
constexpr TCHAR StandardMacrosAsset[] =
    TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros");

const FName StockStubBlueprintName(TEXT("GenerateElectrifiedBoatStockStub"));
const FName ExternalStubBlueprintName(TEXT("GenerateBoatExternalSocketStockStub"));
const FName ModActorBlueprintName(TEXT("GenerateElectrifiedBoatModActor"));
const FName EventGraphName(TEXT("EventGraph"));
const FName ProcessModuleFunction(TEXT("ProcessModule"));
const FName ReadOwnedActorAtFunction(TEXT("ReadOwnedActorAt"));
const FName HandleActorAttachedFunction(TEXT("HandleActorAttached"));
const FName HandleRegisteredActorEndPlayFunction(TEXT("HandleRegisteredActorEndPlay"));
const FName ModuleParameter(TEXT("Module"));
const FName WallActorParameter(TEXT("WallActor"));
const FName BoatParameter(TEXT("Boat"));
const FName LevelComponentParameter(TEXT("LevelComponent"));
const FName ActorParameter(TEXT("Actor"));
const FName EndPlayReasonParameter(TEXT("EndPlayReason"));
const FName ChildParameter(TEXT("Child"));
const FName ParentComponentParameter(TEXT("ParentComponent"));
const FName SocketParameter(TEXT("Socket"));
const FName RegisteredActorsVariable(TEXT("RegisteredExternalActors"));
const FName RegisteredSocketsVariable(TEXT("RegisteredExternalSockets"));
const FName RegisteredOwnersVariable(TEXT("RegisteredExpectedOwners"));
const FName CandidateOwnerVariable(TEXT("ExternalCandidateOwner"));
const FName CandidateViewVariable(TEXT("ExternalCandidateView"));
const FName OwnedActorSnapshotVariable(TEXT("ExternalOwnedActorSnapshot"));
const FName PairedModuleProperty(TEXT("PairedModule"));
const FName StaticMeshProperty(TEXT("StaticMesh"));
const FName SocketViewProperty(TEXT("VoyageModuleSocketView"));
const FName ModuleOwnerProperty(TEXT("ModuleOwner"));
const FName PortProperty(TEXT("Port"));
const FName AutoInitializeProperty(TEXT("bAutoInitialize"));
const FName AddModuleRequirementProperty(TEXT("bAddModuleRequirement"));
const FName IsVirtualProperty(TEXT("bIsVirtual"));
const FName IsVirtualGroupedProperty(TEXT("bIsVirtualGrouped"));
const FName IsVirtualShareToGroupProperty(TEXT("bIsVirtualShareToGroup"));
const FName OwnedActorsProperty(TEXT("OwnedActors"));
const FName OnActorAttachedProperty(TEXT("OnActorAttached"));
const FName OnEndPlayProperty(TEXT("OnEndPlay"));
const FName BoatGroup(TEXT("Boat"));
const FName WallSocketGroup(TEXT("WallSocket"));
const FName BoatElectricTag(TEXT("Boat_Electric"));
const FName ReceiveBeginPlayEvent(TEXT("ReceiveBeginPlay"));
const FName GetObjectClassFunction(TEXT("GetObjectClass"));
const FName GetOwnerFunction(TEXT("GetOwner"));
const FName GetComponentsByClassFunction(TEXT("K2_GetComponentsByClass"));
const FName ActorHasTagFunction(TEXT("ActorHasTag"));
const FName GetObjectNameFunction(TEXT("GetObjectName"));
const FName ConvertStringToNameFunction(TEXT("Conv_StringToName"));
const FName GetParentModuleFunction(TEXT("GetParentModule"));
const FName GetModuleFromActorFunction(TEXT("GetModuleFromActor"));
const FName SetActorTickEnabledFunction(TEXT("SetActorTickEnabled"));
const FName ConvertSoftObjectFunction(TEXT("Conv_SoftObjectReferenceToObject"));
const FName EqualClassFunction(TEXT("EqualEqual_ClassClass"));
const FName NotEqualIntFunction(TEXT("NotEqual_IntInt"));
const FName GreaterEqualIntFunction(TEXT("GreaterEqual_IntInt"));
const FName BooleanNotFunction(TEXT("Not_PreBool"));
const FName ObjectEqualFunction(TEXT("EqualEqual_ObjectObject"));
const FName IntSubtractFunction(TEXT("Subtract_IntInt"));
const FName ArrayAddFunction(TEXT("Array_Add"));
const FName ArrayRemoveFunction(TEXT("Array_Remove"));
const FName ArrayFindFunction(TEXT("Array_Find"));
const FName ArrayGetFunction(TEXT("Array_Get"));
const FName ArrayContainsFunction(TEXT("Array_Contains"));
const FName ArrayLengthFunction(TEXT("Array_Length"));
const FName ArrayClearFunction(TEXT("Array_Clear"));
const FName ForEachLoopWithBreakMacro(TEXT("ForEachLoopWithBreak"));
const FName ForLoopMacro(TEXT("ForLoop"));
const FName FunctionTargetPin(TEXT("self"));
const FName SubsystemClassPin(TEXT("Class"));
const FName TargetArrayPin(TEXT("TargetArray"));
const FName NewItemPin(TEXT("NewItem"));
const FName ItemPin(TEXT("Item"));
const FName ItemToFindPin(TEXT("ItemToFind"));
const FName IndexPin(TEXT("Index"));
const FName FirstIndexPin(TEXT("FirstIndex"));
const FName LastIndexPin(TEXT("LastIndex"));
const FName IndexToRemovePin(TEXT("IndexToRemove"));
const FName ArrayPin(TEXT("Array"));
const FName ArrayElementPin(TEXT("Array Element"));
const FName LoopBodyPin(TEXT("LoopBody"));
const FName CompletedPin(TEXT("Completed"));
const FName BreakPin(TEXT("Break"));
const FName ExecPin(TEXT("Exec"));
const FName ObjectPinName(TEXT("Object"));
const FName SoftObjectPinName(TEXT("SoftObject"));
const FName LeftOperandPin(TEXT("A"));
const FName RightOperandPin(TEXT("B"));
const FName PrimitiveComponentPin(TEXT("PrimitiveComponent"));
const FName IncludeWeldedParentPin(TEXT("bIncludeWeldedParent"));
const FName GroupNamePin(TEXT("GroupName"));
const FName ComponentClassPin(TEXT("ComponentClass"));
const FName TagPin(TEXT("Tag"));
const FName NamePin(TEXT("Name"));
const FName EventPin(TEXT("Delegate"));
const FName InStringPin(TEXT("InString"));
const FName EnabledPin(TEXT("bEnabled"));
const FString TrueValue(TEXT("true"));
const FString FalseValue(TEXT("false"));
const FString ZeroValue(TEXT("0"));
const FString OneValue(TEXT("1"));
const FString EmptyObjectValue(TEXT("None"));

namespace K2Pins
{
const FName Execute(UEdGraphSchema_K2::PN_Execute);
const FName Then(UEdGraphSchema_K2::PN_Then);
const FName ReturnValue(UEdGraphSchema_K2::PN_ReturnValue);
}

FEdGraphPinType ObjectPin(UClass* Class)
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Object;
    Type.PinSubCategoryObject = Class;
    return Type;
}

FEdGraphPinType SoftObjectPin(UClass* Class)
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_SoftObject;
    Type.PinSubCategoryObject = Class;
    return Type;
}

FEdGraphPinType BoolPin()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    return Type;
}

FEdGraphPinType IntPin()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Int;
    return Type;
}

FEdGraphPinType FloatPin()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_Real;
    Type.PinSubCategory = UEdGraphSchema_K2::PC_Float;
    return Type;
}

FEdGraphPinType StringPin()
{
    FEdGraphPinType Type;
    Type.PinCategory = UEdGraphSchema_K2::PC_String;
    return Type;
}

UEdGraph* FindStandardMacro(const FName Name)
{
    UBlueprint* StandardMacros = LoadObject<UBlueprint>(
        nullptr,
        StandardMacrosAsset);
    check(StandardMacros);
    for (UEdGraph* Graph : StandardMacros->MacroGraphs)
    {
        if (Graph && Graph->GetFName() == Name)
        {
            return Graph;
        }
    }
    return nullptr;
}

UBlueprint* CreateBlueprint(
    const TCHAR* PackageName,
    const TCHAR* AssetName,
    UClass* ParentClass,
    const FName BlueprintName)
{
    check(!FPackageName::DoesPackageExist(PackageName));
    UPackage* Package = CreatePackage(PackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        ParentClass,
        Package,
        FName(AssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        BlueprintName);
    check(Blueprint);
    return Blueprint;
}

void Compile(UBlueprint* Blueprint)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    checkf(Blueprint->Status != BS_Error,
        TEXT("Blueprint compilation failed: %s"), *Blueprint->GetPathName());
}

UK2Node_FunctionEntry* AddFunction(
    UBlueprint* Blueprint,
    FName Name,
    UEdGraph*& OutGraph);

void Save(UBlueprint* Blueprint)
{
    Compile(Blueprint);
    UPackage* Package = Blueprint->GetOutermost();
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Package->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    check(UPackage::SavePackage(Package, Blueprint, *Filename, Args));
}

UK2Node_FunctionEntry* AddFunction(
    UBlueprint* Blueprint,
    const FName Name,
    UEdGraph*& OutGraph)
{
    OutGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        Name,
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, OutGraph, false, static_cast<UClass*>(nullptr));
    TArray<UK2Node_FunctionEntry*> Entries;
    OutGraph->GetNodesOfClass(Entries);
    check(Entries.Num() == 1);
    Entries[0]->FindPinChecked(K2Pins::Then)->BreakAllPinLinks();
    return Entries[0];
}

UK2Node_FunctionEntry* AddSignatureFunction(
    UBlueprint* Blueprint,
    const FName Name,
    UFunction* Signature,
    UEdGraph*& OutGraph)
{
    check(Signature);
    OutGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        Name,
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(
        Blueprint, OutGraph, true, Signature);
    TArray<UK2Node_FunctionEntry*> Entries;
    OutGraph->GetNodesOfClass(Entries);
    check(Entries.Num() == 1);
    Entries[0]->FindPinChecked(K2Pins::Then)->BreakAllPinLinks();
    return Entries[0];
}

UK2Node_DynamicCast* AddCast(
    FSocketGraph& Graph,
    UClass* TargetClass,
    UEdGraphPin* Object,
    const int32 Y = 0)
{
    UK2Node_DynamicCast* Cast = Graph.Node(
        NewObject<UK2Node_DynamicCast>(Graph.Graph), Y);
    Cast->TargetType = TargetClass;
    Cast->ReconstructNode();
    Cast->SetPurity(false);
    Graph.Link(Graph.Tail, Graph.Pin(Cast, K2Pins::Execute));
    Graph.Link(Object, Cast->GetCastSourcePin());
    Graph.Tail = Cast->GetValidCastPin();
    return Cast;
}

void AddReadOwnedActorAtFunction(UBlueprint* Blueprint)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddFunction(
        Blueprint, ReadOwnedActorAtFunction, Graph);
    UEdGraphPin* LevelComponent = Entry->CreateUserDefinedPin(
        LevelComponentParameter,
        ObjectPin(UVoyageLevelInstanceComponent::StaticClass()),
        EGPD_Output);
    UEdGraphPin* Index = Entry->CreateUserDefinedPin(
        IndexPin,
        IntPin(),
        EGPD_Output);
    check(LevelComponent && Index);

    FSocketGraph Build(Graph);
    Build.Tail = Build.Pin(Entry, K2Pins::Then);
    UWeakActorArrayGetNode* WeakGet = Build.Node(
        NewObject<UWeakActorArrayGetNode>(Graph));
    Build.Link(
        Build.ReadExternal(
            OwnedActorsProperty,
            UVoyageLevelInstanceComponent::StaticClass(),
            LevelComponent),
        WeakGet->GetArrayPin());
    Build.Link(Index, WeakGet->GetIndexPin());

    UK2Node_FunctionResult* Result = Build.Node(
        NewObject<UK2Node_FunctionResult>(Graph));
    UEdGraphPin* ReturnValue = Result->CreateUserDefinedPin(
        K2Pins::ReturnValue,
        ObjectPin(AActor::StaticClass()),
        EGPD_Input);
    check(ReturnValue);
    Build.Link(Build.Tail, Build.Pin(Result, K2Pins::Execute));
    Build.Link(WeakGet->GetItemPin(), ReturnValue);
}

void RunOwnedActorReadCanary(UBlueprint* Blueprint)
{
    check(Blueprint && Blueprint->GeneratedClass);
    UFunction* ReadFunction = Blueprint->GeneratedClass->FindFunctionByName(
        ReadOwnedActorAtFunction);
    check(ReadFunction);

    UVoyageLevelInstanceComponent* LevelComponent =
        NewObject<UVoyageLevelInstanceComponent>(GetTransientPackage());
    AActor* ActorA = AActor::StaticClass()->GetDefaultObject<AActor>();
    AActor* ActorB =
        AVoyageBoatPawn::StaticClass()->GetDefaultObject<AVoyageBoatPawn>();
    check(LevelComponent && ActorA && ActorB && ActorA != ActorB);
    LevelComponent->OwnedActors = { ActorA, nullptr, ActorB };

    struct FReadOwnedActorAtParameters
    {
        UVoyageLevelInstanceComponent* LevelComponent = nullptr;
        int32 Index = 0;
        AActor* ReturnValue = nullptr;
    };
    const FProperty* LevelComponentField = ReadFunction->FindPropertyByName(
        LevelComponentParameter);
    const FProperty* IndexField = ReadFunction->FindPropertyByName(IndexPin);
    const FProperty* ReturnField = ReadFunction->GetReturnProperty();
    check(LevelComponentField && IndexField && ReturnField);
    check(ReadFunction->ParmsSize == sizeof(FReadOwnedActorAtParameters));
    check(LevelComponentField->GetOffset_ForInternal() ==
        STRUCT_OFFSET(FReadOwnedActorAtParameters, LevelComponent));
    check(IndexField->GetOffset_ForInternal() ==
        STRUCT_OFFSET(FReadOwnedActorAtParameters, Index));
    check(ReturnField->GetOffset_ForInternal() ==
        STRUCT_OFFSET(FReadOwnedActorAtParameters, ReturnValue));

    UObject* ModActorDefaults = Blueprint->GeneratedClass->GetDefaultObject();
    auto ReadAt = [&](const int32 Index)
    {
        FReadOwnedActorAtParameters Parameters;
        Parameters.LevelComponent = LevelComponent;
        Parameters.Index = Index;
        ModActorDefaults->ProcessEvent(ReadFunction, &Parameters);
        return Parameters.ReturnValue;
    };
    checkf(ReadAt(0) == ActorA,
        TEXT("ReadOwnedActorAt canary failed for first weak actor"));
    checkf(ReadAt(1) == nullptr,
        TEXT("ReadOwnedActorAt canary failed for null weak actor"));
    checkf(ReadAt(2) == ActorB,
        TEXT("ReadOwnedActorAt canary failed for second weak actor"));
}

UEdGraphPin* CallOwner(FSocketGraph& Graph, UEdGraphPin* Component, const int32 Y = 0)
{
    UK2Node_CallFunction* GetOwner = Graph.Call(
        UActorComponent::StaticClass(), GetOwnerFunction, Y);
    Graph.Link(Component, Graph.Pin(GetOwner, FunctionTargetPin));
    return Graph.Pin(GetOwner, K2Pins::ReturnValue);
}

UEdGraphPin* ExactClass(
    FSocketGraph& Graph,
    UEdGraphPin* Object,
    UClass* ExpectedClass,
    const int32 Y = 0)
{
    UK2Node_CallFunction* GetClass = Graph.Call(
        UGameplayStatics::StaticClass(), GetObjectClassFunction, Y);
    Graph.Link(Object, Graph.Pin(GetClass, ObjectPinName));
    UK2Node_CallFunction* Equal = Graph.Call(
        UKismetMathLibrary::StaticClass(), EqualClassFunction, Y);
    Graph.Link(Graph.Pin(GetClass, K2Pins::ReturnValue),
        Graph.Pin(Equal, LeftOperandPin));
    Graph.DefaultObject(Equal, RightOperandPin, ExpectedClass);
    return Graph.Pin(Equal, K2Pins::ReturnValue);
}

void AddRegisteredActorEndPlayFunction(UBlueprint* Blueprint)
{
    FMulticastDelegateProperty* EndPlayProperty =
        FindFProperty<FMulticastDelegateProperty>(
            AActor::StaticClass(), OnEndPlayProperty);
    check(EndPlayProperty && EndPlayProperty->SignatureFunction);
    check(EndPlayProperty->SignatureFunction->NumParms == 2);
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddSignatureFunction(
        Blueprint,
        HandleRegisteredActorEndPlayFunction,
        EndPlayProperty->SignatureFunction,
        Graph);
    UEdGraphPin* EndingActor = Entry->FindPinChecked(ActorParameter);
    check(Entry->FindPin(EndPlayReasonParameter));

    FSocketGraph Build(Graph);
    Build.Tail = Build.Pin(Entry, K2Pins::Then);
    UK2Node_CallArrayFunction* FindActor = Build.ArrayCall(ArrayFindFunction);
    Build.Link(Build.Read(RegisteredActorsVariable),
        Build.Pin(FindActor, TargetArrayPin));
    Build.Link(EndingActor, Build.Pin(FindActor, ItemToFindPin));
    UEdGraphPin* FoundIndex = Build.Pin(FindActor, K2Pins::ReturnValue);
    UK2Node_CallFunction* HasIndex = Build.Call(
        UKismetMathLibrary::StaticClass(), GreaterEqualIntFunction);
    Build.Link(FoundIndex, Build.Pin(HasIndex, LeftOperandPin));
    Build.Default(HasIndex, RightOperandPin, ZeroValue);
    Build.Branch(Build.Pin(HasIndex, K2Pins::ReturnValue));

    UK2Node_CallArrayFunction* GetSocket = Build.ArrayCall(ArrayGetFunction);
    Build.Link(Build.Read(RegisteredSocketsVariable),
        Build.Pin(GetSocket, TargetArrayPin));
    Build.Link(FoundIndex, Build.Pin(GetSocket, IndexPin));
    UEdGraphPin* RegisteredSocket = Build.Pin(GetSocket, ItemPin);
    UK2Node_CallArrayFunction* GetOwner = Build.ArrayCall(ArrayGetFunction);
    Build.Link(Build.Read(RegisteredOwnersVariable),
        Build.Pin(GetOwner, TargetArrayPin));
    Build.Link(FoundIndex, Build.Pin(GetOwner, IndexPin));
    UEdGraphPin* ExpectedOwner = Build.Pin(GetOwner, ItemPin);

    UK2Node_ExecutionSequence* CleanupSequence = Build.Node(
        NewObject<UK2Node_ExecutionSequence>(Graph));
    Build.Link(Build.Tail, Build.Pin(CleanupSequence, K2Pins::Execute));

    Build.Tail = CleanupSequence->GetThenPinGivenIndex(0);
    Build.Branch(Build.Valid(RegisteredSocket));
    Build.Branch(Build.Valid(ExpectedOwner));
    UEdGraphPin* CurrentOwner = Build.ReadExternal(
        ModuleOwnerProperty,
        UModuleSocketComponent::StaticClass(),
        RegisteredSocket);
    UK2Node_CallFunction* StillOwned = Build.Call(
        UKismetMathLibrary::StaticClass(), ObjectEqualFunction);
    Build.Link(CurrentOwner, Build.Pin(StillOwned, LeftOperandPin));
    Build.Link(ExpectedOwner, Build.Pin(StillOwned, RightOperandPin));
    Build.Branch(Build.Pin(StillOwned, K2Pins::ReturnValue));
    UK2Node_CallFunction* RemoveExternal = Build.Call(
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            UVoyageModuleComponent,
            RemoveExternalSocket));
    Build.Link(ExpectedOwner, Build.Pin(RemoveExternal, FunctionTargetPin));
    Build.Link(RegisteredSocket, Build.Pin(RemoveExternal, SocketParameter));
    Build.Exec(RemoveExternal);

    Build.Tail = CleanupSequence->GetThenPinGivenIndex(1);
    for (const FName ArrayVariable : {
        RegisteredSocketsVariable,
        RegisteredOwnersVariable,
        RegisteredActorsVariable})
    {
        UK2Node_CallArrayFunction* RemoveAt = Build.ArrayCall(
            ArrayRemoveFunction);
        Build.Link(Build.Read(ArrayVariable),
            Build.Pin(RemoveAt, TargetArrayPin));
        Build.Link(FoundIndex, Build.Pin(RemoveAt, IndexToRemovePin));
        Build.Exec(RemoveAt);
    }
}

void AddHandleActorAttachedFunction(
    UBlueprint* Blueprint,
    UClass* StockSocketClass)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddFunction(
        Blueprint, HandleActorAttachedFunction, Graph);
    UEdGraphPin* Child = Entry->CreateUserDefinedPin(
        ChildParameter,
        ObjectPin(AActor::StaticClass()),
        EGPD_Output);
    UEdGraphPin* ParentComponent = Entry->CreateUserDefinedPin(
        ParentComponentParameter,
        ObjectPin(USceneComponent::StaticClass()),
        EGPD_Output);
    check(Child && ParentComponent);

    FSocketGraph Build(Graph);
    Build.Tail = Build.Pin(Entry, K2Pins::Then);
    Build.Branch(Build.Valid(Child));
    Build.Branch(ExactClass(Build, Child, StockSocketClass));
    UK2Node_DynamicCast* Socket = AddCast(Build, StockSocketClass, Child);
    UEdGraphPin* Module = Build.ReadExternal(
        GET_MEMBER_NAME_CHECKED(AVoyageModuleActor, ModuleComponent),
        AVoyageModuleActor::StaticClass(),
        Socket->GetCastResultPin());
    Build.Branch(Build.Valid(Module));
    UEdGraphPin* StaticMesh = Build.ReadExternal(
        StaticMeshProperty,
        StockSocketClass,
        Socket->GetCastResultPin());
    UK2Node_CallFunction* RootPrimitive = Build.Call(
        UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            UVoyageMiscBlueprintFunctionLibrary,
            GetRootPrimitiveComponent));
    Build.Link(StaticMesh, Build.Pin(RootPrimitive, PrimitiveComponentPin));
    Build.Default(RootPrimitive, IncludeWeldedParentPin, TrueValue);
    UEdGraphPin* RootOwner = CallOwner(
        Build, Build.Pin(RootPrimitive, K2Pins::ReturnValue));
    UK2Node_DynamicCast* Boat = AddCast(
        Build, AVoyageBoatPawn::StaticClass(), RootOwner);

    UK2Node_CallFunction* Process = Build.Call(
        Blueprint->GeneratedClass, ProcessModuleFunction);
    Build.Link(Module, Build.Pin(Process, ModuleParameter));
    Build.Link(Socket->GetCastResultPin(),
        Build.Pin(Process, WallActorParameter));
    Build.Link(Boat->GetCastResultPin(),
        Build.Pin(Process, BoatParameter));
    Build.Exec(Process);
}

void AddProcessFunction(
    UBlueprint* Blueprint,
    UClass* StockSocketClass,
    UClass* StockExternalSocketClass)
{
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddFunction(
        Blueprint, ProcessModuleFunction, Graph);
    UEdGraphPin* Module = Entry->CreateUserDefinedPin(
        ModuleParameter,
        ObjectPin(UVoyageModuleComponent::StaticClass()),
        EGPD_Output);
    UEdGraphPin* WallActor = Entry->CreateUserDefinedPin(
        WallActorParameter,
        ObjectPin(StockSocketClass),
        EGPD_Output);
    UEdGraphPin* Boat = Entry->CreateUserDefinedPin(
        BoatParameter,
        ObjectPin(AVoyageBoatPawn::StaticClass()),
        EGPD_Output);
    check(Module && WallActor && Boat);

    FSocketGraph Build(Graph);
    Build.Tail = Build.Pin(Entry, K2Pins::Then);
    Build.Branch(Build.Valid(Module));
    Build.Branch(Build.Valid(WallActor));
    Build.Branch(Build.Valid(Boat));
    UEdGraphPin* SocketOwner = WallActor;

    UK2Node_CallArrayFunction* AlreadyRegistered = Build.ArrayCall(
        ArrayContainsFunction);
    Build.Link(Build.Read(RegisteredActorsVariable),
        Build.Pin(AlreadyRegistered, TargetArrayPin));
    Build.Link(SocketOwner, Build.Pin(AlreadyRegistered, ItemToFindPin));
    UK2Node_CallFunction* NotRegistered = Build.Call(
        UKismetMathLibrary::StaticClass(), BooleanNotFunction);
    Build.Link(Build.Pin(AlreadyRegistered, K2Pins::ReturnValue),
        Build.Pin(NotRegistered, LeftOperandPin));
    Build.Branch(Build.Pin(NotRegistered, K2Pins::ReturnValue));

    UEdGraphPin* WallView = Build.ReadExternal(
        SocketViewProperty,
        StockSocketClass,
        WallActor);
    Build.Branch(Build.Valid(WallView));

    Build.Write(CandidateOwnerVariable, nullptr, EmptyObjectValue);
    Build.Write(CandidateViewVariable, nullptr, EmptyObjectValue);

    UK2Node_CallFunction* GetLevelComponents = Build.Call(
        AActor::StaticClass(), GetComponentsByClassFunction);
    Build.Link(Boat,
        Build.Pin(GetLevelComponents, FunctionTargetPin));
    Build.DefaultObject(
        GetLevelComponents,
        ComponentClassPin,
        UVoyageLevelInstanceComponent::StaticClass());

    UEdGraph* ForEachWithBreak = FindStandardMacro(ForEachLoopWithBreakMacro);
    check(ForEachWithBreak);
    UK2Node_MacroInstance* ComponentLoop = Build.Node(
        NewObject<UK2Node_MacroInstance>(Graph));
    ComponentLoop->SetMacroGraph(ForEachWithBreak);
    ComponentLoop->ReconstructNode();
    Build.Link(Build.Tail, Build.Pin(ComponentLoop, ExecPin));
    Build.Link(Build.Pin(GetLevelComponents, K2Pins::ReturnValue),
        Build.Pin(ComponentLoop, ArrayPin));

    FSocketGraph Scan(Graph);
    Scan.X = Build.X + 220;
    Scan.Tail = Build.Pin(ComponentLoop, LoopBodyPin);
    UK2Node_DynamicCast* LevelComponent = AddCast(
        Scan,
        UVoyageLevelInstanceComponent::StaticClass(),
        Build.Pin(ComponentLoop, ArrayElementPin));
    UEdGraphPin* OwnedActors = Scan.ReadExternal(
        OwnedActorsProperty,
        UVoyageLevelInstanceComponent::StaticClass(),
        LevelComponent->GetCastResultPin());
    UK2Node_CallArrayFunction* ClearOwnedActorSnapshot = Scan.ArrayCall(
        ArrayClearFunction);
    Scan.Link(Scan.Read(OwnedActorSnapshotVariable),
        Scan.Pin(ClearOwnedActorSnapshot, TargetArrayPin));
    Scan.Exec(ClearOwnedActorSnapshot);

    UK2Node_CallArrayFunction* OwnedActorCount = Scan.ArrayCall(
        ArrayLengthFunction);
    Scan.Link(OwnedActors, Scan.Pin(OwnedActorCount, TargetArrayPin));
    UK2Node_CallFunction* OwnedActorLastIndex = Scan.Call(
        UKismetMathLibrary::StaticClass(), IntSubtractFunction);
    Scan.Link(Scan.Pin(OwnedActorCount, K2Pins::ReturnValue),
        Scan.Pin(OwnedActorLastIndex, LeftOperandPin));
    Scan.Default(OwnedActorLastIndex, RightOperandPin, OneValue);

    UEdGraph* ForLoopGraph = FindStandardMacro(ForLoopMacro);
    check(ForLoopGraph);
    UK2Node_MacroInstance* SnapshotLoop = Scan.Node(
        NewObject<UK2Node_MacroInstance>(Graph));
    SnapshotLoop->SetMacroGraph(ForLoopGraph);
    SnapshotLoop->ReconstructNode();
    Scan.Link(Scan.Tail, Scan.Pin(SnapshotLoop, K2Pins::Execute));
    Scan.Default(SnapshotLoop, FirstIndexPin, ZeroValue);
    Scan.Link(Scan.Pin(OwnedActorLastIndex, K2Pins::ReturnValue),
        Scan.Pin(SnapshotLoop, LastIndexPin));

    FSocketGraph Snapshot(Graph);
    Snapshot.X = Scan.X + 220;
    Snapshot.Tail = Scan.Pin(SnapshotLoop, LoopBodyPin);
    UK2Node_CallFunction* ReadOwnedActor = Snapshot.Call(
        Blueprint->GeneratedClass, ReadOwnedActorAtFunction);
    Snapshot.Link(LevelComponent->GetCastResultPin(),
        Snapshot.Pin(ReadOwnedActor, LevelComponentParameter));
    Snapshot.Link(Scan.Pin(SnapshotLoop, IndexPin),
        Snapshot.Pin(ReadOwnedActor, IndexPin));
    Snapshot.Exec(ReadOwnedActor);
    UEdGraphPin* SnapshotActor = Snapshot.Pin(
        ReadOwnedActor, K2Pins::ReturnValue);
    UK2Node_CallArrayFunction* AddOwnedActor = Snapshot.ArrayCall(
        ArrayAddFunction);
    Snapshot.Link(Snapshot.Read(OwnedActorSnapshotVariable),
        Snapshot.Pin(AddOwnedActor, TargetArrayPin));
    Snapshot.Link(SnapshotActor, Snapshot.Pin(AddOwnedActor, NewItemPin));
    Snapshot.Exec(AddOwnedActor);

    UK2Node_MacroInstance* ActorLoop = Scan.Node(
        NewObject<UK2Node_MacroInstance>(Graph));
    ActorLoop->SetMacroGraph(ForEachWithBreak);
    ActorLoop->ReconstructNode();
    Scan.Link(Scan.Pin(SnapshotLoop, CompletedPin),
        Scan.Pin(ActorLoop, ExecPin));
    Scan.Link(Scan.Read(OwnedActorSnapshotVariable),
        Scan.Pin(ActorLoop, ArrayPin));

    FSocketGraph Candidate(Graph);
    Candidate.X = Scan.X + 220;
    Candidate.Tail = Scan.Pin(ActorLoop, LoopBodyPin);
    UEdGraphPin* ExternalActor = Scan.Pin(ActorLoop, ArrayElementPin);
    Candidate.Branch(ExactClass(
        Candidate,
        ExternalActor,
        StockExternalSocketClass));
    UK2Node_DynamicCast* ExternalSocket = AddCast(
        Candidate,
        StockExternalSocketClass,
        ExternalActor);
    UEdGraphPin* ReferenceView = Candidate.ReadExternal(
        SocketViewProperty,
        AVoyageVirtualActorSocket::StaticClass(),
        ExternalSocket->GetCastResultPin());
    Candidate.Branch(Candidate.Valid(ReferenceView));
    UK2Node_CallFunction* GetReferenceParent = Candidate.Call(
        StockExternalSocketClass, GetParentModuleFunction);
    Candidate.Link(ExternalSocket->GetCastResultPin(),
        Candidate.Pin(GetReferenceParent, FunctionTargetPin));
    Candidate.Exec(GetReferenceParent);
    UEdGraphPin* ReferenceOwnerActor = Candidate.Pin(
        GetReferenceParent, ActorParameter);
    Candidate.Branch(Candidate.Valid(ReferenceOwnerActor));
    UK2Node_CallArrayFunction* OwnerIsOwned = Candidate.ArrayCall(
        ArrayContainsFunction);
    Candidate.Link(Candidate.Read(OwnedActorSnapshotVariable),
        Candidate.Pin(OwnerIsOwned, TargetArrayPin));
    Candidate.Link(ReferenceOwnerActor,
        Candidate.Pin(OwnerIsOwned, ItemToFindPin));
    Candidate.Branch(Candidate.Pin(OwnerIsOwned, K2Pins::ReturnValue));
    UK2Node_CallFunction* HasElectricTag = Candidate.Call(
        AActor::StaticClass(), ActorHasTagFunction);
    Candidate.Link(ReferenceOwnerActor,
        Candidate.Pin(HasElectricTag, FunctionTargetPin));
    Candidate.Default(HasElectricTag, TagPin, BoatElectricTag.ToString());
    Candidate.Branch(Candidate.Pin(HasElectricTag, K2Pins::ReturnValue));

    UK2Node_CallFunction* ResolveReferenceOwner = Candidate.Call(
        UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
        GetModuleFromActorFunction);
    Candidate.Link(ReferenceOwnerActor,
        Candidate.Pin(ResolveReferenceOwner, ActorParameter));
    Candidate.Exec(ResolveReferenceOwner);
    UEdGraphPin* ReferenceOwner = Candidate.Pin(
        ResolveReferenceOwner, K2Pins::ReturnValue);
    Candidate.Branch(Candidate.Valid(ReferenceOwner));
    Candidate.Write(CandidateOwnerVariable, ReferenceOwner, FString());
    Candidate.Write(CandidateViewVariable, ReferenceView, FString());
    Candidate.Link(Candidate.Tail, Candidate.Pin(ActorLoop, BreakPin));

    FSocketGraph FinishComponent(Graph);
    FinishComponent.X = Candidate.X + 220;
    FinishComponent.Tail = Scan.Pin(ActorLoop, CompletedPin);
    UK2Node_IfThenElse* ReferenceFound = FinishComponent.Branch(
        FinishComponent.Valid(
            FinishComponent.Read(CandidateOwnerVariable)));
    FinishComponent.Link(
        ReferenceFound->GetThenPin(),
        FinishComponent.Pin(ComponentLoop, BreakPin));

    FSocketGraph Apply(Graph);
    Apply.X = FMath::Max(
        Build.X,
        FMath::Max(Scan.X, FinishComponent.X)) + 440;
    Apply.Tail = Build.Pin(ComponentLoop, CompletedPin);
    UEdGraphPin* BatteryModule = Apply.Read(CandidateOwnerVariable);
    UEdGraphPin* ReferenceSocket = Apply.Read(CandidateViewVariable);
    UK2Node_IfThenElse* OwnerResolved = Apply.Branch(
        Apply.Valid(BatteryModule));
    UEdGraphPin* MissingReferenceOwner = OwnerResolved->GetElsePin();
    UK2Node_IfThenElse* ViewResolved = Apply.Branch(
        Apply.Valid(ReferenceSocket));
    UEdGraphPin* MissingReferenceView = ViewResolved->GetElsePin();
    check(MissingReferenceOwner && MissingReferenceView);

    UK2Node_CallFunction* GetWallActorName = Apply.Call(
        UKismetSystemLibrary::StaticClass(), GetObjectNameFunction);
    Apply.Link(SocketOwner,
        Apply.Pin(GetWallActorName, ObjectPinName));
    UK2Node_CallFunction* WallActorName = Apply.Call(
        UKismetStringLibrary::StaticClass(), ConvertStringToNameFunction);
    Apply.Link(Apply.Pin(GetWallActorName, K2Pins::ReturnValue),
        Apply.Pin(WallActorName, InStringPin));
    UK2Node_CallFunction* SetSocketId = Apply.Call(
        UModuleSocketComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UModuleSocketComponent, SetSocketID));
    Apply.Link(WallView, Apply.Pin(SetSocketId, FunctionTargetPin));
    Apply.Link(Apply.Pin(WallActorName, K2Pins::ReturnValue),
        Apply.Pin(SetSocketId, NamePin));
    Apply.Exec(SetSocketId);

    Apply.WriteExternal(
        PortProperty,
        UModuleSocketComponent::StaticClass(),
        WallView,
        Apply.ReadExternal(
            PortProperty,
            UModuleSocketComponent::StaticClass(),
            ReferenceSocket),
        FString());
    for (const FName PropertyName : {
        AutoInitializeProperty,
        AddModuleRequirementProperty,
        IsVirtualProperty,
        IsVirtualGroupedProperty,
        IsVirtualShareToGroupProperty})
    {
        Apply.WriteExternal(
            PropertyName,
            UModuleSocketComponent::StaticClass(),
            WallView,
            Apply.ReadExternal(
                PropertyName,
                UModuleSocketComponent::StaticClass(),
                ReferenceSocket),
            FString());
    }

    UK2Node_ExecutionSequence* RegistrationSequence = Apply.Node(
        NewObject<UK2Node_ExecutionSequence>(Graph));
    Apply.Link(Apply.Tail,
        Apply.Pin(RegistrationSequence, K2Pins::Execute));

    // First sequence branch: clear the peer end when a valid stock pair exists.
    Apply.Tail = RegistrationSequence->GetThenPinGivenIndex(0);
    UEdGraphPin* PairedSoftObject = Apply.ReadExternal(
        PairedModuleProperty,
        StockSocketClass,
        WallActor,
        -160);
    UK2Node_CallFunction* PairObject = Apply.Call(
        UKismetSystemLibrary::StaticClass(), ConvertSoftObjectFunction, -160);
    Apply.Link(PairedSoftObject, Apply.Pin(PairObject, SoftObjectPinName));
    UK2Node_DynamicCast* PairModule = AddCast(
        Apply,
        UVoyageModuleComponent::StaticClass(),
        Apply.Pin(PairObject, K2Pins::ReturnValue),
        -160);
    UEdGraphPin* PairOwner = CallOwner(
        Apply, PairModule->GetCastResultPin(), -160);
    UK2Node_DynamicCast* PeerSocket = AddCast(
        Apply, StockSocketClass, PairOwner, -160);
    Apply.WriteExternal(
        PairedModuleProperty,
        StockSocketClass,
        PeerSocket->GetCastResultPin(),
        nullptr,
        EmptyObjectValue,
        -160);
    UK2Node_CallFunction* RemovePeerWallSocket = Apply.Call(
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            UVoyageModuleComponent,
            RemoveSecondaryGroup),
        -160);
    Apply.Link(PairModule->GetCastResultPin(),
        Apply.Pin(RemovePeerWallSocket, FunctionTargetPin));
    Apply.Default(RemovePeerWallSocket, GroupNamePin, WallSocketGroup.ToString());
    Apply.Exec(RemovePeerWallSocket);

    // Second sequence branch: clear this end, remove legacy groups, move the
    // exact wall view into the first suitable Boat-owned stock electric
    // external-port owner, and register
    // end-play cleanup.
    Apply.Tail = RegistrationSequence->GetThenPinGivenIndex(1);
    Apply.WriteExternal(
        PairedModuleProperty,
        StockSocketClass,
        WallActor,
        nullptr,
        EmptyObjectValue,
        160);
    UK2Node_CallFunction* RemoveOwnWallSocket = Apply.Call(
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            UVoyageModuleComponent,
            RemoveSecondaryGroup),
        160);
    Apply.Link(Module, Apply.Pin(RemoveOwnWallSocket, FunctionTargetPin));
    Apply.Default(RemoveOwnWallSocket, GroupNamePin, WallSocketGroup.ToString());
    Apply.Exec(RemoveOwnWallSocket);

    UK2Node_CallFunction* CurrentBoatGroup = Apply.Call(
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            UVoyageModuleComponent,
            GetSecondaryGroupId),
        160);
    Apply.Link(Module, Apply.Pin(CurrentBoatGroup, FunctionTargetPin));
    Apply.Default(CurrentBoatGroup, GroupNamePin, BoatGroup.ToString());
    UK2Node_CallFunction* HasLegacyBoatGroup = Apply.Call(
        UKismetMathLibrary::StaticClass(), NotEqualIntFunction, 160);
    Apply.Link(Apply.Pin(CurrentBoatGroup, K2Pins::ReturnValue),
        Apply.Pin(HasLegacyBoatGroup, LeftOperandPin));
    Apply.Default(HasLegacyBoatGroup, RightOperandPin, ZeroValue);
    UK2Node_ExecutionSequence* LegacySequence = Apply.Node(
        NewObject<UK2Node_ExecutionSequence>(Graph), 160);
    Apply.Link(Apply.Tail, Apply.Pin(LegacySequence, K2Pins::Execute));
    Apply.Tail = LegacySequence->GetThenPinGivenIndex(0);
    Apply.Branch(Apply.Pin(HasLegacyBoatGroup, K2Pins::ReturnValue), 160);
    UK2Node_CallFunction* RemoveLegacyBoatGroup = Apply.Call(
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            UVoyageModuleComponent,
            RemoveSecondaryGroup),
        160);
    Apply.Link(Module,
        Apply.Pin(RemoveLegacyBoatGroup, FunctionTargetPin));
    Apply.Default(
        RemoveLegacyBoatGroup,
        GroupNamePin,
        BoatGroup.ToString());
    Apply.Exec(RemoveLegacyBoatGroup);

    Apply.Tail = LegacySequence->GetThenPinGivenIndex(1);
    UK2Node_CallFunction* DisableStockTick = Apply.Call(
        AActor::StaticClass(), SetActorTickEnabledFunction, 160);
    Apply.Link(SocketOwner, Apply.Pin(DisableStockTick, FunctionTargetPin));
    Apply.Default(DisableStockTick, EnabledPin, FalseValue);
    Apply.Exec(DisableStockTick);

    UK2Node_CallFunction* AddExternalSocket = Apply.Call(
        UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            UVoyageModuleComponent,
            AddExternalSocket),
        160);
    Apply.Link(BatteryModule,
        Apply.Pin(AddExternalSocket, FunctionTargetPin));
    Apply.Link(WallView, Apply.Pin(AddExternalSocket, SocketParameter));
    Apply.Exec(AddExternalSocket);

    for (const TPair<FName, UEdGraphPin*> Registration : {
        TPair<FName, UEdGraphPin*>(RegisteredActorsVariable, SocketOwner),
        TPair<FName, UEdGraphPin*>(RegisteredSocketsVariable, WallView),
        TPair<FName, UEdGraphPin*>(RegisteredOwnersVariable, BatteryModule)})
    {
        UK2Node_CallArrayFunction* AddRegistration = Apply.ArrayCall(
            ArrayAddFunction,
            160);
        Apply.Link(Apply.Read(Registration.Key, 160),
            Apply.Pin(AddRegistration, TargetArrayPin));
        Apply.Link(Registration.Value,
            Apply.Pin(AddRegistration, NewItemPin));
        Apply.Exec(AddRegistration);
    }

    FMulticastDelegateProperty* EndPlayProperty =
        FindFProperty<FMulticastDelegateProperty>(
            AActor::StaticClass(), OnEndPlayProperty);
    check(EndPlayProperty && EndPlayProperty->SignatureFunction);
    UK2Node_CreateDelegate* EndPlayCallback = Apply.Node(
        NewObject<UK2Node_CreateDelegate>(Graph), 160);
    EndPlayCallback->SetFunction(HandleRegisteredActorEndPlayFunction);
    UK2Node_AddDelegate* BindEndPlay = NewObject<UK2Node_AddDelegate>(Graph);
    BindEndPlay->SetFromProperty(
        EndPlayProperty, false, AActor::StaticClass());
    Apply.Node(BindEndPlay, 160);
    Apply.Link(Apply.Tail, Apply.Pin(BindEndPlay, K2Pins::Execute));
    Apply.Link(SocketOwner,
        Apply.Pin(BindEndPlay, FunctionTargetPin));
    Apply.Link(EndPlayCallback->GetDelegateOutPin(),
        Apply.Pin(BindEndPlay, EventPin));
    Apply.Tail = Apply.Pin(BindEndPlay, K2Pins::Then);
}

void AddLifecycle(UBlueprint* Blueprint)
{
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint,
        EventGraphName,
        UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, Graph);
    UFunction* AttachedFunction = Blueprint->GeneratedClass->FindFunctionByName(
        HandleActorAttachedFunction);
    check(AttachedFunction);

    FMulticastDelegateProperty* AttachedProperty =
        FindFProperty<FMulticastDelegateProperty>(
            UVoyagePersistentSubsystem::StaticClass(),
            OnActorAttachedProperty);
    check(AttachedProperty);
    check(AttachedProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable));
    UFunction* AttachedSignature = AttachedProperty->SignatureFunction;
    check(AttachedSignature && AttachedSignature->NumParms == 2);
    check(AttachedSignature->ParmsSize == 16);
    FObjectPropertyBase* ChildProperty =
        FindFProperty<FObjectPropertyBase>(AttachedSignature, ChildParameter);
    FObjectPropertyBase* ParentComponentProperty =
        FindFProperty<FObjectPropertyBase>(
            AttachedSignature, ParentComponentParameter);
    check(ChildProperty && ChildProperty->PropertyClass == AActor::StaticClass());
    check(ChildProperty->GetOffset_ForInternal() == 0);
    check(ParentComponentProperty &&
        ParentComponentProperty->PropertyClass == USceneComponent::StaticClass());
    check(ParentComponentProperty->GetOffset_ForInternal() == 8);

    UK2Node_Event* BeginPlay = NewObject<UK2Node_Event>(Graph);
    BeginPlay->EventReference.SetExternalMember(
        ReceiveBeginPlayEvent, AActor::StaticClass());
    BeginPlay->bOverrideFunction = true;
    FSocketGraph Begin(Graph);
    Begin.Node(BeginPlay, 0);
    Begin.Tail = Begin.Pin(BeginPlay, K2Pins::Then);

    UK2Node_CallFunction* GetPersistentSubsystem = Begin.Call(
        USubsystemBlueprintLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(
            USubsystemBlueprintLibrary,
            GetWorldSubsystem));
    Begin.DefaultObject(
        GetPersistentSubsystem,
        SubsystemClassPin,
        UVoyagePersistentSubsystem::StaticClass());
    UK2Node_DynamicCast* PersistentSubsystem = Begin.Node(
        NewObject<UK2Node_DynamicCast>(Graph));
    PersistentSubsystem->TargetType = UVoyagePersistentSubsystem::StaticClass();
    PersistentSubsystem->ReconstructNode();
    PersistentSubsystem->SetPurity(false);
    Begin.Link(Begin.Tail,
        Begin.Pin(PersistentSubsystem, K2Pins::Execute));
    Begin.Link(Begin.Pin(GetPersistentSubsystem, K2Pins::ReturnValue),
        PersistentSubsystem->GetCastSourcePin());

    UK2Node_CreateDelegate* AttachedCallback = Begin.Node(
        NewObject<UK2Node_CreateDelegate>(Graph));
    AttachedCallback->SetFunction(HandleActorAttachedFunction);
    UK2Node_AddDelegate* BindAttached = NewObject<UK2Node_AddDelegate>(Graph);
    BindAttached->SetFromProperty(
        AttachedProperty, false, UVoyagePersistentSubsystem::StaticClass());
    Begin.Node(BindAttached);
    Begin.Link(PersistentSubsystem->GetValidCastPin(),
        Begin.Pin(BindAttached, K2Pins::Execute));
    Begin.Link(PersistentSubsystem->GetCastResultPin(),
        Begin.Pin(BindAttached, FunctionTargetPin));
    Begin.Link(AttachedCallback->GetDelegateOutPin(),
        BindAttached->GetDelegatePin());
}
UBlueprint* CreateStockSocketStub()
{
    UBlueprint* Stock = CreateBlueprint(
        StockSocketPackage,
        StockSocketAsset,
        AVoyageModuleActor::StaticClass(),
        StockStubBlueprintName);
    check(FBlueprintEditorUtils::AddMemberVariable(
        Stock,
        PairedModuleProperty,
        SoftObjectPin(UVoyageModuleComponent::StaticClass())));
    check(FBlueprintEditorUtils::AddMemberVariable(
        Stock,
        StaticMeshProperty,
        ObjectPin(UStaticMeshComponent::StaticClass())));
    check(FBlueprintEditorUtils::AddMemberVariable(
        Stock,
        SocketViewProperty,
        ObjectPin(UVoyageModuleSocketViewComponent::StaticClass())));
    Save(Stock);
    return Stock;
}

UBlueprint* CreateExternalSocketStub()
{
    UBlueprint* Stock = CreateBlueprint(
        StockExternalSocketPackage,
        StockExternalSocketAsset,
        AVoyageVirtualActorSocket::StaticClass(),
        ExternalStubBlueprintName);
    UEdGraph* Graph = nullptr;
    UK2Node_FunctionEntry* Entry = AddFunction(
        Stock, GetParentModuleFunction, Graph);
    Entry->AddExtraFlags(
        FUNC_Public | FUNC_BlueprintCallable | FUNC_BlueprintEvent);
    FSocketGraph Build(Graph);
    UK2Node_FunctionResult* Result = Build.Node(
        NewObject<UK2Node_FunctionResult>(Graph));
    UEdGraphPin* ParentActor = Result->CreateUserDefinedPin(
        ActorParameter,
        ObjectPin(AActor::StaticClass()),
        EGPD_Input);
    check(ParentActor);
    Build.Link(
        Build.Pin(Entry, K2Pins::Then),
        Build.Pin(Result, K2Pins::Execute));
    Save(Stock);
    return Stock;
}

UBlueprint* CreateModActor(
    UClass* StockSocketClass,
    UClass* StockExternalSocketClass)
{
    // UHT intentionally rejects Blueprint-exposed weak-object arrays. Preserve
    // the exact native FWeakObjectProperty layout in the mirror and expose it
    // only inside this editor process before graph construction.
    ExposeVoyageLevelInstanceOwnedActorsToBlueprint();
    UBlueprint* ModActor = CreateBlueprint(
        ModActorPackage,
        ModActorAsset,
        AActor::StaticClass(),
        ModActorBlueprintName);
    AddTransientObjectArrayVariable(
        ModActor,
        RegisteredActorsVariable,
        AActor::StaticClass());
    AddTransientObjectArrayVariable(
        ModActor,
        RegisteredSocketsVariable,
        UModuleSocketComponent::StaticClass());
    AddTransientObjectArrayVariable(
        ModActor,
        RegisteredOwnersVariable,
        UVoyageModuleComponent::StaticClass());
    AddTransientObjectArrayVariable(
        ModActor,
        OwnedActorSnapshotVariable,
        AActor::StaticClass());
    AddTransientVariable(
        ModActor,
        CandidateOwnerVariable,
        ObjectPin(UVoyageModuleComponent::StaticClass()));
    AddTransientVariable(
        ModActor,
        CandidateViewVariable,
        ObjectPin(UVoyageModuleSocketViewComponent::StaticClass()));

    AddRegisteredActorEndPlayFunction(ModActor);
    AddReadOwnedActorAtFunction(ModActor);
    Compile(ModActor);
    AddProcessFunction(
        ModActor,
        StockSocketClass,
        StockExternalSocketClass);
    Compile(ModActor);
    AddHandleActorAttachedFunction(ModActor, StockSocketClass);
    Compile(ModActor);
    AddLifecycle(ModActor);
    Compile(ModActor);
    RunOwnedActorReadCanary(ModActor);
    UE_LOG(LogTemp, Display,
        TEXT("OwnedActors Blueprint by-reference read canary passed"));

    AActor* Defaults = CastChecked<AActor>(
        ModActor->GeneratedClass->GetDefaultObject());
    Defaults->PrimaryActorTick.bCanEverTick = false;
    Defaults->PrimaryActorTick.bStartWithTickEnabled = false;
    Defaults->PrimaryActorTick.TickInterval = 0.0f;
    Save(ModActor);
    return ModActor;
}
}

UGenerateElectrifiedBoatCommandlet::
    UGenerateElectrifiedBoatCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateElectrifiedBoatCommandlet::Main(const FString& Params)
{
    using namespace ElectrifiedBoat;
    if (FPackageName::DoesPackageExist(StockSocketPackage) ||
        FPackageName::DoesPackageExist(StockExternalSocketPackage) ||
        FPackageName::DoesPackageExist(ModActorPackage))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ElectrifiedBoat generated assets already exist"));
        return 1;
    }

    UBlueprint* StockSocket = CreateStockSocketStub();
    check(StockSocket && StockSocket->GeneratedClass);
    UBlueprint* StockExternalSocket = CreateExternalSocketStub();
    check(StockExternalSocket && StockExternalSocket->GeneratedClass);
    UBlueprint* ModActor = CreateModActor(
        StockSocket->GeneratedClass,
        StockExternalSocket->GeneratedClass);
    check(ModActor && ModActor->GeneratedClass);

    UE_LOG(LogTemp, Display,
        TEXT("Generated additive ElectrifiedBoat ModActor"));
    return 0;
}

#endif
