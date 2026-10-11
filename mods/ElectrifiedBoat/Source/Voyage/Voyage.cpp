#include "Voyage.h"

#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"
#include "VoyageLevelInstanceComponent.h"

namespace
{
class FVoyageEditorMirrorModule final : public FDefaultGameModuleImpl
{
public:
    virtual void StartupModule() override
    {
        ExposeVoyageLevelInstanceOwnedActorsToBlueprint();
    }
};
}

void ExposeVoyageLevelInstanceOwnedActorsToBlueprint()
{
    FArrayProperty* OwnedActors = FindFProperty<FArrayProperty>(
        UVoyageLevelInstanceComponent::StaticClass(),
        GET_MEMBER_NAME_CHECKED(
            UVoyageLevelInstanceComponent,
            OwnedActors));
    check(OwnedActors);
    check(CastField<FWeakObjectProperty>(OwnedActors->Inner));
    OwnedActors->SetPropertyFlags(
        CPF_BlueprintVisible | CPF_BlueprintReadOnly);
}

IMPLEMENT_PRIMARY_GAME_MODULE(FVoyageEditorMirrorModule, Voyage, "Voyage");
