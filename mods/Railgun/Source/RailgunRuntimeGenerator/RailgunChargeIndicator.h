#pragma once

namespace RailgunChargeIndicator
{
inline const FName Owner(TEXT("RailgunChargeIndicatorOwner"));
inline const FName Component(TEXT("RailgunChargeIndicatorComponent"));
inline const FName Material(TEXT("RailgunChargeIndicatorMaterial"));
inline const FName LastLevel(TEXT("RailgunChargeIndicatorLastLevel"));
inline const FName BoundModule(TEXT("RailgunChargeIndicatorModule"));
inline const FName BindFunction(TEXT("BindRailgunChargeIndicator"));
inline const FName RefreshFunction(TEXT("RefreshRailgunChargeIndicator"));
inline const FName CallbackFunction(
    TEXT("OnRailgunChargeIndicatorModuleValueChanged"));
inline const FName ElementIndexPin(TEXT("ElementIndex"));
inline const FName OptionalNamePin(TEXT("OptionalName"));
inline const FName SourceMaterialPin(TEXT("SourceMaterial"));
inline const FName ParameterNamePin(TEXT("ParameterName"));
inline const FName ValuePin(TEXT("Value"));
inline const FName DoubleInputPin(TEXT("InDouble"));
inline const FName DynamicMaterialName(TEXT("RailgunChargeIndicatorMID"));
inline constexpr TCHAR FullLevel[] = TEXT("1.0");
}

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

void SetRailgunChargeIndicatorLevel(FGraph& G, UEdGraphPin* Level)
{
    auto* Set = G.Call(UMaterialInstanceDynamic::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UMaterialInstanceDynamic,
            SetScalarParameterValue));
    G.Link(G.Read(RailgunChargeIndicator::Material),
        G.Pin(Set, P::FunctionTarget));
    G.Default(Set, RailgunChargeIndicator::ParameterNamePin,
        *RailgunModelContract::ChargeIndicatorProgressParameter.ToString());
    if (Level)
    {
        auto* ToFloat = G.Call(UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                Conv_DoubleToFloat));
        G.Link(Level,
            G.Pin(ToFloat, RailgunChargeIndicator::DoubleInputPin));
        G.Link(G.Pin(ToFloat, P::ReturnValue),
            G.Pin(Set, RailgunChargeIndicator::ValuePin));
    }
    else
    {
        G.Default(Set, RailgunChargeIndicator::ValuePin, N::Zero);
    }
    G.Exec(Set);
}

void EnsureRailgunChargeIndicator(FGraph& G)
{
    G.Branch(G.Valid(G.Read(S::Anchor)));
    UEdGraphPin* CurrentOwner = ObserveCall(G, UActorComponent::StaticClass(),
        OP::ComponentOwner, G.Read(S::Anchor));
    G.Branch(G.Valid(CurrentOwner));
    UEdGraphPin* OwnerMatches = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        CurrentOwner, G.Read(RailgunChargeIndicator::Owner));
    UEdGraphPin* CacheObjectsValid = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Valid(G.Read(RailgunChargeIndicator::Component)),
        G.Valid(G.Read(RailgunChargeIndicator::Material)));
    auto* CacheReady = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        OwnerMatches, CacheObjectsValid));
    UEdGraphPin* CachedTail = G.Tail;

    G.Tail = G.Pin(CacheReady, P::Else);
    auto* Find = G.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetComponentsByTag));
    G.Link(CurrentOwner, G.Pin(Find, P::FunctionTarget));
    G.Pin(Find, OP::ComponentClass)->DefaultObject =
        UStaticMeshComponent::StaticClass();
    G.Default(Find, ActorScanGraphNames::ComponentTag,
        *RailgunModelContract::ChargeIndicatorTag.ToString());
    auto* Loop = ContextLoop(G, G.Pin(Find, P::ReturnValue));
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph);
    Cast->TargetType = UStaticMeshComponent::StaticClass();
    Cast->SetPurity(false);
    G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute));
    G.Link(G.Pin(Loop, CE::ArrayElement), Cast->GetCastSourcePin());
    G.Tail = Cast->GetValidCastPin();
    UEdGraphPin* IndicatorComponent = Cast->GetCastResultPin();

    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath));
    G.Default(Path, E::PathString,
        RailgunModelContract::ChargeIndicatorMaterialObjectPath);
    auto* Reference = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            Conv_SoftObjPathToSoftObjRef));
    G.Link(G.Pin(Path, P::ReturnValue),
        G.Pin(Reference, AssetLoadingGraphNames::SoftObjectPath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadAsset_Blocking));
    G.Link(G.Pin(Reference, P::ReturnValue),
        G.Pin(Load, AssetLoadingGraphNames::Asset));
    G.Exec(Load);
    auto* MaterialCast = NewObject<UK2Node_DynamicCast>(G.Graph);
    MaterialCast->TargetType = UMaterialInterface::StaticClass();
    MaterialCast->SetPurity(false);
    G.Node(MaterialCast);
    G.Link(G.Tail, G.Pin(MaterialCast, P::Execute));
    G.Link(G.Pin(Load, P::ReturnValue), MaterialCast->GetCastSourcePin());
    G.Tail = MaterialCast->GetValidCastPin();

    auto* Create = G.Call(UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent,
            CreateDynamicMaterialInstance));
    G.Link(IndicatorComponent, G.Pin(Create, P::FunctionTarget));
    G.Default(Create, RailgunChargeIndicator::ElementIndexPin, N::Zero);
    G.Link(MaterialCast->GetCastResultPin(),
        G.Pin(Create, RailgunChargeIndicator::SourceMaterialPin));
    G.Default(Create, RailgunChargeIndicator::OptionalNamePin,
        *RailgunChargeIndicator::DynamicMaterialName.ToString());
    G.Exec(Create);
    UEdGraphPin* DynamicMaterial = G.Pin(Create, P::ReturnValue);
    G.Branch(G.Valid(DynamicMaterial));
    G.Write(RailgunChargeIndicator::Owner, CurrentOwner);
    G.Write(RailgunChargeIndicator::Component, IndicatorComponent);
    G.Write(RailgunChargeIndicator::Material, DynamicMaterial);
    SetRailgunChargeIndicatorLevel(G, nullptr);
    G.Write(RailgunChargeIndicator::LastLevel, nullptr, N::Zero);
    StationMerge(G, {CachedTail, G.Tail});
}

