#include "GenerateHarpoonProbeCommandlet.h"
#include "ProbeNames.h"
#include "HarpoonInputNames.h"
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
#include "Components/ScaleBox.h"
#include "Engine/Texture2D.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MacroInstance.h"
#include "VoyageDynamicPlayerInputWidget.h"
#include "VoyageModuleComponent.h"
#include "VoyageBaseDataAsset.h"
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
#include "Components/TextBlock.h"
#include "Components/PrimitiveComponent.h"
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
#include "K2Node_GetArrayItem.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetArrayLibrary.h"
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

void AddText(UWidgetBlueprint* BP, UVerticalBox* Rows, FName Name, const TCHAR* Text, int Row)
{
    auto* TextBlock = BP->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
    TextBlock->bIsVariable = true;
    TextBlock->SetText(FText::FromString(Text));
    auto Font = TextBlock->GetFont(); Font.Size = N::HudFontSize; TextBlock->SetFont(Font);
    TextBlock->SetColorAndOpacity(FSlateColor(N::HudForeground));
    TextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
    Rows->AddChildToVerticalBox(TextBlock);
}

void SamplePhysics(FGraph& G, UEdGraphPin* DroneLocation)
{
    auto* Simulating = G.Call(UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, IsSimulatingPhysics));
    G.Link(G.Read(N::PhysicsBody), G.Pin(Simulating, P::FunctionTarget));
    G.BooleanText(N::Physics, G.Pin(Simulating, P::ReturnValue), N::PhysicsYes, N::PhysicsNo);
    auto* Difference = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector),
        DroneLocation, G.Read(N::EntryLocation));
    auto* Length = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(Difference, G.Pin(Length, E::VectorLengthInput));
    G.Number(N::Drift, N::DriftPrefix, G.Pin(Length, P::ReturnValue));
}

UEdGraphPin* CarrierLocalPosition(FGraph& G, UEdGraphPin* DroneLocation)
{
    auto* Transform = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentToWorld));
    G.Link(G.Read(N::Carrier), G.Pin(Transform, P::FunctionTarget));
    auto* Local = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, InverseTransformLocation));
    G.Link(G.Pin(Transform, P::ReturnValue), G.Pin(Local, E::Transform));
    G.Link(DroneLocation, G.Pin(Local, E::Location));
    return G.Pin(Local, P::ReturnValue);
}

void SampleCarrier(FGraph& G, UEdGraphPin* DroneLocation)
{
    G.Require(G.Valid(G.Read(N::Carrier)), N::CarrierLost);
    auto* Parent = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent));
    G.Link(G.Read(N::PhysicsBody), G.Pin(Parent, P::FunctionTarget));
    auto* ParentMatches = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        G.Pin(Parent, P::ReturnValue), G.Read(N::Carrier));
    G.BooleanText(N::AttachmentText, ParentMatches, N::AttachedYes, N::AttachedNo);
    auto* Simulating = G.Call(UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, IsSimulatingPhysics));
    G.Link(G.Read(N::PhysicsBody), G.Pin(Simulating, P::FunctionTarget));
    auto* Broken = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), ParentMatches, N::False),
        G.Pin(Simulating, P::ReturnValue));
    G.Write(N::MountBroken, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Read(N::MountBroken), Broken));
    G.BooleanText(N::FreezeStatus, G.Read(N::MountBroken), N::MountOverridden, N::FreezeApplied);
    auto* Relative = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector),
        CarrierLocalPosition(G, DroneLocation), G.Read(N::LocalAnchor));
    auto* Length = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(Relative, G.Pin(Length, E::VectorLengthInput));
    G.Number(N::LocalDriftText, N::LocalDriftPrefix, G.Pin(Length, P::ReturnValue));
    auto* World = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation));
    G.Link(G.Read(N::Carrier), G.Pin(World, P::FunctionTarget));
    auto* Travel = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), G.Pin(World, P::ReturnValue), G.Read(N::CarrierStart));
    auto* TravelLength = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(Travel, G.Pin(TravelLength, E::VectorLengthInput));
    G.Number(N::CarrierTravelText, N::CarrierTravelPrefix, G.Pin(TravelLength, P::ReturnValue));
}

