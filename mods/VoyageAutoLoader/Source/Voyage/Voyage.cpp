#include "Voyage.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Voyage, "Voyage");

AVoyageWorldSettings::AVoyageWorldSettings(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}
