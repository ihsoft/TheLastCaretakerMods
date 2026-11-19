#pragma once
#include "K2Node_CallFunction.h"
#include "Dom/JsonObject.h"

// Exchange-time argument resolution. Nothing here is serialized on a node.
namespace BlueprintCallArguments
{
constexpr TCHAR Requests[]=TEXT("callArguments");
constexpr TCHAR Overrides[]=TEXT("arguments");
constexpr TCHAR Defaults[]=TEXT("declarationDefaults");
constexpr TCHAR OptionalParameters[]=TEXT("BlueprintText.OptionalParameters");
constexpr TCHAR CppDefaultPrefix[]=TEXT("CPP_Default_");
constexpr TCHAR MathLibrary[]=TEXT("/Script/Engine.KismetMathLibrary");
bool IsArgument(const UEdGraphPin* Pin);
bool DeclarationDefault(const UFunction* Function,FName Name,FString& Value);
bool Apply(UK2Node_CallFunction* Call,const FJsonObject& Request,FString& Error);
TSharedRef<FJsonObject> Snapshot(UK2Node_CallFunction* Call);
}
