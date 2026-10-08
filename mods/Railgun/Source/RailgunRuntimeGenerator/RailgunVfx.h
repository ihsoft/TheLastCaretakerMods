#pragma once

bool SaveDedicatedAsset(UObject* Asset);

// Shared stock-system contract for production impact VFX and manual canaries.
namespace RailgunVfx
{
inline constexpr TCHAR SeaMinePath[] =
    TEXT("/Game/VFX/Environment/Interactive/NS_Explosion_SeaMine.NS_Explosion_SeaMine");

inline const FName ShockwaveEmitter(TEXT("shockwave"));
inline const FName MainEmitter(TEXT("main"));
inline const FName SparkEmitter(TEXT("spark"));
inline const FName LargeSparkEmitter(TEXT("spark_l"));
inline const FName RefractionEmitter(TEXT("refr"));
inline const FName ProjectileEmitter(TEXT("project"));
inline const FName PuffEmitter(TEXT("puff"));
inline const FName DirtMainEmitter(TEXT("dirt_main"));

inline const FName EmitterName(TEXT("EmitterName"));
inline const FName EmitterEnabled(TEXT("bNewEnableState"));
inline constexpr TCHAR True[] = TEXT("true");
inline constexpr TCHAR False[] = TEXT("false");
}

void SetRailgunVfxEmitterEnabled(FGraph& G, UEdGraphPin* Component,
    FName EmitterName, bool Enabled)
{
    auto* Set = G.Call(UFXSystemComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UFXSystemComponent, SetEmitterEnable));
    G.Link(Component, G.Pin(Set, P::FunctionTarget));
    G.Default(Set, RailgunVfx::EmitterName, *EmitterName.ToString());
    G.Default(Set, RailgunVfx::EmitterEnabled,
        Enabled ? RailgunVfx::True : RailgunVfx::False);
    G.Exec(Set);
}

void ConfigureRailgunSeaMineEmitters(FGraph& G, UEdGraphPin* Component,
    bool DirtMainOnly)
{
    const bool MainEffectsEnabled = !DirtMainOnly;
    SetRailgunVfxEmitterEnabled(G, Component,
        RailgunVfx::ShockwaveEmitter, MainEffectsEnabled);
    SetRailgunVfxEmitterEnabled(G, Component,
        RailgunVfx::MainEmitter, MainEffectsEnabled);
    SetRailgunVfxEmitterEnabled(G, Component,
        RailgunVfx::SparkEmitter, MainEffectsEnabled);
    SetRailgunVfxEmitterEnabled(G, Component,
        RailgunVfx::LargeSparkEmitter, MainEffectsEnabled);
    SetRailgunVfxEmitterEnabled(G, Component,
        RailgunVfx::RefractionEmitter, MainEffectsEnabled);
    SetRailgunVfxEmitterEnabled(G, Component,
        RailgunVfx::ProjectileEmitter, MainEffectsEnabled);
    SetRailgunVfxEmitterEnabled(G, Component,
        RailgunVfx::PuffEmitter, MainEffectsEnabled);
    SetRailgunVfxEmitterEnabled(G, Component,
        RailgunVfx::DirtMainEmitter, DirtMainOnly);
}

namespace RailgunImpactVfx
{
inline constexpr TCHAR Package[] =
    TEXT("/Game/Mods/Railgun/Station/BP_RailgunTransientVfx");
inline constexpr TCHAR MaximumLifetimeSeconds[] = TEXT("10.0");
inline const FName Component(TEXT("RailgunImpactVfxComponent"));
inline const FName LifeSpan(TEXT("InLifespan"));
inline const FName Asset(TEXT("InAsset"));
inline const FName ResetOverrides(TEXT("bResetExistingOverrideParameters"));
inline const FName AutoDestroyEnabled(TEXT("bInAutoDestroy"));
inline const FName Reset(TEXT("bReset"));
inline const FName DirtMainOnly(TEXT("DirtMainOnly"));
inline UClass* Class = nullptr;
}

