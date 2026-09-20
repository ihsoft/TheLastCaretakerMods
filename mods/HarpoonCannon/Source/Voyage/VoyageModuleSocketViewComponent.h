// EDITOR-ONLY IDENTITY MIRROR for Steam build 25191271 / UE5.8.
// EXE SHA256 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Method: Inspect-VoyageAsset mappings for both classes, ModuleSocketIOData
// and EModuleSocketType; revalidate all on fingerprint change.
// The shipped Voyage module owns the runtime implementation.  This mirror is
// only used by the HarpoonCannon editor generator to author a native socket
// component into the generated Blueprint.
// Contract evidence: stock BP_Module_WindTurbine_Small and
// BP_Module_Turret component templates; see the ignored asset-cache evidence
// and the mod's GAME_DERIVED_SOURCES.md revalidation gate.

#pragma once

#include "Components/BoxComponent.h"
#include "VoyageModuleSocketViewComponent.generated.h"

UENUM()
enum class EModuleSocketType : uint8 { ST_Any, ST_Input, ST_Output, ST_Max };

// Only the named field written by this tagged shell is mirrored.
USTRUCT()
struct VOYAGE_API FModuleSocketIOData
{
    GENERATED_BODY()
    UPROPERTY()
    EModuleSocketType DefaultDirection = EModuleSocketType::ST_Any;
};

// Exact declaring owner and ancestry from current reviewed mappings.
// Partial fields require TAGGED cooking; unversioned cooking is unsupported.
UCLASS()
class VOYAGE_API UModuleSocketComponent : public UBoxComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    uint32 SocketID = 0;
    UPROPERTY(EditAnywhere)
    bool bAutoInitialize = true;
    UPROPERTY()
    FModuleSocketIOData Port;
};

UCLASS(ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageModuleSocketViewComponent : public UModuleSocketComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    TSoftObjectPtr<UObject> DataAsset;
};
