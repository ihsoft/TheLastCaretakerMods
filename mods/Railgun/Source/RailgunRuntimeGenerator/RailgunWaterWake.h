#pragma once

namespace RailgunWaterWake
{
inline constexpr TCHAR ControllerPackage[] =
    TEXT("/Game/Mods/Railgun/Station/BP_RailgunWaterWakeController");
inline const FName AddSample(TEXT("AddWakeSample"));
inline const FName Finish(TEXT("FinishWake"));
inline const FName ProcessSegment(TEXT("ProcessWaterWakeSegment"));
inline const FName Controller(TEXT("WaterWakeController"));
inline const FName Locations(TEXT("WakeLocations"));
inline const FName Strengths(TEXT("WakeStrengths"));
inline const FName Expiries(TEXT("WakeExpiries"));
inline const FName Accepting(TEXT("WakeAcceptingSamples"));
inline const FName Alive(TEXT("WakeAliveThisTick"));
inline const FName DistanceToNext(TEXT("WaterWakeDistanceToNextSample"));
inline const FName PreviewDistance(TEXT("WaterWakePreviewDistance"));
inline const FName Start(TEXT("Start"));
inline const FName End(TEXT("End"));
inline const FName FlushEnd(TEXT("FlushEnd"));
inline const FName SampleLocation(TEXT("SampleLocation"));
inline const FName SampleStrength(TEXT("SampleStrength"));
inline const FName ContextObject(TEXT("ContextObject"));
inline const FName ClassPin(TEXT("Class"));
inline const FName Impulse(TEXT("Impulse"));
inline const FName Location(TEXT("Location"));
inline const FName Direction(TEXT("Direction"));
inline const FName Radius(TEXT("Radius"));
inline const FName Strength(TEXT("Strength"));
inline const FName X(TEXT("X"));
inline const FName Y(TEXT("Y"));
inline const FName Z(TEXT("Z"));
inline const FName ArrayItem(TEXT("Item"));
inline const FName NewItem(TEXT("NewItem"));

inline const FName LifePin(TEXT("InLifespan"));

inline constexpr TCHAR SampleSpacingCentimeters[] = TEXT("200.0");
inline constexpr TCHAR IntegerOne[] = TEXT("1");
inline constexpr TCHAR MaximumSamplesPerSegment[] = TEXT("256");
inline constexpr TCHAR MaximumSamples[] = TEXT("256");
inline constexpr TCHAR MaximumPreviewDistanceCentimeters[] = TEXT("50000.0");
inline constexpr TCHAR MaximumHeightAboveWaterCentimeters[] = TEXT("800.0");
inline constexpr TCHAR WaterQueryLoweringCentimeters[] =
    TEXT("(X=0.0,Y=0.0,Z=-800.0)");
inline constexpr TCHAR ExactEndToleranceCentimeters[] = TEXT("199.999");
inline constexpr TCHAR ImpulseRadiusCentimeters[] = TEXT("200.0");
inline constexpr TCHAR ZeroDirection[] = TEXT("(X=0.0,Y=0.0)");
inline constexpr TCHAR MaximumImpulseStrength[] = TEXT("0.2");
inline constexpr TCHAR SampleLifetimeSeconds[] = TEXT("0.25");
inline constexpr TCHAR ControllerMaximumLifetimeSeconds[] = TEXT("1.0");
inline UClass* ControllerClass = nullptr;
}

UEdGraphPin* RailgunWaterWakeIntWithLiteral(FGraph& G, FName Function,
    UEdGraphPin* Value, const TCHAR* Literal)
{
    auto* Node = G.Call(UKismetMathLibrary::StaticClass(), Function);
    G.Link(Value, G.Pin(Node, P::Binary::LeftOperand));
    G.Default(Node, P::Binary::RightOperand, Literal);
    return G.Pin(Node, P::ReturnValue);
}

