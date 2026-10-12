// EDITOR-ONLY IDENTITY MIRROR for Steam build 25827816 / UE5.8.
// EXE SHA256 C15F59073ADCE2730487A04A0A29D6D142B5066FD3EDCD4B82689A7A82DB1D33.
// Method: current reviewed mappings plus the stock WindTurbine Small component
// template for both classes, ModuleSocketIOData and EModuleSocketType.
// The shipped Voyage module owns the runtime implementation.  This mirror is
// only used by the Railgun editor generator to author a native socket
// component into the generated Blueprint.
// Contract evidence: stock BP_Module_WindTurbine_Small component templates in
// the current ignored asset cache. See ../../ARCHITECTURE.md.

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
