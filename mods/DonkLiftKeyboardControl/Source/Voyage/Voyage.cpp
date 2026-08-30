// HAND-WRITTEN EDITOR SCAFFOLD: supplies the mirror's required native default
// and module registration. It is used only while generating Blueprint assets.

#include "Voyage.h"
#include "Modules/ModuleManager.h"
#include "VoyageVehiclePawn.h"

TArray<FPlayerInputInterfaceAction> AVoyageVehiclePawn::GetProvidedActionsBP_Implementation()
{
    return {};
}

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Voyage, "Voyage");