void QueueRailgunWaterWakeSample(FGraph& G, UEdGraphPin* Point,
    UEdGraphPin* World)
{
    auto* Lowered = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector));
    G.Link(Point, G.Pin(Lowered, P::Binary::LeftOperand));
    G.Default(Lowered, P::Binary::RightOperand,
        RailgunWaterWake::WaterQueryLoweringCentimeters);
    FRailgunWaterSample Sample = SampleRailgunWater(
        G, World, G.Pin(Lowered, P::ReturnValue));
    auto* Valid = G.Branch(Sample.Valid);
    UEdGraphPin* Invalid = G.Pin(Valid, P::Else);

    auto* BreakPoint = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector));
    G.Link(Point, G.Pin(BreakPoint, RailgunWater::VectorInput));
    UEdGraphPin* HeightAbove = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_DoubleDouble),
        G.Pin(BreakPoint, RailgunWater::VectorZ), Sample.Height);
    UEdGraphPin* AtOrAbove = G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            GreaterEqual_DoubleDouble), HeightAbove, N::Zero);
    UEdGraphPin* WithinHeight = G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            LessEqual_DoubleDouble), HeightAbove,
        RailgunWaterWake::MaximumHeightAboveWaterCentimeters);
    auto* Near = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        AtOrAbove, WithinHeight));
    UEdGraphPin* NotNear = G.Pin(Near, P::Else);

    auto* Surface = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeVector));
    G.Link(G.Pin(BreakPoint, RailgunWaterWake::X),
        G.Pin(Surface, RailgunWaterWake::X));
    G.Link(G.Pin(BreakPoint, RailgunWaterWake::Y),
        G.Pin(Surface, RailgunWaterWake::Y));
    G.Link(Sample.Height, G.Pin(Surface, RailgunWaterWake::Z));

    auto* HeightRatio = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Divide_DoubleDouble));
    G.Link(HeightAbove, G.Pin(HeightRatio, P::Binary::LeftOperand));
    G.Default(HeightRatio, P::Binary::RightOperand,
        RailgunWaterWake::MaximumHeightAboveWaterCentimeters);
    auto* Falloff = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Subtract_DoubleDouble));
    G.Default(Falloff, P::Binary::LeftOperand,
        RailgunWaterWake::IntegerOne);
    G.Link(G.Pin(HeightRatio, P::ReturnValue),
        G.Pin(Falloff, P::Binary::RightOperand));
    auto* Strength = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Multiply_DoubleDouble));
    G.Default(Strength, P::Binary::LeftOperand,
        RailgunWaterWake::MaximumImpulseStrength);
    G.Link(G.Pin(Falloff, P::ReturnValue),
        G.Pin(Strength, P::Binary::RightOperand));
    auto* AddSample = G.Call(RailgunWaterWake::ControllerClass,
        RailgunWaterWake::AddSample);
    G.Link(G.Read(RailgunWaterWake::Controller),
        G.Pin(AddSample, P::FunctionTarget));
    G.Link(G.Pin(Surface, P::ReturnValue),
        G.Pin(AddSample, RailgunWaterWake::SampleLocation));
    G.Link(G.Pin(Strength, P::ReturnValue),
        G.Pin(AddSample, RailgunWaterWake::SampleStrength));
    G.Exec(AddSample);
    UEdGraphPin* SubmittedTail = G.Tail;

    G.Tail = Invalid;
    StationMerge(G, {SubmittedTail, Invalid, NotNear});
}