void UpdateRailgunChargeIndicator(FGraph& G, UEdGraphPin* StoredEnergy)
{
    auto* Ratio = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble));
    G.Link(StoredEnergy, G.Pin(Ratio, P::Binary::LeftOperand));
    G.Link(RequiredEnergyAmount(G), G.Pin(Ratio, P::Binary::RightOperand));
    auto* Clamp = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FClamp));
    G.Link(G.Pin(Ratio, P::ReturnValue), G.Pin(Clamp, OP::ClampValue));
    G.Default(Clamp, OP::ClampMinimum, N::Zero);
    G.Default(Clamp, OP::ClampMaximum,
        RailgunChargeIndicator::FullLevel);
    UEdGraphPin* Level = G.Pin(Clamp, P::ReturnValue);
    G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_DoubleDouble),
        Level, G.Read(RailgunChargeIndicator::LastLevel)));
    SetRailgunChargeIndicatorLevel(G, Level);
    G.Write(RailgunChargeIndicator::LastLevel, Level);
}

UEdGraphPin* ReadRailgunChargeIndicatorEnergy(FGraph& G,
    UEdGraphPin* Module)
{
    auto* Get = G.Call(UVoyageModuleComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageModuleComponent,
            GetResourceAmount));
    G.Link(Module, G.Pin(Get, P::FunctionTarget));
    G.Default(Get, Charge::Type, Charge::Electricity);
    return G.Pin(Get, P::ReturnValue);
}

