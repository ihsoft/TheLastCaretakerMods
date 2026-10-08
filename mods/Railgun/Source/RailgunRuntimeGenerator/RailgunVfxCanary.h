#pragma once

#include "AssetLoadingGraphNames.h"

// Temporary isolated visual canaries. These do not trace, fire, consume
// resources, apply damage, or determine a water surface.
namespace RailgunVfxCanary
{
inline const FName ExplosionSystem(TEXT("RailgunCanaryExplosionSystem"));
inline const FName SplashSystem(TEXT("RailgunCanarySplashSystem"));
inline const FName ExplosionComponent(TEXT("RailgunCanaryExplosionComponent"));
inline const FName SplashComponent(TEXT("RailgunCanarySplashComponent"));
inline constexpr TCHAR TestDistanceCentimeters[] = TEXT("2000.0");
inline constexpr TCHAR MaximumLifetimeSeconds[] = TEXT("10.0");

inline const FName SystemTemplate(TEXT("SystemTemplate"));
inline const FName WorldContextObject(TEXT("WorldContextObject"));
inline const FName SpawnLocation(TEXT("Location"));
inline const FName AutoDestroy(TEXT("bAutoDestroy"));
inline const FName AutoActivate(TEXT("bAutoActivate"));
inline const FName PreCullCheck(TEXT("bPreCullCheck"));
inline const FName Reset(TEXT("bReset"));
inline const FName Duration(TEXT("Duration"));
inline const FName DestroyObject(TEXT("Object"));
}

void DestroyVfxCanaryComponent(FGraph& G, UEdGraphPin* Component)
{
    auto* Valid = G.Branch(G.Valid(Component));
    auto* Destroy = G.Call(UActorComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UActorComponent, K2_DestroyComponent));
    G.Link(Component, G.Pin(Destroy, P::FunctionTarget));
    G.Link(Component, G.Pin(Destroy, RailgunVfxCanary::DestroyObject));
    G.Exec(Destroy);
    StationMerge(G, {G.Tail, G.Pin(Valid, P::Else)});
}

UEdGraphPin* LoadVfxCanarySystem(FGraph& G, const TCHAR* ObjectPath,
    FName StorageField)
{
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath));
    G.Default(Path, E::PathString, ObjectPath);
    auto* Reference = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftObjPathToSoftObjRef));
    G.Link(G.Pin(Path, P::ReturnValue),
        G.Pin(Reference, AssetLoadingGraphNames::SoftObjectPath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadAsset_Blocking));
    G.Link(G.Pin(Reference, P::ReturnValue), G.Pin(Load, AssetLoadingGraphNames::Asset));
    G.Exec(Load);
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph);
    Cast->TargetType = UNiagaraSystem::StaticClass();
    Cast->SetPurity(false);
    G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute));
    G.Link(G.Pin(Load, P::ReturnValue), Cast->GetCastSourcePin());
    G.Tail = Cast->GetValidCastPin();
    G.Write(StorageField, Cast->GetCastResultPin());
    return G.Read(StorageField);
}

void AddRailgunVfxCanary(FGraph& G, UEdGraphPin* StationSelf,
    const TCHAR* InputPath, const TCHAR* SystemPath, FName SystemField,
    FName ComponentField, bool DirtMainOnly)
{
    auto* Action = LoadObject<UInputAction>(nullptr, InputPath);
    check(Action);
    auto* Event = NewObject<UK2Node_EnhancedInputAction>(G.Graph);
    Event->InputAction = Action;
    G.Node(Event);
    G.Tail = G.Pin(Event, DS::Started);
    G.Branch(ObserveCall(G, APawn::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(APawn, IsPlayerControlled), StationSelf));
    G.Branch(G.Read(DS::ViewOwned));
    G.Branch(G.Valid(G.Read(O::Camera)));
    DestroyVfxCanaryComponent(G, G.Read(ComponentField));

    auto* ForwardScale = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_VectorFloat));
    G.Link(ObserveCall(G, USceneComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(USceneComponent, GetForwardVector), G.Read(O::Camera)),
        G.Pin(ForwardScale, P::Binary::LeftOperand));
    G.Default(ForwardScale, P::Binary::RightOperand,
        RailgunVfxCanary::TestDistanceCentimeters);
    UEdGraphPin* TestLocation = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector),
        ObserveCall(G, USceneComponent::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation),
            G.Read(O::Camera)),
        G.Pin(ForwardScale, P::ReturnValue));

    UEdGraphPin* System = LoadVfxCanarySystem(
        G, SystemPath, SystemField);
    auto* Spawn = G.Call(UNiagaraFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UNiagaraFunctionLibrary, SpawnSystemAtLocation));
    G.Link(StationSelf, G.Pin(Spawn, RailgunVfxCanary::WorldContextObject));
    G.Link(System, G.Pin(Spawn, RailgunVfxCanary::SystemTemplate));
    G.Link(TestLocation, G.Pin(Spawn, RailgunVfxCanary::SpawnLocation));
    G.Default(Spawn, RailgunVfxCanary::AutoDestroy, RailgunVfx::True);
    G.Default(Spawn, RailgunVfxCanary::AutoActivate, RailgunVfx::False);
    G.Default(Spawn, RailgunVfxCanary::PreCullCheck, RailgunVfx::False);
    G.Exec(Spawn);
    UEdGraphPin* Component = G.Pin(Spawn, P::ReturnValue);
    auto* Spawned = G.Branch(G.Valid(Component));
    G.Tail = G.Pin(Spawned, P::Then);
    G.Write(ComponentField, Component);
    ConfigureRailgunSeaMineEmitters(G, Component, DirtMainOnly);
    auto* Activate = G.Call(UActorComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UActorComponent, Activate));
    G.Link(Component, G.Pin(Activate, P::FunctionTarget));
    G.Default(Activate, RailgunVfxCanary::Reset, RailgunVfx::True);
    G.Exec(Activate);
    auto* Delay = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Delay));
    G.Default(Delay, RailgunVfxCanary::Duration,
        RailgunVfxCanary::MaximumLifetimeSeconds);
    G.Exec(Delay);
    DestroyVfxCanaryComponent(G, Component);
}

void AddRailgunVfxCanaries(FGraph& G, UEdGraphPin* StationSelf)
{
    AddRailgunVfxCanary(G, StationSelf, RailgunInputNames::ExplosionCanary,
        RailgunVfx::SeaMinePath, RailgunVfxCanary::ExplosionSystem,
        RailgunVfxCanary::ExplosionComponent, false);
    AddRailgunVfxCanary(G, StationSelf, RailgunInputNames::SplashCanary,
        RailgunVfx::SeaMinePath, RailgunVfxCanary::SplashSystem,
        RailgunVfxCanary::SplashComponent, true);

    auto* EndPlay = NewObject<UK2Node_Event>(G.Graph);
    EndPlay->EventReference.SetExternalMember(
        ActorLifecycleGraphNames::EndPlayEvent, AActor::StaticClass());
    EndPlay->bOverrideFunction = true;
    G.Node(EndPlay);
    G.Tail = G.Pin(EndPlay, P::Then);
    DestroyVfxCanaryComponent(G,
        G.Read(RailgunVfxCanary::ExplosionComponent));
    DestroyVfxCanaryComponent(G,
        G.Read(RailgunVfxCanary::SplashComponent));
}
