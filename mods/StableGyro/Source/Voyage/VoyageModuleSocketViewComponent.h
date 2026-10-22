// ORDINAL-COMPLETE EDITOR-ONLY SERIALIZATION MIRROR: Steam build 25982825,
// UE 5.8. Property order and outer property types come from the reviewed Voyage
// mappings. Unused nested structs and one unused enum intentionally carry only
// identity/type placeholders. Authored values come from the current stock Gyro
// socket templates. This module is used only to cook Blueprint assets and is
// never shipped.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Engine/DataAsset.h"
#include "VoyageModuleSocketViewComponent.generated.h"

UENUM()
enum class EModuleSocketType : uint8
{
    ST_Any,
    ST_Input,
    ST_Output,
    ST_Max
};

UENUM()
enum class EModuleResourceType : uint8
{
    Electricity,
    Petrol,
    Oil,
    SaltWater,
    FreshWater,
    PhysicalEnergy,
    Data,
    Custom1,
    Custom2,
    Custom3,
    Methane,
    Oxygen,
    Steam,
    Worker,
    Unknown,
    Mooring,
    Diesel,
    CarbonDioxide,
    Count
};

UENUM()
enum class EModuleResourceCategory : uint8
{
    Unknown,
    Electricity,
    Water,
    Gas,
    Data,
    Mooring,
    Fuel,
    Count
};

// Only the reflected property kind and ordinal matter for this unused field.
UENUM()
enum class EModuleSocketTransferType : uint8
{
    None
};

USTRUCT()
struct VOYAGE_API FModuleCustomRequirement
{
    GENERATED_BODY()
};

USTRUCT()
struct VOYAGE_API FModuleSocketBandwidth
{
    GENERATED_BODY()
};

USTRUCT()
struct VOYAGE_API FVoyageConnectedSocketsCache
{
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct VOYAGE_API FModuleSocketIOData
{
    GENERATED_BODY()

    // The stock component overrides this to ST_Any; a different mirror CDO
    // default forces that explicit zero-valued override into unversioned data.
    UPROPERTY() EModuleSocketType Type = EModuleSocketType::ST_Max;
    UPROPERTY(BlueprintReadOnly) EModuleResourceType ResourceType = EModuleResourceType::Unknown;
    UPROPERTY() FModuleSocketBandwidth Bandwidth;
    UPROPERTY() bool bIgnoreCableBandwidth = false;
    UPROPERTY() EModuleSocketType DefaultDirection = EModuleSocketType::ST_Any;
    UPROPERTY() bool bBalanceResourceLevels = false;
    UPROPERTY() bool bMatchByResourceCategory = false;
    UPROPERTY() EModuleResourceCategory ResourceCategory = EModuleResourceCategory::Unknown;
    UPROPERTY() TSet<EModuleResourceType> AdditionalAcceptedResourceTypes;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FVoyageSocketConnectionDelegate,
    bool,
    bSocketConnection);

UCLASS()
class VOYAGE_API UModuleSocketComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure)
    bool IsSocketConnectedOrHasCable() const { return false; }

    // Exact current mapping order: all 36 own properties are represented.
    UPROPERTY() float ItemTransferTimer = 0.0f;
    UPROPERTY(EditAnywhere) uint32 SocketID = 0;
    UPROPERTY() float LeakMultiplier = 0.0f;
    UPROPERTY() bool bAddModuleRequirement = false;
    UPROPERTY() FModuleCustomRequirement CustomRequirement;
    UPROPERTY(EditAnywhere) bool bAutoInitialize = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FModuleSocketIOData Port;
    UPROPERTY() bool bAcceptsSignals = false;
    UPROPERTY(EditAnywhere) FName OwningModule;
    UPROPERTY() bool bStandalone = false;
    UPROPERTY() bool bIsVirtual = false;
    UPROPERTY() bool bIsVirtualGrouped = false;
    UPROPERTY() bool bIsVirtualShareToGroup = false;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UObject> ConnectSound;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UObject> DisconnectSound;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UObject> SwitchDirectionSound;
    UPROPERTY(BlueprintAssignable) FVoyageSocketConnectionDelegate OnSocketConnectionChanged;
    UPROPERTY() float ResourcesTransferredLastFrame = 0.0f;
    UPROPERTY() float ResourcesTransferredLastFrameAbsolute = 0.0f;
    UPROPERTY() EModuleSocketTransferType ResourcesTransferredLastFrameType = EModuleSocketTransferType::None;
    UPROPERTY() float ResourceTransferPerSecond = 0.0f;
    UPROPERTY(EditAnywhere) TObjectPtr<UObject> IntegratedCable;
    UPROPERTY(EditAnywhere) float IntegratedCableLength = 0.0f;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UObject> IntegratedCablePlaceholderMesh;
    UPROPERTY() TWeakObjectPtr<UObject> ConnectedToSocket;
    UPROPERTY() TWeakObjectPtr<UObject> ModuleOwner;
    UPROPERTY() TWeakObjectPtr<UObject> ModuleOwnerOriginal;
    UPROPERTY() TWeakObjectPtr<UObject> ConnectionCable;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UObject> SocketCollisionBox;
    UPROPERTY() TObjectPtr<UObject> NiagaraFX;
    UPROPERTY() TObjectPtr<UObject> NiagaraFXLeak;
    UPROPERTY() TSoftObjectPtr<UObject> NiagaraFXLeakSystem;
    UPROPERTY() TSoftObjectPtr<UObject> NiagaraFXSystem;
    UPROPERTY() FVoyageConnectedSocketsCache ConnectedSocketsCache[3];
};

UCLASS(ClassGroup = (Voyage), meta = (BlueprintSpawnableComponent))
class VOYAGE_API UVoyageModuleSocketViewComponent : public UModuleSocketComponent
{
    GENERATED_BODY()

public:
    // Exact current mapping order: all four own properties are represented.
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UObject> Socket;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<UObject> mesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UObject> DataAsset;
    UPROPERTY() bool bMultiplyScale = false;
};

// Identity-only source asset used to author the hard stock Hook Cable import.
// The generated stub package is excluded from the cooked production set.
UCLASS()
class VOYAGE_API UVoyageItemCable : public UPrimaryDataAsset
{
    GENERATED_BODY()
};
