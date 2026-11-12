// Independent executable fixture; never uses a mod project or installs files.
#pragma once
#include "Kismet/KismetMathLibrary.h"
namespace BlueprintArgumentTests
{
using namespace BlueprintObjectTests;
constexpr TCHAR CalleePath[]=TEXT("/Game/BPTArgumentCallee");
constexpr TCHAR CallerPath[]=TEXT("/Game/BPTArgumentCaller");
bool Echo(UBlueprint* BP,FName Name,FName Category,const FString& Default,FName SubCategory=UEdGraphSchema_K2::PC_Float)
{
    auto* G=Function(BP,Name);auto* E=Entry(G);FEdGraphPinType Type;Type.PinCategory=Category;
    if(Category==UEdGraphSchema_K2::PC_Real)Type.PinSubCategory=SubCategory;
    auto* In=E->CreateUserDefinedPin(TEXT("Value"),Type,EGPD_Output);In->DefaultValue=Default;
    E->UpdateUserDefinedPinDefaultValues();
    E->MetaData.SetMetaData(TEXT("BlueprintText.OptionalParameters"),FString(TEXT("[\"Value\"]")));
    E->AddExtraFlags(FUNC_BlueprintPure);
    auto* R=Node<UK2Node_FunctionResult>(G);R->AllocateDefaultPins();
    R->CreateUserDefinedPin(TEXT("ReturnValue"),Type,EGPD_Input);
    return Link(E->FindPin(UEdGraphSchema_K2::PN_Then),R->FindPin(UEdGraphSchema_K2::PN_Execute)) && Link(In,R->FindPin(TEXT("ReturnValue")));
}
bool Fixture()
{
    auto* Callee=Create(CalleePath,UObject::StaticClass());
    if(!Callee || !Echo(Callee,TEXT("EchoInt"),UEdGraphSchema_K2::PC_Int,TEXT("7")) ||
       !Echo(Callee,TEXT("EchoBool"),UEdGraphSchema_K2::PC_Boolean,TEXT("true")) ||
       !Echo(Callee,TEXT("EchoString"),UEdGraphSchema_K2::PC_String,TEXT("alpha")) ||
       !Echo(Callee,TEXT("EchoEmpty"),UEdGraphSchema_K2::PC_String,TEXT("")) ||
       !Echo(Callee,TEXT("EchoFloat"),UEdGraphSchema_K2::PC_Real,TEXT("3.4028234663852886e38")) ||
       !Echo(Callee,TEXT("EchoDouble"),UEdGraphSchema_K2::PC_Real,TEXT("17.250000001"),UEdGraphSchema_K2::PC_Double) ||
       !Echo(Callee,TEXT("EchoInt64"),UEdGraphSchema_K2::PC_Int64,TEXT("4294967297")) ||
       !Echo(Callee,TEXT("Required"),UEdGraphSchema_K2::PC_Int,TEXT("0")))return false;
    Entry(Callee->FunctionGraphs.Last())->MetaData.SetMetaData(TEXT("BlueprintText.OptionalParameters"),FString(TEXT("[]")));
    if(!Compile(Callee) || !Save(Callee))return false;
    auto* BP=Create(CallerPath,UObject::StaticClass());if(!BP)return false;
    FEdGraphPinType TargetType;TargetType.PinCategory=UEdGraphSchema_K2::PC_Object;TargetType.PinSubCategoryObject=Callee->GeneratedClass;
    if(!FBlueprintEditorUtils::AddMemberVariable(BP,TEXT("Target"),TargetType) ||
       !Echo(BP,TEXT("LocalEcho"),UEdGraphSchema_K2::PC_Int,TEXT("13")) || !Compile(BP))return false;
    auto* G=Function(BP,TEXT("Probe"));auto* E=Entry(G);
    auto* R=Node<UK2Node_FunctionResult>(G);R->AllocateDefaultPins();
    if(!Link(E->FindPin(UEdGraphSchema_K2::PN_Then),R->FindPin(UEdGraphSchema_K2::PN_Execute)))return false;
    auto* Target=Node<UK2Node_VariableGet>(G);Target->VariableReference.SetSelfMember(TEXT("Target"));Target->AllocateDefaultPins();
    struct FCase {const TCHAR* Name;const TCHAR* Function;const TCHAR* State;};
    for(const FCase& Case:{FCase{TEXT("Inherited"),TEXT("EchoInt"),TEXT("{}")},
        FCase{TEXT("ExplicitEqual"),TEXT("EchoInt"),TEXT("{\"Value\":\"7\"}")},
        FCase{TEXT("ExplicitZero"),TEXT("EchoInt"),TEXT("{\"Value\":\"0\"}")},
        FCase{TEXT("InheritedBool"),TEXT("EchoBool"),TEXT("{}")},
        FCase{TEXT("ExplicitFalse"),TEXT("EchoBool"),TEXT("{\"Value\":\"false\"}")},
        FCase{TEXT("InheritedString"),TEXT("EchoString"),TEXT("{}")},
        FCase{TEXT("ExplicitEmpty"),TEXT("EchoString"),TEXT("{\"Value\":\"\"}")},
        FCase{TEXT("InheritedEmpty"),TEXT("EchoEmpty"),TEXT("{}")},
        FCase{TEXT("Required"),TEXT("Required"),TEXT("{\"Value\":\"4\"}")},
        FCase{TEXT("InheritedFloat"),TEXT("EchoFloat"),TEXT("{}")},
        FCase{TEXT("InheritedDouble"),TEXT("EchoDouble"),TEXT("{}")},
        FCase{TEXT("InheritedInt64"),TEXT("EchoInt64"),TEXT("{}")},
        FCase{TEXT("Local"),TEXT("LocalEcho"),TEXT("{}")}})
    {
        auto* Call=Node<UBlueprintTextCallFunction>(G);Call->Rename(Case.Name,G);
        const bool Local=FString(Case.Name)==TEXT("Local");
        Call->SetFromFunction((Local?BP:Callee)->GeneratedClass->FindFunctionByName(Case.Function));
        Call->AllocateDefaultPins();Call->BPTArgumentOverrides=Case.State;FString Error;
        if(!Call->RefreshArguments(Error))return false;
        auto* Out=Call->FindPin(TEXT("ReturnValue"));R->CreateUserDefinedPin(Case.Name,Out->PinType,EGPD_Input);
        if(!Link(Out,R->FindPin(Case.Name)) || (!Local && !Link(Target->FindPin(TEXT("Target")),Call->FindPin(UEdGraphSchema_K2::PN_Self))))return false;
    }
    auto* Native=Node<UBlueprintTextCallFunction>(G);Native->Rename(TEXT("Native"),G);
    Native->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(TEXT("Add_IntInt")));Native->AllocateDefaultPins();
    Native->BPTArgumentOverrides=TEXT("{\"A\":\"11\",\"B\":\"1\"}");FString Error;if(!Native->RefreshArguments(Error))return false;
    auto* Out=Native->FindPin(TEXT("ReturnValue"));R->CreateUserDefinedPin(TEXT("Native"),Out->PinType,EGPD_Input);
    return Link(Out,R->FindPin(TEXT("Native"))) && Compile(BP) && Save(BP);
}
bool Check(int32 Expected,bool Reconstruct)
{
    auto* Callee=LoadObject<UBlueprint>(nullptr,CalleePath);auto* BP=LoadObject<UBlueprint>(nullptr,CallerPath);
    if(!BP || !Callee)return false;
    if(Reconstruct)
    {
        TArray<UEdGraph*> Graphs;BP->GetAllGraphs(Graphs);
        for(UEdGraph* G:Graphs)for(UEdGraphNode* N:G->Nodes)if(auto* Call=Cast<UBlueprintTextCallFunction>(N))Call->ReconstructNode();
        if(!Compile(BP) || !Save(BP))return false;
    }
    if(!AssetText::CheckCallArguments(BP))return false;
    TStrongObjectPtr<UObject> Target(NewObject<UObject>(GetTransientPackage(),Callee->GeneratedClass));
    TStrongObjectPtr<UObject> Caller(NewObject<UObject>(GetTransientPackage(),BP->GeneratedClass));
    auto* TargetProperty=FindFProperty<FObjectPropertyBase>(BP->GeneratedClass,TEXT("Target"));
    auto* Probe=BP->GeneratedClass->FindFunctionByName(TEXT("Probe"));if(!TargetProperty || !Probe)return false;
    TargetProperty->SetObjectPropertyValue_InContainer(Caller.Get(),Target.Get());
    FStructOnScope Args(Probe);Caller->ProcessEvent(Probe,Args.GetStructMemory());
    auto Int=[&](FName Name,int32 Value){const auto* P=FindFProperty<FIntProperty>(Probe,Name);return P && P->GetPropertyValue_InContainer(Args.GetStructMemory())==Value;};
    auto Bool=[&](FName Name,bool Value){const auto* P=FindFProperty<FBoolProperty>(Probe,Name);return P && P->GetPropertyValue_InContainer(Args.GetStructMemory())==Value;};
    auto String=[&](FName Name,const FString& Value){const auto* P=FindFProperty<FStrProperty>(Probe,Name);return P && P->GetPropertyValue_InContainer(Args.GetStructMemory())==Value;};
    const auto* Float=FindFProperty<FFloatProperty>(Probe,TEXT("InheritedFloat"));
    const auto* Double=FindFProperty<FDoubleProperty>(Probe,TEXT("InheritedDouble"));
    const auto* Int64=FindFProperty<FInt64Property>(Probe,TEXT("InheritedInt64"));
    return Int(TEXT("Inherited"),Expected) && Int(TEXT("ExplicitEqual"),7) && Int(TEXT("ExplicitZero"),0) &&
        Int(TEXT("Local"),13) && Int(TEXT("Native"),12) && Int(TEXT("Required"),4) && Bool(TEXT("InheritedBool"),true) && Bool(TEXT("ExplicitFalse"),false) &&
        String(TEXT("InheritedString"),TEXT("alpha")) && String(TEXT("ExplicitEmpty"),TEXT("")) && String(TEXT("InheritedEmpty"),TEXT("")) &&
        Float && Float->GetPropertyValue_InContainer(Args.GetStructMemory())==TNumericLimits<float>::Max() &&
        Double && Double->GetPropertyValue_InContainer(Args.GetStructMemory())==17.250000001 &&
        Int64 && Int64->GetPropertyValue_InContainer(Args.GetStructMemory())==4294967297LL;
}
bool RejectCorruption()
{
    auto* BP=LoadObject<UBlueprint>(nullptr,CallerPath);if(!BP)return false;
    bool Stale=false,Missing=false;
    for(UEdGraph* G:BP->FunctionGraphs)for(UEdGraphNode* N:G->Nodes)
        if(auto* Call=Cast<UBlueprintTextCallFunction>(N))
        {
            FString Error;
            if(Call->GetFName()==TEXT("Inherited"))
            {
                Call->FindPin(TEXT("Value"))->DefaultValue=TEXT("123");
                if(Call->CheckArguments(Error) || !Error.Contains(TEXT("stale")))return false;
                Call->BPTArgumentOverrides=TEXT("{\"NotAnArgument\":\"1\"}");
                Stale=!Call->RefreshArguments(Error) && Error.Contains(TEXT("Unsupported"));
            }
            if(Call->GetFName()==TEXT("Required"))
            {
                Call->BPTArgumentOverrides=TEXT("{}");
                Missing=!Call->RefreshArguments(Error) && Error.Contains(TEXT("Missing required argument"));
            }
        }
    return Stale && Missing;
}
}
