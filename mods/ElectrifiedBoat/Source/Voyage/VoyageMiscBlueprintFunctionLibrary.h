// EDITOR-ONLY IDENTITY MIRROR for Steam build 25880952 (UE 5.8).
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageMiscBlueprintFunctionLibrary.generated.h"

class UPrimitiveComponent;
class USceneComponent;
class UVoyageModuleComponent;
class AActor;

UCLASS()
class VOYAGE_API UVoyageMiscBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    static UPrimitiveComponent* GetRootPrimitiveComponent(
        USceneComponent* Primitive,
        bool IgnoreAdjustGrabBoxes)
    {
        return nullptr;
    }

    UFUNCTION(BlueprintCallable)
    static UVoyageModuleComponent* GetModuleFromActor(AActor* Actor)
    {
        return nullptr;
    }
};
