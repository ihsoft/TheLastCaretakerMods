#pragma once

#if WITH_EDITOR

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/BlueprintPathsLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "VoyageEditorBlueprintFunctionLibrary.h"
#include "VoyageModSettingsTypes.h"

namespace VoyageModSettings::Graph
{
inline const FName ParseLineFunction(TEXT("ParseVoyageModSettingLine"));
inline const FName ParseNumberFunction(TEXT("ParseVoyageModSettingNumber"));
inline const FName ParseStringFunction(TEXT("ParseVoyageModSettingString"));
inline const FName LinePin(TEXT("Line"));
inline const FName TextPin(TEXT("Text"));
inline const FName NumberPin(TEXT("Number"));
inline const FName KeyPin(TEXT("Key"));
inline const FName ValuePin(TEXT("Value"));
inline const FName ValidPin(TEXT("Valid"));
inline const FName SeparatorLocal(TEXT("VmsSeparator"));
inline const FName CommentLocal(TEXT("VmsComment"));
inline const FName QuotedLocal(TEXT("VmsQuoted"));
inline const FName EscapedLocal(TEXT("VmsEscaped"));
inline const FName InvalidLocal(TEXT("VmsInvalid"));
inline const FName DecodedLocal(TEXT("VmsDecoded"));
inline const FName StandardMacrosPath(TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
inline const FName ForLoopName(TEXT("ForLoop"));
inline const FName FirstIndexPin(TEXT("FirstIndex"));
inline const FName LastIndexPin(TEXT("LastIndex"));
inline const FName LoopBodyPin(TEXT("LoopBody"));
inline const FName IndexPin(TEXT("Index"));
inline const FName CompletedPin(TEXT("Completed"));
inline const FName ForEachLoopName(TEXT("ForEachLoop"));
inline const FName ArrayPin(TEXT("Array"));
inline const FName ArrayElementPin(TEXT("Array Element"));
inline const FName SourceStringPin(TEXT("SourceString"));
inline const FName StartIndexPin(TEXT("StartIndex"));
inline const FName LengthPin(TEXT("Length"));
inline const FName LeftPin(TEXT("A"));
inline const FName RightPin(TEXT("B"));
inline const FName LeftStringPin(TEXT("LeftS"));
inline const FName RightStringPin(TEXT("RightS"));
inline const FName InputPathPin(TEXT("InPath"));
inline constexpr TCHAR ZeroLiteral[] = TEXT("0");
inline constexpr TCHAR OneLiteral[] = TEXT("1");
inline constexpr TCHAR NegativeOneLiteral[] = TEXT("-1");
inline constexpr TCHAR TrueLiteral[] = TEXT("true");
inline constexpr TCHAR FalseLiteral[] = TEXT("false");
inline constexpr TCHAR MaximumDoubleLiteral[] = TEXT("1.7976931348623157e308");
inline constexpr TCHAR MinimumDoubleLiteral[] = TEXT("-1.7976931348623157e308");
inline constexpr TCHAR MaximumFloatLiteral[] = TEXT("3.4028234663852886e38");
inline constexpr TCHAR MinimumFloatLiteral[] = TEXT("-3.4028234663852886e38");

template <typename T>
T* Node(T* Value, UEdGraph* Target, int32 X, int32 Y)
{
    Value->CreateNewGuid();
    Value->PostPlacedNewNode();
    Value->SetFlags(RF_Transactional);
    Value->AllocateDefaultPins();
    Value->NodePosX = X;
    Value->NodePosY = Y;
    Target->AddNode(Value, true, false);
    return Value;
}

inline UEdGraphPin* Pin(UEdGraphNode* NodeValue, const FName Name)
{
    UEdGraphPin* Result = NodeValue->FindPin(Name);
    checkf(Result, TEXT("Missing settings graph pin '%s' on '%s'"),
        *Name.ToString(), *NodeValue->GetName());
    return Result;
}

inline bool Link(UEdGraphPin* From, UEdGraphPin* To)
{
    return GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(From, To);
}

inline UK2Node_CallFunction* Call(
    UEdGraph* Target, UClass* Owner, const FName Function, int32 X, int32 Y)
{
    UFunction* Reflected = Owner->FindFunctionByName(Function);
    checkf(Reflected, TEXT("Missing settings graph function %s.%s"),
        *Owner->GetPathName(), *Function.ToString());
    UK2Node_CallFunction* Result = NewObject<UK2Node_CallFunction>(Target);
    Result->SetFromFunction(Reflected);
    return Node(Result, Target, X, Y);
}

inline UK2Node_IfThenElse* Branch(
    UEdGraph* Target, UEdGraphPin* Execute, UEdGraphPin* Condition, int32 X, int32 Y)
{
    UK2Node_IfThenElse* Result = Node(NewObject<UK2Node_IfThenElse>(Target), Target, X, Y);
    check(Link(Execute, Pin(Result, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Condition, Pin(Result, UEdGraphSchema_K2::PN_Condition)));
    return Result;
}

inline FEdGraphPinType PinType(const FName Category)
{
    FEdGraphPinType Result;
    Result.PinCategory = Category;
    return Result;
}

inline UK2Node_VariableGet* ReadLocal(
    UBlueprint* Blueprint, UEdGraph* Target, const FName Name, int32 X, int32 Y)
{
    UK2Node_VariableGet* Result = NewObject<UK2Node_VariableGet>(Target);
    Result->VariableReference.SetLocalMember(Name, Target->GetName(),
        FBlueprintEditorUtils::FindLocalVariableGuidByName(Blueprint, Target, Name));
    return Node(Result, Target, X, Y);
}

inline UK2Node_VariableSet* WriteLocal(
    UBlueprint* Blueprint, UEdGraph* Target, const FName Name, int32 X, int32 Y)
{
    UK2Node_VariableSet* Result = NewObject<UK2Node_VariableSet>(Target);
    Result->VariableReference.SetLocalMember(Name, Target->GetName(),
        FBlueprintEditorUtils::FindLocalVariableGuidByName(Blueprint, Target, Name));
    return Node(Result, Target, X, Y);
}

inline UK2Node_CallFunction* IntBinary(
    UEdGraph* Target, const FName Function, UEdGraphPin* Left, const TCHAR* Right,
    int32 X, int32 Y)
{
    UK2Node_CallFunction* Result = Call(Target, UKismetMathLibrary::StaticClass(), Function, X, Y);
    check(Link(Left, Pin(Result, LeftPin)));
    Pin(Result, RightPin)->DefaultValue = Right;
    return Result;
}

inline UK2Node_CallFunction* BoolBinary(
    UEdGraph* Target, const FName Function, UEdGraphPin* Left, UEdGraphPin* Right,
    int32 X, int32 Y)
{
    UK2Node_CallFunction* Result = Call(Target, UKismetMathLibrary::StaticClass(), Function, X, Y);
    check(Link(Left, Pin(Result, LeftPin)));
    check(Link(Right, Pin(Result, RightPin)));
    return Result;
}

inline UK2Node_CallFunction* NotBool(UEdGraph* Target, UEdGraphPin* Value, int32 X, int32 Y)
{
    UK2Node_CallFunction* Result = Call(Target, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Not_PreBool), X, Y);
    check(Link(Value, Pin(Result, LeftPin)));
    return Result;
}

inline UEdGraphPin* And(UEdGraph* Target, UEdGraphPin* A, UEdGraphPin* B, int32 X, int32 Y)
{
    return Pin(BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), A, B, X, Y),
        UEdGraphSchema_K2::PN_ReturnValue);
}

inline UEdGraphPin* Or(UEdGraph* Target, UEdGraphPin* A, UEdGraphPin* B, int32 X, int32 Y)
{
    return Pin(BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), A, B, X, Y),
        UEdGraphSchema_K2::PN_ReturnValue);
}

inline UEdGraphPin* StringNotEqual(
    UEdGraph* Target, UEdGraphPin* Text, const TCHAR* Literal, int32 X, int32 Y)
{
    UK2Node_CallFunction* NodeValue = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, NotEqual_StrStr), X, Y);
    check(Link(Text, Pin(NodeValue, LeftPin)));
    Pin(NodeValue, RightPin)->DefaultValue = Literal;
    return Pin(NodeValue, UEdGraphSchema_K2::PN_ReturnValue);
}

