#include "GenerateHarpoonProbeCommandlet.h"
#include "ProbeNames.h"
#include "BlueprintGraphNames.h"
#include "ActorLifecycleGraphNames.h"
#include "Modules/ModuleManager.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "EdGraphSchema_K2.h"
#include "GameFramework/Actor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ClassDynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "UObject/SavePackage.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, HarpoonProbe)
namespace P = BlueprintGraphNames::Pins;
namespace E = ActorLifecycleGraphNames;
namespace N = ProbeNames;

namespace
{
class FGraph
{
public:
    UEdGraph* Graph;
    UEdGraphPin* Tail = nullptr;
    int32 X = 0;
    explicit FGraph(UEdGraph* InGraph) : Graph(InGraph) {}
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
        auto* Fn = Owner->FindFunctionByName(Function); check(Fn);
        checkf(Fn->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure),
            TEXT("Function is not Blueprint-callable: %s"), *Function.ToString());
        auto* Out = NewObject<UK2Node_CallFunction>(Graph); Out->SetFromFunction(Fn);
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
    void Text(FName Component, const TCHAR* Value, UEdGraphPin* DynamicText = nullptr)
    {
        auto* Set = Call(UTextRenderComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UTextRenderComponent, K2_SetText));
        Link(Read(Component), Pin(Set, P::FunctionTarget));
        if (DynamicText) Link(DynamicText, Pin(Set, E::Text));
        else
        {
            auto* Literal = Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
            Default(Literal, E::StringValue, Value);
            Link(Pin(Literal, P::ReturnValue), Pin(Set, E::Text));
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

void AddText(UBlueprint* BP, USCS_Node* Root, FName Name, const TCHAR* Text, int Row)
{
    auto* Node = BP->SimpleConstructionScript->CreateNode(UTextRenderComponent::StaticClass(), Name);
    auto* Component = CastChecked<UTextRenderComponent>(Node->ComponentTemplate);
    Component->SetText(FText::FromString(Text)); Component->SetWorldSize(N::FontSize);
    Component->SetHorizontalAlignment(EHTA_Left); Component->SetVerticalAlignment(EVRTA_TextTop);
    Component->SetTextRenderColor(FColor::Cyan); Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetGenerateOverlapEvents(false); Component->SetRelativeRotation(FRotator(0, 180, 0));
    Component->SetRelativeLocation(FVector(0, 0, -Row * N::RowSpacing));
    Root->AddChildNode(Node);
}

void BuildGraph(UBlueprint* BP)
{
    UEdGraph* Graph = BP->UbergraphPages[0];
    // Start from an empty event graph, removing only newly created default nodes.
    const auto Defaults = Graph->Nodes;
    for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph G(Graph);
    auto* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);
    auto* Pawn = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerPawn));
    G.Branch(G.Valid(G.Pin(Pawn, P::ReturnValue)));
    auto* Camera = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    G.Branch(G.Valid(G.Pin(Camera, P::ReturnValue)));
    auto* CameraLocation = G.Call(APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetCameraLocation));
    auto* CameraRotation = G.Call(APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetCameraRotation));
    G.Link(G.Pin(Camera, P::ReturnValue), G.Pin(CameraLocation, P::FunctionTarget));
    G.Link(G.Pin(Camera, P::ReturnValue), G.Pin(CameraRotation, P::FunctionTarget));
    auto* ViewTransform = G.Transform(G.Pin(CameraLocation, P::ReturnValue), G.Pin(CameraRotation, P::ReturnValue));
    auto* BoardTransform = G.Transform(G.Offset(ViewTransform, N::BoardOffset), G.Pin(CameraRotation, P::ReturnValue));
    auto* Move = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_SetActorTransform));
    G.Link(BoardTransform, G.Pin(Move, E::NewTransform)); G.Default(Move, E::Teleport, N::True); G.Exec(Move);

    auto* First = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt), G.Read(N::State), N::Zero));
    G.Write(N::State, nullptr, N::Observing); // Arm once before any impure spawn/load.
    G.Text(N::Title, N::RunningText);
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftClassPath));
    G.Default(Path, E::PathString, N::DroneClass);
    auto* Ref = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftClassPathToSoftClassRef));
    G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Ref, E::SoftClassPath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadClassAsset_Blocking));
    G.Link(G.Pin(Ref, P::ReturnValue), G.Pin(Load, E::AssetClass)); G.Exec(Load);
    auto* Cast = NewObject<UK2Node_ClassDynamicCast>(Graph); Cast->TargetType = AActor::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(G.Pin(Load, P::ReturnValue), Cast->GetCastSourcePin());
    G.Tail = Cast->GetInvalidCastPin(); G.Text(N::Title, N::LoadFailureText); G.Write(N::State, nullptr, N::Finished);
    G.Tail = Cast->GetValidCastPin();
    auto* SpawnTransform = G.Transform(G.Offset(ViewTransform, N::SpawnOffset), G.Pin(CameraRotation, P::ReturnValue));
    auto* Spawn = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginDeferredActorSpawnFromClass));
    G.Link(Cast->GetCastResultPin(), G.Pin(Spawn, E::ActorClass)); G.Link(SpawnTransform, G.Pin(Spawn, P::SpawnTransform));
    G.Default(Spawn, E::CollisionHandling, N::AlwaysSpawn); G.Exec(Spawn);
    auto* DeferredValid = G.Branch(G.Valid(G.Pin(Spawn, P::ReturnValue)));
    UEdGraphPin* NullDeferred = G.Pin(DeferredValid, P::Else);
    auto* Finish = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor));
    G.Link(G.Pin(Spawn, P::ReturnValue), G.Pin(Finish, P::Actor)); G.Link(SpawnTransform, G.Pin(Finish, P::SpawnTransform)); G.Exec(Finish);
    G.Write(N::Drone, G.Pin(Finish, P::ReturnValue));
    auto* SpawnValid = G.Branch(G.Valid(G.Read(N::Drone)));
    G.Text(N::Spawn, N::SpawnYes);
    G.Tail = G.Pin(SpawnValid, P::Else); G.Text(N::Spawn, N::SpawnNo);
    G.Tail = NullDeferred; G.Text(N::Spawn, N::SpawnNo);

    G.Tail = G.Pin(First, P::Else);
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt), G.Read(N::State), N::Observing));
    auto* AddAge = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble));
    G.Link(G.Read(N::Age), G.Pin(AddAge, P::Binary::LeftOperand)); G.Link(G.Pin(Tick, P::DeltaSeconds), G.Pin(AddAge, P::Binary::RightOperand));
    G.Write(N::Age, G.Pin(AddAge, P::ReturnValue)); G.Number(N::Clock, N::ClockPrefix, G.Read(N::Age));
    auto* Alive = G.Branch(G.Valid(G.Read(N::Drone)));
    G.Text(N::Current, N::CurrentYes);
    auto* DroneLocation = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation));
    G.Link(G.Read(N::Drone), G.Pin(DroneLocation, P::FunctionTarget));
    // Read-only Actor diagnostics. No collision, visibility or item-state mutation.
    auto* Hidden = NewObject<UK2Node_VariableGet>(Graph);
    Hidden->VariableReference.SetExternalMember(E::ActorHidden, AActor::StaticClass()); G.Node(Hidden);
    G.Link(G.Read(N::Drone), G.Pin(Hidden, P::FunctionTarget));
    G.BooleanText(N::Hidden, G.Pin(Hidden, E::ActorHidden), N::HiddenYes, N::HiddenNo);
    auto* Collision = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetActorEnableCollision));
    G.Link(G.Read(N::Drone), G.Pin(Collision, P::FunctionTarget));
    G.BooleanText(N::Collision, G.Pin(Collision, P::ReturnValue), N::CollisionYes, N::CollisionNo);
    G.Text(N::Marker, N::MarkerText);
    auto* MarkerLocation = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector));
    G.Link(G.Pin(DroneLocation, P::ReturnValue), G.Pin(MarkerLocation, P::Binary::LeftOperand));
    G.Default(MarkerLocation, P::Binary::RightOperand, N::MarkerOffset);
    auto* MoveMarker = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_SetWorldLocation));
    G.Link(G.Read(N::Marker), G.Pin(MoveMarker, P::FunctionTarget));
    G.Link(G.Pin(MarkerLocation, P::ReturnValue), G.Pin(MoveMarker, E::NewLocation));
    G.Default(MoveMarker, E::Teleport, N::True); G.Exec(MoveMarker);
    auto* Subtract = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector));
    G.Link(G.Pin(DroneLocation, P::ReturnValue), G.Pin(Subtract, P::Binary::LeftOperand));
    G.Link(G.Pin(CameraLocation, P::ReturnValue), G.Pin(Subtract, P::Binary::RightOperand));
    auto* Length = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(G.Pin(Subtract, P::ReturnValue), G.Pin(Length, E::VectorLengthInput)); G.Number(N::Distance, N::DistancePrefix, G.Pin(Length, P::ReturnValue));
    UEdGraphPin* AliveDone = G.Tail;
    G.Tail = G.Pin(Alive, P::Else); G.Text(N::Current, N::CurrentNo); G.Text(N::Distance, N::NoDistance);
    G.Text(N::Hidden, N::HiddenUnknown); G.Text(N::Collision, N::CollisionUnknown); G.Text(N::Marker, N::EmptyText);
    auto* TimeDone = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble), G.Read(N::Age), N::ObserveSeconds));
    G.Link(AliveDone, G.Pin(TimeDone, P::Execute));
    G.Write(N::State, nullptr, N::Finished); G.Text(N::Title, N::FinishedText);
    G.Text(N::Marker, N::EmptyText);
    G.Branch(G.Valid(G.Read(N::Drone)));
    auto* Destroy = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor));
    G.Link(G.Read(N::Drone), G.Pin(Destroy, P::FunctionTarget)); G.Exec(Destroy);

    // Clear only our own spawned actor when this entry is destroyed early.
    auto* End = NewObject<UK2Node_Event>(Graph);
    End->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveDestroyed, AActor::StaticClass());
    End->bOverrideFunction = true; G.Node(End); G.Tail = G.Pin(End, P::Then);
    G.Branch(G.Valid(G.Read(N::Drone)));
    auto* Cleanup = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor));
    G.Link(G.Read(N::Drone), G.Pin(Cleanup, P::FunctionTarget)); G.Exec(Cleanup);
}
}

