// HAND-WRITTEN EDITOR SCAFFOLD: registers the minimal Voyage mirror used only
// while generating cooked Blueprint assets. Native output is never shipped.

#include "Voyage.h"
#include "Modules/ModuleManager.h"
#include "VoyageVehiclePawn.h"

TArray<FPlayerInputInterfaceAction> AVoyageVehiclePawn::GetProvidedActionsBP_Implementation()
{
    return {};
}

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Voyage, "Voyage");
