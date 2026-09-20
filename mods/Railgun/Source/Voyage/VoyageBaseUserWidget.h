#pragma once
// Identity-only editor stub, NOT a reconstruction of the 47 native properties.
// Steam25191271 / UE5.8 / EXE747DC255...F58B; full owning provenance registry.
// Permitted ONLY as the interface return's type identity. NEVER instantiate,
// access/serialize native fields using this stub. No UNVERSIONED child cook.
// HC18candidate01 proved even a zero-own-property static child serializes BAD
// native CDO fields. HC26 isolates TAGGED cooking with no native-field deltas;
// header flag + independent cooked JSON must pass before a disposable test.
// This exception does not establish runtime lifecycle or a complete schema.
#include "Blueprint/UserWidget.h"
#include "VoyageBaseUserWidget.generated.h"

UCLASS(Abstract, BlueprintType)
class VOYAGE_API UVoyageBaseUserWidget : public UUserWidget
{
    GENERATED_BODY()
};