inline UEdGraphPin* DecimalSyntax(
    UEdGraph* Target, UEdGraphPin* Text, bool IntegerOnly, int32 X, int32 Y)
{
    UK2Node_CallFunction* Numeric = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, IsNumeric), X, Y);
    check(Link(Text, Pin(Numeric, SourceStringPin)));
    UEdGraphPin* Valid = Pin(Numeric, UEdGraphSchema_K2::PN_ReturnValue);
    int32 Column = X + 220;
    for (const TCHAR* Rejected : {TEXT(""), TEXT("+"), TEXT("-"), TEXT("."), TEXT("+."), TEXT("-.")})
    {
        Valid = And(Target, Valid, StringNotEqual(Target, Text, Rejected, Column, Y + 120),
            Column + 220, Y);
        Column += 440;
    }
    if (IntegerOnly)
    {
        UK2Node_CallFunction* ContainsDot = Call(Target, UKismetStringLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Contains), Column, Y + 120);
        check(Link(Text, Pin(ContainsDot, FName(TEXT("SearchIn")))));
        Pin(ContainsDot, FName(TEXT("Substring")))->DefaultValue = TEXT(".");
        Valid = And(Target, Valid,
            Pin(NotBool(Target, Pin(ContainsDot, UEdGraphSchema_K2::PN_ReturnValue),
                Column + 220, Y + 120), UEdGraphSchema_K2::PN_ReturnValue),
            Column + 440, Y);
    }
    return Valid;
}

inline UK2Node_MacroInstance* ForLoop(UEdGraph* Target, int32 X, int32 Y);

