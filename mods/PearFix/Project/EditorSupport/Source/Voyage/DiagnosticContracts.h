#pragma once
// Editor declarations only. No native module or dependency stub enters the package.
// Stock property identities and MessageContent field order: Steam build 25982825.
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/WorldSubsystem.h"
#include "DiagnosticContracts.generated.h"

UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API AVoyageFarmingSeedActor : public AActor { GENERATED_BODY() };
UCLASS(BlueprintType, Blueprintable)
class VOYAGE_API AVoyageDynamicStaticMeshActor : public AActor
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Item") class UVoyageItem* GetItem();
    virtual UVoyageItem* GetItem_Implementation() { return EditorTestItem; }
    UPROPERTY(EditAnywhere, Category="Editor fixture") UVoyageItem* EditorTestItem = nullptr;
};
UCLASS(BlueprintType)
class VOYAGE_API UVoyageItem : public UObject { GENERATED_BODY() };
UCLASS(BlueprintType)
class VOYAGE_API UVoyageSeedFruitInfo : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fruit") UVoyageItem* FruitItem = nullptr;
};
// Complete stock value struct, field order from build 25982825 mappings.
USTRUCT(BlueprintType)
struct VOYAGE_API FVoyageSeedFruitGrowInfo
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fruit") TSoftObjectPtr<UObject> ActorReference;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fruit") float GrowTimer = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fruit") UVoyageSeedFruitInfo* FruitReference = nullptr;
};
UCLASS(BlueprintType)
class VOYAGE_API UVoyageItemSeed : public UVoyageItem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category="Seed") float SpaceRequirement = 0;
};
UCLASS(BlueprintType)
class VOYAGE_API UVoyageOctreeComponent : public UActorComponent { GENERATED_BODY() };
// These three fields are byte-backed enums in the game. Their zero defaults are
// unused by the ungrouped notice; all 30 native fields retain their order.
UENUM(BlueprintType)
enum class EVoyageGroupTextValueSource : uint8 { None, Counter, Value };
USTRUCT(BlueprintType)
struct VOYAGE_API FMessageContent
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FString UniqueContentKey;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FString Group;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") int32 GroupSize = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FText Header;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FText Body;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FText Footer;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") EVoyageGroupTextValueSource GroupHeaderTextValue = EVoyageGroupTextValueSource::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") EVoyageGroupTextValueSource GroupBodyTextValue = EVoyageGroupTextValueSource::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") EVoyageGroupTextValueSource GroupFooterTextValue = EVoyageGroupTextValueSource::None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FText GroupHeaderText;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FText GroupBodyText;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FText GroupFooterText;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") bool bShowIconWhenGrouped = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") float duration = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") float Delay = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") bool bShowImage = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") TSubclassOf<UUserWidget> SubClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") TWeakObjectPtr<UObject> Image;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") bool bMatchImageSize = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") int32 DesiredImageWidth = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") int32 DesiredImageHeight = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") float CustomValue = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") int32 CustomCounter = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") UObject* ActionContext = nullptr;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") TMap<UObject*, FText> Actions;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") TArray<TSubclassOf<UUserWidget>> ExtensionSubClasses;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FText HeaderFormatting;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FText BodyFormatting;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Messages") FText FooterFormatting;
};
static_assert(sizeof(FMessageContent) == 360, "Stock MessageContent size changed");
UCLASS(BlueprintType)
class VOYAGE_API AVoyagePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category="Messages") int32 LootMessageListIndex = 0;
    UPROPERTY(BlueprintReadWrite, Category="Messages") TSubclassOf<UUserWidget> LootMessageSubClass;
    UFUNCTION(BlueprintCallable, Category="Messages")
    bool AddMessageBySlot(FMessageContent MessageContent, int32 Slot, float ThrottleTime) { return true; }
};