void FindSupport(FGraph& G, UEdGraphPin* DroneLocation)
{
    auto* Parent = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent));
    G.Link(G.Read(N::PhysicsBody), G.Pin(Parent, P::FunctionTarget));
    auto* AlreadyParented = G.Branch(G.Valid(G.Pin(Parent, P::ReturnValue)));
    G.Text(N::FreezeStatus, N::ExistingParent); G.Tail = G.Pin(AlreadyParented, P::Else);
    auto* Start = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector));
    auto* End = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector));
    G.Link(DroneLocation, G.Pin(Start, P::Binary::LeftOperand)); G.Default(Start, P::Binary::RightOperand, N::TraceAbove);
    G.Link(DroneLocation, G.Pin(End, P::Binary::LeftOperand)); G.Default(End, P::Binary::RightOperand, N::TraceBelow);
    auto* Trace = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LineTraceSingle));
    G.Link(G.Pin(Start, P::ReturnValue), G.Pin(Trace, E::TraceStart));
    G.Link(G.Pin(End, P::ReturnValue), G.Pin(Trace, E::TraceEnd));
    G.Link(G.ActorArray(G.Read(N::Drone), G.Read(N::OriginalPawn)), G.Pin(Trace, E::ActorsToIgnore));
    G.Default(Trace, E::TraceChannel, N::VisibilityTrace); G.Default(Trace, E::TraceComplex, N::True);
    G.Default(Trace, E::IgnoreSelf, N::True); G.Exec(Trace);
    G.Require(G.Pin(Trace, P::ReturnValue), N::NoSupport);
    auto* Hit = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult));
    G.Link(G.Pin(Trace, E::OutHit), G.Pin(Hit, E::Hit));
    G.Require(G.Valid(G.Pin(Hit, E::HitComponent)), N::NoSupport);
    G.Write(N::Carrier, G.Pin(Hit, E::HitComponent));
    auto* Name = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetDisplayName));
    G.Link(G.Read(N::Carrier), G.Pin(Name, E::DisplayObject));
    auto* NameText = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
    G.Link(G.Pin(Name, P::ReturnValue), G.Pin(NameText, E::StringValue));
    G.Text(N::CarrierNameText, nullptr, G.Pin(NameText, P::ReturnValue));
    G.Write(N::LocalAnchor, CarrierLocalPosition(G, DroneLocation));
    auto* World = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation));
    G.Link(G.Read(N::Carrier), G.Pin(World, P::FunctionTarget));
    G.Write(N::CarrierStart, G.Pin(World, P::ReturnValue));
}

void RestorePhysics(FGraph& G)
{
    auto* Restore = G.Call(UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetSimulatePhysics));
    G.Link(G.Read(N::PhysicsBody), G.Pin(Restore, P::FunctionTarget));
    G.Link(G.Read(N::OriginalSimulation), G.Pin(Restore, E::SimulatePhysics)); G.Exec(Restore);
    G.Write(N::FreezeHeld, nullptr, N::False);
}

