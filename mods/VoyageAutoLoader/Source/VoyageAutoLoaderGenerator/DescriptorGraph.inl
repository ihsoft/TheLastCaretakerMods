// Hand-written, build-time generator. Only Blueprint bytecode/Engine calls ship.
namespace Descriptor
{
const FName Parse(TEXT("ParseAutoLoadDescriptor"));
const FName Text(TEXT("DescriptorInput"));
const FName DefaultEntry(TEXT("DescriptorDefaultEntry"));
const FName Entry(TEXT("ParsedEntry"));
const FName Menu(TEXT("ParsedMenu"));
const FName Gameplay(TEXT("ParsedGameplay"));
const FName Valid(TEXT("DescriptorValid"));
const FName Error(TEXT("DescriptorError"));
const FName SeenEntry(TEXT("DescriptorSeenEntry"));
const FName SeenPhase(TEXT("DescriptorSeenPhase"));
const FName Line(TEXT("DescriptorLine"));
const FName Key(TEXT("DescriptorKey"));
const FName Value(TEXT("DescriptorValue"));
constexpr TCHAR EntryKey[] = TEXT("entryClass");
constexpr TCHAR EmptyValue[] = TEXT("");
constexpr TCHAR PhaseKey[] = TEXT("activateIn");
constexpr TCHAR MenuValue[] = TEXT("menu");
constexpr TCHAR GameplayValue[] = TEXT("gameplay");
constexpr TCHAR Colon[] = TEXT(":");
constexpr TCHAR Comma[] = TEXT(",");
constexpr TCHAR Comment[] = TEXT("#");
constexpr TCHAR Slash[] = TEXT("/");
constexpr TCHAR Dot[] = TEXT(".");
constexpr TCHAR ClassSuffix[] = TEXT("_C");
constexpr TCHAR InvalidLine[] = TEXT("missing-colon");
constexpr TCHAR UnknownField[] = TEXT("unknown-field");
constexpr TCHAR DuplicateField[] = TEXT("duplicate-field");
constexpr TCHAR InvalidEntry[] = TEXT("invalid-entryClass");
constexpr TCHAR InvalidPhase[] = TEXT("invalid-activateIn");
constexpr TCHAR TooLarge[] = TEXT("descriptor-too-large");
constexpr int32 MaxCharacters = 4096;
constexpr int32 MaxLines = 64;

// Semantic engine pin roles kept here until a second consumer needs them.
const FName Delimiter(TEXT("Delimiter"));
const FName CullEmpty(TEXT("CullEmptyStrings"));
const FName SplitDelimiter(TEXT("InStr"));
const FName SplitLeft(TEXT("LeftS"));
const FName SplitRight(TEXT("RightS"));
const FName Suffix(TEXT("InSuffix"));
const FName SearchIn(TEXT("SearchIn"));
const FName Substring(TEXT("Substring"));

struct FGraph
{
    UEdGraph* Graph;
    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    UEdGraphPin* Tail = nullptr;
    bool Ok = true;
    int32 X = 0;
    UEdGraphPin* Pin(UEdGraphNode* Node, FName Name) { return RequirePin(Node, Name); }
    void Link(UEdGraphPin* From, UEdGraphPin* To) { Ok &= Connect(Schema, From, To); }
    UK2Node_CallFunction* Call(UClass* Owner, FName Name) { return AddCall(Graph, Owner->FindFunctionByName(Name), X, -200); }
    UK2Node_CallFunction* String(FName Name) { return Call(UKismetStringLibrary::StaticClass(), Name); }
    UEdGraphPin* Read(FName Name)
    {
        auto* Node = NewObject<UK2Node_VariableGet>(Graph); Node->VariableReference.SetSelfMember(Name);
        FinishNode(Node, Graph, X, -400); return Pin(Node, Name);
    }
    void Write(FName Name, UEdGraphPin* Input, FString Literal = FString())
    {
        auto* Node = NewObject<UK2Node_VariableSet>(Graph); Node->VariableReference.SetSelfMember(Name);
        FinishNode(Node, Graph, X += 200, 0);
        if (Input) Link(Input, Pin(Node, Name)); else SetDefault(Node, Name, Literal);
        Link(Tail, Pin(Node, ExecutePinName)); Tail = Pin(Node, ThenPinName);
    }
    void Bool(FName Name, bool bValue) { Write(Name, nullptr, LexToString(bValue)); }
    void Fail(const TCHAR* Reason) { Bool(Valid, false); Write(Error, nullptr, Reason); }
    UEdGraphPin* Trim(UEdGraphPin* Input)
    {
        auto* Leading = String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Trim));
        auto* Trailing = String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, TrimTrailing));
        Link(Input, Pin(Leading, RegistryDiscoveryGraphNames::SourceString));
        Link(Pin(Leading, ReturnValuePinName), Pin(Trailing, RegistryDiscoveryGraphNames::SourceString));
        return Pin(Trailing, ReturnValuePinName);
    }
    UEdGraphPin* Equal(UEdGraphPin* Input, const TCHAR* Literal)
    {
        auto* Node = String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
        Link(Input, Pin(Node, BlueprintGraphNames::Pins::Binary::LeftOperand));
        SetDefault(Node, BlueprintGraphNames::Pins::Binary::RightOperand, Literal);
        return Pin(Node, ReturnValuePinName);
    }
    UEdGraphPin* Match(FName Function, UEdGraphPin* Input, FName TestPin, const TCHAR* Literal)
    {
        auto* Node = String(Function);
        Link(Input, Pin(Node, Function == GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Contains) ? SearchIn : RegistryDiscoveryGraphNames::SourceString));
        SetDefault(Node, TestPin, Literal); return Pin(Node, ReturnValuePinName);
    }
    UK2Node_IfThenElse* Branch(UEdGraphPin* Condition)
    {
        auto* Node = FinishNode(NewObject<UK2Node_IfThenElse>(Graph), Graph, X += 200, 0);
        Link(Tail, Pin(Node, ExecutePinName)); Link(Condition, Pin(Node, BlueprintGraphNames::Pins::Condition));
        Tail = Pin(Node, ThenPinName); return Node;
    }
    UEdGraphPin* Array(UEdGraphPin* Input, const TCHAR* Separator)
    {
        auto* Node = String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, ParseIntoArray));
        Link(Input, Pin(Node, RegistryDiscoveryGraphNames::SourceString));
        SetDefault(Node, Delimiter, Separator); SetDefault(Node, CullEmpty, LexToString(false));
        return Pin(Node, ReturnValuePinName);
    }
    UK2Node_MacroInstance* ForEach(UEdGraphPin* Input)
    {
        auto* Macros = LoadObject<UBlueprint>(nullptr, BlueprintGraphNames::EngineAssets::StandardMacrosObjectPath);
        UEdGraph* Macro = nullptr;
        for (UEdGraph* Candidate : Macros->MacroGraphs) if (Candidate->GetFName() == RegistryDiscoveryGraphNames::ForEachLoop) Macro = Candidate;
        check(Macro);
        auto* Node = NewObject<UK2Node_MacroInstance>(Graph); Node->SetMacroGraph(Macro); FinishNode(Node, Graph, X += 200, 0);
        Link(Input, Pin(Node, RegistryDiscoveryGraphNames::Array));
        for (auto* InputPin : Node->Pins) if (InputPin->Direction == EGPD_Input && InputPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) Link(Tail, InputPin);
        Tail = Pin(Node, BlueprintGraphNames::Pins::LoopBody); return Node;
    }
    void Return()
    {
        auto* Node = NewObject<UK2Node_FunctionResult>(Graph);
        Node->CreateNewGuid(); Node->SetFlags(RF_Transactional);
        Node->FunctionReference.SetSelfMember(Parse);
        Graph->AddNode(Node, true, false);
        // PostPlaced synchronizes/reconstructs result pins; allocating again
        // creates a second unconnected exec pin on this special node type.
        Node->PostPlacedNewNode();
        if (Node->Pins.IsEmpty()) Node->AllocateDefaultPins();
        Node->NodePosX = X += 200;
        Link(Tail, Pin(Node, ExecutePinName));
    }
    UEdGraphPin* Limit(UEdGraphPin* Input, int32 Max)
    {
        auto* Node = Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_IntInt));
        Link(Input, Pin(Node, BlueprintGraphNames::Pins::Binary::LeftOperand));
        SetDefault(Node, BlueprintGraphNames::Pins::Binary::RightOperand, LexToString(Max)); return Pin(Node, ReturnValuePinName);
    }
};
}

