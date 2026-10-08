#include "RailgunWater.h"

#include "StationAttachmentGraph.h"
#include "StationEntryGraph.h"
#include "RailgunVfx.h"
#include "StationOpticsGraph.h"
#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
FEdGraphPinType RailgunWaterPinType(FName Category, UObject* Type)
{
    FEdGraphPinType PinType;
    PinType.PinCategory = Category;
    PinType.PinSubCategoryObject = Type;
    if (Category == UEdGraphSchema_K2::PC_Real)
        PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    return PinType;
}

FBPVariableDescription AddRailgunWaterLocal(UK2Node_FunctionEntry* Entry,
    FName Name, const FEdGraphPinType& Type)
{
    FBPVariableDescription Description;
    Description.VarName = Name;
    Description.VarGuid = FGuid::NewGuid();
    Description.VarType = Type;
    Description.PropertyFlags = CPF_BlueprintVisible;
    Description.FriendlyName = FName::NameToDisplayString(Name.ToString(),
        Type.PinCategory == UEdGraphSchema_K2::PC_Boolean);
    Description.Category = UEdGraphSchema_K2::VR_DefaultCategory;
    Entry->LocalVariables.Add(Description);
    return Description;
}

UEdGraphPin* ReadRailgunWaterLocal(FGraph& G,
    const FBPVariableDescription& Local)
{
    auto* Get = NewObject<UK2Node_VariableGet>(G.Graph);
    Get->VariableReference.SetLocalMember(Local.VarName, G.Graph->GetName(),
        Local.VarGuid);
    G.Node(Get);
    return G.Pin(Get, Local.VarName);
}

void WriteRailgunWaterLocal(FGraph& G, const FBPVariableDescription& Local,
    UEdGraphPin* Value, const TCHAR* Literal)
{
    auto* Set = NewObject<UK2Node_VariableSet>(G.Graph);
    Set->VariableReference.SetLocalMember(Local.VarName, G.Graph->GetName(),
        Local.VarGuid);
    G.Node(Set);
    if (Value)
        G.Link(Value, G.Pin(Set, Local.VarName));
    else
        G.Default(Set, Local.VarName, Literal);
    G.Exec(Set);
}

UK2Node_MacroInstance* RailgunWaterLoop(FGraph& G, UEdGraphPin* LastIndex,
    const TCHAR* LiteralLastIndex, const TCHAR* FirstIndex)
{
    auto* Macros = LoadObject<UBlueprint>(nullptr, CE::LoopPackage);
    check(Macros);
    UEdGraph* LoopGraph = nullptr;
    for (UEdGraph* Graph : Macros->MacroGraphs)
        if (Graph->GetFName() == RailgunWater::ForLoopWithBreak)
            LoopGraph = Graph;
    check(LoopGraph);
    auto* Loop = NewObject<UK2Node_MacroInstance>(G.Graph);
    Loop->SetMacroGraph(LoopGraph);
    G.Node(Loop);
    G.Link(G.Tail, G.Pin(Loop, P::Execute));
    G.Default(Loop, P::FirstIndex, FirstIndex);
    if (LastIndex)
        G.Link(LastIndex, G.Pin(Loop, P::LastIndex));
    else
        G.Default(Loop, P::LastIndex, LiteralLastIndex);
    G.Tail = G.Pin(Loop, P::LoopBody);
    return Loop;
}

UEdGraphPin* RailgunWaterVectorLerp(FGraph& G, UEdGraphPin* Start,
    UEdGraphPin* End, UEdGraphPin* Alpha, const TCHAR* LiteralAlpha)
{
    auto* Lerp = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VLerp));
    G.Link(Start, G.Pin(Lerp, P::Select::WhenTrue));
    G.Link(End, G.Pin(Lerp, P::Select::WhenFalse));
    if (Alpha)
        G.Link(Alpha, G.Pin(Lerp, RailgunWater::Alpha));
    else
        G.Default(Lerp, RailgunWater::Alpha, LiteralAlpha);
    return G.Pin(Lerp, P::ReturnValue);
}

UEdGraphPin* GetRailgunWaterHeight(FGraph& G, UEdGraphPin* World,
    UEdGraphPin* Location)
{
    auto* GetHeight = G.Call(
        UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageMiscBlueprintFunctionLibrary,
            GetWaterHeightAtLocation));
    G.Link(World, G.Pin(GetHeight, RailgunWater::World));
    G.Link(Location, G.Pin(GetHeight, RailgunWater::Loc));
    return G.Pin(GetHeight, P::ReturnValue);
}

