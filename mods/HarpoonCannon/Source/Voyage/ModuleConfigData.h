#pragma once
// Editor-only, tagged partial mirror. Steam25191271 / UE5.8, EXE
// 747DC2553F7E68D8EA7ED0B2E0CAC6D08943EA3F50DD6ED822E9293E0B45F58B.
// Public ModuleConfigData mappings and stock battery/refinery CDOs.
// Revalidate with the game fingerprint; never ship native mirrors.
#include "CoreMinimal.h"
#include "ModuleResourceType.h"
#include "ModuleConfigData.generated.h"
UENUM()
enum class EVoyageModuleType : uint8 { Passive, Active };
USTRUCT()
struct VOYAGE_API FModuleConfigData
{
    GENERATED_BODY()
    UPROPERTY() EModuleResourceType ModuleResourceType = EModuleResourceType::Electricity;
    UPROPERTY() bool bAutoStartModule = false;
    UPROPERTY() EVoyageModuleType ModuleType = EVoyageModuleType::Passive;
    UPROPERTY() bool bAcceptResourceOffer = false;
    UPROPERTY() bool bAcceptResourceOfferOff = false;
    UPROPERTY() bool bAcceptResourceOfferProduction = false;
    UPROPERTY() double ResourceBandwidthInput = 0;
    UPROPERTY() double DefaultResourceAmount = 0;
    UPROPERTY() double MaxResourceAmount = 0;
    UPROPERTY() double ResourceConsumptionStandby = 0;
    UPROPERTY() double ResourceConsumptionOn = 0;
};
