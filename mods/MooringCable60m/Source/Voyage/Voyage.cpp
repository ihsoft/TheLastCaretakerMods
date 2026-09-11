#include "ModulesConnectCable.h"
#include "Modules/ModuleManager.h"
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Voyage, "Voyage");
namespace { const FName CableRootName(TEXT("Root")); }
AUtilityCable::AUtilityCable()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(CableRootName);
}