bool BuildDescriptorParser(UBlueprint* Blueprint)
{
    using namespace Descriptor;
    auto Member = [&](FName Name, FName Category)
    {
        FEdGraphPinType Type; Type.PinCategory = Category;
        return FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, Type);
    };
    for (FName Name : { Text, DefaultEntry, Entry, Error, Line, Key, Value }) if (!Member(Name, UEdGraphSchema_K2::PC_String)) return false;
    for (FName Name : { Menu, Gameplay, Valid, SeenEntry, SeenPhase }) if (!Member(Name, UEdGraphSchema_K2::PC_Boolean)) return false;
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    auto* Graph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, Parse, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, Graph, true, nullptr);
    UK2Node_FunctionEntry* EntryNode = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes) if (auto* EntryCandidate = Cast<UK2Node_FunctionEntry>(Node)) EntryNode = EntryCandidate;
    check(EntryNode);
    FGraph G { Graph };
    G.Tail = RequirePin(EntryNode, ThenPinName); G.Tail->BreakAllPinLinks();
    G.Write(Entry, G.Read(DefaultEntry)); G.Bool(Menu, false); G.Bool(Gameplay, true);
    G.Bool(Valid, true); G.Bool(SeenEntry, false); G.Bool(SeenPhase, false); G.Write(Error, nullptr);
    auto* Len = G.String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Len));
    G.Link(G.Read(Text), G.Pin(Len, StringLengthInputPin));
    auto* SizeGate = G.Branch(G.Limit(G.Pin(Len, ReturnValuePinName), MaxCharacters));
    auto* Lines = G.Array(G.Read(Text), DescriptorLineSeparator);
    auto* LineCount = G.Call(UKismetArrayLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length));
    G.Link(Lines, G.Pin(LineCount, BlueprintGraphNames::Pins::TargetArray));
    auto* LinesGate = G.Branch(G.Limit(G.Pin(LineCount, ReturnValuePinName), MaxLines));
    auto* Loop = G.ForEach(Lines);
    G.Write(Line, G.Trim(G.Pin(Loop, RegistryDiscoveryGraphNames::ArrayElement)));
    auto* Empty = G.Branch(G.Equal(G.Read(Line), EmptyValue));
    G.Tail = G.Pin(Empty, BlueprintGraphNames::Pins::Else);
    auto* CommentGate = G.Branch(G.Match(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, StartsWith), G.Read(Line), RegistryDiscoveryGraphNames::InPrefix, Comment));
    G.Tail = G.Pin(CommentGate, BlueprintGraphNames::Pins::Else);
    auto* Split = G.String(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Split));
    G.Link(G.Read(Line), G.Pin(Split, RegistryDiscoveryGraphNames::SourceString)); SetDefault(Split, SplitDelimiter, Colon);
    auto* HasColon = G.Branch(G.Pin(Split, ReturnValuePinName));
    G.Write(Key, G.Trim(G.Pin(Split, SplitLeft))); G.Write(Value, G.Trim(G.Pin(Split, SplitRight)));
    auto* IsEntry = G.Branch(G.Equal(G.Read(Key), EntryKey));
    auto* DuplicateEntry = G.Branch(G.Read(SeenEntry)); G.Fail(DuplicateField);
    G.Tail = G.Pin(DuplicateEntry, BlueprintGraphNames::Pins::Else); G.Bool(SeenEntry, true);
    auto* Starts = G.Branch(G.Match(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, StartsWith), G.Read(Value), RegistryDiscoveryGraphNames::InPrefix, Slash));
    auto* Ends = G.Branch(G.Match(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EndsWith), G.Read(Value), Suffix, ClassSuffix));
    auto* HasDot = G.Branch(G.Match(GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Contains), G.Read(Value), Substring, Dot));
    // Invalid/unknown object paths still face LoadClass and Actor-cast gates.
    G.Write(Entry, G.Read(Value));
    for (auto* Gate : { Starts, Ends, HasDot }) { G.Tail = G.Pin(Gate, BlueprintGraphNames::Pins::Else); G.Fail(InvalidEntry); }
    G.Tail = G.Pin(IsEntry, BlueprintGraphNames::Pins::Else);
    auto* IsPhase = G.Branch(G.Equal(G.Read(Key), PhaseKey));
    auto* DuplicatePhase = G.Branch(G.Read(SeenPhase)); G.Fail(DuplicateField);
    G.Tail = G.Pin(DuplicatePhase, BlueprintGraphNames::Pins::Else);
    G.Bool(SeenPhase, true); G.Bool(Menu, false); G.Bool(Gameplay, false);
    auto* EmptyPhase = G.Branch(G.Equal(G.Read(Value), EmptyValue)); G.Fail(InvalidPhase);
    G.Tail = G.Pin(EmptyPhase, BlueprintGraphNames::Pins::Else);
    auto* PhaseLoop = G.ForEach(G.Array(G.Read(Value), Comma));
    auto* PhaseText = G.Trim(G.Pin(PhaseLoop, RegistryDiscoveryGraphNames::ArrayElement));
    auto* IsMenu = G.Branch(G.Equal(PhaseText, MenuValue));
    auto* RepeatedMenu = G.Branch(G.Read(Menu)); G.Fail(InvalidPhase);
    G.Tail = G.Pin(RepeatedMenu, BlueprintGraphNames::Pins::Else); G.Bool(Menu, true);
    G.Tail = G.Pin(IsMenu, BlueprintGraphNames::Pins::Else);
    auto* IsGameplay = G.Branch(G.Equal(PhaseText, GameplayValue));
    auto* RepeatedGameplay = G.Branch(G.Read(Gameplay)); G.Fail(InvalidPhase);
    G.Tail = G.Pin(RepeatedGameplay, BlueprintGraphNames::Pins::Else); G.Bool(Gameplay, true);
    G.Tail = G.Pin(IsGameplay, BlueprintGraphNames::Pins::Else); G.Fail(InvalidPhase);
    G.Tail = G.Pin(IsPhase, BlueprintGraphNames::Pins::Else); G.Fail(UnknownField);
    G.Tail = G.Pin(HasColon, BlueprintGraphNames::Pins::Else); G.Fail(InvalidLine);
    G.Tail = G.Pin(Loop, BlueprintGraphNames::Pins::Completed); G.Return();
    for (auto* Gate : { SizeGate, LinesGate }) { G.Tail = G.Pin(Gate, BlueprintGraphNames::Pins::Else); G.Fail(TooLarge); G.Return(); }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return G.Ok && Blueprint->Status != BS_Error;
}

