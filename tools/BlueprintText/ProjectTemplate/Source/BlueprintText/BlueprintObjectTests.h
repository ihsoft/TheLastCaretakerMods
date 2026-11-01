// Focused editor-only fixtures for schema-2 UObject source integration.
#pragma once
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_CallFunction.h"
#include "K2Node_GenericCreateObject.h"
#include "K2Node_Self.h"
#include "UObject/StrongObjectPtr.h"

namespace BlueprintObjectTests
{
constexpr TCHAR FixtureMode[]=TEXT("ObjectFixture");
constexpr TCHAR ConsumerMode[]=TEXT("ObjectConsumer");
constexpr TCHAR CheckMode[]=TEXT("ObjectCheck");
constexpr TCHAR SourcePackage[]=TEXT("/Game/BPTObjectSource");
constexpr TCHAR ValueName[]=TEXT("Value");
constexpr TCHAR LiteralName[]=TEXT("RootComponent");
constexpr TCHAR MapName[]=TEXT("Values");
constexpr TCHAR ReadName[]=TEXT("ReadValue");
constexpr TCHAR ResultName[]=TEXT("ReturnValue");
constexpr TCHAR SelfReadName[]=TEXT("ReadViaSelf");
constexpr TCHAR LiteralValue[]=TEXT("/Game/BPTObjectSource.BPTObjectSource_C");
constexpr TCHAR MapDefault[]=TEXT("((\"Setting\", \"0.124\"))");
constexpr TCHAR SourceSuffix[]=TEXT(".BPTObjectSource");
constexpr TCHAR ConsumerPackage[]=TEXT("/Game/BPTObjectConsumer");
constexpr TCHAR StoredName[]=TEXT("Stored");
constexpr TCHAR MakeName[]=TEXT("MakeObject");
constexpr TCHAR ClassPin[]=TEXT("Class");
constexpr TCHAR ConsumerObject[]=TEXT("/Game/BPTObjectConsumer.BPTObjectConsumer");
template<class T> T* Node(UEdGraph* Graph)
{
    auto* Result=NewObject<T>(Graph);Graph->AddNode(Result);Result->CreateNewGuid();return Result;
}
bool Link(UEdGraphPin* A,UEdGraphPin* B)
{
    if(A && B && GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(A,B))return true;
    UE_LOG(LogTemp,Error,TEXT("Object fixture connection failed: %s.%s -> %s.%s"),
        A?*GetNameSafe(A->GetOwningNode()):TEXT("None"),A?*A->PinName.ToString():TEXT("None"),
        B?*GetNameSafe(B->GetOwningNode()):TEXT("None"),B?*B->PinName.ToString():TEXT("None"));
    return false;
}
UEdGraph* Function(UBlueprint* BP,FName Name)
{
    auto* Graph=FBlueprintEditorUtils::CreateNewGraph(BP,Name,UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP,Graph,false,static_cast<UFunction*>(nullptr));return Graph;
}
UK2Node_FunctionEntry* Entry(UEdGraph* Graph)
{
    for(UEdGraphNode* N:Graph->Nodes)if(auto* E=Cast<UK2Node_FunctionEntry>(N))return E;return nullptr;
}
FEdGraphPinType RealType()
{
    FEdGraphPinType Type;Type.PinCategory=UEdGraphSchema_K2::PC_Real;Type.PinSubCategory=UEdGraphSchema_K2::PC_Double;return Type;
}
bool Compile(UBlueprint* BP)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);return BP->Status!=BS_Error && BP->GeneratedClass;
}
UBlueprint* Create(const FString& Path,UClass* Parent)
{
    if(FPackageName::DoesPackageExist(Path))return nullptr;
    return FKismetEditorUtilities::CreateBlueprint(Parent,CreatePackage(*Path),*FPackageName::GetLongPackageAssetName(Path),BPTYPE_Normal);
}
bool Save(UBlueprint* BP)
{
    const FString Path=FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;return UPackage::SavePackage(BP->GetOutermost(),BP,*Path,Args);
}
bool Fixture(const FString& Output)
{
    auto* BP=Create(SourcePackage,UObject::StaticClass());if(!BP)return false;
    FEdGraphPinType StringType;StringType.PinCategory=UEdGraphSchema_K2::PC_String;
    FEdGraphPinType MapType=StringType;MapType.ContainerType=EPinContainerType::Map;
    MapType.PinValueType.TerminalCategory=UEdGraphSchema_K2::PC_String;
    if(!FBlueprintEditorUtils::AddMemberVariable(BP,ValueName,RealType()) ||
       !FBlueprintEditorUtils::AddMemberVariable(BP,LiteralName,StringType) ||
       !FBlueprintEditorUtils::AddMemberVariable(BP,MapName,MapType))return false;
    auto* Graph=Function(BP,ReadName);auto* Start=Entry(Graph);if(!Start)return false;
    auto* Result=Node<UK2Node_FunctionResult>(Graph);Result->AllocateDefaultPins();
    Result->CreateUserDefinedPin(ResultName,RealType(),EGPD_Input);
    auto* Get=Node<UK2Node_VariableGet>(Graph);Get->VariableReference.SetSelfMember(ValueName);Get->AllocateDefaultPins();
    if(!Link(Start->FindPin(UEdGraphSchema_K2::PN_Then),Result->FindPin(UEdGraphSchema_K2::PN_Execute)) ||
       !Link(Get->FindPin(ValueName),Result->FindPin(ResultName)) || !Compile(BP))return false;
    // An explicit self-class reference exercises relocation, separate from bSelfContext.
    Graph=Function(BP,SelfReadName);Start=Entry(Graph);
    Result=Node<UK2Node_FunctionResult>(Graph);Result->AllocateDefaultPins();
    Result->CreateUserDefinedPin(ResultName,RealType(),EGPD_Input);
    auto* Call=Node<UK2Node_CallFunction>(Graph);Call->SetFromFunction(BP->GeneratedClass->FindFunctionByName(ReadName));Call->AllocateDefaultPins();
    if(!Link(Start->FindPin(UEdGraphSchema_K2::PN_Then),Call->FindPin(UEdGraphSchema_K2::PN_Execute)) ||
       !Link(Call->FindPin(UEdGraphSchema_K2::PN_Then),Result->FindPin(UEdGraphSchema_K2::PN_Execute)) ||
       !Link(Call->FindPin(ResultName),Result->FindPin(ResultName)) || !Compile(BP))return false;
    UObject* CDO=BP->GeneratedClass->GetDefaultObject();
    auto* Value=FindFProperty<FDoubleProperty>(BP->GeneratedClass,ValueName);
    auto* Literal=FindFProperty<FStrProperty>(BP->GeneratedClass,LiteralName);
    auto* Map=FindFProperty<FMapProperty>(BP->GeneratedClass,MapName);
    if(!Value||!Literal||!Map)return false;
    Value->SetPropertyValue_InContainer(CDO,12.5);
    Literal->SetPropertyValue_InContainer(CDO,LiteralValue);
    if(!Map->ImportText_Direct(MapDefault,Map->ContainerPtrToValuePtr<void>(CDO),CDO,PPF_None))return false;
    return Save(BP) && AssetText::Export(BP->GetOutermost()->GetName(),Output);
}
bool Consumer(const FString& Asset,const FString& Output)
{
    auto* Source=LoadObject<UBlueprint>(nullptr,*(Asset+SourceSuffix));if(!Source)return false;
    auto* BP=Create(ConsumerPackage,AActor::StaticClass());if(!BP)return false;
    FEdGraphPinType ObjectType;ObjectType.PinCategory=UEdGraphSchema_K2::PC_Object;ObjectType.PinSubCategoryObject=Source->GeneratedClass;
    if(!FBlueprintEditorUtils::AddMemberVariable(BP,StoredName,ObjectType) || !Compile(BP))return false;
    auto* Graph=Function(BP,MakeName);auto* Start=Entry(Graph);
    auto* Make=Node<UK2Node_GenericCreateObject>(Graph);Make->AllocateDefaultPins();
    GetDefault<UEdGraphSchema_K2>()->TrySetDefaultObject(*Make->FindPin(ClassPin),Source->GeneratedClass);
    auto* Self=Node<UK2Node_Self>(Graph);Self->AllocateDefaultPins();
    auto* Set=Node<UK2Node_VariableSet>(Graph);Set->VariableReference.SetSelfMember(StoredName);Set->AllocateDefaultPins();
    if(!Link(Start->FindPin(UEdGraphSchema_K2::PN_Then),Make->FindPin(UEdGraphSchema_K2::PN_Execute)) ||
       !Link(Self->FindPin(UEdGraphSchema_K2::PN_Self),Make->GetOuterPin()) ||
       !Link(Make->FindPin(UEdGraphSchema_K2::PN_Then),Set->FindPin(UEdGraphSchema_K2::PN_Execute)) ||
       !Link(Make->FindPin(UEdGraphSchema_K2::PN_ReturnValue),Set->FindPin(StoredName)) || !Compile(BP))return false;
    return Save(BP) && AssetText::Export(BP->GetOutermost()->GetName(),Output);
}
bool Check(const FString& Asset)
{
    auto* Source=LoadObject<UBlueprint>(nullptr,*(Asset+SourceSuffix));
    auto* Consumer=LoadObject<UBlueprint>(nullptr,ConsumerObject);
    if(!Source || !Consumer || Source->SimpleConstructionScript)return false;
    auto* Stored=FindFProperty<FObjectProperty>(Consumer->GeneratedClass,StoredName);
    auto* Value=FindFProperty<FDoubleProperty>(Source->GeneratedClass,ValueName);
    auto* Literal=FindFProperty<FStrProperty>(Source->GeneratedClass,LiteralName);
    auto* Map=FindFProperty<FMapProperty>(Source->GeneratedClass,MapName);
    auto* Read=Source->GeneratedClass->FindFunctionByName(SelfReadName);
    auto* Make=Consumer->GeneratedClass->FindFunctionByName(MakeName);
    if(!Stored||!Value||!Literal||!Map||!Read||!Make)return false;
    UObject* Owner=Consumer->GeneratedClass->GetDefaultObject();Owner->ProcessEvent(Make,nullptr);
    TStrongObjectPtr<UObject> First(Stored->GetObjectPropertyValue_InContainer(Owner));
    Owner->ProcessEvent(Make,nullptr);UObject* Second=Stored->GetObjectPropertyValue_InContainer(Owner);
    if(!First.IsValid()||!Second||First.Get()==Second)return false;
    Value->SetPropertyValue_InContainer(First.Get(),42.0);
    double A=0,B=0;First->ProcessEvent(Read,&A);Second->ProcessEvent(Read,&B);
    if(A!=42.0||B!=12.5 || Literal->GetPropertyValue_InContainer(Second)!=LiteralValue)return false;
    FScriptMapHelper Values(Map,Map->ContainerPtrToValuePtr<void>(Second));
    if(Values.Num()!=1)return false;
    TWeakObjectPtr<UObject> Retained=Second;CollectGarbage(RF_NoFlags);
    if(!Retained.IsValid())return false;
    B=0;Retained->ProcessEvent(Read,&B);
    UE_LOG(LogTemp,Display,TEXT("BPT UObject checks: construct, functions, defaults, map, instance isolation and retained reference after GC."));
    return B==12.5;
}
}
