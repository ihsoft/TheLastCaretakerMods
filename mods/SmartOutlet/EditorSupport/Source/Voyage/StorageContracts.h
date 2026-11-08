// EDITOR-ONLY identity mirrors. Steam 25982825, UE 5.8.
// Reconstructed from reviewed mappings via Get-VoyageMappings/Inspect-VoyageAsset
// and bounded native registration/thunk inspection via the executable inspector.
// Compile Blueprint member access only; never serialize these native components
// or ship this module. Revalidate names, types and signatures after a game update.
#pragma once
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Subsystems/WorldSubsystem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/FileHelper.h"
#include "StorageContracts.generated.h"

UCLASS(BlueprintType)
class VOYAGE_API AVoyageModuleActor : public AActor
{
    GENERATED_BODY()
};

UENUM(BlueprintType)
enum class EModuleSocketType : uint8 { ST_Any, ST_Input, ST_Output, ST_Max };
UENUM(BlueprintType)
enum class EModuleResourceType : uint8
{
    Electricity, Petrol, Oil, SaltWater, FreshWater, PhysicalEnergy, Data,
    Custom1, Custom2, Custom3, Methane, Oxygen, Steam, Worker, Unknown,
    Mooring, Diesel, CarbonDioxide, Count
};
USTRUCT(BlueprintType)
struct VOYAGE_API FModuleSocketBandwidth
{
    GENERATED_BODY()
    UPROPERTY(Category="SmartOutlet", EditAnywhere, BlueprintReadWrite) float Input = -1;
    UPROPERTY(Category="SmartOutlet", EditAnywhere, BlueprintReadWrite) float Output = -1;
    UPROPERTY(Category="SmartOutlet", EditAnywhere, BlueprintReadWrite) float ItemBandwidthInKg = 0;
};
// Only the members addressed by Blueprint are declared. Other native members
// are preserved by Set Members in Struct on the live Port reference.
USTRUCT(BlueprintType)
struct VOYAGE_API FModuleSocketIOData
{
    GENERATED_BODY()
    UPROPERTY(Category="SmartOutlet", EditAnywhere, BlueprintReadWrite) EModuleSocketType Type = EModuleSocketType::ST_Any;
    UPROPERTY(Category="SmartOutlet", EditAnywhere, BlueprintReadWrite) EModuleResourceType ResourceType = EModuleResourceType::Electricity;
    UPROPERTY(Category="SmartOutlet", EditAnywhere, BlueprintReadWrite) FModuleSocketBandwidth Bandwidth;
};
UCLASS(BlueprintType)
class VOYAGE_API UModuleSocketComponent : public UBoxComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="SmartOutlet", BlueprintReadWrite) FModuleSocketIOData Port;
    UPROPERTY(Category="SmartOutlet", BlueprintReadWrite) bool bAutoInitialize = true;
    UFUNCTION(Category="SmartOutlet", BlueprintCallable) void SetSocketDirectionType(EModuleSocketType CustomType) {}
    UFUNCTION(Category="SmartOutlet", BlueprintPure) EModuleSocketType GetPlugType() const { return Port.Type; }
};
UCLASS(BlueprintType)
class VOYAGE_API UVoyageModuleSocketViewComponent : public UModuleSocketComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="SmartOutlet", BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> mesh;
};
class UVoyageModuleComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVoyageModuleCompDelegate, UVoyageModuleComponent*, Module);
UCLASS(BlueprintType)
class VOYAGE_API UVoyageModuleComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="SmartOutlet", BlueprintAssignable) FVoyageModuleCompDelegate OnModuleValueChanged;
    UPROPERTY(Category="SmartOutlet", BlueprintAssignable) FVoyageModuleCompDelegate OnModuleSocketConnectionChanged;
    // Test inputs exist only in the editor provider. Runtime Blueprints reference
    // the native getters above/below, never these validation properties.
    UPROPERTY(Category="EditorValidation", EditAnywhere) double EditorValidationAmount = 0;
    UPROPERTY(Category="EditorValidation", EditAnywhere) double EditorValidationCapacity = 6000;
    UPROPERTY(Category="EditorValidation", EditAnywhere) TMap<EModuleResourceType, double> EditorValidationAmounts;
    UPROPERTY(Category="EditorValidation", EditAnywhere) TMap<EModuleResourceType, double> EditorValidationCapacities;
    UPROPERTY(Category="EditorValidation", VisibleAnywhere) int32 EditorValidationNetworkRefreshes = 0;
    // Steam25982825: native callable, no parameters. Notifies reachable modules
    // through ConnectableModule.ConnectionsChanged after socket limits change.
    UFUNCTION(Category="SmartOutlet", BlueprintCallable) void BroadcastConnectionsChanged() { ++EditorValidationNetworkRefreshes; }
    UFUNCTION(Category="SmartOutlet", BlueprintPure) double GetResourceAmount(EModuleResourceType Type) const { const double* Value = EditorValidationAmounts.Find(Type); return Value ? *Value : EditorValidationAmount; }
    UFUNCTION(Category="SmartOutlet", BlueprintPure) double GetMaxResourceAmount(EModuleResourceType Type) const { const double* Value = EditorValidationCapacities.Find(Type); return Value ? *Value : EditorValidationCapacity; }
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoyagePersistentSubsystemActorAttached, AActor*, Child, USceneComponent*, ParentComponent);
UCLASS(BlueprintType)
class VOYAGE_API UVoyagePersistentSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="SmartOutlet", BlueprintAssignable) FVoyagePersistentSubsystemActorAttached OnActorAttached;
};

// Existing game API; file loading implementation and counter are editor-only.
UCLASS()
class VOYAGE_API UVoyageEditorBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    inline static int32 ReadCount = 0;
public:
    UFUNCTION(BlueprintCallable, Category="SmartOutlet")
    static TArray<FString> LoadFileToArray(FString InPath)
    {
        ++ReadCount;
        FString Text;
        TArray<FString> Lines;
        if (FFileHelper::LoadFileToString(Text, *InPath)) Text.ParseIntoArrayLines(Lines, false);
        return Lines;
    }
    UFUNCTION(BlueprintPure, Category="EditorValidation")
    static int32 GetTestReadCount() { return ReadCount; }
    UFUNCTION(BlueprintCallable, Category="EditorValidation")
    static UActorComponent* AddTestComponent(AActor* Owner, TSubclassOf<UActorComponent> ComponentClass)
    {
        UActorComponent* Component = NewObject<UActorComponent>(Owner, ComponentClass);
        Owner->AddInstanceComponent(Component);
        Component->RegisterComponent();
        return Component;
    }
};