void AddRailgunWaterWakeSegmentFunction(UBlueprint* BP)
{
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP,
        RailgunWaterWake::ProcessSegment, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
        if (auto* Candidate = Cast<UK2Node_FunctionResult>(Node)) Result = Candidate;
    }
    check(Entry);
    if (!Result)
    {
        Result = NewObject<UK2Node_FunctionResult>(Graph);
        Result->FunctionReference.SetSelfMember(
            RailgunWaterWake::ProcessSegment);
        Result->CreateNewGuid();
        Result->PostPlacedNewNode();
        Result->AllocateDefaultPins();
        Graph->AddNode(Result, true, false);
    }

    const FEdGraphPinType VectorType = RailgunWaterPinType(
        UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
    const FEdGraphPinType BoolType = RailgunWaterPinType(
        UEdGraphSchema_K2::PC_Boolean);
    UEdGraphPin* Start = Entry->CreateUserDefinedPin(
        RailgunWaterWake::Start, VectorType, EGPD_Output);
    UEdGraphPin* End = Entry->CreateUserDefinedPin(
        RailgunWaterWake::End, VectorType, EGPD_Output);
    UEdGraphPin* FlushEnd = Entry->CreateUserDefinedPin(
        RailgunWaterWake::FlushEnd, BoolType, EGPD_Output);
    check(Start && End && FlushEnd);

    Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
    Result->FindPinChecked(P::Execute)->BreakAllPinLinks();
    FGraph G(Graph);
    G.Tail = G.Pin(Entry, P::Then);
    TArray<UEdGraphPin*> ExitPaths;
    auto* ControllerValid = G.Branch(
        G.Valid(G.Read(RailgunWaterWake::Controller)));
    ExitPaths.Add(G.Pin(ControllerValid, P::Else));
    auto* RemainingPreviewNode = G.Call(
        UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Subtract_DoubleDouble));
    G.Default(RemainingPreviewNode, P::Binary::LeftOperand,
        RailgunWaterWake::MaximumPreviewDistanceCentimeters);
    G.Link(G.Read(RailgunWaterWake::PreviewDistance),
        G.Pin(RemainingPreviewNode, P::Binary::RightOperand));
    UEdGraphPin* RemainingPreview =
        G.Pin(RemainingPreviewNode, P::ReturnValue);
    auto* HasPreview = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble), RemainingPreview, N::Zero));
    UEdGraphPin* PreviewComplete = G.Pin(HasPreview, P::Else);

    UEdGraphPin* Delta = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Subtract_VectorVector), End, Start);
    auto* LengthNode = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(Delta, G.Pin(LengthNode, E::VectorLengthInput));
    UEdGraphPin* Length = G.Pin(LengthNode, P::ReturnValue);
    auto* NonEmpty = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble), Length, N::Zero));
    ExitPaths.Add(G.Pin(NonEmpty, P::Else));
    auto* Bounded = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            LessEqual_DoubleDouble), Length,
        RailgunWater::MaximumSegmentCentimeters));
    ExitPaths.Add(G.Pin(Bounded, P::Else));

    auto* EffectiveLengthNode = G.Call(
        UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMin));
    G.Link(Length,
        G.Pin(EffectiveLengthNode, P::Binary::LeftOperand));
    G.Link(RemainingPreview,
        G.Pin(EffectiveLengthNode, P::Binary::RightOperand));
    UEdGraphPin* EffectiveLength =
        G.Pin(EffectiveLengthNode, P::ReturnValue);
    UEdGraphPin* EffectiveAlpha = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Divide_DoubleDouble), EffectiveLength, Length);
    UEdGraphPin* EffectiveEnd = RailgunWaterVectorLerp(
        G, Start, End, EffectiveAlpha);

    UEdGraphPin* Offset = G.Read(RailgunWaterWake::DistanceToNext);
    UEdGraphPin* Remaining = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Subtract_DoubleDouble), EffectiveLength, Offset);
    auto* Ratio = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble));
    G.Link(Remaining, G.Pin(Ratio, P::Binary::LeftOperand));
    G.Default(Ratio, P::Binary::RightOperand,
        RailgunWaterWake::SampleSpacingCentimeters);
    auto* Floor = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FFloor));
    G.Link(G.Pin(Ratio, P::ReturnValue),
        G.Pin(Floor, P::Binary::LeftOperand));
    UEdGraphPin* RawCount = RailgunWaterWakeIntWithLiteral(G,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt),
        G.Pin(Floor, P::ReturnValue), RailgunWaterWake::IntegerOne);
    auto* CountNode = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Clamp));
    G.Link(RawCount, G.Pin(CountNode, P::Value));
    G.Default(CountNode, P::Min, N::Zero);
    G.Default(CountNode, P::Max,
        RailgunWaterWake::MaximumSamplesPerSegment);
    UEdGraphPin* Count = G.Pin(CountNode, P::ReturnValue);

    auto* CountAsDouble = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_IntToDouble));
    G.Link(Count, G.Pin(CountAsDouble, RailgunWater::IntegerInput));
    auto* CoveredNode = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Multiply_DoubleDouble));
    G.Link(G.Pin(CountAsDouble, P::ReturnValue),
        G.Pin(CoveredNode, P::Binary::LeftOperand));
    G.Default(CoveredNode, P::Binary::RightOperand,
        RailgunWaterWake::SampleSpacingCentimeters);
    UEdGraphPin* CoveredAfterOffset = G.Pin(CoveredNode, P::ReturnValue);
    UEdGraphPin* NewOffset = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Subtract_DoubleDouble),
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Add_DoubleDouble), Offset, CoveredAfterOffset), EffectiveLength);
    UEdGraphPin* NeedEndpoint = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), FlushEnd,
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Less_DoubleDouble), NewOffset,
            RailgunWaterWake::ExactEndToleranceCentimeters));
    UEdGraphPin* NeedAny = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_IntInt), Count, N::Zero), NeedEndpoint);
    auto* HasSamples = G.Branch(NeedAny);
    UEdGraphPin* NoSamples = G.Pin(HasSamples, P::Else);

    auto* GetWorld = G.Call(UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageMiscBlueprintFunctionLibrary,
            GetActorWorld));
    G.Link(OpticalSelf(G), G.Pin(GetWorld, P::Actor));
    UEdGraphPin* World = G.Pin(GetWorld, P::ReturnValue);
    auto* WorldValid = G.Branch(G.Valid(World));
    UEdGraphPin* InvalidWorld = G.Pin(WorldValid, P::Else);

    UEdGraphPin* LastIndex = RailgunWaterWakeIntWithLiteral(G,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt),
        Count, RailgunWaterWake::IntegerOne);
    auto* Loop = RailgunWaterLoop(G, LastIndex, nullptr, N::Zero);
    auto* IndexAsDouble = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_IntToDouble));
    G.Link(G.Pin(Loop, P::Index),
        G.Pin(IndexAsDouble, RailgunWater::IntegerInput));
    auto* DistanceScale = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Multiply_DoubleDouble));
    G.Link(G.Pin(IndexAsDouble, P::ReturnValue),
        G.Pin(DistanceScale, P::Binary::LeftOperand));
    G.Default(DistanceScale, P::Binary::RightOperand,
        RailgunWaterWake::SampleSpacingCentimeters);
    UEdGraphPin* SampleDistance = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_DoubleDouble), Offset,
        G.Pin(DistanceScale, P::ReturnValue));
    UEdGraphPin* Alpha = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Divide_DoubleDouble), SampleDistance, EffectiveLength);
    UEdGraphPin* Point = RailgunWaterVectorLerp(
        G, Start, EffectiveEnd, Alpha);
    QueueRailgunWaterWakeSample(G, Point, World);
    G.Tail = G.Pin(Loop, P::Completed);

    auto* Flush = G.Branch(NeedEndpoint);
    UEdGraphPin* NoFlush = G.Pin(Flush, P::Else);
    QueueRailgunWaterWakeSample(G, EffectiveEnd, World);
    StationMerge(G, {G.Tail, NoFlush});
    UEdGraphPin* SamplesDone = G.Tail;

    G.Tail = InvalidWorld;
    UEdGraphPin* InvalidWorldTail = G.Tail;
    StationMerge(G, {SamplesDone, InvalidWorldTail, NoSamples});
    G.Write(RailgunWaterWake::DistanceToNext, NewOffset);
    G.Write(RailgunWaterWake::PreviewDistance,
        G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Add_DoubleDouble),
            G.Read(RailgunWaterWake::PreviewDistance), EffectiveLength));
    auto* ReachedLimit = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            GreaterEqual_DoubleDouble),
        G.Read(RailgunWaterWake::PreviewDistance),
        RailgunWaterWake::MaximumPreviewDistanceCentimeters));
    UEdGraphPin* MorePreview = G.Pin(ReachedLimit, P::Else);
    auto* Finish = G.Call(RailgunWaterWake::ControllerClass,
        RailgunWaterWake::Finish);
    G.Link(G.Read(RailgunWaterWake::Controller),
        G.Pin(Finish, P::FunctionTarget));
    G.Exec(Finish);
    StationMerge(G, {G.Tail, MorePreview});

    ExitPaths.Add(G.Tail);
    G.Tail = PreviewComplete;
    auto* FinishComplete = G.Call(RailgunWaterWake::ControllerClass,
        RailgunWaterWake::Finish);
    G.Link(G.Read(RailgunWaterWake::Controller),
        G.Pin(FinishComplete, P::FunctionTarget));
    G.Exec(FinishComplete);
    ExitPaths.Add(G.Tail);
    for (UEdGraphPin* Path : ExitPaths)
        G.Link(Path, G.Pin(Result, P::Execute));
}