FRailgunWaterSample SampleRailgunWater(FGraph& G, UEdGraphPin* World,
    UEdGraphPin* Point)
{
    UEdGraphPin* Height = GetRailgunWaterHeight(G, World, Point);
    UEdGraphPin* NotSentinel = G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            NotEqual_DoubleDouble), Height, RailgunWater::InvalidHeight);
    UEdGraphPin* NotNan = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            EqualEqual_DoubleDouble), Height, Height);
    auto* AbsoluteHeight = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Abs));
    G.Link(Height, G.Pin(AbsoluteHeight, P::Binary::LeftOperand));
    UEdGraphPin* Bounded = G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble),
        G.Pin(AbsoluteHeight, P::ReturnValue),
        RailgunWater::MaximumFiniteMagnitude);
    UEdGraphPin* Valid = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        NotSentinel, G.Binary(
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
            NotNan, Bounded));

    auto* BreakPoint = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BreakVector));
    G.Link(Point, G.Pin(BreakPoint, RailgunWater::VectorInput));
    UEdGraphPin* Z = G.Pin(BreakPoint, RailgunWater::VectorZ);
    UEdGraphPin* Below = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            LessEqual_DoubleDouble), Z, Height);
    auto* Delta = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Subtract_DoubleDouble));
    G.Link(Z, G.Pin(Delta, P::Binary::LeftOperand));
    G.Link(Height, G.Pin(Delta, P::Binary::RightOperand));
    auto* Residual = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Abs));
    G.Link(G.Pin(Delta, P::ReturnValue),
        G.Pin(Residual, P::Binary::LeftOperand));
    return {Height, Valid, Below, G.Pin(Residual, P::ReturnValue)};
}

void ConsiderRailgunWaterCandidate(FGraph& G, UEdGraphPin* Point,
    const FRailgunWaterSample& Sample,
    const FBPVariableDescription& BestPoint,
    const FBPVariableDescription& BestResidual,
    const FBPVariableDescription& BestValid)
{
    auto* Valid = G.Branch(Sample.Valid);
    auto* Better = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_DoubleDouble),
        Sample.Residual, ReadRailgunWaterLocal(G, BestResidual)));
    WriteRailgunWaterLocal(G, BestPoint, Point);
    WriteRailgunWaterLocal(G, BestResidual, Sample.Residual);
    WriteRailgunWaterLocal(G, BestValid, nullptr, N::True);
    StationMerge(G, {G.Tail, G.Pin(Better, P::Else),
        G.Pin(Valid, P::Else)});
}

