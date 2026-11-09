#pragma once
// EDITOR ONLY: Steam25982825 reflected native function registration.
// No runtime implementation or native component serialization is shipped.
#include "Components/SceneComponent.h"
#include "InteractiveObjectComponent.h"
#include "InteractiveDetectorPointerComponent.generated.h"
// Signature-only mirror: the mod never reads or serializes this struct.
USTRUCT(BlueprintType)
struct VOYAGE_API FVoyagePointerActionArguments { GENERATED_BODY() };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FInteractiveDetectionPointerPreviewDelegate,
    UInteractiveObjectComponent*, InteractiveObjectComponent, bool, bTextChangedOnly,
    const TArray<FVoyagePointerActionArguments>&, ActiveActions);
UCLASS(BlueprintType)
class VOYAGE_API UInteractiveDetectorPointerComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    // Steam25982825: delegate descriptor 0x149AC6810; native hover thunk 0x14524DEB0.
    UPROPERTY(BlueprintAssignable, Category="Interaction")
    FInteractiveDetectionPointerPreviewDelegate OnHoverChanged;
    UFUNCTION(BlueprintCallable, Category="Interaction")
    USceneComponent* GetHoverReference() { return EditorHoverReference; }
    UPROPERTY() TObjectPtr<USceneComponent> EditorHoverReference;
};
