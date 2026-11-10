#include "NativeDefaultsFixture.h"
#include "Components/SceneComponent.h"
#include "Modules/ModuleManager.h"
IMPLEMENT_MODULE(FDefaultModuleImpl, BPTNativeDefaults)
UBPTNativeComponent::UBPTNativeComponent()
{
    Nested=CreateDefaultSubobject<UBPTNativeChild>(TEXT("Nested"));
}
ABPTNativeActor::ABPTNativeActor()
{
    ModuleComponent=CreateDefaultSubobject<UBPTNativeComponent>(TEXT("ModuleComponent"));
    OtherComponent=CreateDefaultSubobject<USceneComponent>(TEXT("OtherComponent"));
    ModuleComponent->Peer=OtherComponent;
    ModuleComponent->ConfigData.ResourceBandwidthInput=17;
    ModuleComponent->Nested->Value=23;
}