void ProcessRailgunWaterWakeSegment(FGraph& G, UClass* ShotClass,
    UEdGraphPin* Start, UEdGraphPin* End, bool FlushEnd)
{
    auto* Process = G.Call(ShotClass, RailgunWaterWake::ProcessSegment);
    G.Link(Start, G.Pin(Process, RailgunWaterWake::Start));
    G.Link(End, G.Pin(Process, RailgunWaterWake::End));
    G.Default(Process, RailgunWaterWake::FlushEnd,
        FlushEnd ? N::True : N::False);
    G.Exec(Process);
}

UEdGraphPin* RailgunWaterWakeArrayLength(FGraph& G, FName ArrayName)
{
    auto* Length = G.ArrayCall(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length));
    G.Link(G.Read(ArrayName), G.Pin(Length, P::TargetArray));
    return G.Pin(Length, P::ReturnValue);
}

UEdGraphPin* RailgunWaterWakeArrayGet(FGraph& G, FName ArrayName,
    UEdGraphPin* Index)
{
    auto* Get = G.ArrayCall(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Get));
    G.Link(G.Read(ArrayName), G.Pin(Get, P::TargetArray));
    G.Link(Index, G.Pin(Get, P::Index));
    return G.Pin(Get, RailgunWaterWake::ArrayItem);
}

