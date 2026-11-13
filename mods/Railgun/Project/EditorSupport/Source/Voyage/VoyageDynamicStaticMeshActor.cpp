#include "VoyageDynamicStaticMeshActor.h"

namespace
{
const FName MeshComponentName(TEXT("MeshComponent"));
}

AVoyageDynamicStaticMeshActor::AVoyageDynamicStaticMeshActor()
{
    UVoyageFastSceneComponent* NativeCarrier =
        CreateDefaultSubobject<UVoyageFastSceneComponent>(
        MeshComponentName);
    SetRootComponent(NativeCarrier);
}