bool TestDescriptorParser(UBlueprint* Blueprint)
{
    using namespace Descriptor;
    // These fixtures test the actual generated Blueprint VM, not a C++ parser.
    constexpr TCHAR TestDefault[] = TEXT("/Game/Mods/Physical_P/ModActor.ModActor_C");
    constexpr TCHAR TestOverride[] = TEXT("/Game/Mods/Virtual/Entry.Entry_C");
    struct FCase { FString Input; bool Valid; bool Override; bool Menu; bool Gameplay; };
    TArray<FCase> Cases {
        { EmptyValue, true, false, false, true },
        { TEXT(" \r\n# comment\n"), true, false, false, true },
        { TEXT("entryClass: /Game/Mods/Virtual/Entry.Entry_C"), true, true, false, true },
        { TEXT("activateIn: menu"), true, false, true, false },
        { TEXT("activateIn: gameplay"), true, false, false, true },
        { TEXT("activateIn: menu,gameplay"), true, false, true, true },
        { TEXT("activateIn: gameplay, menu\r\nentryClass: /Game/Mods/Virtual/Entry.Entry_C\r\n"), true, true, true, true },
        { TEXT(" entryClass : /Game/Mods/Virtual/Entry.Entry_C \n activateIn : menu \n"), true, true, true, false },
        { TEXT("activateIn:"), false, false, false, false },
        { TEXT("activateIn: menu,"), false, false, false, false },
        { TEXT("activateIn: ,menu"), false, false, false, false },
        { TEXT("activateIn: menu,menu"), false, false, false, false },
        { TEXT("activateIn: Menu"), false, false, false, false },
        { TEXT("activateIn: level"), false, false, false, false },
        { TEXT("activateIn: menu\nactivateIn: gameplay"), false, false, false, false },
        { TEXT("entryClass: /Game/A.A_C\nentryClass: /Game/B.B_C"), false, false, false, false },
        { TEXT("entryClass:"), false, false, false, false },
        { TEXT("entryClass: Relative.ModActor_C"), false, false, false, false },
        { TEXT("entryClass: /Game/A.A"), false, false, false, false },
        { TEXT("typo: value"), false, false, false, false },
        { TEXT("activateIn menu"), false, false, false, false },
        { TEXT("{\"activateIn\":[\"menu\"]}"), false, false, false, false },
        { FString::ChrN(MaxCharacters + 1, ' '), false, false, false, false },
        { FString::ChrN(MaxLines + 1, '\n'), false, false, false, false }
    };
    UObject* Target = Blueprint->GeneratedClass->GetDefaultObject();
    auto StringProperty = [&](FName Name) { auto* Property = FindFProperty<FStrProperty>(Blueprint->GeneratedClass, Name); check(Property); return Property; };
    auto BoolProperty = [&](FName Name) { auto* Property = FindFProperty<FBoolProperty>(Blueprint->GeneratedClass, Name); check(Property); return Property; };
    auto ReadBool = [&](FName Name) { return BoolProperty(Name)->GetPropertyValue_InContainer(Target); };
    UFunction* Function = Blueprint->GeneratedClass->FindFunctionByName(Parse); check(Function);
    FEditorScriptExecutionGuard Guard;
    for (int32 Index = 0; Index < Cases.Num(); ++Index)
    {
        const FCase& Case = Cases[Index];
        StringProperty(Text)->SetPropertyValue_InContainer(Target, Case.Input);
        StringProperty(DefaultEntry)->SetPropertyValue_InContainer(Target, TestDefault);
        Target->ProcessEvent(Function, nullptr);
        if (ReadBool(Valid) != Case.Valid || (Case.Valid &&
            (ReadBool(Menu) != Case.Menu || ReadBool(Gameplay) != Case.Gameplay ||
             StringProperty(Entry)->GetPropertyValue_InContainer(Target) != (Case.Override ? TestOverride : TestDefault))))
        {
            UE_LOG(LogTemp, Error, TEXT("Descriptor VM case %d failed: valid=%d menu=%d gameplay=%d entry=%s error=%s"),
                Index, ReadBool(Valid), ReadBool(Menu), ReadBool(Gameplay),
                *StringProperty(Entry)->GetPropertyValue_InContainer(Target), *StringProperty(Error)->GetPropertyValue_InContainer(Target));
            return false;
        }
    }
    // Do not ship fixtures or test state as actor defaults.
    for (FName Name : { Text, DefaultEntry, Entry, Error, Line, Key, Value }) StringProperty(Name)->SetPropertyValue_InContainer(Target, FString());
    for (FName Name : { Menu, Gameplay, Valid, SeenEntry, SeenPhase }) BoolProperty(Name)->SetPropertyValue_InContainer(Target, false);
    UE_LOG(LogTemp, Display, TEXT("Descriptor Blueprint VM: %d/%d cases passed"), Cases.Num(), Cases.Num());
    return true;
}