void RailgunWaterWakeArrayAdd(FGraph& G, FName ArrayName,
    UEdGraphPin* Item)
{
    auto* Add = G.ArrayCall(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Add));
    G.Link(G.Read(ArrayName), G.Pin(Add, P::TargetArray));
    G.Link(Item, G.Pin(Add, RailgunWaterWake::NewItem));
    G.Exec(Add);
}

void CreateRailgunWaterWakeFunction(UBlueprint* BP, FName Name,
    UEdGraph*& Graph, UK2Node_FunctionEntry*& Entry,
    UK2Node_FunctionResult*& Result)
{
    Graph = FBlueprintEditorUtils::CreateNewGraph(BP, Name,
        UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
        static_cast<UClass*>(nullptr));
    Entry = nullptr;
    Result = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node))
            Entry = Candidate;
        if (auto* Candidate = Cast<UK2Node_FunctionResult>(Node))
            Result = Candidate;
    }
    check(Entry);
    if (!Result)
    {
        Result = NewObject<UK2Node_FunctionResult>(Graph);
        Result->FunctionReference.SetSelfMember(Name);
        Result->CreateNewGuid();
        Result->PostPlacedNewNode();
        Result->AllocateDefaultPins();
        Graph->AddNode(Result, true, false);
    }
    Entry->FindPinChecked(P::Then)->BreakAllPinLinks();
    Result->FindPinChecked(P::Execute)->BreakAllPinLinks();
}

