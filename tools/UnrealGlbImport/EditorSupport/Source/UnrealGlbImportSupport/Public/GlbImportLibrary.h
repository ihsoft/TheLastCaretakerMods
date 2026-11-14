#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GlbImportLibrary.generated.h"

// Stock advanced-copy's explicit map overload is not exposed to Python.
// The Python operation owns dependency selection; Unreal owns reference remapping.
UCLASS()
class UNREALGLBIMPORTSUPPORT_API UGlbImportLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="GLB Import")
    static bool CopyMaterialDependencies(const TMap<FString, FString>& Packages);
};
