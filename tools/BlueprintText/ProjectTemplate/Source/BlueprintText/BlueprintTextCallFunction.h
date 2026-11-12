#pragma once
#include "K2Node_CallFunction.h"
#include "BlueprintTextCallFunction.generated.h"

// Editor-only call authoring state. Compilation remains UK2Node_CallFunction's.
UCLASS()
class UBlueprintTextCallFunction : public UK2Node_CallFunction
{
    GENERATED_BODY()
public:
    // JSON object keyed by input pin name. Absence means declaration default.
    UPROPERTY()
    FString BPTArgumentOverrides = TEXT("{}");
    UPROPERTY()
    FString BPTDeclarationDefaults = TEXT("{}");

    static bool IsArgument(const UEdGraphPin* Pin);
    static bool DeclarationDefault(const UFunction* Function,FName Name,FString& Value);
    bool RefreshArguments(FString& Error);
    bool CheckArguments(FString& Error) const;
    virtual void ReconstructNode() override;
    virtual void PostReconstructNode() override;
    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
    virtual void ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const override;
private:
    bool bUpdatingArguments = false;
    bool Arguments(bool bWrite, FString& Error) const;
};
