#pragma once
// CALL/FIELD mirror. Steam25956314 / UE5.8.
// Editor-only; the packaged item points to the shipping native class.
#include "GameFramework/Actor.h"
#include "VoyageInputAction.h"
#include "VoyageEquipmentActor.generated.h"

UENUM(BlueprintType)
enum class EVoyageEquipmentInterpStateType : uint8
{
    ADS = 0
};

USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageEquipmentInterpStateConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRequireHold = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bExclusiveState = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float InterpInSpeed = 4.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float InterpOutSpeed = 4.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float CameraInterpSpeed = 3.0f;
    UPROPERTY() TObjectPtr<UObject> InteractClass;
    UPROPERTY() TObjectPtr<UObject> EnterStateSound;
    UPROPERTY() TObjectPtr<UObject> ExitStateSound;
};

UCLASS(BlueprintType)
class VOYAGE_API AVoyageEquipmentActor : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY() TObjectPtr<UVoyageInputAction> ADSAction;
    UPROPERTY() bool bEquipped = false;
    UPROPERTY() TMap<EVoyageEquipmentInterpStateType,
        FVoyageEquipmentInterpStateConfig> InterpStates;
    UPROPERTY() FName SkeletonAttachSocket;

    UFUNCTION(BlueprintCallable, Category="Voyage Mirror")
    void SetInterpState(EVoyageEquipmentInterpStateType InType, bool bIsSet) {}

    UFUNCTION(BlueprintPure, Category="Voyage Mirror")
    float GetInterpStateAmount(EVoyageEquipmentInterpStateType InType) const
    {
        return 0.0f;
    }

    UFUNCTION(BlueprintPure, Category="Voyage Mirror")
    bool HasInterpState(EVoyageEquipmentInterpStateType InType) const
    {
        return false;
    }

};