inline UK2Node_FunctionEntry* AddNumberParser(UBlueprint* Blueprint)
{
    UEdGraph* Target = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, ParseNumberFunction, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(Blueprint, Target, false, static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Existing : Target->Nodes)
    {
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Existing)) Entry = Candidate;
        if (UK2Node_FunctionResult* Candidate = Cast<UK2Node_FunctionResult>(Existing)) Result = Candidate;
    }
    check(Entry);
    if (!Result) Result = Node(NewObject<UK2Node_FunctionResult>(Target), Target, 4700, 0);
    Entry->AddExtraFlags(FUNC_Public | FUNC_BlueprintCallable);
    Entry->CreateUserDefinedPin(TextPin, PinType(UEdGraphSchema_K2::PC_String), EGPD_Output);
    FEdGraphPinType DoubleType = PinType(UEdGraphSchema_K2::PC_Real);
    DoubleType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    Result->CreateUserDefinedPin(NumberPin, DoubleType, EGPD_Input);
    Result->CreateUserDefinedPin(ValidPin, PinType(UEdGraphSchema_K2::PC_Boolean), EGPD_Input);
    Entry->ReconstructNode();
    Result->ReconstructNode();

    UK2Node_CallFunction* Lower = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, ToLower), 220, -200);
    check(Link(Pin(Entry, TextPin), Pin(Lower, SourceStringPin)));
    UK2Node_CallFunction* SplitExponent = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Split), 440, -200);
    check(Link(Pin(Lower, UEdGraphSchema_K2::PN_ReturnValue), Pin(SplitExponent, SourceStringPin)));
    Pin(SplitExponent, FName(TEXT("InStr")))->DefaultValue = TEXT("e");

    UEdGraphPin* WholeValid = DecimalSyntax(Target,
        Pin(Lower, UEdGraphSchema_K2::PN_ReturnValue), false, 660, 300);
    UEdGraphPin* MantissaValid = DecimalSyntax(Target,
        Pin(SplitExponent, LeftStringPin), false, 660, -500);
    UEdGraphPin* ExponentValid = DecimalSyntax(Target,
        Pin(SplitExponent, RightStringPin), true, 660, -100);
    UK2Node_CallFunction* ExtraExponent = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Contains), 3300, -80);
    check(Link(Pin(SplitExponent, RightStringPin),
        Pin(ExtraExponent, FName(TEXT("SearchIn")))));
    Pin(ExtraExponent, FName(TEXT("Substring")))->DefaultValue = TEXT("e");
    UEdGraphPin* NoExtraExponent = Pin(NotBool(Target,
        Pin(ExtraExponent, UEdGraphSchema_K2::PN_ReturnValue), 3520, -80),
        UEdGraphSchema_K2::PN_ReturnValue);
    UEdGraphPin* ExponentForm = And(Target, MantissaValid, ExponentValid, 3740, -300);
    ExponentForm = And(Target, ExponentForm, NoExtraExponent, 3960, -300);

    UK2Node_CallFunction* Parsed = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Conv_StringToDouble), 3300, 180);
    check(Link(Pin(Entry, TextPin), Pin(Parsed, FName(TEXT("InString")))));
    UK2Node_CallFunction* ParsedNonzero = Call(Target, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_DoubleDouble), 3520, 180);
    check(Link(Pin(Parsed, UEdGraphSchema_K2::PN_ReturnValue), Pin(ParsedNonzero, LeftPin)));
    Pin(ParsedNonzero, RightPin)->DefaultValue = ZeroLiteral;

    UK2Node_CallFunction* SelectSignificand = Call(Target, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, SelectString), 3080, 420);
    check(Link(Pin(SplitExponent, LeftStringPin), Pin(SelectSignificand, FName(TEXT("A")))));
    check(Link(Pin(Lower, UEdGraphSchema_K2::PN_ReturnValue), Pin(SelectSignificand, FName(TEXT("B")))));
    check(Link(Pin(SplitExponent, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(SelectSignificand, FName(TEXT("bPickA")))));
    UEdGraphPin* Significand = Pin(SelectSignificand, UEdGraphSchema_K2::PN_ReturnValue);
    UK2Node_CallFunction* ZeroRemainder = nullptr;
    UEdGraphPin* Reduced = Significand;
    int32 ReplaceX = 3300;
    for (const TCHAR* Token : {ZeroLiteral, TEXT("."), TEXT("+"), TEXT("-")})
    {
        ZeroRemainder = Call(Target, UKismetStringLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Replace), ReplaceX, 420);
        check(Link(Reduced, Pin(ZeroRemainder, SourceStringPin)));
        Pin(ZeroRemainder, FName(TEXT("From")))->DefaultValue = Token;
        Pin(ZeroRemainder, FName(TEXT("To")))->DefaultValue = TEXT("");
        Reduced = Pin(ZeroRemainder, UEdGraphSchema_K2::PN_ReturnValue);
        ReplaceX += 220;
    }
    UK2Node_CallFunction* RemainderLength = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Len), ReplaceX, 420);
    check(Link(Reduced, Pin(RemainderLength, FName(TEXT("S")))));
    UK2Node_CallFunction* LexicalZero = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(RemainderLength, UEdGraphSchema_K2::PN_ReturnValue), ZeroLiteral, ReplaceX + 220, 420);
    UEdGraphPin* Representable = Or(Target,
        Pin(ParsedNonzero, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(LexicalZero, UEdGraphSchema_K2::PN_ReturnValue), ReplaceX + 440, 260);

    UEdGraphPin* HasExponent = Pin(SplitExponent, UEdGraphSchema_K2::PN_ReturnValue);
    UEdGraphPin* ExponentCase = And(Target, HasExponent, ExponentForm, 4180, -300);
    UEdGraphPin* NoExponent = Pin(NotBool(Target, HasExponent, 3960, 300),
        UEdGraphSchema_K2::PN_ReturnValue);
    UEdGraphPin* WholeCase = And(Target, NoExponent, WholeValid, 4180, 300);
    UEdGraphPin* GrammarValid = Or(Target, ExponentCase, WholeCase, 4400, 0);
    UEdGraphPin* FullyValid = And(Target, GrammarValid, Representable, 4620, 0);
    check(Link(Pin(Entry, UEdGraphSchema_K2::PN_Then),
        Pin(Result, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(Parsed, UEdGraphSchema_K2::PN_ReturnValue), Pin(Result, NumberPin)));
    check(Link(FullyValid, Pin(Result, ValidPin)));
    return Entry;
}

inline UK2Node_FunctionEntry* AddStringParser(UBlueprint* Blueprint)
{
    UEdGraph* Target = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, ParseStringFunction, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(Blueprint, Target, false, static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Existing : Target->Nodes)
    {
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Existing)) Entry = Candidate;
        if (UK2Node_FunctionResult* Candidate = Cast<UK2Node_FunctionResult>(Existing)) Result = Candidate;
    }
    check(Entry);
    if (!Result) Result = Node(NewObject<UK2Node_FunctionResult>(Target), Target, 4200, 0);
    Entry->AddExtraFlags(FUNC_Public | FUNC_BlueprintCallable);
    Entry->CreateUserDefinedPin(TextPin, PinType(UEdGraphSchema_K2::PC_String), EGPD_Output);
    Result->CreateUserDefinedPin(ValuePin, PinType(UEdGraphSchema_K2::PC_String), EGPD_Input);
    Result->CreateUserDefinedPin(ValidPin, PinType(UEdGraphSchema_K2::PC_Boolean), EGPD_Input);
    Entry->ReconstructNode();
    Result->ReconstructNode();
    check(FBlueprintEditorUtils::AddLocalVariable(Blueprint, Target, DecodedLocal,
        PinType(UEdGraphSchema_K2::PC_String), TEXT("")));
    check(FBlueprintEditorUtils::AddLocalVariable(Blueprint, Target, EscapedLocal,
        PinType(UEdGraphSchema_K2::PC_Boolean), FalseLiteral));
    check(FBlueprintEditorUtils::AddLocalVariable(Blueprint, Target, InvalidLocal,
        PinType(UEdGraphSchema_K2::PC_Boolean), FalseLiteral));

    UEdGraphPin* Tail = Pin(Entry, UEdGraphSchema_K2::PN_Then);
    for (const auto& Reset : {
        TPair<FName,const TCHAR*>(DecodedLocal,TEXT("")),
        TPair<FName,const TCHAR*>(EscapedLocal,FalseLiteral),
        TPair<FName,const TCHAR*>(InvalidLocal,FalseLiteral)})
    {
        UK2Node_VariableSet* Set = WriteLocal(Blueprint, Target, Reset.Key, 220, -500);
        Pin(Set, Reset.Key)->DefaultValue = Reset.Value;
        check(Link(Tail, Pin(Set, UEdGraphSchema_K2::PN_Execute)));
        Tail = Pin(Set, UEdGraphSchema_K2::PN_Then);
    }
    UK2Node_CallFunction* Length = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Len), 440, -160);
    check(Link(Pin(Entry, TextPin), Pin(Length, FName(TEXT("S")))));
    UK2Node_MacroInstance* Loop = ForLoop(Target, 660, -500);
    check(Link(Tail, Pin(Loop, UEdGraphSchema_K2::PN_Execute)));
    Pin(Loop, FirstIndexPin)->DefaultValue = OneLiteral;
    UK2Node_CallFunction* Last = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt),
        Pin(Length, UEdGraphSchema_K2::PN_ReturnValue), TEXT("2"), 440, 0);
    check(Link(Pin(Last, UEdGraphSchema_K2::PN_ReturnValue), Pin(Loop, LastIndexPin)));
    UK2Node_CallFunction* CharacterCode = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, GetCharacterAsNumber), 880, -180);
    check(Link(Pin(Entry, TextPin), Pin(CharacterCode, SourceStringPin)));
    check(Link(Pin(Loop, IndexPin), Pin(CharacterCode, IndexPin)));
    UK2Node_CallFunction* CharacterText = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, GetSubstring), 880, 0);
    check(Link(Pin(Entry, TextPin), Pin(CharacterText, SourceStringPin)));
    check(Link(Pin(Loop, IndexPin), Pin(CharacterText, StartIndexPin)));
    Pin(CharacterText, LengthPin)->DefaultValue = OneLiteral;
    UK2Node_VariableGet* Escaped = ReadLocal(Blueprint, Target, EscapedLocal, 1100, -180);
    UK2Node_IfThenElse* EscapeBranch = Branch(Target, Pin(Loop, LoopBodyPin),
        Pin(Escaped, EscapedLocal), 1320, -500);

    UK2Node_VariableSet* ClearEscape = WriteLocal(Blueprint, Target, EscapedLocal, 1540, -700);
    Pin(ClearEscape, EscapedLocal)->DefaultValue = FalseLiteral;
    check(Link(Pin(EscapeBranch, UEdGraphSchema_K2::PN_Then), Pin(ClearEscape, UEdGraphSchema_K2::PN_Execute)));
    UK2Node_CallFunction* EscapedQuote = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(CharacterCode, UEdGraphSchema_K2::PN_ReturnValue), TEXT("34"), 1760, -780);
    UK2Node_CallFunction* EscapedSlash = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(CharacterCode, UEdGraphSchema_K2::PN_ReturnValue), TEXT("92"), 1760, -620);
    UEdGraphPin* EscapeValid = Or(Target,
        Pin(EscapedQuote, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(EscapedSlash, UEdGraphSchema_K2::PN_ReturnValue), 1980, -700);
    UK2Node_IfThenElse* EscapeValidity = Branch(Target,
        Pin(ClearEscape, UEdGraphSchema_K2::PN_Then), EscapeValid, 2200, -700);

    auto AppendCharacter = [&](UEdGraphPin* Execute, int32 X, int32 Y)
    {
        UK2Node_VariableGet* Current = ReadLocal(Blueprint, Target, DecodedLocal, X, Y + 160);
        UK2Node_CallFunction* Concat = Call(Target, UKismetStringLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Concat_StrStr), X + 220, Y + 160);
        check(Link(Pin(Current, DecodedLocal), Pin(Concat, LeftPin)));
        check(Link(Pin(CharacterText, UEdGraphSchema_K2::PN_ReturnValue), Pin(Concat, RightPin)));
        UK2Node_VariableSet* Set = WriteLocal(Blueprint, Target, DecodedLocal, X + 440, Y);
        check(Link(Execute, Pin(Set, UEdGraphSchema_K2::PN_Execute)));
        check(Link(Pin(Concat, UEdGraphSchema_K2::PN_ReturnValue), Pin(Set, DecodedLocal)));
    };
    AppendCharacter(Pin(EscapeValidity, UEdGraphSchema_K2::PN_Then), 2420, -780);
    UK2Node_VariableSet* InvalidEscape = WriteLocal(Blueprint, Target, InvalidLocal, 2420, -560);
    Pin(InvalidEscape, InvalidLocal)->DefaultValue = TrueLiteral;
    check(Link(Pin(EscapeValidity, UEdGraphSchema_K2::PN_Else),
        Pin(InvalidEscape, UEdGraphSchema_K2::PN_Execute)));

    UK2Node_CallFunction* IsSlash = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(CharacterCode, UEdGraphSchema_K2::PN_ReturnValue), TEXT("92"), 1540, -260);
    UK2Node_IfThenElse* SlashBranch = Branch(Target, Pin(EscapeBranch, UEdGraphSchema_K2::PN_Else),
        Pin(IsSlash, UEdGraphSchema_K2::PN_ReturnValue), 1760, -420);
    UK2Node_VariableSet* SetEscape = WriteLocal(Blueprint, Target, EscapedLocal, 1980, -420);
    Pin(SetEscape, EscapedLocal)->DefaultValue = TrueLiteral;
    check(Link(Pin(SlashBranch, UEdGraphSchema_K2::PN_Then), Pin(SetEscape, UEdGraphSchema_K2::PN_Execute)));
    UK2Node_CallFunction* IsQuote = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(CharacterCode, UEdGraphSchema_K2::PN_ReturnValue), TEXT("34"), 1980, -160);
    UK2Node_IfThenElse* QuoteBranch = Branch(Target, Pin(SlashBranch, UEdGraphSchema_K2::PN_Else),
        Pin(IsQuote, UEdGraphSchema_K2::PN_ReturnValue), 2200, -340);
    UK2Node_VariableSet* InvalidQuote = WriteLocal(Blueprint, Target, InvalidLocal, 2420, -340);
    Pin(InvalidQuote, InvalidLocal)->DefaultValue = TrueLiteral;
    check(Link(Pin(QuoteBranch, UEdGraphSchema_K2::PN_Then), Pin(InvalidQuote, UEdGraphSchema_K2::PN_Execute)));
    AppendCharacter(Pin(QuoteBranch, UEdGraphSchema_K2::PN_Else), 2420, -120);

    UK2Node_CallFunction* Starts = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, StartsWith), 2860, 300);
    check(Link(Pin(Entry, TextPin), Pin(Starts, SourceStringPin)));
    Pin(Starts, FName(TEXT("InPrefix")))->DefaultValue = TEXT("\"");
    UK2Node_CallFunction* Ends = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EndsWith), 2860, 460);
    check(Link(Pin(Entry, TextPin), Pin(Ends, SourceStringPin)));
    Pin(Ends, FName(TEXT("InSuffix")))->DefaultValue = TEXT("\"");
    UK2Node_CallFunction* AtLeastTwo = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_IntInt),
        Pin(Length, UEdGraphSchema_K2::PN_ReturnValue), TEXT("2"), 2860, 620);
    UEdGraphPin* Structure = And(Target,
        Pin(Starts, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(Ends, UEdGraphSchema_K2::PN_ReturnValue), 3080, 380);
    Structure = And(Target, Structure, Pin(AtLeastTwo, UEdGraphSchema_K2::PN_ReturnValue), 3300, 460);
    UK2Node_VariableGet* FinalEscape = ReadLocal(Blueprint, Target, EscapedLocal, 3080, 700);
    UK2Node_VariableGet* FinalInvalid = ReadLocal(Blueprint, Target, InvalidLocal, 3080, 840);
    UEdGraphPin* NoEscape = Pin(NotBool(Target, Pin(FinalEscape, EscapedLocal), 3300, 700),
        UEdGraphSchema_K2::PN_ReturnValue);
    UEdGraphPin* NoInvalid = Pin(NotBool(Target, Pin(FinalInvalid, InvalidLocal), 3300, 840),
        UEdGraphSchema_K2::PN_ReturnValue);
    UEdGraphPin* Valid = And(Target, Structure, NoEscape, 3520, 560);
    Valid = And(Target, Valid, NoInvalid, 3740, 560);
    UK2Node_VariableGet* Decoded = ReadLocal(Blueprint, Target, DecodedLocal, 3740, 760);
    check(Link(Pin(Loop, CompletedPin), Pin(Result, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(Decoded, DecodedLocal), Pin(Result, ValuePin)));
    check(Link(Valid, Pin(Result, ValidPin)));
    return Entry;
}

inline UK2Node_MacroInstance* ForLoop(UEdGraph* Target, int32 X, int32 Y)
{
    UBlueprint* Macros = LoadObject<UBlueprint>(nullptr, *StandardMacrosPath.ToString());
    check(Macros);
    UEdGraph* Macro = nullptr;
    for (UEdGraph* Candidate : Macros->MacroGraphs)
        if (Candidate->GetFName() == ForLoopName) Macro = Candidate;
    check(Macro);
    UK2Node_MacroInstance* Result = NewObject<UK2Node_MacroInstance>(Target);
    Result->SetMacroGraph(Macro);
    return Node(Result, Target, X, Y);
}

inline UK2Node_MacroInstance* ForEachLoop(UEdGraph* Target, int32 X, int32 Y)
{
    UBlueprint* Macros = LoadObject<UBlueprint>(nullptr, *StandardMacrosPath.ToString());
    check(Macros);
    UEdGraph* Macro = nullptr;
    for (UEdGraph* Candidate : Macros->MacroGraphs)
        if (Candidate->GetFName() == ForEachLoopName) Macro = Candidate;
    check(Macro);
    UK2Node_MacroInstance* Result = NewObject<UK2Node_MacroInstance>(Target);
    Result->SetMacroGraph(Macro);
    return Node(Result, Target, X, Y);
}

inline UEdGraphPin* FirstExecInput(UEdGraphNode* Value)
{
    for (UEdGraphPin* Candidate : Value->Pins)
        if (Candidate->Direction == EGPD_Input &&
            Candidate->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            return Candidate;
    checkNoEntry();
    return nullptr;
}

inline UK2Node_VariableSet* WriteSelf(UEdGraph* Target, const FName Name, int32 X, int32 Y)
{
    UK2Node_VariableSet* Result = NewObject<UK2Node_VariableSet>(Target);
    Result->VariableReference.SetSelfMember(Name);
    return Node(Result, Target, X, Y);
}

template <typename Predicate>
bool AddSettingMembers(
    UBlueprint* Blueprint, const FSetting* Settings, int32 Count, Predicate Include)
{
    bool Ok = true;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FSetting& Setting = Settings[Index];
        if (!Include(Setting)) continue;
        FEdGraphPinType Type;
        if (Setting.Type == ESettingType::Number)
        {
            Type.PinCategory = UEdGraphSchema_K2::PC_Real;
            Type.PinSubCategory = Setting.NumberStorage == ENumberStorage::Float
                ? UEdGraphSchema_K2::PC_Float : UEdGraphSchema_K2::PC_Double;
        }
        else if (Setting.Type == ESettingType::Boolean)
            Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        else Type.PinCategory = UEdGraphSchema_K2::PC_String;
        Ok &= FBlueprintEditorUtils::AddMemberVariable(
            Blueprint, Setting.Field, Type, Setting.Default);
    }
    return Ok;
}

inline UEdGraphPin* CompareDouble(
    UEdGraph* Target, const FName Function, UEdGraphPin* Value,
    const TCHAR* Literal, int32 X, int32 Y)
{
    UK2Node_CallFunction* Result = Call(Target, UKismetMathLibrary::StaticClass(), Function, X, Y);
    check(Link(Value, Pin(Result, LeftPin)));
    Pin(Result, RightPin)->DefaultValue = Literal;
    return Pin(Result, UEdGraphSchema_K2::PN_ReturnValue);
}

template <typename Predicate>
UK2Node_FunctionEntry* AddLoader(
    UBlueprint* Blueprint, const FName FunctionName, const TCHAR* RelativePath,
    const FSetting* Settings, int32 Count, Predicate Include)
{
    UEdGraph* Target = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, FunctionName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(Blueprint, Target, false, static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Existing : Target->Nodes)
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Existing)) Entry = Candidate;
    check(Entry);
    Entry->AddExtraFlags(FUNC_Public | FUNC_BlueprintCallable);
    UEdGraphPin* Tail = Pin(Entry, UEdGraphSchema_K2::PN_Then);
    int32 DefaultY = -700;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FSetting& Setting = Settings[Index];
        if (!Include(Setting)) continue;
        UK2Node_VariableSet* SetDefault = WriteSelf(Target, Setting.Field, 220, DefaultY);
        Pin(SetDefault, Setting.Field)->DefaultValue = Setting.Default;
        check(Link(Tail, Pin(SetDefault, UEdGraphSchema_K2::PN_Execute)));
        Tail = Pin(SetDefault, UEdGraphSchema_K2::PN_Then);
        DefaultY += 140;
    }
    UK2Node_CallFunction* Content = Call(Target, UBlueprintPathsLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UBlueprintPathsLibrary, ProjectContentDir), 440, -500);
    UK2Node_CallFunction* Path = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Concat_StrStr), 660, -500);
    check(Link(Pin(Content, UEdGraphSchema_K2::PN_ReturnValue), Pin(Path, LeftPin)));
    Pin(Path, RightPin)->DefaultValue = RelativePath;
    UK2Node_CallFunction* Load = Call(Target, UVoyageEditorBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageEditorBlueprintFunctionLibrary, LoadFileToArray), 880, -700);
    check(Link(Tail, Pin(Load, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(Path, UEdGraphSchema_K2::PN_ReturnValue), Pin(Load, InputPathPin)));
    UK2Node_MacroInstance* Loop = ForEachLoop(Target, 1100, -700);
    check(Link(Pin(Load, UEdGraphSchema_K2::PN_Then), FirstExecInput(Loop)));
    check(Link(Pin(Load, UEdGraphSchema_K2::PN_ReturnValue), Pin(Loop, ArrayPin)));
    UK2Node_CallFunction* ParseLine = Call(Target, Blueprint->GeneratedClass,
        ParseLineFunction, 1320, -700);
    check(Link(Pin(Loop, LoopBodyPin), Pin(ParseLine, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(Loop, ArrayElementPin), Pin(ParseLine, LinePin)));

    UEdGraphPin* NextKey = Pin(ParseLine, UEdGraphSchema_K2::PN_Then);
    int32 SettingY = -700;
    for (int32 Index = 0; Index < Count; ++Index)
    {
        const FSetting& Setting = Settings[Index];
        if (!Include(Setting)) continue;
        UK2Node_CallFunction* KeyMatch = Call(Target, UKismetStringLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr), 1540, SettingY + 100);
        check(Link(Pin(ParseLine, KeyPin), Pin(KeyMatch, LeftPin)));
        Pin(KeyMatch, RightPin)->DefaultValue = Setting.Key;
        UK2Node_IfThenElse* IsKey = Branch(Target, NextKey,
            Pin(KeyMatch, UEdGraphSchema_K2::PN_ReturnValue), 1760, SettingY);
        NextKey = Pin(IsKey, UEdGraphSchema_K2::PN_Else);

        if (Setting.Type == ESettingType::Number)
        {
            UK2Node_CallFunction* Parse = Call(Target, Blueprint->GeneratedClass,
                ParseNumberFunction, 1980, SettingY);
            check(Link(Pin(IsKey, UEdGraphSchema_K2::PN_Then), Pin(Parse, UEdGraphSchema_K2::PN_Execute)));
            check(Link(Pin(ParseLine, ValuePin), Pin(Parse, TextPin)));
            UEdGraphPin* Valid = And(Target, Pin(ParseLine, ValidPin), Pin(Parse, ValidPin),
                2200, SettingY + 120);
            Valid = And(Target, Valid, CompareDouble(Target,
                GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble),
                Pin(Parse, NumberPin), MinimumDoubleLiteral, 2420, SettingY + 120),
                2640, SettingY + 120);
            Valid = And(Target, Valid, CompareDouble(Target,
                GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble),
                Pin(Parse, NumberPin), MaximumDoubleLiteral, 2860, SettingY + 120),
                3080, SettingY + 120);
            if (Setting.NumberStorage == ENumberStorage::Float)
            {
                Valid = And(Target, Valid, CompareDouble(Target,
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble),
                    Pin(Parse, NumberPin), MinimumFloatLiteral, 4180, SettingY + 120),
                    4400, SettingY + 120);
                Valid = And(Target, Valid, CompareDouble(Target,
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble),
                    Pin(Parse, NumberPin), MaximumFloatLiteral, 4620, SettingY + 120),
                    4840, SettingY + 120);
                UK2Node_CallFunction* ToFloat = Call(Target, UKismetMathLibrary::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_DoubleToFloat), 4180, SettingY + 280);
                check(Link(Pin(Parse, NumberPin), Pin(ToFloat, FName(TEXT("InDouble")))));
                UK2Node_CallFunction* Back = Call(Target, UKismetMathLibrary::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Conv_FloatToDouble), 4400, SettingY + 280);
                check(Link(Pin(ToFloat, UEdGraphSchema_K2::PN_ReturnValue), Pin(Back, FName(TEXT("InFloat")))));
                UEdGraphPin* OriginalZero = CompareDouble(Target,
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_DoubleDouble),
                    Pin(Parse, NumberPin), ZeroLiteral, 4620, SettingY + 280);
                UEdGraphPin* StoredNonzero = CompareDouble(Target,
                    GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_DoubleDouble),
                    Pin(Back, UEdGraphSchema_K2::PN_ReturnValue), ZeroLiteral, 4620, SettingY + 400);
                Valid = And(Target, Valid, Or(Target, OriginalZero, StoredNonzero,
                    4840, SettingY + 340), 5060, SettingY + 120);
                // Float constraints apply to the represented value. This preserves the
                // old Container behavior where a tiny positive double may round to the
                // smallest positive float, while a value that rounds to zero is invalid.
                if (Setting.Minimum)
                    Valid = And(Target, Valid, CompareDouble(Target,
                        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble),
                        Pin(Back, UEdGraphSchema_K2::PN_ReturnValue), Setting.Minimum,
                        5060, SettingY + 280), 5280, SettingY + 180);
                if (Setting.Maximum)
                    Valid = And(Target, Valid, CompareDouble(Target,
                        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble),
                        Pin(Back, UEdGraphSchema_K2::PN_ReturnValue), Setting.Maximum,
                        5280, SettingY + 320), 5500, SettingY + 220);
            }
            else
            {
                if (Setting.Minimum)
                    Valid = And(Target, Valid, CompareDouble(Target,
                        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_DoubleDouble),
                        Pin(Parse, NumberPin), Setting.Minimum, 3300, SettingY + 120), 3520, SettingY + 120);
                if (Setting.Maximum)
                    Valid = And(Target, Valid, CompareDouble(Target,
                        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, LessEqual_DoubleDouble),
                        Pin(Parse, NumberPin), Setting.Maximum, 3740, SettingY + 120), 3960, SettingY + 120);
            }
            UK2Node_IfThenElse* IsValid = Branch(Target, Pin(Parse, UEdGraphSchema_K2::PN_Then),
                Valid, 5280, SettingY);
            UK2Node_VariableSet* SetValue = WriteSelf(Target, Setting.Field, 5500, SettingY - 80);
            check(Link(Pin(IsValid, UEdGraphSchema_K2::PN_Then), Pin(SetValue, UEdGraphSchema_K2::PN_Execute)));
            check(Link(Pin(Parse, NumberPin), Pin(SetValue, Setting.Field)));
            UK2Node_VariableSet* SetDefault = WriteSelf(Target, Setting.Field, 5500, SettingY + 100);
            Pin(SetDefault, Setting.Field)->DefaultValue = Setting.Default;
            check(Link(Pin(IsValid, UEdGraphSchema_K2::PN_Else), Pin(SetDefault, UEdGraphSchema_K2::PN_Execute)));
        }
        else if (Setting.Type == ESettingType::Boolean)
        {
            UK2Node_CallFunction* Lower = Call(Target, UKismetStringLibrary::StaticClass(),
                GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, ToLower), 1980, SettingY + 140);
            check(Link(Pin(ParseLine, ValuePin), Pin(Lower, SourceStringPin)));
            auto EqualLiteral = [&](const TCHAR* Literal, int32 Y)
            {
                UK2Node_CallFunction* Equal = Call(Target, UKismetStringLibrary::StaticClass(),
                    GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr), 2200, Y);
                check(Link(Pin(Lower, UEdGraphSchema_K2::PN_ReturnValue), Pin(Equal, LeftPin)));
                Pin(Equal, RightPin)->DefaultValue = Literal;
                return Pin(Equal, UEdGraphSchema_K2::PN_ReturnValue);
            };
            UEdGraphPin* TrueValue = EqualLiteral(TrueLiteral, SettingY + 80);
            UEdGraphPin* FalseValue = EqualLiteral(FalseLiteral, SettingY + 220);
            UEdGraphPin* TypeValid = Or(Target, TrueValue, FalseValue, 2420, SettingY + 140);
            UEdGraphPin* Valid = And(Target, Pin(ParseLine, ValidPin), TypeValid, 2640, SettingY + 140);
            UK2Node_IfThenElse* IsValid = Branch(Target, Pin(IsKey, UEdGraphSchema_K2::PN_Then),
                Valid, 2860, SettingY);
            UK2Node_IfThenElse* IsTrue = Branch(Target, Pin(IsValid, UEdGraphSchema_K2::PN_Then),
                TrueValue, 3080, SettingY - 80);
            UK2Node_VariableSet* SetTrue = WriteSelf(Target, Setting.Field, 3300, SettingY - 160);
            Pin(SetTrue, Setting.Field)->DefaultValue = TrueLiteral;
            check(Link(Pin(IsTrue, UEdGraphSchema_K2::PN_Then), Pin(SetTrue, UEdGraphSchema_K2::PN_Execute)));
            UK2Node_VariableSet* SetFalse = WriteSelf(Target, Setting.Field, 3300, SettingY);
            Pin(SetFalse, Setting.Field)->DefaultValue = FalseLiteral;
            check(Link(Pin(IsTrue, UEdGraphSchema_K2::PN_Else), Pin(SetFalse, UEdGraphSchema_K2::PN_Execute)));
            UK2Node_VariableSet* SetDefault = WriteSelf(Target, Setting.Field, 3080, SettingY + 180);
            Pin(SetDefault, Setting.Field)->DefaultValue = Setting.Default;
            check(Link(Pin(IsValid, UEdGraphSchema_K2::PN_Else), Pin(SetDefault, UEdGraphSchema_K2::PN_Execute)));
        }
        else
        {
            UK2Node_CallFunction* Parse = Call(Target, Blueprint->GeneratedClass,
                ParseStringFunction, 1980, SettingY);
            check(Link(Pin(IsKey, UEdGraphSchema_K2::PN_Then), Pin(Parse, UEdGraphSchema_K2::PN_Execute)));
            check(Link(Pin(ParseLine, ValuePin), Pin(Parse, TextPin)));
            UEdGraphPin* Valid = And(Target, Pin(ParseLine, ValidPin), Pin(Parse, ValidPin),
                2200, SettingY + 140);
            UK2Node_IfThenElse* IsValid = Branch(Target, Pin(Parse, UEdGraphSchema_K2::PN_Then),
                Valid, 2420, SettingY);
            UK2Node_VariableSet* SetValue = WriteSelf(Target, Setting.Field, 2640, SettingY - 80);
            check(Link(Pin(IsValid, UEdGraphSchema_K2::PN_Then), Pin(SetValue, UEdGraphSchema_K2::PN_Execute)));
            check(Link(Pin(Parse, ValuePin), Pin(SetValue, Setting.Field)));
            UK2Node_VariableSet* SetDefault = WriteSelf(Target, Setting.Field, 2640, SettingY + 100);
            Pin(SetDefault, Setting.Field)->DefaultValue = Setting.Default;
            check(Link(Pin(IsValid, UEdGraphSchema_K2::PN_Else), Pin(SetDefault, UEdGraphSchema_K2::PN_Execute)));
        }
        SettingY += 520;
    }
    return Entry;
}