void AddRailgunWaterWakeControllerFunctions(UBlueprint* BP)
{
    UEdGraph* AddGraph = nullptr;
    UK2Node_FunctionEntry* AddEntry = nullptr;
    UK2Node_FunctionResult* AddResult = nullptr;
    CreateRailgunWaterWakeFunction(BP, RailgunWaterWake::AddSample,
        AddGraph, AddEntry, AddResult);
    UEdGraphPin* AddLocation = AddEntry->CreateUserDefinedPin(
        RailgunWaterWake::SampleLocation,
        RailgunWaterPinType(UEdGraphSchema_K2::PC_Struct,
            TBaseStructure<FVector>::Get()), EGPD_Output);
    UEdGraphPin* AddStrength = AddEntry->CreateUserDefinedPin(
        RailgunWaterWake::SampleStrength,
        RailgunWaterPinType(UEdGraphSchema_K2::PC_Real), EGPD_Output);
    check(AddLocation && AddStrength);
    FGraph Add(AddGraph);
    Add.Tail = Add.Pin(AddEntry, P::Then);
    auto* Accepting = Add.Branch(
        Add.Read(RailgunWaterWake::Accepting));
    UEdGraphPin* Rejected = Add.Pin(Accepting, P::Else);
    auto* Capacity = Add.Branch(Add.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_IntInt),
        RailgunWaterWakeArrayLength(Add,
            RailgunWaterWake::Locations),
        RailgunWaterWake::MaximumSamples));
    UEdGraphPin* Full = Add.Pin(Capacity, P::Else);
    RailgunWaterWakeArrayAdd(Add,
        RailgunWaterWake::Locations, AddLocation);
    RailgunWaterWakeArrayAdd(Add,
        RailgunWaterWake::Strengths, AddStrength);
    auto* Time = Add.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            GetGameTimeInSeconds));
    auto* Expiry = Add.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Add_DoubleDouble));
    Add.Link(Add.Pin(Time, P::ReturnValue),
        Add.Pin(Expiry, P::Binary::LeftOperand));
    Add.Default(Expiry, P::Binary::RightOperand,
        RailgunWaterWake::SampleLifetimeSeconds);
    RailgunWaterWakeArrayAdd(Add, RailgunWaterWake::Expiries,
        Add.Pin(Expiry, P::ReturnValue));
    UEdGraphPin* Added = Add.Tail;
    StationMerge(Add, {Added, Full, Rejected});
    Add.Link(Add.Tail, Add.Pin(AddResult, P::Execute));

    UEdGraph* FinishGraph = nullptr;
    UK2Node_FunctionEntry* FinishEntry = nullptr;
    UK2Node_FunctionResult* FinishResult = nullptr;
    CreateRailgunWaterWakeFunction(BP, RailgunWaterWake::Finish,
        FinishGraph, FinishEntry, FinishResult);
    FGraph Finish(FinishGraph);
    Finish.Tail = Finish.Pin(FinishEntry, P::Then);
    Finish.Write(RailgunWaterWake::Accepting, nullptr, N::False);
    Finish.Link(Finish.Tail, Finish.Pin(FinishResult, P::Execute));
}

void SubmitRailgunWaterWakeControllerImpulse(FGraph& G,
    UEdGraphPin* Weather, UEdGraphPin* Index)
{
    auto* Impulse = NewObject<UK2Node_MakeStruct>(G.Graph);
    Impulse->StructType = FVoyageFluidImpulse::StaticStruct();
    Impulse->bMadeAfterOverridePinRemoval = true;
    G.Node(Impulse);
    G.Link(RailgunWaterWakeArrayGet(G,
        RailgunWaterWake::Locations, Index),
        G.Pin(Impulse, RailgunWaterWake::Location));
    G.Default(Impulse, RailgunWaterWake::Direction,
        RailgunWaterWake::ZeroDirection);
    G.Default(Impulse, RailgunWaterWake::Radius,
        RailgunWaterWake::ImpulseRadiusCentimeters);
    G.Link(RailgunWaterWakeArrayGet(G,
        RailgunWaterWake::Strengths, Index),
        G.Pin(Impulse, RailgunWaterWake::Strength));
    auto* AddImpulse = G.Call(UVoyageWeatherSubsystem::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageWeatherSubsystem,
            AddFluidImpulse));
    G.Link(Weather, G.Pin(AddImpulse, P::FunctionTarget));
    for (UEdGraphPin* Pin : Impulse->Pins)
        if (Pin->Direction == EGPD_Output)
            G.Link(Pin, G.Pin(AddImpulse, RailgunWaterWake::Impulse));
    G.Exec(AddImpulse);
}

UK2Node_Event* RailgunWaterWakeEvent(FGraph& G, FName Name)
{
    auto* Event = NewObject<UK2Node_Event>(G.Graph);
    Event->EventReference.SetExternalMember(Name, AActor::StaticClass());
    Event->bOverrideFunction = true;
    G.Node(Event);
    G.Tail = G.Pin(Event, P::Then);
    return Event;
}

