// EDITOR-ONLY IDENTITY MIRROR for Steam build 25191271 (UE 5.8).
// Runtime implementation is supplied by Voyage; never ship this module.

#pragma once

#include "Components/ActorComponent.h"
#include "VoyageModuleComponent.generated.h"

class UModuleSocketComponent;

UCLASS(BlueprintType, ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageModuleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly)
    TArray<TObjectPtr<UModuleSocketComponent>> Sockets;

    UFUNCTION(BlueprintPure)
    static UVoyageModuleComponent* GetMasterModuleFromActor(AActor* Actor)
    {
        return nullptr;
    }

    UFUNCTION(BlueprintPure)
    int32 GetSecondaryGroupId(FName GroupName) const
    {
        return 0;
    }

    UFUNCTION(BlueprintCallable)
    void UpdateSecondaryGroup(FName GroupName, int32 GroupId) {}

    UFUNCTION(BlueprintCallable)
    void RemoveSecondaryGroup(FName GroupName) {}

    UFUNCTION(BlueprintCallable)
    void AddExternalSocket(UModuleSocketComponent* Socket) {}

    UFUNCTION(BlueprintCallable)
    void RemoveExternalSocket(UModuleSocketComponent* Socket) {}
};
