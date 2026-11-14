// Editor-only reflected identities inspected in Steam 25982825, UE 5.8.
// Implementations are supplied by the installed game. Never package this plugin.
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/WorldSubsystem.h"
#include "RuntimeContracts.generated.h"

UENUM(BlueprintType)
enum class EModuleResourceCategory : uint8 { Unknown, Electricity, Water, Gas, Data, Mooring, Fuel, Count };

// Only the default enumerator is needed; reflected storage remains a byte.
UENUM(BlueprintType)
enum class FModuleRuntimeRequirementType : uint8 { MustBeFilled = 0 };

USTRUCT(BlueprintType)
struct VOYAGE_API FModuleRuntimeRequirements
{
    GENERATED_BODY()
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) bool bEnabled = true;
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) FModuleRuntimeRequirementType RequirementType = FModuleRuntimeRequirementType::MustBeFilled;
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) bool bFilled = false;
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) TMap<FName, float> Requirements;
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) TMap<FName, float> RequirementsFills;
};

UCLASS(BlueprintType)
class VOYAGE_API AVoyageLocationVolume : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) TMap<FString, FModuleRuntimeRequirements> RuntimeRequirements;
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) TSet<AActor*> IncludeActors;
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) TSet<AActor*> ExcludeActors;
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) TSet<AActor*> Overlaps;
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) TSet<AActor*> OverlapsTagged;
};

class UModuleSocketComponent;
class UVoyageModuleComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVoyageModuleCompDelegate, UVoyageModuleComponent*, Module);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVoyageModuleRegisteredDelegate, UVoyageModuleComponent*, Module);

UCLASS()
class VOYAGE_API UVoyageModuleSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable, Category="ElectrifiedBuildings") FVoyageModuleRegisteredDelegate OnModuleRegistered;
    UPROPERTY(BlueprintAssignable, Category="ElectrifiedBuildings") FVoyageModuleRegisteredDelegate OnModuleUnregistered;
};

UCLASS(BlueprintType)
class VOYAGE_API UVoyageModuleComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(Category="ElectrifiedBuildings", BlueprintReadOnly) TMap<FName, FModuleRuntimeRequirements> RuntimeRequirements;
    UPROPERTY(BlueprintAssignable, Category="ElectrifiedBuildings") FVoyageModuleCompDelegate OnModuleGroupsChanged;
    UFUNCTION(Category="ElectrifiedBuildings", BlueprintCallable) void AddExternalSocket(UModuleSocketComponent* Socket) {}
    UFUNCTION(Category="ElectrifiedBuildings", BlueprintCallable) void RemoveExternalSocket(UModuleSocketComponent* Socket) {}
};

UENUM()
enum class EModuleSocketType : uint8
{
    ST_Any,
    ST_Input,
    ST_Output,
    ST_Max
};

UENUM()
enum class EModuleResourceType : uint8 { Electricity, Petrol, Oil, SaltWater, FreshWater, PhysicalEnergy, Data, Custom1, Custom2, Custom3, Methane, Oxygen, Steam, Worker, Unknown, Mooring, Diesel, CarbonDioxide, Count };

// Unauthored enum values below are identity-only; authored category/socket values use current mappings.
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
    UPROPERTY() float Input = 0.f;
    UPROPERTY() float Output = 0.f;
    UPROPERTY() float ItemBandwidthInKg = 0.f;
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

    // A distinct mirror default preserves the authored zero-valued ST_Any
    // override in unversioned data.
    UPROPERTY() EModuleSocketType Type = EModuleSocketType::ST_Max;
    UPROPERTY(Category="ModuleSocketIOData", BlueprintReadOnly) EModuleResourceType ResourceType = EModuleResourceType::Unknown;
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

