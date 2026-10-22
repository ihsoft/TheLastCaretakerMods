// HAND-RECONSTRUCTED GAME API MIRROR: Steam build 25982825, UE 5.8.2.
// The reflected owner, parameter order/types and native thunk were revalidated
// from the current executable. Editor-only; never ship.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoyageModuleSocketViewComponent.h"
#include "VoyageMiscBlueprintFunctionLibrary.generated.h"

UCLASS()
class VOYAGE_API UVoyageMiscBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    static UObject* GetClassDefaultObject(TSubclassOf<UObject> Class)
    {
        return Class ? Class->GetDefaultObject() : nullptr;
    }

    UFUNCTION(BlueprintCallable)
    static void DisconnectCableFromSocket(
        UModuleSocketComponent* Socket,
        bool bDestroy)
    {
    }
};
