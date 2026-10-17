#pragma once
// CALL-TYPE mirror. Steam25956314 / UE5.8.
// Used only for exact Blueprint call signatures; the game supplies the class.
#include "GameFramework/PlayerController.h"
#include "VoyagePlayerController.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API AVoyagePlayerController : public APlayerController
{
    GENERATED_BODY()
};
