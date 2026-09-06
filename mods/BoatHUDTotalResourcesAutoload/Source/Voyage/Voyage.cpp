// HAND-WRITTEN EDITOR SCAFFOLD: never shipped.

#include "Voyage.h"
#include "VoyageModuleComponent.h"
#include "VoyageModuleSubsystem.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Voyage, "Voyage");

double UVoyageModuleComponent::GetResourceAmount(EModuleResourceType Type) const
{
    return 0.0;
}

void UVoyageModuleSubsystem::GetModulesInSameGrid(
    UVoyageModuleComponent* Module,
    TArray<UVoyageModuleComponent*>& OutModules)
{
    OutModules.Reset();
}
