#pragma once
// Editor-only mirrors; provenance is in ARCHITECTURE.md, Editor dependencies and provenance.
// Implementations below serve editor tests only; no native module is shipped.
#include "Components/SceneComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/FileHelper.h"
#include "Engine/DataAsset.h"
#include "InventoryContracts.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryDelegate);

// Steam25982825: stock PlayerBox icon bytecode and native mapping contract.
UCLASS(BlueprintType)
class VOYAGE_API UVoyageIconPaletteData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="ContainerLidLabel", EditAnywhere, BlueprintReadOnly)
    TArray<TSoftObjectPtr<UObject>> IconPalette;
};

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class VOYAGE_API UVoyageBaseInventoryComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="ContainerLidLabel", EditAnywhere, BlueprintReadOnly) FText Name;
    UPROPERTY(Category="ContainerLidLabel", EditAnywhere, BlueprintReadOnly) int32 PrimaryIconIndex = 0;
    UPROPERTY(Category="ContainerLidLabel", EditAnywhere, BlueprintReadOnly) int32 SecondaryIconIndex = 0;
    UPROPERTY(Category="ContainerLidLabel", EditAnywhere, BlueprintReadOnly) UObject* IconPaletteData = nullptr;
    UPROPERTY(Category="ContainerLidLabel", BlueprintAssignable) FInventoryDelegate OnInventoryNameUpdated;
    UPROPERTY(Category="ContainerLidLabel", BlueprintAssignable) FInventoryDelegate OnInventoryIconChanged;
    // Test-only emitters. Runtime Blueprint assets must never call these.
    UFUNCTION(Category="ContainerLidLabel", BlueprintCallable) void TestRename(FText Value) { Name = Value; OnInventoryNameUpdated.Broadcast(); }
    UFUNCTION(Category="ContainerLidLabel", BlueprintCallable) void TestIcons(int32 Left, int32 Right) { PrimaryIconIndex = Left; SecondaryIconIndex = Right; OnInventoryIconChanged.Broadcast(); }
    UFUNCTION(Category="Editor Tests", BlueprintPure) bool TestHasPresentationBindings() const
    { return OnInventoryNameUpdated.IsBound() || OnInventoryIconChanged.IsBound(); }
};

UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class VOYAGE_API UVoyageInventoryWeightLimitedComponent : public UVoyageBaseInventoryComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="ContainerLidLabel", EditAnywhere, BlueprintReadOnly) float MaxWeightLimit = 0.0f;
    UFUNCTION(Category="ContainerLidLabel", BlueprintCallable) void SetMaxWeightLimit(float NewDefaultMax) { MaxWeightLimit = NewDefaultMax; }
    UFUNCTION(Category="ContainerLidLabel", BlueprintPure) float GetPercentWeightUsed() const { return 0.0f; }
};

UCLASS()
class VOYAGE_API UVoyageEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(Category="ContainerLidLabel", BlueprintCallable) static TArray<FString> LoadFileToArray(FString InPath)
    {
        FString Text;
        TArray<FString> Lines;
        if (FFileHelper::LoadFileToString(Text, *InPath)) Text.ParseIntoArrayLines(Lines, false);
        return Lines;
    }
};
