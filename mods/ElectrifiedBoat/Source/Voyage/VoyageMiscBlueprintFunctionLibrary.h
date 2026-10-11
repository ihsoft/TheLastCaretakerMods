// EDITOR-ONLY IDENTITY MIRROR for Steam build 25191271 (UE 5.8).
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageMiscBlueprintFunctionLibrary.generated.h"

class UPrimitiveComponent;
class UVoyageModuleComponent;
class AActor;

UCLASS()
class VOYAGE_API UVoyageMiscBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure)
    static UPrimitiveComponent* GetRootPrimitiveComponent(
        UPrimitiveComponent* PrimitiveComponent,
        bool bIncludeWeldedParent)
    {
        return nullptr;
    }

    UFUNCTION(BlueprintCallable)
    static UVoyageModuleComponent* GetModuleFromActor(AActor* Actor)
    {
        return nullptr;
    }
};