UClass* CreateRailgunWaterWakeController()
{
    UPackage* Package = CreatePackage(
        RailgunWaterWake::ControllerPackage);
    UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(
        AActor::StaticClass(), Package,
        FName(FPackageName::GetLongPackageAssetName(
            RailgunWaterWake::ControllerPackage)), BPTYPE_Normal,
        UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    check(BP);
    const auto Nodes = BP->UbergraphPages[0]->Nodes;
    for (UEdGraphNode* Node : Nodes)
        Node->DestroyNode();
    AddArrayVariable(BP, RailgunWaterWake::Locations,
        UEdGraphSchema_K2::PC_Struct,
        TBaseStructure<FVector>::Get());
    AddArrayVariable(BP, RailgunWaterWake::Strengths,
        UEdGraphSchema_K2::PC_Real);
    AddArrayVariable(BP, RailgunWaterWake::Expiries,
        UEdGraphSchema_K2::PC_Real);
    AddVariable(BP, RailgunWaterWake::Accepting,
        UEdGraphSchema_K2::PC_Boolean);
    AddVariable(BP, RailgunWaterWake::Alive,
        UEdGraphSchema_K2::PC_Boolean);
    FKismetEditorUtilities::CompileBlueprint(BP);
    AddRailgunWaterWakeControllerFunctions(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);

    FGraph G(BP->UbergraphPages[0]);
    RailgunWaterWakeEvent(G, TimerGraphNames::ActorBeginPlay);
    G.Write(RailgunWaterWake::Accepting, nullptr, N::True);
    auto* Life = G.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, SetLifeSpan));
    G.Default(Life, RailgunWaterWake::LifePin,
        RailgunWaterWake::ControllerMaximumLifetimeSeconds);
    G.Exec(Life);

    RailgunWaterWakeEvent(
        G, BlueprintGraphNames::Events::ActorReceiveTick);
    G.Write(RailgunWaterWake::Alive, nullptr, N::False);
    UEdGraphPin* Count = RailgunWaterWakeArrayLength(
        G, RailgunWaterWake::Locations);
    auto* HasSamples = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Greater_IntInt),
        Count, N::Zero));
    UEdGraphPin* NoSamples = G.Pin(HasSamples, P::Else);
    UEdGraphPin* LastIndex = RailgunWaterWakeIntWithLiteral(G,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt),
        Count, RailgunWaterWake::IntegerOne);
    auto* AliveLoop = RailgunWaterLoop(
        G, LastIndex, nullptr, N::Zero);
    UEdGraphPin* AliveIndex = G.Pin(AliveLoop, P::Index);
    auto* Now = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            GetGameTimeInSeconds));
    auto* Active = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Less_DoubleDouble), G.Pin(Now, P::ReturnValue),
        RailgunWaterWakeArrayGet(G,
            RailgunWaterWake::Expiries, AliveIndex)));
    UEdGraphPin* Expired = G.Pin(Active, P::Else);
    G.Write(RailgunWaterWake::Alive, nullptr, N::True);
    StationMerge(G, {G.Tail, Expired});
    G.Tail = G.Pin(AliveLoop, P::Completed);
    UEdGraphPin* AliveScanComplete = G.Tail;
    StationMerge(G, {AliveScanComplete, NoSamples});

    auto* HasActiveSamples = G.Branch(
        G.Read(RailgunWaterWake::Alive));
    UEdGraphPin* NoneActive = G.Pin(HasActiveSamples, P::Else);
    auto* GetSubsystem = G.Call(
        USubsystemBlueprintLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(USubsystemBlueprintLibrary,
            GetWorldSubsystem));
    G.Link(OpticalSelf(G), G.Pin(GetSubsystem,
        RailgunWaterWake::ContextObject));
    G.Pin(GetSubsystem, RailgunWaterWake::ClassPin)->DefaultObject =
        UVoyageWeatherSubsystem::StaticClass();
    auto* WeatherCast = NewObject<UK2Node_DynamicCast>(G.Graph);
    WeatherCast->TargetType = UVoyageWeatherSubsystem::StaticClass();
    WeatherCast->SetPurity(false);
    G.Node(WeatherCast);
    G.Link(G.Tail, G.Pin(WeatherCast, P::Execute));
    G.Link(G.Pin(GetSubsystem, P::ReturnValue),
        WeatherCast->GetCastSourcePin());
    G.Tail = WeatherCast->GetValidCastPin();
    UEdGraphPin* Weather = WeatherCast->GetCastResultPin();
    UEdGraphPin* InvalidWeather = WeatherCast->GetInvalidCastPin();
    auto* SubmitLoop = RailgunWaterLoop(
        G, LastIndex, nullptr, N::Zero);
    UEdGraphPin* SubmitIndex = G.Pin(SubmitLoop, P::Index);
    auto* SubmitNow = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            GetGameTimeInSeconds));
    auto* SubmitActive = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Less_DoubleDouble), G.Pin(SubmitNow, P::ReturnValue),
        RailgunWaterWakeArrayGet(G,
            RailgunWaterWake::Expiries, SubmitIndex)));
    UEdGraphPin* SkipExpired = G.Pin(SubmitActive, P::Else);
    SubmitRailgunWaterWakeControllerImpulse(G, Weather, SubmitIndex);
    StationMerge(G, {G.Tail, SkipExpired});
    G.Tail = G.Pin(SubmitLoop, P::Completed);
    UEdGraphPin* SubmissionComplete = G.Tail;
    G.Tail = InvalidWeather;
    StationMerge(G, {SubmissionComplete, G.Tail, NoneActive});

    UEdGraphPin* Finished = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_BoolBool),
            G.Read(RailgunWaterWake::Accepting), N::False),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_BoolBool),
            G.Read(RailgunWaterWake::Alive), N::False));
    G.Branch(Finished);
    auto* Destroy = G.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor));
    G.Exec(Destroy);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error);
    auto* CDO = BP->GeneratedClass->GetDefaultObject<AActor>();
    CDO->PrimaryActorTick.bCanEverTick = true;
    CDO->PrimaryActorTick.bStartWithTickEnabled = true;
    CDO->PrimaryActorTick.TickGroup = TG_PostPhysics;
    CDO->SetActorEnableCollision(false);
    check(SaveDedicatedAsset(BP));
    return BP->GeneratedClass;
}