void PhysicsExperiment(FGraph& G, UEdGraphPin* ControlsDrone, UEdGraphPin* DroneLocation)
{
    auto* Attempted = G.Branch(G.Read(N::EntryAttempted));
    G.Branch(G.Read(N::FreezeHeld));
    G.Branch(G.Valid(G.Read(N::PhysicsBody)));
    SamplePhysics(G, DroneLocation); // Observe any native re-enable; never fight it every Tick.
    SampleCarrier(G, DroneLocation);

    G.Tail = G.Pin(Attempted, P::Else);
    // One pre-entry mount after a settling window; neither entry nor exit rearms it.
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble),
        G.Read(N::Age), N::MountDelay), N::FreezeWaiting);
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        ControlsDrone, N::False), N::EarlyEntry);
    G.Write(N::EntryAttempted, nullptr, N::True);
    auto* Root = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent));
    G.Link(G.Read(N::Drone), G.Pin(Root, P::FunctionTarget));
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph);
    Cast->TargetType = UPrimitiveComponent::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(G.Pin(Root, P::ReturnValue), Cast->GetCastSourcePin());
    G.Tail = Cast->GetInvalidCastPin(); G.Text(N::FreezeStatus, N::FreezeFailed);
    G.Tail = Cast->GetValidCastPin();
    G.Write(N::PhysicsBody, Cast->GetCastResultPin());
    FindSupport(G, DroneLocation); // Fail before physics mutation if no unambiguous support component.
    auto* WasSimulating = G.Call(UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, IsSimulatingPhysics));
    G.Link(G.Read(N::PhysicsBody), G.Pin(WasSimulating, P::FunctionTarget));
    G.Write(N::OriginalSimulation, G.Pin(WasSimulating, P::ReturnValue));
    G.BooleanText(N::OriginalPhysicsText, G.Read(N::OriginalSimulation), N::OriginalPhysicsYes, N::OriginalPhysicsNo);
    G.Write(N::EntryLocation, DroneLocation);
    auto* Freeze = G.Call(UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetSimulatePhysics));
    G.Link(G.Read(N::PhysicsBody), G.Pin(Freeze, P::FunctionTarget));
    G.Default(Freeze, E::SimulatePhysics, N::False); G.Exec(Freeze);
    G.Write(N::FreezeHeld, nullptr, N::True);
    auto* Attach = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_AttachToComponent));
    G.Link(G.Read(N::PhysicsBody), G.Pin(Attach, P::FunctionTarget));
    G.Link(G.Read(N::Carrier), G.Pin(Attach, E::AttachmentParent));
    G.Default(Attach, E::LocationRule, N::KeepWorld); G.Default(Attach, E::RotationRule, N::KeepWorld);
    G.Default(Attach, E::ScaleRule, N::KeepWorld); G.Default(Attach, E::WeldBodies, N::False); G.Exec(Attach);
    auto* Attached = G.Branch(G.Pin(Attach, P::ReturnValue));
    auto* AttachSuccess = G.Tail; G.Tail = G.Pin(Attached, P::Else);
    RestorePhysics(G); G.Text(N::FreezeStatus, N::AttachFailed);
    G.Tail = AttachSuccess;
    G.Text(N::FreezeStatus, N::FreezeApplied);
    SamplePhysics(G, DroneLocation);
    SampleCarrier(G, DroneLocation);

    // HC07 deliberately leaves attachment/physics unchanged across possession.
    // A native override is observed and latched, never repaired on Tick.
}

