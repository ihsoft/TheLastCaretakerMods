#pragma once
#include "K2Node_CallFunction.h"
#include "BlueprintTextCallFunction.generated.h"

// Legacy load compatibility only. New imports save ordinary UK2Node_CallFunction nodes.
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
private:
    bool bUpdatingArguments = false;
    bool Arguments(bool bWrite, FString& Error) const;
};
