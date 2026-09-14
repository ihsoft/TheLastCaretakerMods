#pragma once
// HC28: provider -> unchanged stock hint widget in OWN HUD. No input/physics edit.
namespace Hint
{
inline const FName ProviderSeen(TEXT("HarpoonActionProviderObserved"));
inline const FName HintsReady(TEXT("HarpoonHintWidgetReady"));
inline const FName HintInstance(TEXT("HarpoonActionHints"));
inline const FName Root(TEXT("HarpoonHintHost"));
inline const FName Status(TEXT("HarpoonHintStatus"));
inline const FName InputAction(TEXT("InputAction"));
inline const FName Name(TEXT("Name"));
inline const FName Category(TEXT("Category"));
inline const FName Text(TEXT("Text"));
inline const FName Enabled(TEXT("bEnabled"));
inline const FName Type(TEXT("Type"));
inline const FName WidgetType(TEXT("WidgetType"));
inline const FName OwningPlayer(TEXT("OwningPlayer"));
inline const FName Content(TEXT("Content"));
inline constexpr TCHAR StockWidget[] = TEXT("/Game/UI/Game/BP_DynamicPlayerInputHorizontalWidget.BP_DynamicPlayerInputHorizontalWidget_C");
inline constexpr TCHAR ActionName[] = TEXT("HarpoonExit");
inline constexpr TCHAR ActionCategory[] = TEXT("HarpoonCannon");
inline constexpr TCHAR ExitLabel[] = TEXT("Exit Harpoon");
inline constexpr TCHAR ZoomName[] = TEXT("HarpoonZoom");
inline constexpr TCHAR ZoomLabel[] = TEXT("Toggle scope");
inline constexpr TCHAR Central[] = TEXT("EPlayerInputInterfaceActionType::Central");
inline constexpr TCHAR NotReady[] = TEXT("HC28: native hint widget NOT READY; E exits, F8 fallback");
inline constexpr TCHAR NoProvider[] = TEXT("HC28: hint widget READY; action provider NOT OBSERVED");
inline constexpr TCHAR ProviderReady[] = TEXT("HC28: hint widget READY; action provider OBSERVED");
}

void AddStationActions(UBlueprint* BP)
{
    const FName Name = GET_FUNCTION_NAME_CHECKED(AVoyageVehiclePawn, GetProvidedActionsBP);
    check(AVoyageVehiclePawn::StaticClass()->FindFunctionByName(Name)->GetOuterUClass() == AVoyageVehiclePawn::StaticClass());
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, Name, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false, AVoyageVehiclePawn::StaticClass());
    UK2Node_FunctionEntry* Entry = nullptr; UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* Found = Cast<UK2Node_FunctionEntry>(Node)) Entry = Found;
        if (auto* Found = Cast<UK2Node_FunctionResult>(Node)) Result = Found;
    }
    check(Entry && Result); FGraph G(Graph, nullptr);
    G.Pin(Entry, P::Then)->BreakAllPinLinks(); G.Pin(Result, P::Execute)->BreakAllPinLinks();
    G.Tail = G.Pin(Entry, P::Then); G.Write(Hint::ProviderSeen, nullptr, N::True);
    G.Link(G.Tail, G.Pin(Result, P::Execute));
    auto* Action = NewObject<UK2Node_MakeStruct>(Graph); Action->StructType = FPlayerInputInterfaceAction::StaticStruct();
    Action->bMadeAfterOverridePinRemoval = true; G.Node(Action);
    auto* ExitAction = LoadObject<UInputAction>(nullptr, HarpoonInputNames::Exit); check(ExitAction);
    G.Pin(Action, Hint::InputAction)->DefaultObject = ExitAction;
    G.Default(Action, Hint::Name, Hint::ActionName); G.Default(Action, Hint::Category, Hint::ActionCategory);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultText(*G.Pin(Action, Hint::Text), FText::FromString(Hint::ExitLabel));
    G.Link(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), OpticalSelf(G)), G.Pin(Action, Hint::Enabled));
    G.Default(Action, Hint::Type, Hint::Central);
    auto* Array = G.Node(NewObject<UK2Node_MakeArray>(Graph));
    UEdGraphPin* ActionOutput = nullptr; UEdGraphPin* FirstElement = nullptr;
    for (auto* Pin : Action->Pins) if (Pin->Direction == EGPD_Output) ActionOutput = Pin;
    for (auto* Pin : Array->Pins) if (Pin->Direction == EGPD_Input) { FirstElement = Pin; break; }
    check(ActionOutput && FirstElement); G.Link(ActionOutput, FirstElement);
    auto* ZoomAction = NewObject<UK2Node_MakeStruct>(Graph); ZoomAction->StructType = FPlayerInputInterfaceAction::StaticStruct();
    ZoomAction->bMadeAfterOverridePinRemoval = true; G.Node(ZoomAction);
    auto* ZoomInput = LoadObject<UInputAction>(nullptr, HarpoonInputNames::Zoom); check(ZoomInput);
    G.Pin(ZoomAction, Hint::InputAction)->DefaultObject = ZoomInput;
    G.Default(ZoomAction, Hint::Name, Hint::ZoomName); G.Default(ZoomAction, Hint::Category, Hint::ActionCategory);
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultText(*G.Pin(ZoomAction, Hint::Text), FText::FromString(Hint::ZoomLabel));
    G.Link(ObserveCall(G, APawn::StaticClass(), GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), OpticalSelf(G)), G.Pin(ZoomAction, Hint::Enabled));
    G.Default(ZoomAction, Hint::Type, Hint::Central);
    Array->AddInputPin();
    UEdGraphPin* LastElement = nullptr;
    for (auto* Pin : Array->Pins) if (Pin->Direction == EGPD_Input) LastElement = Pin;
    for (auto* Pin : ZoomAction->Pins) if (Pin->Direction == EGPD_Output) G.Link(Pin, LastElement);
    G.Link(Array->GetOutputPin(), G.Pin(Result, P::ReturnValue));
}