void BuildGraph(UBlueprint* BP, UClass* HudClass)
{
    UEdGraph* Graph = BP->UbergraphPages[0];
    // Start from an empty event graph, removing only newly created default nodes.
    const auto Defaults = Graph->Nodes;
    for (UEdGraphNode* Node : Defaults) Node->DestroyNode();
    FGraph G(Graph, HudClass);
    auto* Tick = NewObject<UK2Node_Event>(Graph);
    Tick->EventReference.SetExternalMember(BlueprintGraphNames::Events::ActorReceiveTick, AActor::StaticClass());
    Tick->bOverrideFunction = true; G.Node(Tick); G.Tail = G.Pin(Tick, P::Then);
    auto* Pawn = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerPawn));
    G.Branch(G.Valid(G.Pin(Pawn, P::ReturnValue)));
    auto* Camera = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerCameraManager));
    G.Branch(G.Valid(G.Pin(Camera, P::ReturnValue)));
    auto* HudReady = G.Branch(G.Valid(G.Read(N::HudInstance)));
    auto* ReadyTail = G.Tail; G.Tail = G.Pin(HudReady, P::Else);
    auto* Controller = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, GetPlayerController));
    G.Branch(G.Valid(G.Pin(Controller, P::ReturnValue)));
    auto* CreateHud = G.Call(UWidgetBlueprintLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create));
    G.Pin(CreateHud, E::WidgetType)->DefaultObject = HudClass;
    G.Link(G.Pin(Controller, P::ReturnValue), G.Pin(CreateHud, E::OwningPlayer)); G.Exec(CreateHud);
    auto* HudCast = NewObject<UK2Node_DynamicCast>(Graph);
    HudCast->TargetType = HudClass; HudCast->SetPurity(false); G.Node(HudCast);
    G.Link(G.Tail, G.Pin(HudCast, P::Execute)); G.Link(G.Pin(CreateHud, P::ReturnValue), HudCast->GetCastSourcePin());
    G.Tail = HudCast->GetValidCastPin(); G.Write(N::HudInstance, HudCast->GetCastResultPin());
    auto* AddHud = G.Call(UUserWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UUserWidget, AddToViewport));
    G.Link(G.Read(N::HudInstance), G.Pin(AddHud, P::FunctionTarget)); G.Default(AddHud, E::ZOrder, N::HudZOrder); G.Exec(AddHud);
    // Creation branch stops here; observation starts next normal sample.
    G.Tail = ReadyTail;
    auto* CameraLocation = G.Call(APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetCameraLocation));
    auto* CameraRotation = G.Call(APlayerCameraManager::StaticClass(), GET_FUNCTION_NAME_CHECKED(APlayerCameraManager, GetCameraRotation));
    G.Link(G.Pin(Camera, P::ReturnValue), G.Pin(CameraLocation, P::FunctionTarget));
    G.Link(G.Pin(Camera, P::ReturnValue), G.Pin(CameraRotation, P::FunctionTarget));
    auto* ViewTransform = G.Transform(G.Pin(CameraLocation, P::ReturnValue), G.Pin(CameraRotation, P::ReturnValue));

    auto* First = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt), G.Read(N::State), N::Zero));
    G.Write(N::State, nullptr, N::Observing); // Arm once before any impure spawn/load.
    G.Write(N::OriginalPawn, G.Pin(Pawn, P::ReturnValue));
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
    auto* ControlsDrone = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        G.Pin(Pawn, P::ReturnValue), G.Read(N::Drone));
    G.BooleanText(N::Control, ControlsDrone, N::ControlYes, N::ControlNo);
    G.Write(N::SawDroneControl, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Read(N::SawDroneControl), ControlsDrone));
    auto* BackAtOriginal = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        G.Pin(Pawn, P::ReturnValue), G.Read(N::OriginalPawn));
    auto* ReturnAfterControl = G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Read(N::SawDroneControl), BackAtOriginal);
    G.Write(N::SawReturn, G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Read(N::SawReturn), ReturnAfterControl));
    G.BooleanText(N::Entered, G.Read(N::SawDroneControl), N::EnteredYes, N::EnteredNo);
    G.BooleanText(N::Returned, G.Read(N::SawReturn), N::ReturnedYes, N::ReturnedNo);
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
    auto* Subtract = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector));
    G.Link(G.Pin(DroneLocation, P::ReturnValue), G.Pin(Subtract, P::Binary::LeftOperand));
    G.Link(G.Pin(CameraLocation, P::ReturnValue), G.Pin(Subtract, P::Binary::RightOperand));
    auto* Length = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(G.Pin(Subtract, P::ReturnValue), G.Pin(Length, E::VectorLengthInput)); G.Number(N::Distance, N::DistancePrefix, G.Pin(Length, P::ReturnValue));
    PhysicsExperiment(G, ControlsDrone, G.Pin(DroneLocation, P::ReturnValue));
    G.Tail = G.Pin(Alive, P::Else); G.Text(N::Current, N::CurrentNo); G.Text(N::Distance, N::NoDistance);
    G.Text(N::Hidden, N::HiddenUnknown); G.Text(N::Collision, N::CollisionUnknown); G.Text(N::Marker, N::EmptyText);
    auto* EndPlay = NewObject<UK2Node_Event>(Graph);
    EndPlay->EventReference.SetExternalMember(E::EndPlayEvent, AActor::StaticClass());
    EndPlay->bOverrideFunction = true; G.Node(EndPlay); G.Tail = G.Pin(EndPlay, P::Then);
    G.Branch(G.Valid(G.Read(N::HudInstance)));
    auto* RemoveHud = G.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, RemoveFromParent));
    G.Link(G.Read(N::HudInstance), G.Pin(RemoveHud, P::FunctionTarget)); G.Exec(RemoveHud);
    // The probe cleans up only its HUD, never destroys the Drone.
    // Test in a disposable session: native persistence is not claimed safe.
}
}

namespace
{
#include "HelmObservationProbe.h"
#include "StationFixationProbe.h"
#include "StationNativeHudProbe.h"
#include "NativeVehicleProbe.h"
#include "ForkliftEntryProbe.h"
#include "StationActionHints.h"
#include "ContextEntryProbe.h"
#include "CannonShotProbe.h"
#include "DedicatedStationProbe.h"
#include "ContextStationCoordinator.h"
}

UGenerateHarpoonProbeCommandlet::UGenerateHarpoonProbeCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}

