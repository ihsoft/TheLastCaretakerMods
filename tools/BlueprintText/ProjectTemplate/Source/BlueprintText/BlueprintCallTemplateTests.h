// Isolated source fixture and independent post-reopen assertions.
#pragma once
#include "Components/BoxComponent.h"
#include "UObject/StructOnScope.h"

namespace BlueprintCallTemplateTests
{
constexpr TCHAR FixtureMode[]=TEXT("CallTemplateFixture");
constexpr TCHAR CheckMode[]=TEXT("CallTemplateCheck");
constexpr TCHAR Path[]=TEXT("/Game/BPTCallTemplateFixture");
constexpr TCHAR Probe[]=TEXT("Probe");
constexpr TCHAR TargetBox[]=TEXT("TargetBox");
constexpr TCHAR Parent[]=TEXT("ParentComponent");
constexpr TCHAR Children[]=TEXT("Children");
constexpr TCHAR GetChildren[]=TEXT("GetChildrenComponents");
constexpr TCHAR RelativeTransform[]=TEXT("RelativeTransform");
constexpr TCHAR Weld[]=TEXT("bWeldSimulatedBodies");
constexpr TCHAR UpdateOverlaps[]=TEXT("bUpdateOverlaps");
constexpr TCHAR FalseValue[]=TEXT("false");
constexpr TCHAR AttachNode[]=TEXT("Attach");
constexpr TCHAR ExtentNode[]=TEXT("Extent");
constexpr TCHAR AddNode[]=TEXT("Add");
constexpr TCHAR ChildrenNode[]=TEXT("ChildrenCall");
constexpr TCHAR SetClassProbe[]=TEXT("SetClassProbe");
constexpr TCHAR SetClassNode[]=TEXT("SetClass");
constexpr TCHAR SetClassFunction[]=TEXT("SetClassPropertyByName");
constexpr TCHAR ObjectPin[]=TEXT("Object");
constexpr TCHAR PropertyNamePin[]=TEXT("PropertyName");
constexpr TCHAR ValuePin[]=TEXT("Value");
constexpr TCHAR TargetPath[]=TEXT("/Game/BPTClassPropertyTarget");
constexpr TCHAR StoredClass[]=TEXT("StoredClass");
constexpr TCHAR MissingClass[]=TEXT("MissingClass");

bool Fixture()
{
    using namespace BlueprintObjectTests;
    auto* BP=Create(Path,AActor::StaticClass());if(!BP)return false;
    auto* G=Function(BP,Probe);auto* E=Entry(G);
    FEdGraphPinType Box;Box.PinCategory=UEdGraphSchema_K2::PC_Object;Box.PinSubCategoryObject=UBoxComponent::StaticClass();
    FEdGraphPinType Scene=Box;Scene.PinSubCategoryObject=USceneComponent::StaticClass();
    E->CreateUserDefinedPin(TargetBox,Box,EGPD_Output);E->CreateUserDefinedPin(Parent,Scene,EGPD_Output);
    auto* Get=Node<UK2Node_CallFunction>(G);Get->SetFromFunction(USceneComponent::StaticClass()->FindFunctionByName(GetChildren));Get->AllocateDefaultPins();
    auto* R=Node<UK2Node_FunctionResult>(G);R->AllocateDefaultPins();Scene.ContainerType=EPinContainerType::Array;
    R->CreateUserDefinedPin(Children,Scene,EGPD_Input);
    if(!Link(E->FindPin(UEdGraphSchema_K2::PN_Then),R->FindPin(UEdGraphSchema_K2::PN_Execute)) ||
       !Link(E->FindPin(TargetBox),Get->FindPin(UEdGraphSchema_K2::PN_Self)) ||
       !Link(Get->FindPin(Children),R->FindPin(Children)))return false;
    G=Function(BP,SetClassProbe);E=Entry(G);
    FEdGraphPinType ObjectType;ObjectType.PinCategory=UEdGraphSchema_K2::PC_Object;ObjectType.PinSubCategoryObject=UObject::StaticClass();
    FEdGraphPinType ClassType=ObjectType;ClassType.PinCategory=UEdGraphSchema_K2::PC_Class;
    FEdGraphPinType NameType;NameType.PinCategory=UEdGraphSchema_K2::PC_Name;
    E->CreateUserDefinedPin(ObjectPin,ObjectType,EGPD_Output);
    E->CreateUserDefinedPin(PropertyNamePin,NameType,EGPD_Output);
    E->CreateUserDefinedPin(ValuePin,ClassType,EGPD_Output);
    R=Node<UK2Node_FunctionResult>(G);R->AllocateDefaultPins();
    if(!Link(E->FindPin(UEdGraphSchema_K2::PN_Then),R->FindPin(UEdGraphSchema_K2::PN_Execute)))return false;
    auto* Target=Create(TargetPath,UObject::StaticClass());if(!Target)return false;
    ClassType.PinSubCategoryObject=USceneComponent::StaticClass();
    return FBlueprintEditorUtils::AddMemberVariable(Target,StoredClass,ClassType) && Compile(Target) && Save(Target) &&
        Compile(BP) && BlueprintComponentDefaultsTests::Save(BP);
}

bool CheckClassWrite(UBlueprint* BP)
{
    UEdGraph* Graph=nullptr;for(UEdGraph* G:BP->FunctionGraphs)if(G->GetFName()==SetClassProbe)Graph=G;
    UK2Node_CallFunction* Call=nullptr;
    if(Graph)for(UEdGraphNode* N:Graph->Nodes)if(N->GetFName()==SetClassNode)Call=Cast<UK2Node_CallFunction>(N);
    if(!Call || Call->FunctionReference.GetMemberName()!=SetClassFunction || Call->IsNodePure())return false;
    auto* Object=Call->FindPin(ObjectPin);auto* Name=Call->FindPin(PropertyNamePin);auto* Value=Call->FindPin(ValuePin);
    if(!Object || !Name || !Value || Object->PinType.PinCategory!=UEdGraphSchema_K2::PC_Object ||
       Name->PinType.PinCategory!=UEdGraphSchema_K2::PC_Name || Value->PinType.PinCategory!=UEdGraphSchema_K2::PC_Class ||
       Value->PinType.PinSubCategoryObject!=UObject::StaticClass() || Object->LinkedTo.Num()!=1 ||
       Name->LinkedTo.Num()!=1 || Value->LinkedTo.Num()!=1)return false;
    auto* Target=LoadObject<UBlueprint>(nullptr,TargetPath);
    auto* Function=BP->GeneratedClass->FindFunctionByName(SetClassProbe);
    if(!Target || !Function)return false;
    auto* Property=FindFProperty<FClassProperty>(Target->GeneratedClass,StoredClass);
    auto* ObjectArg=FindFProperty<FObjectPropertyBase>(Function,ObjectPin);
    auto* NameArg=FindFProperty<FNameProperty>(Function,PropertyNamePin);
    auto* ValueArg=FindFProperty<FClassProperty>(Function,ValuePin);
    if(!Property || Property->MetaClass!=USceneComponent::StaticClass() || !ObjectArg || !NameArg || !ValueArg)return false;
    TStrongObjectPtr<UObject> Instance(NewObject<UObject>(GetTransientPackage(),Target->GeneratedClass));
    TStrongObjectPtr<UObject> Untouched(NewObject<UObject>(GetTransientPackage(),Target->GeneratedClass));
    if(Property->GetObjectPropertyValue_InContainer(Instance.Get()))return false;
    auto Invoke=[&](FName NameValue,UClass* ClassValue)
    {
        FStructOnScope Args(Function);auto* Memory=Args.GetStructMemory();
        ObjectArg->SetObjectPropertyValue_InContainer(Memory,Instance.Get());
        NameArg->SetPropertyValue_InContainer(Memory,NameValue);
        ValueArg->SetObjectPropertyValue_InContainer(Memory,ClassValue);
        BP->GeneratedClass->GetDefaultObject()->ProcessEvent(Function,Memory);
    };
    Invoke(StoredClass,UBoxComponent::StaticClass());
    if(Property->GetObjectPropertyValue_InContainer(Instance.Get())!=UBoxComponent::StaticClass())return false;
    Invoke(MissingClass,USceneComponent::StaticClass());
    Invoke(StoredClass,AActor::StaticClass());
    if(Property->GetObjectPropertyValue_InContainer(Instance.Get())!=UBoxComponent::StaticClass() ||
       Property->GetObjectPropertyValue_InContainer(Untouched.Get()) ||
       Property->GetObjectPropertyValue_InContainer(Target->GeneratedClass->GetDefaultObject()))return false;
    Invoke(StoredClass,nullptr);
    if(Property->GetObjectPropertyValue_InContainer(Instance.Get()))return false;
    UE_LOG(LogTemp,Display,TEXT("Class setter graph executed: compatible write, missing/incompatible no-op, null clear and instance isolation passed."));
    return true;
}

bool Check()
{
    auto* BP=LoadObject<UBlueprint>(nullptr,Path);if(!BP)return false;
    UEdGraph* G=nullptr;for(UEdGraph* Graph:BP->FunctionGraphs)if(Graph->GetFName()==Probe)G=Graph;
    if(!G)return false;
    TMap<FName,UK2Node_CallFunction*> Calls;
    for(UEdGraphNode* Node:G->Nodes)if(auto* Call=Cast<UK2Node_CallFunction>(Node))Calls.Add(Node->GetFName(),Call);
    auto* Attach=Calls.FindRef(AttachNode);auto* Extent=Calls.FindRef(ExtentNode);
    auto* Add=Calls.FindRef(AddNode);auto* Child=Calls.FindRef(ChildrenNode);
    if(!Attach || !Extent || !Add || !Child)return false;
    auto* WeldPin=Attach->FindPin(Weld);auto* OverlapsPin=Extent->FindPin(UpdateOverlaps);
    auto* TransformPin=Add->FindPin(RelativeTransform);
    if(!WeldPin || !OverlapsPin || !TransformPin ||
       !WeldPin->DefaultValue.Equals(FalseValue,ESearchCase::IgnoreCase) ||
       !OverlapsPin->DefaultValue.Equals(FalseValue,ESearchCase::IgnoreCase) ||
       WeldPin->AutogeneratedDefaultValue.IsEmpty() || OverlapsPin->AutogeneratedDefaultValue.IsEmpty() ||
       !Child->IsNodePure() || Child->FindPin(UEdGraphSchema_K2::PN_Execute) || Child->FindPin(UEdGraphSchema_K2::PN_Then) ||
       !TransformPin->PinType.bIsReference || !TransformPin->PinType.bIsConst || TransformPin->LinkedTo.Num()!=1)
    {UE_LOG(LogTemp,Error,TEXT("Call template reopen changed explicit false, pure pins or required transform input."));return false;}
    return CheckClassWrite(BP);
}
}
