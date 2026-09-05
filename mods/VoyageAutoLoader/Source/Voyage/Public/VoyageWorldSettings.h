#pragma once

// Editor-only identity mirror, not a shipped native implementation.
// Voyage Steam 25056839 / UE5.8.1, executable SHA256
// CA84428CF4562C703BEDFF053DB727D14CC70C593451C09BE75A92828EFD9933.
// Hand-reconstructed from reviewed reflection/Empty-map inspection; inherited
// from the owned test14 generator. Evidence is in README, Current game API evidence.
// Revalidate class/property identities and map defaults after fingerprint change.

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "VoyageWorldSettings.generated.h"

UCLASS()
class VOYAGE_API AVoyageWorldSettings : public AWorldSettings
{
    GENERATED_BODY()

public:
    AVoyageWorldSettings(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voyage")
    bool bUseSaveGame = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voyage")
    bool bUseConnected = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voyage")
    FString ConnectedZone;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voyage")
    bool bSyncPlayer = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voyage")
    bool bReplaceDynamicPersistentActors = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voyage")
    bool bSpawnGameplayActors = true;
};