void UpdateStationHintMarker(FGraph& G, UEdGraphPin* Station, UClass* StationClass)
{
    auto SetStatus = [&](const TCHAR* Message)
    {
        auto* Set = G.Call(UTextBlock::StaticClass(), GET_FUNCTION_NAME_CHECKED(UTextBlock, SetText));
        G.Link(G.Read(Hint::Status), G.Pin(Set, P::FunctionTarget));
        GetDefault<UEdGraphSchema_K2>()->TrySetDefaultText(*G.Pin(Set, E::WidgetText), FText::FromString(Message)); G.Exec(Set);
    };
    auto* Ready = G.Branch(G.Read(Hint::HintsReady)); auto* ReadyTail = G.Tail;
    G.Tail = G.Pin(Ready, P::Else); SetStatus(Hint::NotReady); G.Tail = ReadyTail;
    auto* Seen = G.Branch(ReadNativeInputField(G, Station, StationClass, Hint::ProviderSeen));
    auto* SeenTail = G.Tail; G.Tail = G.Pin(Seen, P::Else); SetStatus(Hint::NoProvider);
    G.Tail = SeenTail; SetStatus(Hint::ProviderReady);
}

// UMG Construct occurs when attached: configure context before AddChild.
void AddStationHintConstruction(UWidgetBlueprint* Hud)
{
    UEdGraph* Graph = Hud->UbergraphPages[0]; FGraph G(Graph, nullptr);
    auto* Construct = NewObject<UK2Node_Event>(Graph);
    Construct->EventReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UUserWidget, Construct), UUserWidget::StaticClass());
    Construct->bOverrideFunction = true; G.Node(Construct); G.Tail = G.Pin(Construct, P::Then);
    // Reconstruct after Slate removal must not duplicate the same nested widget.
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool), G.Read(Hint::HintsReady), N::False));
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftClassPath));
    G.Default(Path, E::PathString, Hint::StockWidget);
    auto* Ref = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftClassPathToSoftClassRef));
    G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Ref, E::SoftClassPath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadClassAsset_Blocking));
    G.Link(G.Pin(Ref, P::ReturnValue), G.Pin(Load, E::AssetClass)); G.Exec(Load);
    auto* ClassCast = NewObject<UK2Node_ClassDynamicCast>(Graph); ClassCast->TargetType = UVoyageDynamicPlayerInputWidget::StaticClass(); ClassCast->SetPurity(false); G.Node(ClassCast);
    G.Link(G.Tail, G.Pin(ClassCast, P::Execute)); G.Link(G.Pin(Load, P::ReturnValue), ClassCast->GetCastSourcePin()); G.Tail = ClassCast->GetValidCastPin();
    [[maybe_unused]] constexpr auto OwningPlayerSignature = static_cast<APlayerController* (UUserWidget::*)() const>(&UUserWidget::GetOwningPlayer);
    auto* Player = G.Call(UUserWidget::StaticClass(), OP::OwningPlayerGetter);
    auto* Create = G.Call(UWidgetBlueprintLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, Create));
    G.Link(ClassCast->GetCastResultPin(), G.Pin(Create, Hint::WidgetType)); G.Link(G.Pin(Player, P::ReturnValue), G.Pin(Create, Hint::OwningPlayer)); G.Exec(Create);
    auto* Cast = NewObject<UK2Node_DynamicCast>(Graph); Cast->TargetType = UVoyageDynamicPlayerInputWidget::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(G.Pin(Create, P::ReturnValue), Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin();
    G.Write(Hint::HintInstance, Cast->GetCastResultPin());
    auto* Context = LoadObject<UVoyageInputContextAsset>(nullptr, HarpoonInputNames::Context); check(Context);
    auto* SetContext = NewObject<UK2Node_VariableSet>(Graph);
    const FName ContextField = GET_MEMBER_NAME_CHECKED(UVoyageDynamicPlayerInputWidget, ContextAsset);
    SetContext->VariableReference.SetExternalMember(ContextField, UVoyageDynamicPlayerInputWidget::StaticClass()); G.Node(SetContext);
    G.Link(Cast->GetCastResultPin(), G.Pin(SetContext, P::FunctionTarget)); G.Pin(SetContext, ContextField)->DefaultObject = Context; G.Exec(SetContext);
    auto* Filter = NewObject<UK2Node_VariableSet>(Graph);
    const FName FilterField = GET_MEMBER_NAME_CHECKED(UVoyageDynamicPlayerInputWidget, bFilterByActionType);
    Filter->VariableReference.SetExternalMember(FilterField, UVoyageDynamicPlayerInputWidget::StaticClass()); G.Node(Filter);
    G.Link(Cast->GetCastResultPin(), G.Pin(Filter, P::FunctionTarget)); G.Default(Filter, FilterField, N::True); G.Exec(Filter);
    auto* Add = G.Call(UVerticalBox::StaticClass(), GET_FUNCTION_NAME_CHECKED(UVerticalBox, AddChildToVerticalBox));
    G.Link(G.Read(Hint::Root), G.Pin(Add, P::FunctionTarget)); G.Link(Cast->GetCastResultPin(), G.Pin(Add, Hint::Content)); G.Exec(Add);
    G.Branch(G.Valid(G.Pin(Add, P::ReturnValue))); G.Write(Hint::HintsReady, nullptr, N::True);
}
