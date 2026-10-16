#pragma once
// CALL-TYPE mirror. Steam25956314 / UE5.8, EXE
// 0DAC6826ED94666064D06463597487970B8397CA0C124B74764AD37E8705135A.
// Used only for exact Blueprint call signatures; the game supplies the class.
#include "GameFramework/PlayerController.h"
#include "VoyagePlayerController.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API AVoyagePlayerController : public APlayerController
{
    GENERATED_BODY()
};