int32 UGenerateHarpoonProbeCommandlet::Main(const FString& Params)
{
    if (FParse::Param(*Params, DedicatedStationNames::VerifySwitch))
    {
        TArray<const TCHAR*> VerifyPackages {N::Package, N::HudPackage, DedicatedStationNames::OperatorPackage, DedicatedStationNames::HudPackage,
            HarpoonInputNames::LookYaw, HarpoonInputNames::LookPitch, HarpoonInputNames::Exit, HarpoonInputNames::Zoom, HarpoonInputNames::Fire, Shot::Package,
            ZoomTest::MaskPackage, HarpoonInputNames::Keyboard, HarpoonInputNames::Context};
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
    Shot::Class=CreateCannonShot();
    UClass* StationClass = CreateDedicatedStation();
    UPackage* HudPackage = CreatePackage(N::HudPackage);
    auto* Hud = CastChecked<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UUserWidget::StaticClass(),
        HudPackage, FName(N::HudAsset), BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
    if (!Hud->WidgetTree) Hud->WidgetTree = NewObject<UWidgetTree>(Hud, N::HudTree);
    auto* Canvas = Hud->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), N::HudCanvas);
    Hud->WidgetTree->RootWidget = Canvas;
    auto* Border = Hud->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), N::HudBorder);
    Border->SetBrushColor(N::HudBackground); Border->SetPadding(FMargin(N::HudPadding));
    Border->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto* Slot = Canvas->AddChildToCanvas(Border);
    Slot->SetAnchors(FAnchors(N::HudAnchor.X, N::HudAnchor.Y)); Slot->SetAlignment(N::HudAnchor);
    Slot->SetPosition(N::HudOffset); Slot->SetSize(NativeCameraNames::PanelSize);
    auto* Rows = Hud->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), N::HudRows);
    Border->SetContent(Rows);
    AddText(Hud, Rows, N::Title, DedicatedStationNames::Title, 0);
    AddText(Hud, Rows, N::Current, N::EmptyText, 2);
    AddText(Hud, Rows, N::Control, N::EmptyText, 3);
    AddText(Hud, Rows, N::Clock, N::EmptyText, 4);
    AddText(Hud, Rows, N::Drift, N::EmptyText, 5);
    AddText(Hud, Rows, N::Physics, N::EmptyText, 6);
    AddText(Hud, Rows, NativeCameraNames::RequestedText, N::EmptyText, 7);
    AddText(Hud, Rows, NativeCameraNames::ModeText, N::EmptyText, 8);
    auto* Scope = Hud->WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), H::ScopePanel);
    Scope->bIsVariable = true; Scope->SetVisibility(ESlateVisibility::Collapsed);
    auto* ScopeSlot = Canvas->AddChildToCanvas(Scope);
    ScopeSlot->SetAnchors(FAnchors(O::ReticleCenter.X, O::ReticleCenter.Y));
    ScopeSlot->SetAlignment(FVector2D(O::ReticleCenter.X, 0.0f)); ScopeSlot->SetSize(H::TargetSize); ScopeSlot->SetPosition(H::TargetOffset);
    AddText(Hud, Scope, Range::TargetName, N::EmptyText, 0);
    AddText(Hud, Scope, Range::TargetRange, N::EmptyText, 1);
    AddText(Hud, Scope, H::ScopeFooter, H::Footer, 2);
    for (UWidget* Child : Scope->GetAllChildren())
    {
        auto* Label = CastChecked<UTextBlock>(Child); auto Font = Label->GetFont(); Font.Size = H::TargetFontSize; Label->SetFont(Font);
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White)); Label->SetJustification(ETextJustify::Center);
        Label->SetShadowColorAndOpacity(FLinearColor::Black); Label->SetShadowOffset(H::ShadowOffset);
    }
    auto* Notice = Hud->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), N::FreezeStatus);
    Notice->bIsVariable = true; Notice->SetText(FText::FromString(DedicatedStationNames::Ready)); Notice->SetVisibility(ESlateVisibility::HitTestInvisible);
    auto NoticeFont = Notice->GetFont(); NoticeFont.Size = H::NoticeFontSize; Notice->SetFont(NoticeFont); Notice->SetJustification(ETextJustify::Center);
    Notice->SetShadowColorAndOpacity(FLinearColor::Black); Notice->SetShadowOffset(H::ShadowOffset);
    auto* NoticeSlot = Canvas->AddChildToCanvas(Notice); NoticeSlot->SetAnchors(FAnchors(H::NoticeAnchor.X, H::NoticeAnchor.Y));
    NoticeSlot->SetAlignment(H::NoticeAnchor); NoticeSlot->SetSize(H::NoticeSize); NoticeSlot->SetPosition(H::NoticeOffset);
    auto* Reticle = Hud->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), O::Reticle);
    Reticle->bIsVariable = true; Reticle->SetText(FText::FromString(O::ReticleText));
    auto ReticleFont = Reticle->GetFont(); ReticleFont.Size = O::ReticleFontSize; Reticle->SetFont(ReticleFont);
    Reticle->SetJustification(ETextJustify::Center); Reticle->SetVisibility(ESlateVisibility::Collapsed);
    auto* ReticleSlot = Canvas->AddChildToCanvas(Reticle);
    ReticleSlot->SetAnchors(FAnchors(O::ReticleCenter.X, O::ReticleCenter.Y));
    ReticleSlot->SetAlignment(O::ReticleCenter); ReticleSlot->SetSize(O::ReticleSize);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Hud);
    FKismetEditorUtilities::CompileBlueprint(Hud);
    if (Hud->Status == BS_Error) return 1;
    auto* HudCDO = CastChecked<UUserWidget>(Hud->GeneratedClass->GetDefaultObject());
    HudCDO->SetIsFocusable(false); HudCDO->SetVisibility(ESlateVisibility::HitTestInvisible);
    const FString HudFilename = FPackageName::LongPackageNameToFilename(N::HudPackage, FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(HudFilename), true);
    FSavePackageArgs HudSave; HudSave.TopLevelFlags = RF_Public | RF_Standalone; HudSave.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(HudPackage, Hud, *HudFilename, HudSave)) return 1;
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
    for (const auto& HudRoot : H::Roots)
    {
        AddVariable(BP, HudRoot.Widget, UEdGraphSchema_K2::PC_Object, UUserWidget::StaticClass());
        AddVariable(BP, HudRoot.Prior, UEdGraphSchema_K2::PC_Byte, StaticEnum<ESlateVisibility>());
        AddVariable(BP, HudRoot.Owned, UEdGraphSchema_K2::PC_Boolean);
    }
    AddVariable(BP, O::PreviousView, UEdGraphSchema_K2::PC_Object, AActor::StaticClass());
    AddVariable(BP, O::BaselineFov, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, O::RequestedFov, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, Aim::Yaw, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, Aim::Pitch, UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, O::CharacterHidden, UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, N::HudInstance, UEdGraphSchema_K2::PC_Object, Hud->GeneratedClass);
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
    AddVariable(BP, NativeCameraNames::Camera, UEdGraphSchema_K2::PC_Object, UCameraComponent::StaticClass());
    AddVariable(BP, ExitActionNames::Expected, UEdGraphSchema_K2::PC_Object, UVoyageInputAction::StaticClass());
    AddVariable(BP, NativeInputNames::Context, UEdGraphSchema_K2::PC_Object, UVoyageInputContextAsset::StaticClass());
    AddVariable(BP, NativeInputNames::LookRight, UEdGraphSchema_K2::PC_Object, UInputAction::StaticClass());
    AddVariable(BP, NativeInputNames::LookUp, UEdGraphSchema_K2::PC_Object, UInputAction::StaticClass());
    AddVariable(BP, V::Body, UEdGraphSchema_K2::PC_Object, UPrimitiveComponent::StaticClass());
    for (FName Flag : {V::Occupied, V::Attempted, V::ExitSent, V::PreparedFlag})
        AddVariable(BP, Flag, UEdGraphSchema_K2::PC_Boolean);
    FKismetEditorUtilities::CompileBlueprint(BP);
    BuildContextCoordinator(BP, Hud->GeneratedClass, StationClass);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    if (BP->Status == BS_Error) return 1;
    auto* CDO = CastChecked<AActor>(BP->GeneratedClass->GetDefaultObject());
    CDO->PrimaryActorTick.bCanEverTick = true; CDO->PrimaryActorTick.bStartWithTickEnabled = true;
    CDO->PrimaryActorTick.TickGroup = TG_PostPhysics;
    CDO->PrimaryActorTick.TickInterval = CE::ScanInterval; CDO->SetActorEnableCollision(false);
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