UCLASS(BlueprintType)
class VOYAGE_API UModuleSocketComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(Category="ElectrifiedBuildings", BlueprintCallable) void SetSocketID(FName Name) {}

    // Preserve native property order for unversioned root-component serialization.
    UPROPERTY() float ItemTransferTimer = 0.0f;
    UPROPERTY(Category="ModuleSocketComponent", EditAnywhere) uint32 SocketID = 0;
    UPROPERTY() float LeakMultiplier = 0.0f;
    UPROPERTY(EditAnywhere, Category="ElectrifiedBuildings") bool bAddModuleRequirement = true;
    UPROPERTY() FModuleCustomRequirement CustomRequirement;
    UPROPERTY(Category="ModuleSocketComponent", EditAnywhere) bool bAutoInitialize = true;
    UPROPERTY(Category="ModuleSocketComponent", EditAnywhere, BlueprintReadOnly) FModuleSocketIOData Port;
    UPROPERTY() bool bAcceptsSignals = false;
    UPROPERTY(Category="ModuleSocketComponent", EditAnywhere) FName OwningModule;
    UPROPERTY() bool bStandalone = false;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ElectrifiedBuildings") bool bIsVirtual = false;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ElectrifiedBuildings") bool bIsVirtualGrouped = false;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="ElectrifiedBuildings") bool bIsVirtualShareToGroup = false;
    UPROPERTY(Category="ModuleSocketComponent", EditAnywhere) TSoftObjectPtr<UObject> ConnectSound;
    UPROPERTY(Category="ModuleSocketComponent", EditAnywhere) TSoftObjectPtr<UObject> DisconnectSound;
    UPROPERTY(Category="ModuleSocketComponent", EditAnywhere) TSoftObjectPtr<UObject> SwitchDirectionSound;
    UPROPERTY(Category="ModuleSocketComponent", BlueprintAssignable) FVoyageSocketConnectionDelegate OnSocketConnectionChanged;
    UPROPERTY() float ResourcesTransferredLastFrame = 0.0f;
    UPROPERTY() float ResourcesTransferredLastFrameAbsolute = 0.0f;
    UPROPERTY() EModuleSocketTransferType ResourcesTransferredLastFrameType = EModuleSocketTransferType::None;
    UPROPERTY() float ResourceTransferPerSecond = 0.0f;
    UPROPERTY(Category="ModuleSocketComponent", EditAnywhere) TObjectPtr<UObject> IntegratedCable;
    UPROPERTY(Category="ModuleSocketComponent", EditAnywhere) float IntegratedCableLength = 0.0f;
    UPROPERTY(Category="ModuleSocketComponent", BlueprintReadOnly) TObjectPtr<UObject> IntegratedCablePlaceholderMesh;
    UPROPERTY() TWeakObjectPtr<UObject> ConnectedToSocket;
    UPROPERTY(BlueprintReadOnly, Category="ElectrifiedBuildings") TWeakObjectPtr<UVoyageModuleComponent> ModuleOwner;
    UPROPERTY() TWeakObjectPtr<UObject> ModuleOwnerOriginal;
    UPROPERTY() TWeakObjectPtr<UObject> ConnectionCable;
    UPROPERTY(Category="ModuleSocketComponent", BlueprintReadOnly) TObjectPtr<UObject> SocketCollisionBox;
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
    UPROPERTY(Category="VoyageModuleSocketViewComponent", BlueprintReadOnly) TObjectPtr<UObject> Socket;
    UPROPERTY(Category="VoyageModuleSocketViewComponent", BlueprintReadOnly) TObjectPtr<UObject> mesh;
    UPROPERTY(Category="VoyageModuleSocketViewComponent", EditAnywhere) TSoftObjectPtr<UObject> DataAsset;
    UPROPERTY() bool bMultiplyScale = false;
};

// Mirrors the single native default subobject of VoyageVirtualActorSocket.
// This constructor is editor support only; the installed game supplies runtime behavior.
UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API AVoyageVirtualActorSocket : public AActor
{
    GENERATED_BODY()
public:
    AVoyageVirtualActorSocket()
    {
        VoyageModuleSocketView = CreateDefaultSubobject<UVoyageModuleSocketViewComponent>(TEXT("VoyageModuleSocketView"));
        RootComponent = VoyageModuleSocketView;
    }
    UPROPERTY(BlueprintReadOnly, Category="ElectrifiedBuildings") TObjectPtr<UVoyageModuleSocketViewComponent> VoyageModuleSocketView;
};

UCLASS()
class VOYAGE_API UVoyageMiscBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(Category="ElectrifiedBuildings", BlueprintCallable) static UVoyageModuleComponent* GetModuleFromActor(AActor* Actor) { return nullptr; }
    UFUNCTION(Category="ElectrifiedBuildings", BlueprintCallable) static bool RenameObject(UObject* Object, FString NewName, UObject* NewOuter) { return false; }
};