inline UK2Node_FunctionEntry* AddLineParser(UBlueprint* Blueprint)
{
    UEdGraph* Target = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, ParseLineFunction, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(Blueprint, Target, false, static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* Entry = nullptr;
    UK2Node_FunctionResult* Result = nullptr;
    for (UEdGraphNode* Existing : Target->Nodes)
    {
        if (UK2Node_FunctionEntry* Candidate = Cast<UK2Node_FunctionEntry>(Existing)) Entry = Candidate;
        if (UK2Node_FunctionResult* Candidate = Cast<UK2Node_FunctionResult>(Existing)) Result = Candidate;
    }
    check(Entry);
    if (!Result) Result = Node(NewObject<UK2Node_FunctionResult>(Target), Target, 5200, 0);
    Entry->AddExtraFlags(FUNC_Public | FUNC_BlueprintCallable);
    Entry->CreateUserDefinedPin(LinePin, PinType(UEdGraphSchema_K2::PC_String), EGPD_Output);
    Result->CreateUserDefinedPin(KeyPin, PinType(UEdGraphSchema_K2::PC_String), EGPD_Input);
    Result->CreateUserDefinedPin(ValuePin, PinType(UEdGraphSchema_K2::PC_String), EGPD_Input);
    Result->CreateUserDefinedPin(ValidPin, PinType(UEdGraphSchema_K2::PC_Boolean), EGPD_Input);
    Entry->ReconstructNode();
    Result->ReconstructNode();

    check(FBlueprintEditorUtils::AddLocalVariable(Blueprint, Target, SeparatorLocal,
        PinType(UEdGraphSchema_K2::PC_Int), NegativeOneLiteral));
    check(FBlueprintEditorUtils::AddLocalVariable(Blueprint, Target, CommentLocal,
        PinType(UEdGraphSchema_K2::PC_Int), ZeroLiteral));
    for (const FName Name : {QuotedLocal, EscapedLocal, InvalidLocal})
        check(FBlueprintEditorUtils::AddLocalVariable(Blueprint, Target, Name,
            PinType(UEdGraphSchema_K2::PC_Boolean), FalseLiteral));

    UEdGraphPin* Tail = Pin(Entry, UEdGraphSchema_K2::PN_Then);
    auto SetLiteral = [&](const FName Name, const TCHAR* Literal, int32 X, int32 Y)
    {
        UK2Node_VariableSet* Set = WriteLocal(Blueprint, Target, Name, X, Y);
        Pin(Set, Name)->DefaultValue = Literal;
        check(Link(Tail, Pin(Set, UEdGraphSchema_K2::PN_Execute)));
        Tail = Pin(Set, UEdGraphSchema_K2::PN_Then);
    };
    SetLiteral(SeparatorLocal, NegativeOneLiteral, 220, -500);
    SetLiteral(QuotedLocal, FalseLiteral, 440, -500);
    SetLiteral(EscapedLocal, FalseLiteral, 660, -500);
    SetLiteral(InvalidLocal, FalseLiteral, 880, -500);

    UK2Node_CallFunction* Length = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Len), 880, -180);
    check(Link(Pin(Entry, LinePin), Pin(Length, FName(TEXT("S")))));
    UK2Node_VariableSet* SetComment = WriteLocal(Blueprint, Target, CommentLocal, 1100, -500);
    check(Link(Tail, Pin(SetComment, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(Length, UEdGraphSchema_K2::PN_ReturnValue), Pin(SetComment, CommentLocal)));
    Tail = Pin(SetComment, UEdGraphSchema_K2::PN_Then);

    UK2Node_MacroInstance* Loop = ForLoop(Target, 1320, -500);
    check(Link(Tail, Pin(Loop, UEdGraphSchema_K2::PN_Execute)));
    Pin(Loop, FirstIndexPin)->DefaultValue = ZeroLiteral;
    UK2Node_CallFunction* Last = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt),
        Pin(Length, UEdGraphSchema_K2::PN_ReturnValue), OneLiteral, 1100, -80);
    check(Link(Pin(Last, UEdGraphSchema_K2::PN_ReturnValue), Pin(Loop, LastIndexPin)));

    UK2Node_VariableGet* Comment = ReadLocal(Blueprint, Target, CommentLocal, 1540, -250);
    UK2Node_CallFunction* Active = Call(Target, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt), 1760, -250);
    check(Link(Pin(Comment, CommentLocal), Pin(Active, LeftPin)));
    check(Link(Pin(Length, UEdGraphSchema_K2::PN_ReturnValue), Pin(Active, RightPin)));
    UK2Node_IfThenElse* ActiveBranch = Branch(Target, Pin(Loop, LoopBodyPin),
        Pin(Active, UEdGraphSchema_K2::PN_ReturnValue), 1980, -500);

    UK2Node_CallFunction* Character = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, GetCharacterAsNumber), 1980, -180);
    check(Link(Pin(Entry, LinePin), Pin(Character, SourceStringPin)));
    check(Link(Pin(Loop, IndexPin), Pin(Character, IndexPin)));
    UK2Node_VariableGet* Escaped = ReadLocal(Blueprint, Target, EscapedLocal, 2200, -180);
    UK2Node_IfThenElse* EscapedBranch = Branch(Target, Pin(ActiveBranch, UEdGraphSchema_K2::PN_Then),
        Pin(Escaped, EscapedLocal), 2420, -500);

    UK2Node_VariableSet* ClearEscaped = WriteLocal(Blueprint, Target, EscapedLocal, 2640, -700);
    Pin(ClearEscaped, EscapedLocal)->DefaultValue = FalseLiteral;
    check(Link(Pin(EscapedBranch, UEdGraphSchema_K2::PN_Then), Pin(ClearEscaped, UEdGraphSchema_K2::PN_Execute)));
    UK2Node_CallFunction* IsQuoteEscaped = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(Character, UEdGraphSchema_K2::PN_ReturnValue), TEXT("34"), 2860, -790);
    UK2Node_CallFunction* IsSlashEscaped = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(Character, UEdGraphSchema_K2::PN_ReturnValue), TEXT("92"), 2860, -630);
    UK2Node_CallFunction* ValidEscape = BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR),
        Pin(IsQuoteEscaped, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(IsSlashEscaped, UEdGraphSchema_K2::PN_ReturnValue), 3080, -700);
    UK2Node_IfThenElse* EscapeValidBranch = Branch(Target,
        Pin(ClearEscaped, UEdGraphSchema_K2::PN_Then),
        Pin(ValidEscape, UEdGraphSchema_K2::PN_ReturnValue), 3300, -700);
    UK2Node_VariableSet* SetInvalid = WriteLocal(Blueprint, Target, InvalidLocal, 3520, -620);
    Pin(SetInvalid, InvalidLocal)->DefaultValue = TrueLiteral;
    check(Link(Pin(EscapeValidBranch, UEdGraphSchema_K2::PN_Else), Pin(SetInvalid, UEdGraphSchema_K2::PN_Execute)));

    UK2Node_VariableGet* Quoted = ReadLocal(Blueprint, Target, QuotedLocal, 2420, -120);
    UK2Node_CallFunction* IsSlash = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(Character, UEdGraphSchema_K2::PN_ReturnValue), TEXT("92"), 2420, 20);
    UK2Node_CallFunction* QuotedSlash = BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND), Pin(Quoted, QuotedLocal),
        Pin(IsSlash, UEdGraphSchema_K2::PN_ReturnValue), 2640, -80);
    UK2Node_IfThenElse* SlashBranch = Branch(Target, Pin(EscapedBranch, UEdGraphSchema_K2::PN_Else),
        Pin(QuotedSlash, UEdGraphSchema_K2::PN_ReturnValue), 2860, -300);
    UK2Node_VariableSet* SetEscaped = WriteLocal(Blueprint, Target, EscapedLocal, 3080, -420);
    Pin(SetEscaped, EscapedLocal)->DefaultValue = TrueLiteral;
    check(Link(Pin(SlashBranch, UEdGraphSchema_K2::PN_Then), Pin(SetEscaped, UEdGraphSchema_K2::PN_Execute)));

    UK2Node_CallFunction* IsQuote = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(Character, UEdGraphSchema_K2::PN_ReturnValue), TEXT("34"), 3080, -40);
    UK2Node_IfThenElse* QuoteBranch = Branch(Target, Pin(SlashBranch, UEdGraphSchema_K2::PN_Else),
        Pin(IsQuote, UEdGraphSchema_K2::PN_ReturnValue), 3300, -260);
    UK2Node_CallFunction* ToggleQuoted = NotBool(Target, Pin(Quoted, QuotedLocal), 3300, -20);
    UK2Node_VariableSet* SetQuoted = WriteLocal(Blueprint, Target, QuotedLocal, 3520, -340);
    check(Link(Pin(QuoteBranch, UEdGraphSchema_K2::PN_Then), Pin(SetQuoted, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(ToggleQuoted, UEdGraphSchema_K2::PN_ReturnValue), Pin(SetQuoted, QuotedLocal)));

    UK2Node_CallFunction* NotQuoted = NotBool(Target, Pin(Quoted, QuotedLocal), 3520, -40);
    UK2Node_CallFunction* IsHash = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(Character, UEdGraphSchema_K2::PN_ReturnValue), TEXT("35"), 3520, 100);
    UK2Node_CallFunction* UnquotedHash = BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Pin(NotQuoted, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(IsHash, UEdGraphSchema_K2::PN_ReturnValue), 3740, 20);
    UK2Node_IfThenElse* HashBranch = Branch(Target, Pin(QuoteBranch, UEdGraphSchema_K2::PN_Else),
        Pin(UnquotedHash, UEdGraphSchema_K2::PN_ReturnValue), 3960, -180);
    UK2Node_VariableSet* SetCommentIndex = WriteLocal(Blueprint, Target, CommentLocal, 4180, -260);
    check(Link(Pin(HashBranch, UEdGraphSchema_K2::PN_Then), Pin(SetCommentIndex, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(Loop, IndexPin), Pin(SetCommentIndex, CommentLocal)));

    UK2Node_VariableGet* Separator = ReadLocal(Blueprint, Target, SeparatorLocal, 3960, 120);
    UK2Node_CallFunction* SeparatorUnset = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_IntInt), Pin(Separator, SeparatorLocal),
        ZeroLiteral, 4180, 120);
    UK2Node_CallFunction* IsEquals = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt),
        Pin(Character, UEdGraphSchema_K2::PN_ReturnValue), TEXT("61"), 4180, 260);
    UK2Node_CallFunction* CanSetSeparator = BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Pin(NotQuoted, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(SeparatorUnset, UEdGraphSchema_K2::PN_ReturnValue), 4400, 100);
    UK2Node_CallFunction* ShouldSetSeparator = BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Pin(CanSetSeparator, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(IsEquals, UEdGraphSchema_K2::PN_ReturnValue), 4620, 120);
    UK2Node_IfThenElse* SeparatorBranch = Branch(Target, Pin(HashBranch, UEdGraphSchema_K2::PN_Else),
        Pin(ShouldSetSeparator, UEdGraphSchema_K2::PN_ReturnValue), 4840, -100);
    UK2Node_VariableSet* SetSeparator = WriteLocal(Blueprint, Target, SeparatorLocal, 5060, -100);
    check(Link(Pin(SeparatorBranch, UEdGraphSchema_K2::PN_Then), Pin(SetSeparator, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(Loop, IndexPin), Pin(SetSeparator, SeparatorLocal)));

    UK2Node_VariableGet* FinalSeparator = ReadLocal(Blueprint, Target, SeparatorLocal, 3300, 520);
    UK2Node_VariableGet* FinalComment = ReadLocal(Blueprint, Target, CommentLocal, 3300, 650);
    UK2Node_VariableGet* FinalQuoted = ReadLocal(Blueprint, Target, QuotedLocal, 3300, 780);
    UK2Node_VariableGet* FinalEscaped = ReadLocal(Blueprint, Target, EscapedLocal, 3300, 910);
    UK2Node_VariableGet* FinalInvalid = ReadLocal(Blueprint, Target, InvalidLocal, 3300, 1040);
    UK2Node_CallFunction* HasSeparator = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, GreaterEqual_IntInt),
        Pin(FinalSeparator, SeparatorLocal), ZeroLiteral, 3520, 520);
    UK2Node_CallFunction* ClosedQuote = NotBool(Target, Pin(FinalQuoted, QuotedLocal), 3520, 780);
    UK2Node_CallFunction* NoEscape = NotBool(Target, Pin(FinalEscaped, EscapedLocal), 3520, 910);
    UK2Node_CallFunction* NoInvalid = NotBool(Target, Pin(FinalInvalid, InvalidLocal), 3520, 1040);
    UK2Node_CallFunction* SyntaxA = BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Pin(HasSeparator, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(ClosedQuote, UEdGraphSchema_K2::PN_ReturnValue), 3740, 620);
    UK2Node_CallFunction* SyntaxB = BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Pin(NoEscape, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(NoInvalid, UEdGraphSchema_K2::PN_ReturnValue), 3740, 920);
    UK2Node_CallFunction* Syntax = BoolBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        Pin(SyntaxA, UEdGraphSchema_K2::PN_ReturnValue),
        Pin(SyntaxB, UEdGraphSchema_K2::PN_ReturnValue), 3960, 760);

    UK2Node_CallFunction* KeyText = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, GetSubstring), 3740, 420);
    check(Link(Pin(Entry, LinePin), Pin(KeyText, SourceStringPin)));
    Pin(KeyText, StartIndexPin)->DefaultValue = ZeroLiteral;
    check(Link(Pin(FinalSeparator, SeparatorLocal), Pin(KeyText, LengthPin)));
    UK2Node_CallFunction* TrimKeyStart = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Trim), 4180, 420);
    check(Link(Pin(KeyText, UEdGraphSchema_K2::PN_ReturnValue), Pin(TrimKeyStart, SourceStringPin)));
    UK2Node_CallFunction* TrimKey = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, TrimTrailing), 4400, 420);
    check(Link(Pin(TrimKeyStart, UEdGraphSchema_K2::PN_ReturnValue), Pin(TrimKey, SourceStringPin)));

    UK2Node_CallFunction* ValueStart = IntBinary(Target,
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt),
        Pin(FinalSeparator, SeparatorLocal), OneLiteral, 3520, 650);
    UK2Node_CallFunction* ValueLength = Call(Target, UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_IntInt), 3740, 650);
    check(Link(Pin(FinalComment, CommentLocal), Pin(ValueLength, LeftPin)));
    check(Link(Pin(ValueStart, UEdGraphSchema_K2::PN_ReturnValue), Pin(ValueLength, RightPin)));
    UK2Node_CallFunction* ValueText = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, GetSubstring), 4180, 650);
    check(Link(Pin(Entry, LinePin), Pin(ValueText, SourceStringPin)));
    check(Link(Pin(ValueStart, UEdGraphSchema_K2::PN_ReturnValue), Pin(ValueText, StartIndexPin)));
    check(Link(Pin(ValueLength, UEdGraphSchema_K2::PN_ReturnValue), Pin(ValueText, LengthPin)));
    UK2Node_CallFunction* TrimValueStart = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, Trim), 4400, 650);
    check(Link(Pin(ValueText, UEdGraphSchema_K2::PN_ReturnValue), Pin(TrimValueStart, SourceStringPin)));
    UK2Node_CallFunction* TrimValue = Call(Target, UKismetStringLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, TrimTrailing), 4620, 650);
    check(Link(Pin(TrimValueStart, UEdGraphSchema_K2::PN_ReturnValue), Pin(TrimValue, SourceStringPin)));

    check(Link(Pin(Loop, CompletedPin), Pin(Result, UEdGraphSchema_K2::PN_Execute)));
    check(Link(Pin(TrimKey, UEdGraphSchema_K2::PN_ReturnValue), Pin(Result, KeyPin)));
    check(Link(Pin(TrimValue, UEdGraphSchema_K2::PN_ReturnValue), Pin(Result, ValuePin)));
    check(Link(Pin(Syntax, UEdGraphSchema_K2::PN_ReturnValue), Pin(Result, ValidPin)));
    return Entry;
}

