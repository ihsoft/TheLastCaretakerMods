#pragma once
// EDITOR-ONLY identity for the native hover delegate. Steam25982825.
// The native scene component owns interaction acquisition; no native defaults
// or implementation are serialized or shipped by SmartOutlet.
#include "Components/SceneComponent.h"
#include "InteractiveObjectComponent.generated.h"
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class VOYAGE_API UInteractiveObjectComponent : public USceneComponent
{
    GENERATED_BODY()
};