UGenerateHarpoonProbeCommandlet::UGenerateHarpoonProbeCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

int32 UGenerateHarpoonProbeCommandlet::Main(const FString& Params)
{
    UPackage* Package = CreatePackage(N::Package);
    UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, FName(N::Asset),
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    check(BP);
    auto* Root = BP->SimpleConstructionScript->CreateNode(USceneComponent::StaticClass(), N::Root);
    BP->SimpleConstructionScript->AddNode(Root);
    AddText(BP, Root, N::Title, N::WaitingText, 0);
    AddText(BP, Root, N::Spawn, N::SpawnWaitingText, 1);
    AddText(BP, Root, N::Current, N::CurrentWaitingText, 2);
    AddText(BP, Root, N::Clock, N::ClockPrefix, 3);
    AddText(BP, Root, N::Distance, N::NoDistance, 4);
    AddText(BP, Root, N::Hidden, N::HiddenUnknown, 5);
    AddText(BP, Root, N::Collision, N::CollisionUnknown, 6);
    AddText(BP, Root, N::Marker, N::EmptyText, 7);
    AddVariable(BP, N::State, UEdGraphSchema_K2::PC_Int);
    AddVariable(BP, N::Age, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, N::Drone, UEdGraphSchema_K2::PC_Object, AActor::StaticClass());
    FKismetEditorUtilities::CompileBlueprint(BP);
    BuildGraph(BP);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    if (BP->Status == BS_Error) return 1;
    auto* CDO = CastChecked<AActor>(BP->GeneratedClass->GetDefaultObject());
    CDO->PrimaryActorTick.bCanEverTick = true; CDO->PrimaryActorTick.bStartWithTickEnabled = true;
    CDO->PrimaryActorTick.TickInterval = N::TickInterval; CDO->SetActorEnableCollision(false);
    Package->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(N::Package, FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    FSavePackageArgs Save; Save.TopLevelFlags = RF_Public | RF_Standalone; Save.SaveFlags = SAVE_NoError;
    return UPackage::SavePackage(Package, BP, *Filename, Save) ? 0 : 1;
}
