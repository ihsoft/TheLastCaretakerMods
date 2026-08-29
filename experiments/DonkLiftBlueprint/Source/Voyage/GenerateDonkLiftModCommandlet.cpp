#include "GenerateDonkLiftModCommandlet.h"

#if WITH_EDITOR

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "VoyageVehicleForkliftPawn.h"

namespace
{
constexpr TCHAR PackageName[] = TEXT("/Game/Mods/DonkLiftKeyboardControl/ModActor");
constexpr TCHAR AssetName[] = TEXT("ModActor");

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

UK2Node_CallFunction* AddCall(UEdGraph* Graph, UFunction* Function, int32 X, int32 Y)
{
    UK2Node_CallFunction* Node = NewObject<UK2Node_CallFunction>(Graph);
    Node->SetFromFunction(Function);
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableGet* AddVariableGet(
    UEdGraph* Graph,
    const FName VariableName,
    UClass* ExternalClass,
    int32 X,
    int32 Y)
{
    UK2Node_VariableGet* Node = NewObject<UK2Node_VariableGet>(Graph);
    if (ExternalClass)
    {
        Node->VariableReference.SetExternalMember(VariableName, ExternalClass);
    }
    else
    {
        Node->VariableReference.SetSelfMember(VariableName);
    }
    return FinishNode(Node, Graph, X, Y);
}

UK2Node_VariableSet* AddVariableSet(
    UEdGraph* Graph,
    const FName VariableName,
    UClass* ExternalClass,
    int32 X,
    int32 Y)
{
    UK2Node_VariableSet* Node = NewObject<UK2Node_VariableSet>(Graph);
    if (ExternalClass)
    {
        Node->VariableReference.SetExternalMember(VariableName, ExternalClass);
    }
    else
    {
        Node->VariableReference.SetSelfMember(VariableName);
    }
    return FinishNode(Node, Graph, X, Y);
}

UEdGraphPin* RequirePin(UEdGraphNode* Node, const FName Name)
{
    UEdGraphPin* Pin = Node->FindPin(Name);
    checkf(Pin, TEXT("Missing pin '%s' on node '%s'"), *Name.ToString(), *Node->GetName());
    return Pin;
}

void SetDefault(UEdGraphNode* Node, const FName Name, const TCHAR* Value)
{
    RequirePin(Node, Name)->DefaultValue = Value;
}

bool Connect(const UEdGraphSchema_K2* Schema, UEdGraphPin* A, UEdGraphPin* B)
{
    if (!Schema->TryCreateConnection(A, B))
    {
        UE_LOG(LogTemp, Error, TEXT("Could not connect %s.%s to %s.%s"),
            *A->GetOwningNode()->GetName(), *A->PinName.ToString(),
            *B->GetOwningNode()->GetName(), *B->PinName.ToString());
        return false;
    }
    return true;
}

UFunction* MathFunction(const TCHAR* Name)
{
    return UKismetMathLibrary::StaticClass()->FindFunctionByName(FName(Name));
}
}

UGenerateDonkLiftModCommandlet::UGenerateDonkLiftModCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
    ShowErrorCount = true;
}

int32 UGenerateDonkLiftModCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(PackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("Asset already exists: %s"), PackageName);
        return 1;
    }

    UPackage* Package = CreatePackage(PackageName);
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        AActor::StaticClass(),
        Package,
        FName(AssetName),
        BPTYPE_Normal,
        UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        FName(TEXT("GenerateDonkLiftMod")));

    if (!Blueprint)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create %s"), PackageName);
        return 1;
    }

    FEdGraphPinType DoubleType;
    DoubleType.PinCategory = UEdGraphSchema_K2::PC_Real;
    DoubleType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    if (!FBlueprintEditorUtils::AddMemberVariable(
            Blueprint,
            FName(TEXT("IntegratedThrottle")),
            DoubleType,
            TEXT("0.0")))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to add IntegratedThrottle"));
        return 1;
    }

    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph)
    {
        UE_LOG(LogTemp, Error, TEXT("Event graph was not created"));
        return 1;
    }

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

    UK2Node_Event* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(FName(TEXT("ReceiveTick")), AActor::StaticClass());
    Tick->bOverrideFunction = true;
    FinishNode(Tick, Graph, 0, 0);

    UK2Node_CallFunction* GetPawn = AddCall(
        Graph,
        UGameplayStatics::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerPawn)),
        220,
        180);
    SetDefault(GetPawn, FName(TEXT("PlayerIndex")), TEXT("0"));

    UK2Node_DynamicCast* CastPawn = NewObject<UK2Node_DynamicCast>(Graph);
    CastPawn->TargetType = AVoyageVehicleForkliftPawn::StaticClass();
    FinishNode(CastPawn, Graph, 450, 0);
    CastPawn->SetPurity(false);

    UK2Node_VariableGet* RawThrottle = AddVariableGet(
        Graph,
        FName(TEXT("ThrottleInput")),
        AVoyageVehicleForkliftPawn::StaticClass(),
        700,
        220);
    UK2Node_VariableGet* CurrentThrottle = AddVariableGet(
        Graph,
        FName(TEXT("IntegratedThrottle")),
        nullptr,
        1180,
        500);

    UK2Node_CallFunction* IsForward = AddCall(Graph, MathFunction(TEXT("EqualEqual_DoubleDouble")), 940, 120);
    UK2Node_CallFunction* IsReverse = AddCall(Graph, MathFunction(TEXT("EqualEqual_DoubleDouble")), 940, 300);
    SetDefault(IsForward, FName(TEXT("B")), TEXT("1.0"));
    SetDefault(IsReverse, FName(TEXT("B")), TEXT("-1.0"));

    UK2Node_CallFunction* ForwardDirection = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 1190, 80);
    UK2Node_CallFunction* ReverseDirection = AddCall(Graph, MathFunction(TEXT("SelectFloat")), 1190, 260);
    SetDefault(ForwardDirection, FName(TEXT("A")), TEXT("1.0"));
    SetDefault(ForwardDirection, FName(TEXT("B")), TEXT("0.0"));
    SetDefault(ReverseDirection, FName(TEXT("A")), TEXT("-1.0"));
    SetDefault(ReverseDirection, FName(TEXT("B")), TEXT("0.0"));

    UK2Node_CallFunction* AddDirection = AddCall(Graph, MathFunction(TEXT("Add_DoubleDouble")), 1450, 170);
    UK2Node_CallFunction* ScaleRate = AddCall(Graph, MathFunction(TEXT("Multiply_DoubleDouble")), 1680, 170);
    UK2Node_CallFunction* ScaleTime = AddCall(Graph, MathFunction(TEXT("Multiply_DoubleDouble")), 1900, 170);
    UK2Node_CallFunction* AddCurrent = AddCall(Graph, MathFunction(TEXT("Add_DoubleDouble")), 2130, 280);
    UK2Node_CallFunction* Clamp = AddCall(Graph, MathFunction(TEXT("FClamp")), 2360, 280);
    SetDefault(ScaleRate, FName(TEXT("B")), TEXT("0.3333333333333333"));
    SetDefault(Clamp, FName(TEXT("Min")), TEXT("-0.9999"));
    SetDefault(Clamp, FName(TEXT("Max")), TEXT("0.9999"));

    UK2Node_VariableSet* SetIntegrated = AddVariableSet(
        Graph,
        FName(TEXT("IntegratedThrottle")),
        nullptr,
        2620,
        0);
    UK2Node_VariableSet* SetThrottle = AddVariableSet(
        Graph,
        FName(TEXT("ThrottleInput")),
        AVoyageVehicleForkliftPawn::StaticClass(),
        2870,
        0);

    bool Ok = true;
    Ok &= Connect(Schema, RequirePin(Tick, UEdGraphSchema_K2::PN_Then), RequirePin(CastPawn, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, GetPawn->GetReturnValuePin(), CastPawn->GetCastSourcePin());
    Ok &= Connect(Schema, CastPawn->GetValidCastPin(), RequirePin(SetIntegrated, UEdGraphSchema_K2::PN_Execute));
    Ok &= Connect(Schema, RequirePin(SetIntegrated, UEdGraphSchema_K2::PN_Then), RequirePin(SetThrottle, UEdGraphSchema_K2::PN_Execute));

    Ok &= Connect(Schema, CastPawn->GetCastResultPin(), RequirePin(RawThrottle, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, CastPawn->GetCastResultPin(), RequirePin(SetThrottle, UEdGraphSchema_K2::PN_Self));
    Ok &= Connect(Schema, RawThrottle->GetValuePin(), RequirePin(IsForward, FName(TEXT("A"))));
    Ok &= Connect(Schema, RawThrottle->GetValuePin(), RequirePin(IsReverse, FName(TEXT("A"))));
    Ok &= Connect(Schema, IsForward->GetReturnValuePin(), RequirePin(ForwardDirection, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, IsReverse->GetReturnValuePin(), RequirePin(ReverseDirection, FName(TEXT("bPickA"))));
    Ok &= Connect(Schema, ForwardDirection->GetReturnValuePin(), RequirePin(AddDirection, FName(TEXT("A"))));
    Ok &= Connect(Schema, ReverseDirection->GetReturnValuePin(), RequirePin(AddDirection, FName(TEXT("B"))));
    Ok &= Connect(Schema, AddDirection->GetReturnValuePin(), RequirePin(ScaleRate, FName(TEXT("A"))));
    Ok &= Connect(Schema, ScaleRate->GetReturnValuePin(), RequirePin(ScaleTime, FName(TEXT("A"))));
    Ok &= Connect(Schema, RequirePin(Tick, FName(TEXT("DeltaSeconds"))), RequirePin(ScaleTime, FName(TEXT("B"))));
    Ok &= Connect(Schema, ScaleTime->GetReturnValuePin(), RequirePin(AddCurrent, FName(TEXT("B"))));
    Ok &= Connect(Schema, CurrentThrottle->GetValuePin(), RequirePin(AddCurrent, FName(TEXT("A"))));
    Ok &= Connect(Schema, AddCurrent->GetReturnValuePin(), RequirePin(Clamp, FName(TEXT("Value"))));
    Ok &= Connect(Schema, Clamp->GetReturnValuePin(), RequirePin(SetIntegrated, FName(TEXT("IntegratedThrottle"))));
    Ok &= Connect(Schema, Clamp->GetReturnValuePin(), RequirePin(SetThrottle, FName(TEXT("ThrottleInput"))));

    if (!Ok)
    {
        return 1;
    }

    UK2Node_Event* BeginPlay = NewObject<UK2Node_Event>(Graph);
    BeginPlay->EventReference.SetExternalMember(FName(TEXT("ReceiveBeginPlay")), AActor::StaticClass());
    BeginPlay->bOverrideFunction = true;
    FinishNode(BeginPlay, Graph, 0, 720);

    UK2Node_CallFunction* PrintLoaded = AddCall(
        Graph,
        UKismetSystemLibrary::StaticClass()->FindFunctionByName(
            GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, PrintString)),
        260,
        720);
    SetDefault(PrintLoaded, FName(TEXT("InString")), TEXT("DonkLift Blueprint mod loaded"));
    SetDefault(PrintLoaded, FName(TEXT("Duration")), TEXT("5.0"));
    Ok &= Connect(Schema, RequirePin(BeginPlay, UEdGraphSchema_K2::PN_Then), RequirePin(PrintLoaded, UEdGraphSchema_K2::PN_Execute));

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error)
    {
        UE_LOG(LogTemp, Error, TEXT("Blueprint compilation failed"));
        return 1;
    }

    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        PackageName,
        FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save %s"), *Filename);
        return 1;
    }

    UE_LOG(LogTemp, Display, TEXT("Generated %s"), PackageName);
    return 0;
}

#endif
