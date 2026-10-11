// EDITOR-ONLY IDENTITY MIRROR for Steam build 25191271 (UE 5.8).
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Components/BoxComponent.h"
#include "ModuleSocketComponent.generated.h"

class UVoyageModuleComponent;

// Identity-only shell. Runtime reflection supplies the current native layout;
// the generator copies the complete value and never constructs field defaults.
USTRUCT(BlueprintType)
struct VOYAGE_API FModuleSocketIOData
{
    GENERATED_BODY()
};

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UModuleSocketComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void SetSocketID(FName Name) {}

    UPROPERTY(BlueprintReadWrite)
    FModuleSocketIOData Port;

    UPROPERTY(BlueprintReadWrite)
    bool bAutoInitialize = false;

    UPROPERTY(BlueprintReadWrite)
    bool bAddModuleRequirement = false;

    UPROPERTY(BlueprintReadWrite)
    bool bIsVirtual = false;

    UPROPERTY(BlueprintReadWrite)
    bool bIsVirtualGrouped = false;

    UPROPERTY(BlueprintReadWrite)
    bool bIsVirtualShareToGroup = false;

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<UVoyageModuleComponent> ModuleOwner;
};