UClass* CreateRailgunImpactVfx()
{
    auto* BP = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(),
        CreatePackage(RailgunImpactVfx::Package),
        FName(FPackageName::GetLongPackageAssetName(RailgunImpactVfx::Package)),
        BPTYPE_Normal, UBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass());
    const auto Nodes = BP->UbergraphPages[0]->Nodes;
    for (UEdGraphNode* Node : Nodes)
        Node->DestroyNode();

    auto* ComponentNode = BP->SimpleConstructionScript->CreateNode(
        UNiagaraComponent::StaticClass(), RailgunImpactVfx::Component);
    BP->SimpleConstructionScript->AddNode(ComponentNode);
    auto* ComponentTemplate = CastChecked<UNiagaraComponent>(
        ComponentNode->ComponentTemplate);
    ComponentTemplate->SetAutoActivate(false);
    AddVariable(BP, RailgunImpactVfx::DirtMainOnly,
        UEdGraphSchema_K2::PC_Boolean);

    FKismetEditorUtilities::CompileBlueprint(BP);
    FGraph G(BP->UbergraphPages[0]);
    auto* BeginPlay = NewObject<UK2Node_Event>(G.Graph);
    BeginPlay->EventReference.SetExternalMember(
        TimerGraphNames::ActorBeginPlay, AActor::StaticClass());
    BeginPlay->bOverrideFunction = true;
    G.Node(BeginPlay);
    G.Tail = G.Pin(BeginPlay, P::Then);

    auto* Life = G.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, SetLifeSpan));
    G.Default(Life, RailgunImpactVfx::LifeSpan,
        RailgunImpactVfx::MaximumLifetimeSeconds);
    G.Exec(Life);

    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath));
    G.Default(Path, E::PathString, RailgunVfx::SeaMinePath);
    auto* Reference = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,
            Conv_SoftObjPathToSoftObjRef));
    G.Link(G.Pin(Path, P::ReturnValue),
        G.Pin(Reference, AssetLoadingGraphNames::SoftObjectPath));
    auto* Load = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LoadAsset_Blocking));
    G.Link(G.Pin(Reference, P::ReturnValue),
        G.Pin(Load, AssetLoadingGraphNames::Asset));
    G.Exec(Load);
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph);
    Cast->TargetType = UNiagaraSystem::StaticClass();
    Cast->SetPurity(false);
    G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute));
    G.Link(G.Pin(Load, P::ReturnValue), Cast->GetCastSourcePin());
    G.Tail = Cast->GetValidCastPin();

    UEdGraphPin* Component = G.Read(RailgunImpactVfx::Component);
    auto* SetAsset = G.Call(UNiagaraComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UNiagaraComponent, SetAsset));
    G.Link(Component, G.Pin(SetAsset, P::FunctionTarget));
    G.Link(Cast->GetCastResultPin(), G.Pin(SetAsset, RailgunImpactVfx::Asset));
    G.Default(SetAsset, RailgunImpactVfx::ResetOverrides, RailgunVfx::True);
    G.Exec(SetAsset);

    auto* DisableAutoDestroy = G.Call(UNiagaraComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UNiagaraComponent, SetAutoDestroy));
    G.Link(Component, G.Pin(DisableAutoDestroy, P::FunctionTarget));
    G.Default(DisableAutoDestroy, RailgunImpactVfx::AutoDestroyEnabled,
        RailgunVfx::False);
    G.Exec(DisableAutoDestroy);

    auto* FountainMode = G.Branch(G.Read(RailgunImpactVfx::DirtMainOnly));
    ConfigureRailgunSeaMineEmitters(G, Component, true);
    UEdGraphPin* FountainMaskTail = G.Tail;
    G.Tail = G.Pin(FountainMode, P::Else);
    ConfigureRailgunSeaMineEmitters(G, Component, false);
    StationMerge(G, {FountainMaskTail, G.Tail});
    auto* Activate = G.Call(UActorComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UActorComponent, Activate));
    G.Link(Component, G.Pin(Activate, P::FunctionTarget));
    G.Default(Activate, RailgunImpactVfx::Reset, RailgunVfx::True);
    G.Exec(Activate);

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error);
    check(SaveDedicatedAsset(BP));
    return BP->GeneratedClass;
}

void SpawnRailgunVfx(FGraph& G, UEdGraphPin* Location, bool DirtMainOnly)
{
    check(RailgunImpactVfx::Class);
    auto* Transform = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeTransform));
    G.Link(Location, G.Pin(Transform, E::Location));
    G.Default(Transform, E::Scale, N::UnitScale);

    auto* Spawn = G.Call(UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics,
            BeginDeferredActorSpawnFromClass));
    G.Pin(Spawn, E::ActorClass)->DefaultObject =
        RailgunImpactVfx::Class;
    G.Link(G.Pin(Transform, P::ReturnValue),
        G.Pin(Spawn, P::SpawnTransform));
    G.Default(Spawn, E::CollisionHandling, N::AlwaysSpawn);
    G.Exec(Spawn);

    UEdGraphPin* Actor = G.Pin(Spawn, P::ReturnValue);
    auto* Spawned = G.Branch(G.Valid(Actor));
    auto* Typed = NewObject<UK2Node_DynamicCast>(G.Graph);
    Typed->TargetType = RailgunImpactVfx::Class;
    Typed->SetPurity(false);
    G.Node(Typed);
    G.Link(G.Tail, G.Pin(Typed, P::Execute));
    G.Link(Actor, Typed->GetCastSourcePin());
    G.Tail = Typed->GetValidCastPin();
    ContextSet(G, Typed->GetCastResultPin(), RailgunImpactVfx::Class,
        RailgunImpactVfx::DirtMainOnly, nullptr,
        DirtMainOnly ? RailgunVfx::True : RailgunVfx::False);
    auto* Finish = G.Call(UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor));
    G.Link(Actor, G.Pin(Finish, P::Actor));
    G.Link(G.Pin(Transform, P::ReturnValue),
        G.Pin(Finish, P::SpawnTransform));
    G.Exec(Finish);
    UEdGraphPin* FinishedTail = G.Tail;
    G.Tail = Typed->GetInvalidCastPin();
    auto* Destroy = G.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_DestroyActor));
    G.Link(Actor, G.Pin(Destroy, P::FunctionTarget));
    G.Exec(Destroy);
    StationMerge(G, {FinishedTail, G.Tail, G.Pin(Spawned, P::Else)});
}

void SpawnRailgunImpactVfx(FGraph& G, UEdGraphPin* HitResult)
{
    auto* BrokenHit = G.Call(UGameplayStatics::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult));
    G.Link(HitResult, G.Pin(BrokenHit, E::Hit));
    SpawnRailgunVfx(G, G.Pin(BrokenHit, OP::ImpactPoint), false);
}