void AddRailgunWaterSegmentFunction(UBlueprint* BP)
{
    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP,
        RailgunWater::ProcessSegment, UEdGraph::StaticClass(),
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
        Result->FunctionReference.SetSelfMember(RailgunWater::ProcessSegment);
        Result->CreateNewGuid();
        Result->PostPlacedNewNode();
        Result->AllocateDefaultPins();
        Graph->AddNode(Result, true, false);
    }
    const FEdGraphPinType VectorType = RailgunWaterPinType(
        UEdGraphSchema_K2::PC_Struct, TBaseStructure<FVector>::Get());
    const FEdGraphPinType BoolType = RailgunWaterPinType(
        UEdGraphSchema_K2::PC_Boolean);
    const FEdGraphPinType RealType = RailgunWaterPinType(
        UEdGraphSchema_K2::PC_Real);
    UEdGraphPin* Start = Entry->CreateUserDefinedPin(
        RailgunWater::Start, VectorType, EGPD_Output);
    UEdGraphPin* End = Entry->CreateUserDefinedPin(
        RailgunWater::End, VectorType, EGPD_Output);
    check(Start && End);

    const FBPVariableDescription PreviousPoint = AddRailgunWaterLocal(
        Entry, RailgunWater::PreviousPoint, VectorType);
    const FBPVariableDescription PreviousValid = AddRailgunWaterLocal(
        Entry, RailgunWater::PreviousValid, BoolType);
    const FBPVariableDescription PreviousBelow = AddRailgunWaterLocal(
        Entry, RailgunWater::PreviousBelow, BoolType);
    const FBPVariableDescription InsidePoint = AddRailgunWaterLocal(
        Entry, RailgunWater::InsidePoint, VectorType);
    const FBPVariableDescription OutsidePoint = AddRailgunWaterLocal(
        Entry, RailgunWater::OutsidePoint, VectorType);
    const FBPVariableDescription BestPoint = AddRailgunWaterLocal(
        Entry, RailgunWater::BestPoint, VectorType);
    const FBPVariableDescription BestResidual = AddRailgunWaterLocal(
        Entry, RailgunWater::BestResidual, RealType);
    const FBPVariableDescription BestValid = AddRailgunWaterLocal(
        Entry, RailgunWater::BestValid, BoolType);

    UEdGraphPin* EntryThen = Entry->FindPinChecked(P::Then);
    UEdGraphPin* ResultExecute = Result->FindPinChecked(P::Execute);
    EntryThen->BreakAllPinLinks();
    ResultExecute->BreakAllPinLinks();
    check(Graph->GetSchema()->TryCreateConnection(EntryThen, ResultExecute));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    checkf(BP->Status != BS_Error,
        TEXT("Failed to establish ProcessWaterSegment local variables"));
    EntryThen->BreakAllPinLinks();
    ResultExecute->BreakAllPinLinks();

    FGraph G(Graph);
    G.Tail = G.Pin(Entry, P::Then);
    TArray<UEdGraphPin*> ExitPaths;
    auto* NotLatched = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_BoolBool),
        G.Read(RailgunWater::FirstCrossingDone), N::False));
    ExitPaths.Add(G.Pin(NotLatched, P::Else));

    auto* GetWorld = G.Call(UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageMiscBlueprintFunctionLibrary,
            GetActorWorld));
    G.Link(OpticalSelf(G), G.Pin(GetWorld, P::Actor));
    UEdGraphPin* World = G.Pin(GetWorld, P::ReturnValue);
    auto* WorldValid = G.Branch(G.Valid(World));
    ExitPaths.Add(G.Pin(WorldValid, P::Else));

    UEdGraphPin* SegmentDelta = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Subtract_VectorVector), End, Start);
    auto* VectorLength = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(SegmentDelta, G.Pin(VectorLength, E::VectorLengthInput));
    UEdGraphPin* Length = G.Pin(VectorLength, P::ReturnValue);
    auto* NonEmpty = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble), Length, N::Zero));
    ExitPaths.Add(G.Pin(NonEmpty, P::Else));
    auto* BoundedSegment = G.Branch(G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            LessEqual_DoubleDouble), Length,
        RailgunWater::MaximumSegmentCentimeters));
    ExitPaths.Add(G.Pin(BoundedSegment, P::Else));

    auto* SampleRatio = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble));
    G.Link(Length, G.Pin(SampleRatio, P::Binary::LeftOperand));
    G.Default(SampleRatio, P::Binary::RightOperand,
        RailgunWater::SampleStepCentimeters);
    auto* SampleCeil = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FCeil));
    G.Link(G.Pin(SampleRatio, P::ReturnValue),
        G.Pin(SampleCeil, P::Binary::LeftOperand));
    auto* SampleCount = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Clamp));
    G.Link(G.Pin(SampleCeil, P::ReturnValue), G.Pin(SampleCount, P::Value));
    G.Default(SampleCount, P::Min, RailgunWater::MinimumSamples);
    G.Default(SampleCount, P::Max, RailgunWater::MaximumSamples);
    UEdGraphPin* SampleCountValue = G.Pin(SampleCount, P::ReturnValue);

    FRailgunWaterSample StartSample = SampleRailgunWater(G, World, Start);
    WriteRailgunWaterLocal(G, PreviousPoint, Start);
    WriteRailgunWaterLocal(G, PreviousValid, StartSample.Valid);
    WriteRailgunWaterLocal(G, PreviousBelow, StartSample.Below);

    auto* OuterLoop = RailgunWaterLoop(G, SampleCountValue, nullptr,
        RailgunWater::FirstLoopIndex);
    auto* IndexAsDouble = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_IntToDouble));
    G.Link(G.Pin(OuterLoop, P::Index),
        G.Pin(IndexAsDouble, RailgunWater::IntegerInput));
    auto* CountAsDouble = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_IntToDouble));
    G.Link(SampleCountValue,
        G.Pin(CountAsDouble, RailgunWater::IntegerInput));
    UEdGraphPin* Alpha = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble),
        G.Pin(IndexAsDouble, P::ReturnValue),
        G.Pin(CountAsDouble, P::ReturnValue));
    UEdGraphPin* Point = RailgunWaterVectorLerp(G, Start, End, Alpha);
    FRailgunWaterSample CurrentSample = SampleRailgunWater(G, World, Point);

    UEdGraphPin* PreviousInside = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        ReadRailgunWaterLocal(G, PreviousValid),
        ReadRailgunWaterLocal(G, PreviousBelow));
    UEdGraphPin* CurrentInside = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        CurrentSample.Valid, CurrentSample.Below);
    auto* DifferentSides = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_BoolBool));
    G.Link(PreviousInside, G.Pin(DifferentSides, P::Binary::LeftOperand));
    G.Link(CurrentInside, G.Pin(DifferentSides, P::Binary::RightOperand));
    auto* Bracket = G.Branch(G.Pin(DifferentSides, P::ReturnValue));
    UEdGraphPin* NoBracketTail = G.Pin(Bracket, P::Else);

    auto* SelectInside = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectVector));
    G.Link(ReadRailgunWaterLocal(G, PreviousPoint),
        G.Pin(SelectInside, P::Select::WhenTrue));
    G.Link(Point, G.Pin(SelectInside, P::Select::WhenFalse));
    G.Link(PreviousInside, G.Pin(SelectInside, P::Select::Condition));
    auto* SelectOutside = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectVector));
    G.Link(Point, G.Pin(SelectOutside, P::Select::WhenTrue));
    G.Link(ReadRailgunWaterLocal(G, PreviousPoint),
        G.Pin(SelectOutside, P::Select::WhenFalse));
    G.Link(PreviousInside, G.Pin(SelectOutside, P::Select::Condition));
    WriteRailgunWaterLocal(G, InsidePoint,
        G.Pin(SelectInside, P::ReturnValue));
    WriteRailgunWaterLocal(G, OutsidePoint,
        G.Pin(SelectOutside, P::ReturnValue));
    WriteRailgunWaterLocal(G, BestResidual, nullptr,
        RailgunWater::InitialBestResidual);
    WriteRailgunWaterLocal(G, BestValid, nullptr, N::False);
    ConsiderRailgunWaterCandidate(G, Point, CurrentSample,
        BestPoint, BestResidual, BestValid);

    auto* Refinement = RailgunWaterLoop(G, nullptr,
        RailgunWater::BisectionLastIndex, N::Zero);
    UEdGraphPin* Midpoint = RailgunWaterVectorLerp(G,
        ReadRailgunWaterLocal(G, InsidePoint),
        ReadRailgunWaterLocal(G, OutsidePoint), nullptr,
        RailgunWater::MidpointAlpha);
    FRailgunWaterSample MidpointSample = SampleRailgunWater(G, World, Midpoint);
    ConsiderRailgunWaterCandidate(G, Midpoint, MidpointSample,
        BestPoint, BestResidual, BestValid);
    UEdGraphPin* MidpointInside = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        MidpointSample.Valid, MidpointSample.Below);
    auto* RefineInside = G.Branch(MidpointInside);
    WriteRailgunWaterLocal(G, InsidePoint, Midpoint);
    UEdGraphPin* RefinedInsideTail = G.Tail;
    G.Tail = G.Pin(RefineInside, P::Else);
    WriteRailgunWaterLocal(G, OutsidePoint, Midpoint);
    StationMerge(G, {RefinedInsideTail, G.Tail});
    G.Tail = G.Pin(Refinement, P::Completed);

    UEdGraphPin* Accepted = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        ReadRailgunWaterLocal(G, BestValid),
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            LessEqual_DoubleDouble),
            ReadRailgunWaterLocal(G, BestResidual),
            RailgunWater::MaximumResidualCentimeters));
    auto* AcceptedCrossing = G.Branch(Accepted);
    UEdGraphPin* RejectedTail = G.Pin(AcceptedCrossing, P::Else);
    G.Write(RailgunWater::FirstCrossingDone, nullptr, N::True);
    SpawnRailgunVfx(G, ReadRailgunWaterLocal(G, BestPoint), true);
    G.Link(G.Tail, G.Pin(OuterLoop, RailgunWater::BreakLoop));

    StationMerge(G, {NoBracketTail, RejectedTail});
    WriteRailgunWaterLocal(G, PreviousPoint, Point);
    WriteRailgunWaterLocal(G, PreviousValid, CurrentSample.Valid);
    WriteRailgunWaterLocal(G, PreviousBelow, CurrentSample.Below);

    ExitPaths.Add(G.Pin(OuterLoop, P::Completed));
    for (UEdGraphPin* Path : ExitPaths)
        G.Link(Path, G.Pin(Result, P::Execute));
}

void ProcessRailgunWaterSegment(FGraph& G, UClass* ShotClass,
    UEdGraphPin* SegmentStart, UEdGraphPin* SegmentEnd)
{
    auto* Process = G.Call(ShotClass, RailgunWater::ProcessSegment);
    G.Link(SegmentStart, G.Pin(Process, RailgunWater::Start));
    G.Link(SegmentEnd, G.Pin(Process, RailgunWater::End));
    G.Exec(Process);
}
}