template <typename Predicate>
bool AddSettingsLoader(
    UBlueprint* Blueprint, const FName FunctionName, const TCHAR* RelativePath,
    const FSetting* Settings, int32 Count, Predicate Include)
{
    AddLineParser(Blueprint);
    AddNumberParser(Blueprint);
    AddStringParser(Blueprint);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) return false;
    AddLoader(Blueprint, FunctionName, RelativePath, Settings, Count, Include);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return Blueprint->Status != BS_Error;
}

inline void TestLineParser(UBlueprint* Blueprint)
{
    check(Blueprint && Blueprint->GeneratedClass);
    UFunction* Function = Blueprint->GeneratedClass->FindFunctionByName(
        ParseLineFunction, EIncludeSuperFlag::ExcludeSuper);
    check(Function && Function->GetOwnerClass() == Blueprint->GeneratedClass);
    struct FParameters
    {
        FString Line;
        FString Key;
        FString Value;
        bool Valid = false;
    };
    struct FCase
    {
        const TCHAR* Line;
        const TCHAR* Key;
        const TCHAR* Value;
        bool Valid;
    };
    const FCase Cases[] = {
        {TEXT("  Name = \"a=#b\" # note"), TEXT("Name"), TEXT("\"a=#b\""), true},
        {TEXT("Text=\"slash\\\\ and quote\\\" # kept\"#tail"), TEXT("Text"),
            TEXT("\"slash\\\\ and quote\\\" # kept\""), true},
        {TEXT("YawLimitDegrees = 80 # note"), TEXT("YawLimitDegrees"), TEXT("80"), true},
        {TEXT("Unknown=\"bad\\q\""), TEXT("Unknown"), TEXT("\"bad\\q\""), false},
        {TEXT("Unknown=\"unterminated"), TEXT("Unknown"), TEXT("\"unterminated"), false},
        {TEXT("Next=true"), TEXT("Next"), TrueLiteral, true},
        {TEXT("NoSeparator"), TEXT(""), TEXT("NoSeparator"), false},
    };
    UObject* Defaults = Blueprint->GeneratedClass->GetDefaultObject();
    for (const FCase& Case : Cases)
    {
        FParameters Parameters;
        Parameters.Line = Case.Line;
        Defaults->ProcessEvent(Function, &Parameters);
        checkf(Parameters.Valid == Case.Valid && Parameters.Key == Case.Key &&
            Parameters.Value == Case.Value,
            TEXT("Generated settings line parser failed for '%s': key='%s' value='%s' valid=%d"),
            Case.Line, *Parameters.Key, *Parameters.Value, Parameters.Valid);
    }
}

