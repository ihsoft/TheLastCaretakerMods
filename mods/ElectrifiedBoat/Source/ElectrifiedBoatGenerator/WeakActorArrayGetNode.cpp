#include "WeakActorArrayGetNode.h"

#include "EdGraphSchema_K2.h"
#include "GameFramework/Actor.h"
#include "K2Node_GetArrayItem.h"
#include "KismetCompiler.h"

namespace
{
const FName WeakActorArrayPin(TEXT("WeakActorArray"));
const FName ArrayIndexPin(TEXT("Index"));
const FName ActorItemPin(TEXT("Actor"));
}

void UWeakActorArrayGetNode::AllocateDefaultPins()
{
    FEdGraphPinType ArrayType;
    ArrayType.PinCategory = UEdGraphSchema_K2::PC_Object;
    ArrayType.PinSubCategoryObject = AActor::StaticClass();
    ArrayType.ContainerType = EPinContainerType::Array;
    ArrayType.bIsWeakPointer = true;
    CreatePin(EGPD_Input, ArrayType, WeakActorArrayPin);

    FEdGraphPinType IndexType;
    IndexType.PinCategory = UEdGraphSchema_K2::PC_Int;
    CreatePin(EGPD_Input, IndexType, ArrayIndexPin);

    FEdGraphPinType ActorType;
    ActorType.PinCategory = UEdGraphSchema_K2::PC_Object;
    ActorType.PinSubCategoryObject = AActor::StaticClass();
    CreatePin(EGPD_Output, ActorType, ActorItemPin);
}

void UWeakActorArrayGetNode::ExpandNode(
    FKismetCompilerContext& CompilerContext,
    UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    UK2Node_GetArrayItem* GetByReference =
        CompilerContext.SpawnIntermediateNode<UK2Node_GetArrayItem>(
            this,
            SourceGraph);
    GetByReference->AllocateDefaultPins();

    CompilerContext.MovePinLinksToIntermediate(
        *GetArrayPin(),
        *GetByReference->GetTargetArrayPin());
    GetByReference->NotifyPinConnectionListChanged(
        GetByReference->GetTargetArrayPin());
    checkf(GetByReference->GetResultPin()->PinType.bIsReference,
        TEXT("Transient standard array getter did not preserve by-reference output"));
    CompilerContext.MovePinLinksToIntermediate(
        *GetIndexPin(),
        *GetByReference->GetIndexPin());
    CompilerContext.MovePinLinksToIntermediate(
        *GetItemPin(),
        *GetByReference->GetResultPin());
    BreakAllNodeLinks();
}

UEdGraphPin* UWeakActorArrayGetNode::GetArrayPin() const
{
    return FindPinChecked(WeakActorArrayPin);
}

UEdGraphPin* UWeakActorArrayGetNode::GetIndexPin() const
{
    return FindPinChecked(ArrayIndexPin);
}

UEdGraphPin* UWeakActorArrayGetNode::GetItemPin() const
{
    return FindPinChecked(ActorItemPin);
}
