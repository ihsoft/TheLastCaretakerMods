#pragma once

#include "K2Node.h"
#include "WeakActorArrayGetNode.generated.h"

UCLASS()
class ELECTRIFIEDBOATGENERATOR_API UWeakActorArrayGetNode final
    : public UK2Node
{
    GENERATED_BODY()

public:
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void ExpandNode(
        class FKismetCompilerContext& CompilerContext,
        UEdGraph* SourceGraph) override;

    UEdGraphPin* GetArrayPin() const;
    UEdGraphPin* GetIndexPin() const;
    UEdGraphPin* GetItemPin() const;
};