inline void TestNumberParser(UBlueprint* Blueprint)
{
    check(Blueprint && Blueprint->GeneratedClass);
    UFunction* Function = Blueprint->GeneratedClass->FindFunctionByName(
        ParseNumberFunction, EIncludeSuperFlag::ExcludeSuper);
    check(Function && Function->GetOwnerClass() == Blueprint->GeneratedClass);
    struct FParameters { FString Text; double Number = 0.0; bool Valid = false; };
    struct FCase { const TCHAR* Text; bool Valid; double Number; bool CheckNumber; };
    const FCase Cases[] = {
        {TEXT("-12.5"), true, -12.5, true}, {TEXT("+0.25"), true, 0.25, true},
        {TEXT("1e3"), true, 1000.0, true}, {TEXT("0e-999"), true, 0.0, true},
        {TEXT("1e-999"), false, 0.0, false}, {TEXT("1e999"), true, 0.0, false},
        {TEXT("."), false, 0.0, false}, {TEXT("1.2.3"), false, 0.0, false},
        {TEXT("1e2e3"), false, 0.0, false}, {TEXT("1;note"), false, 0.0, false},
    };
    UObject* Defaults = Blueprint->GeneratedClass->GetDefaultObject();
    for (const FCase& Case : Cases)
    {
        FParameters Parameters;
        Parameters.Text = Case.Text;
        Defaults->ProcessEvent(Function, &Parameters);
        checkf(Parameters.Valid == Case.Valid &&
            (!Case.CheckNumber || Parameters.Number == Case.Number),
            TEXT("Generated settings number parser failed for '%s': number=%g valid=%d"),
            Case.Text, Parameters.Number, Parameters.Valid);
    }
    FParameters Underflow;
    Underflow.Text = TEXT("0.") + FString::ChrN(400, TEXT('0')) + OneLiteral;
    Defaults->ProcessEvent(Function, &Underflow);
    checkf(!Underflow.Valid, TEXT("Generated settings number parser accepted decimal underflow"));
}

