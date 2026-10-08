#pragma once

#include "EdGraph/EdGraph.h"
#include "RailgunRuntimeGeneratorFwd.h"

namespace Railgun::Runtime
{
class FGraph
{
public:
    UEdGraph* Graph;
    UEdGraphPin* Tail = nullptr;
    int32 X = 0;

    explicit FGraph(UEdGraph* InGraph);

    template<class T>
    T* Node(T* In)
    {
        In->CreateNewGuid();
        In->PostPlacedNewNode();
        In->AllocateDefaultPins();
        In->NodePosX = X;
        X += 180;
        Graph->AddNode(In, true, false);
        return In;
    }

    UEdGraphPin* Pin(UEdGraphNode* In, FName Name);
    void Link(UEdGraphPin* From, UEdGraphPin* To);
    void Default(UEdGraphNode* In, FName Name, const TCHAR* Value);
    UK2Node_CallFunction* Call(UClass* Owner, FName Function);
    UK2Node_CallArrayFunction* ArrayCall(FName Function);
    void Exec(UEdGraphNode* In);
    UEdGraphPin* Read(FName Name);
    void Write(FName Name, UEdGraphPin* Value, const TCHAR* Literal = nullptr);
    UK2Node_IfThenElse* Branch(UEdGraphPin* Condition);
    void Require(UEdGraphPin* Condition);
    UEdGraphPin* ActorArray(UEdGraphPin* FirstActor, UEdGraphPin* SecondActor);
    UEdGraphPin* Valid(UEdGraphPin* Object);
    UEdGraphPin* Compare(FName Function, UEdGraphPin* Value,
        const TCHAR* Other);
    UEdGraphPin* Binary(FName Function, UEdGraphPin* Left,
        UEdGraphPin* Right);
    UEdGraphPin* Transform(UEdGraphPin* Location, UEdGraphPin* Rotation);
    UEdGraphPin* Offset(UEdGraphPin* TransformValue, const TCHAR* Value);
};

void AddVariable(UBlueprint* BP, FName Name, FName Category,
    UObject* Type = nullptr);
void MarkVariableTransient(UBlueprint* BP, FName Name);
void AddArrayVariable(UBlueprint* BP, FName Name, FName Category,
    UObject* Type = nullptr);
}
