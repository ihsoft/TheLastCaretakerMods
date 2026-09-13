#pragma once
// Editor identity only. Steam25191271 / UE5.8, EXE747DC255...F58B.
// Current native registration: interface ctor1451fba80 -> registrar145216af0
// -> one name/thunk pair14996acd0 (GetHUDOverrideWidget/1452203e0).
// JetSki export: no inputs, ClassProperty<VoyageBaseUserWidget> return.
// GAME_DERIVED_SOURCES.md owns full fingerprint and revalidation requirements.
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "VoyageBaseUserWidget.h"
#include "VoyageActorWidgetInterface.generated.h"

UINTERFACE(BlueprintType)
class VOYAGE_API UVoyageActorWidgetInterface : public UInterface
{
    GENERATED_BODY()
};

class VOYAGE_API IVoyageActorWidgetInterface
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="StationHUD")
    TSubclassOf<UVoyageBaseUserWidget> GetHUDOverrideWidget();
};