inline void TestStringParser(UBlueprint* Blueprint)
{
    check(Blueprint && Blueprint->GeneratedClass);
    UFunction* Function = Blueprint->GeneratedClass->FindFunctionByName(
        ParseStringFunction, EIncludeSuperFlag::ExcludeSuper);
    check(Function && Function->GetOwnerClass() == Blueprint->GeneratedClass);
    struct FParameters { FString Text; FString Value; bool Valid = false; };
    struct FCase { const TCHAR* Text; const TCHAR* Value; bool Valid; };
    const FCase Cases[] = {
        {TEXT("\"\""), TEXT(""), true},
        {TEXT("\"a=#b\""), TEXT("a=#b"), true},
        {TEXT("\"slash\\\\ and quote\\\"\""), TEXT("slash\\ and quote\""), true},
        {TEXT("\"bad\\q\""), TEXT("bad"), false},
        {TEXT("\"a\"junk"), TEXT("a"), false},
        {TEXT("\"a\"\"b\""), TEXT("ab"), false},
        {TEXT("plain"), TEXT("lai"), false},
    };
    UObject* Defaults = Blueprint->GeneratedClass->GetDefaultObject();
    for (const FCase& Case : Cases)
    {
        FParameters Parameters;
        Parameters.Text = Case.Text;
        Defaults->ProcessEvent(Function, &Parameters);
        checkf(Parameters.Valid == Case.Valid &&
            (!Case.Valid || Parameters.Value == Case.Value),
            TEXT("Generated settings string parser failed for '%s': value='%s' valid=%d"),
            Case.Text, *Parameters.Value, Parameters.Valid);
    }
}
}

#endif