void AddRailgunChargeIndicatorFunctions(UBlueprint* BP)
{
    using namespace RailgunChargeIndicator;
    check(BP && BP->GeneratedClass);
    FMulticastDelegateProperty* DelegateProperty =
        RailgunModuleDelegateProperty(Charge::OnModuleValueChanged);

    UEdGraph* RefreshGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        RefreshFunction, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, RefreshGraph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* RefreshEntry = nullptr;
    for (UEdGraphNode* Node : RefreshGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            RefreshEntry = Candidate;
    check(RefreshEntry);
    RefreshEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Refresh(RefreshGraph, nullptr);
    Refresh.Tail = Refresh.Pin(RefreshEntry, P::Then);
    Refresh.Branch(Refresh.Valid(Refresh.Read(BoundModule)));
    EnsureRailgunChargeIndicator(Refresh);
    UpdateRailgunChargeIndicator(Refresh,
        ReadRailgunChargeIndicatorEnergy(Refresh,
            Refresh.Read(BoundModule)));

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(RefreshFunction));

    UEdGraph* CallbackGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        CallbackFunction, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, CallbackGraph, true,
        DelegateProperty->SignatureFunction.Get());
    UK2Node_FunctionEntry* CallbackEntry = nullptr;
    for (UEdGraphNode* Node : CallbackGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            CallbackEntry = Candidate;
    check(CallbackEntry);
    CallbackEntry->FindPinChecked(P::Then)->BreakAllPinLinks();
    FGraph Callback(CallbackGraph, nullptr);
    Callback.Tail = Callback.Pin(CallbackEntry, P::Then);
    UEdGraphPin* ChangedModule = Callback.Pin(CallbackEntry,
        Charge::ModuleParameter);
    Callback.Branch(Callback.Valid(ChangedModule));
    Callback.Branch(Callback.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_ObjectObject), ChangedModule,
        Callback.Read(BoundModule)));
    auto* RefreshCall = Callback.Call(BP->GeneratedClass,
        RefreshFunction);
    Callback.Exec(RefreshCall);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(CallbackFunction));

    UEdGraph* BindGraph = FBlueprintEditorUtils::CreateNewGraph(BP,
        BindFunction, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, BindGraph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* BindEntry = nullptr;
    for (UEdGraphNode* Node : BindGraph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            BindEntry = Candidate;
    check(BindEntry);
    BindEntry->FindPinChecked(P::Then)->BreakAllPinLinks();

    FGraph Bind(BindGraph, nullptr);
    Bind.Tail = Bind.Pin(BindEntry, P::Then);
    auto* CallbackDelegate = Bind.Node(
        NewObject<UK2Node_CreateDelegate>(BindGraph));
    CallbackDelegate->SetFunction(CallbackFunction);
    auto* RemovePrevious = NewObject<UK2Node_RemoveDelegate>(BindGraph);
    RemovePrevious->SetFromProperty(DelegateProperty, false,
        UVoyageModuleComponent::StaticClass());
    Bind.Node(RemovePrevious);
    auto* RemoveCurrent = NewObject<UK2Node_RemoveDelegate>(BindGraph);
    RemoveCurrent->SetFromProperty(DelegateProperty, false,
        UVoyageModuleComponent::StaticClass());
    Bind.Node(RemoveCurrent);
    auto* Add = NewObject<UK2Node_AddDelegate>(BindGraph);
    Add->SetFromProperty(DelegateProperty, false,
        UVoyageModuleComponent::StaticClass());
    Bind.Node(Add);
    Bind.Link(CallbackDelegate->GetDelegateOutPin(),
        RemovePrevious->GetDelegatePin());
    Bind.Link(CallbackDelegate->GetDelegateOutPin(),
        RemoveCurrent->GetDelegatePin());
    Bind.Link(CallbackDelegate->GetDelegateOutPin(),
        Add->GetDelegatePin());

    auto* PreviousValid = Bind.Branch(Bind.Valid(Bind.Read(BoundModule)));
    Bind.Link(Bind.Read(BoundModule),
        Bind.Pin(RemovePrevious, P::FunctionTarget));
    Bind.Exec(RemovePrevious);
    UEdGraphPin* RemovedTail = Bind.Tail;
    Bind.Tail = Bind.Pin(PreviousValid, P::Else);
    StationMerge(Bind, {RemovedTail, Bind.Tail});
    Bind.Write(BoundModule, nullptr);

    Bind.Branch(Bind.Valid(Bind.Read(Charge::Module)));
    UEdGraphPin* Module = Bind.Read(Charge::Module);
    Bind.Write(BoundModule, Module);
    Bind.Link(Module, Bind.Pin(RemoveCurrent, P::FunctionTarget));
    Bind.Exec(RemoveCurrent);
    Bind.Link(Module, Bind.Pin(Add, P::FunctionTarget));
    Bind.Exec(Add);
    auto* InitialRefresh = Bind.Call(BP->GeneratedClass,
        RefreshFunction);
    Bind.Exec(InitialRefresh);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(BindFunction));
}

void AddRailgunChargeIndicatorTeardown(UBlueprint* BP)
{
    using namespace RailgunChargeIndicator;
    check(BP && BP->UbergraphPages.Num() == 1);
    UEdGraph* Graph = BP->UbergraphPages[0];
    UK2Node_Event* EndPlay = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        auto* Event = Cast<UK2Node_Event>(Node);
        if (!Event || Event->EventReference.GetMemberName() !=
            ActorLifecycleGraphNames::EndPlayEvent) continue;
        check(!EndPlay);
        EndPlay = Event;
    }
    check(EndPlay);
    UEdGraphPin* EndPlayTail = EndPlay->FindPinChecked(P::Then);
    check(EndPlayTail->LinkedTo.Num() == 1);
    UEdGraphPin* ExistingWork = EndPlayTail->LinkedTo[0];
    EndPlayTail->BreakAllPinLinks();

    FGraph G(Graph, nullptr);
    auto* Work = G.Node(NewObject<UK2Node_ExecutionSequence>(Graph));
    G.Link(EndPlayTail, G.Pin(Work, P::Execute));
    G.Link(Work->GetThenPinGivenIndex(0), ExistingWork);
    G.Tail = Work->GetThenPinGivenIndex(1);

    auto* DelegateProperty = RailgunModuleDelegateProperty(
        Charge::OnModuleValueChanged);
    auto* Callback = G.Node(NewObject<UK2Node_CreateDelegate>(Graph));
    Callback->SetFunction(CallbackFunction);
    auto* Remove = NewObject<UK2Node_RemoveDelegate>(Graph);
    Remove->SetFromProperty(DelegateProperty, false,
        UVoyageModuleComponent::StaticClass());
    G.Node(Remove);
    G.Link(Callback->GetDelegateOutPin(), Remove->GetDelegatePin());
    auto* BoundValid = G.Branch(G.Valid(G.Read(BoundModule)));
    G.Link(G.Read(BoundModule), G.Pin(Remove, P::FunctionTarget));
    G.Exec(Remove);
    UEdGraphPin* RemovedTail = G.Tail;
    G.Tail = G.Pin(BoundValid, P::Else);
    StationMerge(G, {RemovedTail, G.Tail});
    for (FName Field : {BoundModule, Owner, Component, Material})
        G.Write(Field, nullptr);
    G.Write(LastLevel, nullptr, N::Zero);
}
