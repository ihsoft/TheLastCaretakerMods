#pragma once

#include "EdGraph/EdGraph.h"

class UBlueprint;
class UK2Node_CallArrayFunction;
class UK2Node_CallFunction;
class UK2Node_IfThenElse;
class UK2Node_VariableSet;

namespace ElectrifiedBoat
{
class FSocketGraph
{
public:
    explicit FSocketGraph(UEdGraph* InGraph);

    template<class T>
    T* Node(T* InNode, int32 Y = 0)
    {
        InNode->CreateNewGuid();
        InNode->PostPlacedNewNode();
        InNode->SetFlags(RF_Transactional);
        InNode->AllocateDefaultPins();
        InNode->NodePosX = X;
        InNode->NodePosY = Y;
        X += 220;
        Graph->AddNode(InNode, true, false);
        return InNode;
    }

    UEdGraphPin* Pin(UEdGraphNode* InNode, FName Name) const;
    void Link(UEdGraphPin* From, UEdGraphPin* To) const;
    void Default(UEdGraphNode* InNode, FName Name, const FString& Value) const;
    void DefaultObject(UEdGraphNode* InNode, FName Name, UObject* Value) const;
    UK2Node_CallFunction* Call(UClass* Owner, FName FunctionName, int32 Y = 0);
    UK2Node_CallArrayFunction* ArrayCall(FName FunctionName, int32 Y = 0);
    void Exec(UEdGraphNode* InNode);
    UEdGraphPin* Read(FName Name, int32 Y = 0);
    UK2Node_VariableSet* Write(
        FName Name,
        UEdGraphPin* Value,
        const FString& Literal,
        int32 Y = 0);
    UEdGraphPin* ReadExternal(
        FName Name,
        UClass* Owner,
        UEdGraphPin* Object,
        int32 Y = 0);
    UK2Node_VariableSet* WriteExternal(
        FName Name,
        UClass* Owner,
        UEdGraphPin* Object,
        UEdGraphPin* Value,
        const FString& Literal,
        int32 Y = 0);
    UK2Node_IfThenElse* Branch(UEdGraphPin* Condition, int32 Y = 0);
    UEdGraphPin* Valid(UEdGraphPin* Object, int32 Y = 0);

    UEdGraph* Graph;
    UEdGraphPin* Tail = nullptr;
    int32 X = 0;
};

void AddTransientObjectArrayVariable(
    UBlueprint* Blueprint,
    FName Name,
    UClass* ObjectClass);
void AddTransientVariable(
    UBlueprint* Blueprint,
    FName Name,
    const FEdGraphPinType& Type);
}
