// HAND-RECONSTRUCTED GAME API MIRROR: The Last Caretaker Steam build 23962331
// (Unreal Engine 5.7.4; VoyageSteam-Win64-Shipping.exe SHA-256
// 6A9AE86E5CE5D7D1B6555F579091AAB1E0E67FF7A96276FA2570052F99102E8D).
// Editor-only exact identity used by generated Blueprint component templates.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "InteractiveObjectComponent.generated.h"

UENUM()
enum class EHarpoonInteractiveTypeMirror : uint8
{
    Default
};

UENUM()
enum class EHarpoonInteractiveActionMirror : uint8
{
    Default
};

UENUM()
enum class EHarpoonInteractiveFilterMirror : uint8
{
    Default
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHarpoonInteractActionTriggeredMirror);

UCLASS(BlueprintType, ClassGroup = Interaction, meta = (BlueprintSpawnableComponent))
class VOYAGE_API UInteractiveObjectComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    // Exact 20-property runtime prefix. Enum members, object classes, and the
    // delegate signature are editor-only alignment placeholders because this
    // probe serializes none of these own fields. Their order keeps inherited
    // SceneComponent fields at the runtime unversioned property indices.
    UPROPERTY()
    EHarpoonInteractiveTypeMirror InteractType = EHarpoonInteractiveTypeMirror::Default;

    UPROPERTY()
    int32 PartId = 0;

    UPROPERTY()
    bool bButtonInfoActionRequirement = false;

    UPROPERTY()
    TArray<EHarpoonInteractiveActionMirror> ButtonInfoRequiredActions;

    UPROPERTY()
    bool bButtonInfoNotAllowedAction = false;

    UPROPERTY()
    TArray<EHarpoonInteractiveActionMirror> ButtonInfoNotAllowedActions;

    UPROPERTY()
    TObjectPtr<UObject> OverlayWidget;

    UPROPERTY()
    TObjectPtr<UObject> DialogWidget;

    UPROPERTY()
    TObjectPtr<UObject> FocusWidget;

    UPROPERTY()
    TObjectPtr<UObject> EquipmentFilterItem;

    UPROPERTY()
    TObjectPtr<UObject> HolsterFilterItem;

    UPROPERTY()
    float RequiredHoldTime = 0.0f;

    UPROPERTY()
    float Priority = 0.0f;

    UPROPERTY()
    EHarpoonInteractiveFilterMirror InteractFilter = EHarpoonInteractiveFilterMirror::Default;

    UPROPERTY()
    TObjectPtr<UObject> CrosshairWidget;

    UPROPERTY()
    TArray<int32> InteractIndexWhitelist;

    UPROPERTY()
    TArray<int32> InteractIndexBlacklist;

    UPROPERTY()
    TArray<TObjectPtr<UObject>> InteractIndexInputActions;

    UPROPERTY()
    bool bScanText = false;

    UPROPERTY()
    FHarpoonInteractActionTriggeredMirror OnInteractActionTriggered;
};
