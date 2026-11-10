#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "NativeDefaultsFixture.generated.h"

USTRUCT(BlueprintType)
struct FBPTNativeConfig
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fixture") bool bAutoStartModule=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fixture") double ResourceBandwidthInput=3.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fixture") int32 MaxResourceAmount=5;
};
UCLASS(DefaultToInstanced, EditInlineNew)
class BPTNATIVEDEFAULTS_API UBPTNativeChild : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Fixture") int32 Value=7;
};
UCLASS()
class BPTNATIVEDEFAULTS_API UBPTNativeComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UBPTNativeComponent();
    UPROPERTY(EditAnywhere, Category="Fixture") TObjectPtr<UObject> ItemAsset;
    UPROPERTY(EditAnywhere, Category="Fixture") FBPTNativeConfig ConfigData;
    UPROPERTY(VisibleAnywhere, Instanced, Category="Fixture") TObjectPtr<UBPTNativeChild> Nested;
    UPROPERTY(EditAnywhere, Category="Fixture") TObjectPtr<UActorComponent> Peer;
};
UCLASS(Blueprintable)
class BPTNATIVEDEFAULTS_API ABPTNativeActor : public AActor
{
    GENERATED_BODY()
public:
    ABPTNativeActor();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fixture") TObjectPtr<UBPTNativeComponent> ModuleComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fixture") TObjectPtr<UActorComponent> OtherComponent;
};