void FinishRailgunWaterWake(FGraph& G)
{
    auto* Valid = G.Branch(
        G.Valid(G.Read(RailgunWaterWake::Controller)));
    UEdGraphPin* Missing = G.Pin(Valid, P::Else);
    auto* Finish = G.Call(RailgunWaterWake::ControllerClass,
        RailgunWaterWake::Finish);
    G.Link(G.Read(RailgunWaterWake::Controller),
        G.Pin(Finish, P::FunctionTarget));
    G.Exec(Finish);
    StationMerge(G, {G.Tail, Missing});
}

void SpawnRailgunWaterWakeController(FGraph& G)
{
    check(RailgunWaterWake::ControllerClass);
    auto* Transform = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeTransform));
    G.Link(ObserveCall(G, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_GetActorLocation),
        OpticalSelf(G)), G.Pin(Transform, E::Location));
    G.Default(Transform, E::Scale, N::UnitScale);
    auto* Spawn = G.Call(UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics,
            BeginDeferredActorSpawnFromClass));
    G.Pin(Spawn, E::ActorClass)->DefaultObject =
        RailgunWaterWake::ControllerClass;
    G.Link(G.Pin(Transform, P::ReturnValue),
        G.Pin(Spawn, P::SpawnTransform));
    G.Default(Spawn, E::CollisionHandling, N::AlwaysSpawn);
    G.Exec(Spawn);
    UEdGraphPin* Actor = G.Pin(Spawn, P::ReturnValue);
    auto* Valid = G.Branch(G.Valid(Actor));
    UEdGraphPin* SpawnFailed = G.Pin(Valid, P::Else);
    auto* Typed = NewObject<UK2Node_DynamicCast>(G.Graph);
    Typed->TargetType = RailgunWaterWake::ControllerClass;
    Typed->SetPurity(false);
    G.Node(Typed);
    G.Link(G.Tail, G.Pin(Typed, P::Execute));
    G.Link(Actor, Typed->GetCastSourcePin());
    G.Tail = Typed->GetValidCastPin();
    UEdGraphPin* Controller = Typed->GetCastResultPin();
    G.Write(RailgunWaterWake::Controller, Controller);
    auto* Finish = G.Call(UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics,
            FinishSpawningActor));
    G.Link(Actor, G.Pin(Finish, P::Actor));
    G.Link(G.Pin(Transform, P::ReturnValue),
        G.Pin(Finish, P::SpawnTransform));
    G.Exec(Finish);
    UEdGraphPin* Spawned = G.Tail;
    G.Tail = Typed->GetInvalidCastPin();
    auto* Destroy = G.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor));
    G.Link(Actor, G.Pin(Destroy, P::FunctionTarget));
    G.Exec(Destroy);
    StationMerge(G, {G.Tail, SpawnFailed});
    StationMerge(G, {Spawned, G.Tail});
}
