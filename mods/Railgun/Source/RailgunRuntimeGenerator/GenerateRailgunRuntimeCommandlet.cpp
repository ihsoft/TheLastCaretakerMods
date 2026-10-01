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
#include "K2Node_GetArrayItem.h"
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
#include "HAL/FileManager.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, RailgunRuntimeGenerator)
namespace P = BlueprintGraphNames::Pins;
namespace E = ActorLifecycleGraphNames;
namespace N = RailgunRuntimeNames;

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

void AddArrayVariable(UBlueprint* BP, FName Name, FName Category, UObject* Type)
{
    FEdGraphPinType PinType;
    PinType.PinCategory = Category;
    PinType.PinSubCategoryObject = Type;
    PinType.ContainerType = EPinContainerType::Array;
    check(FBlueprintEditorUtils::AddMemberVariable(BP, Name, PinType));
}

void SetSingleObjectArrayDefault(UObject* Object, FName PropertyName, UObject* Value)
{
    auto* Property = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
    check(Property);
    auto* Inner = CastFieldChecked<FObjectPropertyBase>(Property->Inner);
    FScriptArrayHelper Values(Property, Property->ContainerPtrToValuePtr<void>(Object));
    Values.EmptyValues();
    const int32 Index = Values.AddValue();
    Inner->SetObjectPropertyValue(Values.GetRawPtr(Index), Value);
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
#include "StationActionHints.h"
#include "ContextEntryProbe.h"
#include "RailgunShot.h"
#include "DedicatedStationProbe.h"
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
    if (FParse::Param(*Params, TEXT("PatchStockRegistry")))
    {
        constexpr int32 ExpectedRegistryVersion = 24;
        constexpr int32 RegistryVersionOffset = 16;
        constexpr int32 RegistryFilterOffset = 20;
        FString StockFile;
        FString OutputFile;
        if (!FParse::Value(*Params, TEXT("StockRegistry="), StockFile) ||
            !FParse::Value(*Params, TEXT("OutputRegistry="), OutputFile) ||
            !FPaths::FileExists(StockFile) || StockFile == OutputFile)
        {
            UE_LOG(LogTemp, Error, TEXT("PatchStockRegistry requires distinct StockRegistry and OutputRegistry paths"));
            return 1;
        }
        TArray<uint8> StockBytes;
        if (!FFileHelper::LoadFileToArray(StockBytes, *StockFile) ||
            StockBytes.Num() < RegistryFilterOffset + static_cast<int32>(sizeof(int32)))
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot read complete stock registry header"));
            return 1;
        }
        int32 StockVersion = -1;
        int32 StockFilter = -1;
        FMemory::Memcpy(&StockVersion, StockBytes.GetData() + RegistryVersionOffset, sizeof(int32));
        FMemory::Memcpy(&StockFilter, StockBytes.GetData() + RegistryFilterOffset, sizeof(int32));
        if (StockVersion != ExpectedRegistryVersion || StockFilter != 1)
        {
            UE_LOG(LogTemp, Error, TEXT("Unreviewed stock registry header version=%d filter=%d"), StockVersion, StockFilter);
            return 1;
        }
        FAssetRegistryState Registry;
        {
            TUniquePtr<FArchive> Input(IFileManager::Get().CreateFileReader(*StockFile));
            if (!Input || !Registry.Load(*Input) || Input->IsError())
            {
                UE_LOG(LogTemp, Error, TEXT("Cannot load complete stock AssetRegistry.bin"));
                return 1;
            }
        }
        FAssetRegistrySerializationOptions Options(UE::AssetRegistry::ESerializationTarget::ForDevelopment);
        const FString NoopFile = OutputFile + TEXT(".noop");
        {
            TUniquePtr<FArchive> Noop(IFileManager::Get().CreateFileWriter(*NoopFile));
            if (!Noop)
            {
                UE_LOG(LogTemp, Error, TEXT("Cannot create stock registry no-op output"));
                return 1;
            }
            Noop->SetFilterEditorOnly(true);
            if (!Registry.Save(*Noop, Options) || Noop->IsError())
            {
                UE_LOG(LogTemp, Error, TEXT("Stock registry no-op serialization failed"));
                return 1;
            }
        }
        TArray<uint8> NoopBytes;
        if (!FFileHelper::LoadFileToArray(NoopBytes, *NoopFile) ||
            NoopBytes.Num() < RegistryFilterOffset + static_cast<int32>(sizeof(int32)))
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot read no-op registry header"));
            return 1;
        }
        int32 NoopVersion = -1;
        int32 NoopFilter = -1;
        FMemory::Memcpy(&NoopVersion, NoopBytes.GetData() + RegistryVersionOffset, sizeof(int32));
        FMemory::Memcpy(&NoopFilter, NoopBytes.GetData() + RegistryFilterOffset, sizeof(int32));
        if (NoopVersion != StockVersion || NoopFilter != StockFilter)
        {
            UE_LOG(LogTemp, Error, TEXT("No-op registry header differs: version=%d filter=%d"), NoopVersion, NoopFilter);
            return 1;
        }
        FAssetRegistryState NoopRegistry;
        if (!FAssetRegistryState::LoadFromDisk(*NoopFile, FAssetRegistryLoadOptions(), NoopRegistry) ||
            NoopRegistry.GetNumAssets() != Registry.GetNumAssets())
        {
            UE_LOG(LogTemp, Error, TEXT("No-op registry cannot be reopened or asset count differs"));
            return 1;
        }
        TArray<FString> DumpFields { TEXT("All"), TEXT("Tag") };
        TArray<FString> StockDump;
        TArray<FString> NoopDump;
        Registry.Dump(DumpFields, StockDump, 0);
        NoopRegistry.Dump(DumpFields, NoopDump, 0);
        if (StockDump != NoopDump)
        {
            UE_LOG(LogTemp, Error, TEXT("Stock registry no-op semantic dump differs: originalPages=%d outputPages=%d"),
                StockDump.Num(), NoopDump.Num());
            return 1;
        }
        UE_LOG(LogTemp, Display, TEXT("STOCK REGISTRY NO-OP SEMANTIC DUMP MATCH originalBytes=%d outputBytes=%d assets=%d"),
            StockBytes.Num(), NoopBytes.Num(), Registry.GetNumAssets());
        const int32 OriginalCount = Registry.GetNumAssets();
        const FAssetData* Stock = Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::StockAmmoPath));
        const FAssetData* Existing = Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::FullClonePath));
        const FAssetData* StockGun = Registry.GetAssetByObjectPath(
            FSoftObjectPath(RailgunAmmo::StockGunItemPath));
        const FAssetData* ExistingGun = Registry.GetAssetByObjectPath(
            FSoftObjectPath(RailgunAmmo::GunItemObjectPath));
        const FAssetData* StockSkill = Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::StockSkillPath));
        const FAssetData* ExistingSkill = Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::SkillObjectPath));
        if (!Stock || Existing || !StockGun || ExistingGun || !StockSkill || ExistingSkill ||
            OriginalCount < 1000 ||
            StockSkill->AssetClassPath.GetAssetName() != FName(TEXT("VoyageSkill")))
        {
            UE_LOG(LogTemp, Error, TEXT("Stock registry control missing, test asset already present, or registry incomplete: assets=%d"), OriginalCount);
            return 1;
        }
        const FName ClonePackage(RailgunAmmo::FullClonePackage);
        const FName CloneAsset(RailgunAmmo::FullCloneAsset);
        FAssetData* Clone = new FAssetData(*Stock);
        Clone->PackageName = ClonePackage;
        Clone->PackagePath = FName(RailgunAmmo::FullClonePackagePath);
        Clone->AssetName = CloneAsset;
        FAssetDataTagMap CloneTags = Clone->TagsAndValues.CopyMap();
        CloneTags.Add(FPrimaryAssetId::PrimaryAssetNameTag, CloneAsset.ToString());
        Clone->SetTagsAndAssetBundles(MoveTemp(CloneTags));
        const FPrimaryAssetId StockId = Stock->GetPrimaryAssetId();
        const FPrimaryAssetId CloneId = Clone->GetPrimaryAssetId();
        const FPrimaryAssetId ExpectedCloneId(RailgunAmmo::PrimaryAssetTypeName, CloneAsset);
        if (!StockId.IsValid() || CloneId != ExpectedCloneId)
        {
            UE_LOG(LogTemp, Error, TEXT("Clone primary ID mismatch: stock=%s clone=%s expected=%s"),
                *StockId.ToString(), *CloneId.ToString(), *ExpectedCloneId.ToString());
            delete Clone;
            return 1;
        }
        Registry.AddAssetData(Clone);
        if (const FAssetPackageData* StockPackage = Registry.GetAssetPackageData(Stock->PackageName))
        {
            *Registry.CreateOrGetAssetPackageData(ClonePackage) = *StockPackage;
        }
        const FName GunPackage(RailgunAmmo::GunItemPackage);
        const FName GunAsset(RailgunAmmo::GunItemAsset);
        FAssetData* GunClone = new FAssetData(*StockGun);
        GunClone->PackageName = GunPackage;
        GunClone->PackagePath = FName(RailgunAmmo::GunItemPackagePath);
        GunClone->AssetName = GunAsset;
        FAssetDataTagMap GunTags = GunClone->TagsAndValues.CopyMap();
        GunTags.Add(FPrimaryAssetId::PrimaryAssetNameTag, GunAsset.ToString());
        GunClone->SetTagsAndAssetBundles(MoveTemp(GunTags));
        const FPrimaryAssetId StockGunId = StockGun->GetPrimaryAssetId();
        const FPrimaryAssetId ExpectedGunId(RailgunAmmo::PrimaryAssetTypeName, GunAsset);
        if (!StockGunId.IsValid() || GunClone->GetPrimaryAssetId() != ExpectedGunId)
        {
            UE_LOG(LogTemp, Error, TEXT("Gun primary ID mismatch: stock=%s clone=%s expected=%s"),
                *StockGunId.ToString(), *GunClone->GetPrimaryAssetId().ToString(),
                *ExpectedGunId.ToString());
            delete GunClone;
            return 1;
        }
        Registry.AddAssetData(GunClone);
        if (const FAssetPackageData* StockGunPackage =
            Registry.GetAssetPackageData(StockGun->PackageName))
        {
            *Registry.CreateOrGetAssetPackageData(GunPackage) = *StockGunPackage;
        }
        const FName SkillPackage(RailgunAmmo::SkillPackage);
        const FName SkillAsset(RailgunAmmo::SkillAsset);
        FAssetData* SkillClone = new FAssetData(*StockSkill);
        SkillClone->PackageName = SkillPackage;
        SkillClone->PackagePath = FName(RailgunAmmo::SkillPackagePath);
        SkillClone->AssetName = SkillAsset;
        FAssetDataTagMap SkillTags = SkillClone->TagsAndValues.CopyMap();
        SkillTags.Add(FPrimaryAssetId::PrimaryAssetNameTag, SkillAsset.ToString());
        SkillClone->SetTagsAndAssetBundles(MoveTemp(SkillTags));
        const FPrimaryAssetId StockSkillId = StockSkill->GetPrimaryAssetId();
        const FPrimaryAssetId ExpectedSkillId(StockSkillId.PrimaryAssetType, SkillAsset);
        if (!StockSkillId.IsValid() || SkillClone->GetPrimaryAssetId() != ExpectedSkillId)
        {
            UE_LOG(LogTemp, Error, TEXT("Skill primary ID mismatch: stock=%s clone=%s expected=%s"),
                *StockSkillId.ToString(), *SkillClone->GetPrimaryAssetId().ToString(), *ExpectedSkillId.ToString());
            delete SkillClone;
            return 1;
        }
        Registry.AddAssetData(SkillClone);
        if (const FAssetPackageData* StockSkillPackage = Registry.GetAssetPackageData(StockSkill->PackageName))
        {
            *Registry.CreateOrGetAssetPackageData(SkillPackage) = *StockSkillPackage;
        }
        if (Registry.GetNumAssets() != OriginalCount + 3 ||
            !Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::StockAmmoPath)) ||
            !Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::FullClonePath)) ||
            !Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::StockGunItemPath)) ||
            !Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::GunItemObjectPath)) ||
            !Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::StockSkillPath)) ||
            !Registry.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::SkillObjectPath)))
        {
            UE_LOG(LogTemp, Error,
                TEXT("Stock registry gun/ammo/skill clone invariant failed before serialization"));
            return 1;
        }
        {
            TUniquePtr<FArchive> Output(IFileManager::Get().CreateFileWriter(*OutputFile));
            if (!Output)
            {
                UE_LOG(LogTemp, Error, TEXT("Cannot create patched complete AssetRegistry.bin"));
                return 1;
            }
            Output->SetFilterEditorOnly(true);
            if (!Registry.Save(*Output, Options) || Output->IsError())
            {
                UE_LOG(LogTemp, Error, TEXT("Cannot serialize patched complete AssetRegistry.bin"));
                return 1;
            }
        }
        TArray<uint8> PatchedBytes;
        int32 PatchedVersion = -1;
        int32 PatchedFilter = -1;
        if (!FFileHelper::LoadFileToArray(PatchedBytes, *OutputFile) ||
            PatchedBytes.Num() < RegistryFilterOffset + static_cast<int32>(sizeof(int32)))
        {
            UE_LOG(LogTemp, Error, TEXT("Cannot read patched registry header"));
            return 1;
        }
        FMemory::Memcpy(&PatchedVersion, PatchedBytes.GetData() + RegistryVersionOffset, sizeof(int32));
        FMemory::Memcpy(&PatchedFilter, PatchedBytes.GetData() + RegistryFilterOffset, sizeof(int32));
        if (PatchedVersion != StockVersion || PatchedFilter != StockFilter)
        {
            UE_LOG(LogTemp, Error, TEXT("Patched registry header differs: version=%d filter=%d"),
                PatchedVersion, PatchedFilter);
            return 1;
        }
        FAssetRegistryState Reopened;
        if (!FAssetRegistryState::LoadFromDisk(*OutputFile, FAssetRegistryLoadOptions(), Reopened) ||
            Reopened.GetNumAssets() != OriginalCount + 3 ||
            !Reopened.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::StockAmmoPath)) ||
            !Reopened.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::FullClonePath)) ||
            !Reopened.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::StockGunItemPath)) ||
            !Reopened.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::GunItemObjectPath)) ||
            !Reopened.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::StockSkillPath)) ||
            !Reopened.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::SkillObjectPath)))
        {
            UE_LOG(LogTemp, Error,
                TEXT("Patched registry reopen/count/gun/ammo/skill verification failed"));
            return 1;
        }
        const FAssetData* ReopenedSkill = Reopened.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::SkillObjectPath));
        const FAssetData* ReopenedItem = Reopened.GetAssetByObjectPath(FSoftObjectPath(RailgunAmmo::FullClonePath));
        const FAssetData* ReopenedGun = Reopened.GetAssetByObjectPath(
            FSoftObjectPath(RailgunAmmo::GunItemObjectPath));
        if (ReopenedSkill->GetPrimaryAssetId() != ExpectedSkillId ||
            ReopenedSkill->AssetClassPath != StockSkill->AssetClassPath ||
            ReopenedItem->GetPrimaryAssetId() != ExpectedCloneId ||
            ReopenedItem->AssetClassPath != Stock->AssetClassPath ||
            ReopenedGun->GetPrimaryAssetId() != ExpectedGunId ||
            ReopenedGun->AssetClassPath != StockGun->AssetClassPath)
        {
            UE_LOG(LogTemp, Error, TEXT("Patched registry primary IDs or native classes changed on reopening"));
            return 1;
        }
        UE_LOG(LogTemp, Display,
            TEXT("STOCK REGISTRY PATCH VERIFIED original=%d patched=%d gun=%s ammo=%s skill=%s skillId=%s"),
            OriginalCount, Reopened.GetNumAssets(), RailgunAmmo::GunItemObjectPath,
            RailgunAmmo::FullClonePath, RailgunAmmo::SkillObjectPath,
            *ExpectedSkillId.ToString());
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
