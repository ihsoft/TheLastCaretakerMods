// HAND-RECONSTRUCTED GAME API MIRROR: Steam 25191271 / UE5.8 parser target;
// Method: reviewed mapping plus the stock Diesel Generator Blueprint function
// contract. Revalidate on Steam build ID change. Editor-only: never package this.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PersistentInterface.generated.h"

UINTERFACE(BlueprintType)
class VOYAGE_API UPersistentInterface : public UInterface
{
    GENERATED_BODY()
};

class VOYAGE_API IPersistentInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
    void OnPersistentActorPostLoad(int32 Version);
};
