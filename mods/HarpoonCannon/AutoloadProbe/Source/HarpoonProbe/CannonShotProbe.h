#pragma once
#include "../../../HarpoonModelContract.h"
// Native swept projectile with a single direct-hit combat submission.
namespace Shot
{
inline constexpr TCHAR Package[] = TEXT("/Game/Mods/HarpoonCannon/Station/BP_HarpoonTestShot");
inline constexpr TCHAR Speed[] = TEXT("200000.0"), Range[] = TEXT("99999.0"), Life[] = TEXT("4.0");
inline const FName Body(TEXT("ShotCollision")), Move(TEXT("ShotMovement"));
inline const FName Start(TEXT("ShotOrigin")), Direction(TEXT("ShotDirection")), Done(TEXT("ShotDone"));
inline const FName SpawnedThisPress(TEXT("ShotSpawnedThisPress"));
inline const FName Cannon(TEXT("ShotCannon")), Operator(TEXT("ShotOperator")), Station(TEXT("ShotStation"));
inline const FName LifePin(TEXT("InLifespan")), HitEvent(TEXT("ReceiveHit"));
inline const FName Other(TEXT("Other")), Velocity(TEXT("Velocity")), Sweep(TEXT("bSweepCollision"));
inline const FName Penetration(TEXT("bAllowPenetration")), Ricochet(TEXT("bAllowRicochet"));
inline const FName Updated(TEXT("NewUpdatedComponent")), Reset(TEXT("bReset"));
inline const FName IgnoreActor(TEXT("Actor")), ShouldIgnore(TEXT("bShouldIgnore"));
inline const FName Prerequisite(TEXT("PrerequisiteActor")), Transform(TEXT("T"));
inline const FName Location(TEXT("Location")), ActorRotation(TEXT("Rotation"));
inline UClass* Class=nullptr;
}
namespace ShotAttack
{
inline const FName Controller(TEXT("ShotController")), DamageClass(TEXT("ShotDamageType"));
inline const FName GetController(TEXT("GetController")); // APawn also has a templated C++ overload.
inline const FName Damage(TEXT("Damage")), Variance(TEXT("DamageVariance")), Impulse(TEXT("ImpulseOverride"));
inline const FName Type(TEXT("AttackType")), Hit(TEXT("Hit")), Target(TEXT("Target"));
inline const FName Instigator(TEXT("Instigator")), Causer(TEXT("DamageCauser")), Id(TEXT("AttackID"));
inline const FName TypeClass(TEXT("DamageTypeClass")), Attack(TEXT("Attack")), Duration(TEXT("bAcceptDuration"));
inline const FName ClassPin(TEXT("Class")), Context(TEXT("ContextObject"));
inline constexpr TCHAR PhysicalType[] = TEXT("/Game/Blueprints/DamageTypes/BP_DamageTypePhysicalForce.BP_DamageTypePhysicalForce_C");
inline constexpr TCHAR BaseDamage[] = TEXT("200.0"), Directional[] = TEXT("Directional");
}
bool SaveDedicatedAsset(UObject* Asset);
UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);
UEdGraphPin* ShotLength(FGraph& G, UEdGraphPin* Vector)
{
    auto* Length=G.Call(UKismetMathLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,VSize));
    G.Link(Vector,G.Pin(Length,E::VectorLengthInput)); return G.Pin(Length,P::ReturnValue);
}
UEdGraphPin* ShotScale(FGraph& G, UEdGraphPin* Vector, const TCHAR* Scalar, UEdGraphPin* Value=nullptr)
{
    auto* Scale=G.Call(UKismetMathLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,Multiply_VectorFloat));
    G.Link(Vector,G.Pin(Scale,P::Binary::LeftOperand));
    if(Value) G.Link(Value,G.Pin(Scale,P::Binary::RightOperand)); else G.Default(Scale,P::Binary::RightOperand,Scalar);
    return G.Pin(Scale,P::ReturnValue);
}
UK2Node_Event* ShotEvent(FGraph& G,FName Name)
{
    auto* Event=NewObject<UK2Node_Event>(G.Graph); Event->EventReference.SetExternalMember(Name,AActor::StaticClass());
    Event->bOverrideFunction=true; G.Node(Event); G.Tail=G.Pin(Event,P::Then); return Event;
}
void StopShot(FGraph& G)
{
    G.Write(Shot::Done,nullptr,N::True);
    auto* Stop=G.Call(UMovementComponent::StaticClass(),GET_FUNCTION_NAME_CHECKED(UMovementComponent,StopMovementImmediately));
    G.Link(G.Read(Shot::Move),G.Pin(Stop,P::FunctionTarget)); G.Exec(Stop);
    auto* Deactivate=G.Call(UActorComponent::StaticClass(),GET_FUNCTION_NAME_CHECKED(UActorComponent,Deactivate));
    G.Link(G.Read(Shot::Move),G.Pin(Deactivate,P::FunctionTarget)); G.Exec(Deactivate);
    auto* Collision=G.Call(AActor::StaticClass(),GET_FUNCTION_NAME_CHECKED(AActor,SetActorEnableCollision));
    G.Default(Collision,SP::CollisionEnabled,N::False); G.Exec(Collision);
}
UClass* CreateCannonShot()
{
    auto* BP=FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(),CreatePackage(Shot::Package),FName(FPackageName::GetLongPackageAssetName(Shot::Package)),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
    const auto Nodes=BP->UbergraphPages[0]->Nodes; for(UEdGraphNode* Node:Nodes) Node->DestroyNode();
    auto* Root=BP->SimpleConstructionScript->CreateNode(USphereComponent::StaticClass(),Shot::Body); BP->SimpleConstructionScript->AddNode(Root);
    auto* Sphere=CastChecked<USphereComponent>(Root->ComponentTemplate); Sphere->SetSphereRadius(1);
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Sphere->SetCollisionObjectType(ECC_WorldDynamic);
    Sphere->SetCollisionResponseToAllChannels(ECR_Block); Sphere->SetGenerateOverlapEvents(false);
    auto* Move=BP->SimpleConstructionScript->CreateNode(UVoyageProjectileMovementComponent::StaticClass(),Shot::Move); BP->SimpleConstructionScript->AddNode(Move);
    auto* Template=CastChecked<UProjectileMovementComponent>(Move->ComponentTemplate); Template->SetAutoActivate(false);
    Template->ProjectileGravityScale=0; Template->MaxSpeed=300000; Template->bShouldBounce=false; Template->bSweepCollision=true;
    for(FName Name:{Shot::Cannon,Shot::Operator,Shot::Station}) AddVariable(BP,Name,UEdGraphSchema_K2::PC_Object,AActor::StaticClass());
    AddVariable(BP,ShotAttack::Controller,UEdGraphSchema_K2::PC_Object,AController::StaticClass());
    AddVariable(BP,ShotAttack::DamageClass,UEdGraphSchema_K2::PC_Class,UVoyageDamageType::StaticClass());
    for(FName Name:{Shot::Start,Shot::Direction}) AddVariable(BP,Name,UEdGraphSchema_K2::PC_Struct,TBaseStructure<FVector>::Get());
    AddVariable(BP,Shot::Done,UEdGraphSchema_K2::PC_Boolean);
    FKismetEditorUtilities::CompileBlueprint(BP); FGraph G(BP->UbergraphPages[0],nullptr);
    ShotEvent(G,TimerGraphNames::ActorBeginPlay);
    auto* TypePath=G.Call(UKismetSystemLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,MakeSoftClassPath)); G.Default(TypePath,E::PathString,ShotAttack::PhysicalType);
    auto* TypeRef=G.Call(UKismetSystemLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,Conv_SoftClassPathToSoftClassRef)); G.Link(G.Pin(TypePath,P::ReturnValue),G.Pin(TypeRef,E::SoftClassPath));
    auto* TypeLoad=G.Call(UKismetSystemLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,LoadClassAsset_Blocking)); G.Link(G.Pin(TypeRef,P::ReturnValue),G.Pin(TypeLoad,E::AssetClass)); G.Exec(TypeLoad);
    auto* TypeCast=NewObject<UK2Node_ClassDynamicCast>(G.Graph); TypeCast->TargetType=UVoyageDamageType::StaticClass(); TypeCast->SetPurity(true); G.Node(TypeCast); G.Link(G.Pin(TypeLoad,P::ReturnValue),TypeCast->GetCastSourcePin());
    G.Write(ShotAttack::DamageClass,TypeCast->GetCastResultPin());
    auto* Life=G.Call(AActor::StaticClass(),GET_FUNCTION_NAME_CHECKED(AActor,SetLifeSpan)); G.Default(Life,Shot::LifePin,Shot::Life); G.Exec(Life);
    G.Write(Shot::Start,ObserveCall(G,AActor::StaticClass(),GET_FUNCTION_NAME_CHECKED(AActor,K2_GetActorLocation),OpticalSelf(G)));
    G.Write(Shot::Direction,ObserveCall(G,AActor::StaticClass(),GET_FUNCTION_NAME_CHECKED(AActor,GetActorForwardVector),OpticalSelf(G)));
    for(FName Name:{Shot::Cannon,Shot::Operator,Shot::Station}) {
        auto* Ignore=G.Call(UPrimitiveComponent::StaticClass(),GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent,IgnoreActorWhenMoving));
        G.Link(G.Read(Shot::Body),G.Pin(Ignore,P::FunctionTarget)); G.Link(G.Read(Name),G.Pin(Ignore,Shot::IgnoreActor)); G.Default(Ignore,Shot::ShouldIgnore,N::True); G.Exec(Ignore);
    }
    // The ship is deliberately NOT ignored: a deck/superstructure obstruction is real.
    ContextSet(G,G.Read(Shot::Move),UVoyageProjectileMovementComponent::StaticClass(),Shot::Penetration,nullptr,N::False);
    ContextSet(G,G.Read(Shot::Move),UVoyageProjectileMovementComponent::StaticClass(),Shot::Ricochet,nullptr,N::False);
    ContextSet(G,G.Read(Shot::Move),UProjectileMovementComponent::StaticClass(),Shot::Sweep,nullptr,N::True);
    auto* Updated=G.Call(UMovementComponent::StaticClass(),GET_FUNCTION_NAME_CHECKED(UMovementComponent,SetUpdatedComponent));
    G.Link(G.Read(Shot::Move),G.Pin(Updated,P::FunctionTarget)); G.Link(G.Read(Shot::Body),G.Pin(Updated,Shot::Updated)); G.Exec(Updated);
    // Actor bounds this frame's displacement BEFORE movement ticks, including its first tick.
    auto* Prereq=G.Call(UActorComponent::StaticClass(),GET_FUNCTION_NAME_CHECKED(UActorComponent,AddTickPrerequisiteActor));
    G.Link(G.Read(Shot::Move),G.Pin(Prereq,P::FunctionTarget)); G.Link(OpticalSelf(G),G.Pin(Prereq,Shot::Prerequisite)); G.Exec(Prereq);
    auto* Tick=ShotEvent(G,BlueprintGraphNames::Events::ActorReceiveTick);
    // Exit/destroy of the operator ends remaining flight.
    auto* Controlled=G.Call(APawn::StaticClass(),GET_FUNCTION_NAME_CHECKED(APawn,IsPlayerControlled));
    auto* StationCast=NewObject<UK2Node_DynamicCast>(G.Graph); StationCast->TargetType=APawn::StaticClass(); StationCast->SetPurity(true); G.Node(StationCast);
    G.Link(G.Read(Shot::Station),StationCast->GetCastSourcePin());
    auto* Valid=G.Branch(G.Valid(StationCast->GetCastResultPin())); auto* ValidTail=G.Tail;
    G.Tail=G.Pin(Valid,P::Else); auto* Destroy=G.Call(AActor::StaticClass(),GET_FUNCTION_NAME_CHECKED(AActor,K2_DestroyActor)); G.Exec(Destroy);
    G.Tail=ValidTail; G.Link(StationCast->GetCastResultPin(),G.Pin(Controlled,P::FunctionTarget));
    auto* Occupied=G.Branch(G.Pin(Controlled,P::ReturnValue)); auto* OccupiedTail=G.Tail;
    G.Tail=G.Pin(Occupied,P::Else); auto* ExitDestroy=G.Call(AActor::StaticClass(),GET_FUNCTION_NAME_CHECKED(AActor,K2_DestroyActor)); G.Exec(ExitDestroy); G.Tail=OccupiedTail;
    auto* Done=G.Branch(G.Read(Shot::Done)); G.Tail=G.Pin(Done,P::Else);
    auto* Delta=G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,Subtract_VectorVector),ObserveCall(G,AActor::StaticClass(),GET_FUNCTION_NAME_CHECKED(AActor,K2_GetActorLocation),OpticalSelf(G)),G.Read(Shot::Start));
    auto* Remaining=G.Call(UKismetMathLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,Subtract_DoubleDouble));
    G.Default(Remaining,P::Binary::LeftOperand,Shot::Range); G.Link(ShotLength(G,Delta),G.Pin(Remaining,P::Binary::RightOperand));
    auto* InRange=G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,Greater_DoubleDouble),G.Pin(Remaining,P::ReturnValue),CE::One)); auto* FlyingTail=G.Tail;
    G.Tail=G.Pin(InRange,P::Else); StopShot(G); G.Tail=FlyingTail;
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,Greater_DoubleDouble),G.Pin(Tick,P::DeltaSeconds),N::Zero));
    auto* Rate=G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,Divide_DoubleDouble),G.Pin(Remaining,P::ReturnValue),G.Pin(Tick,P::DeltaSeconds));
    auto* Limited=G.Call(UKismetMathLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,FMin));
    G.Link(Rate,G.Pin(Limited,P::Binary::LeftOperand)); G.Default(Limited,P::Binary::RightOperand,Shot::Speed);
    ContextSet(G,G.Read(Shot::Move),UMovementComponent::StaticClass(),Shot::Velocity,ShotScale(G,G.Read(Shot::Direction),nullptr,G.Pin(Limited,P::ReturnValue)));
    auto* Activate=G.Call(UActorComponent::StaticClass(),GET_FUNCTION_NAME_CHECKED(UActorComponent,Activate)); G.Link(G.Read(Shot::Move),G.Pin(Activate,P::FunctionTarget)); G.Exec(Activate);
    auto* Hit=ShotEvent(G,Shot::HitEvent); auto* WasDone=G.Branch(G.Read(Shot::Done)); G.Tail=G.Pin(WasDone,P::Else);
    StopShot(G);
    // Done was set before this path: a second ReceiveHit cannot submit twice.
    G.Branch(G.Valid(G.Pin(Hit,Shot::Other)));
    G.Branch(G.Valid(G.Read(ShotAttack::Controller)));
    auto* ValidType=G.Call(UKismetSystemLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary,IsValidClass));
    G.Link(G.Read(ShotAttack::DamageClass),G.Pin(ValidType,ShotAttack::ClassPin)); G.Branch(G.Pin(ValidType,P::ReturnValue));
    auto* Subsystem=G.Call(USubsystemBlueprintLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(USubsystemBlueprintLibrary,GetWorldSubsystem));
    G.Link(OpticalSelf(G),G.Pin(Subsystem,ShotAttack::Context)); G.Pin(Subsystem,ShotAttack::ClassPin)->DefaultObject=UVoyageCombatSubsystem::StaticClass();
    auto* Combat=NewObject<UK2Node_DynamicCast>(G.Graph); Combat->TargetType=UVoyageCombatSubsystem::StaticClass(); Combat->SetPurity(false); G.Node(Combat);
    G.Link(G.Tail,G.Pin(Combat,P::Execute)); G.Link(G.Pin(Subsystem,P::ReturnValue),Combat->GetCastSourcePin()); G.Tail=Combat->GetValidCastPin();
    auto* Attack=NewObject<UK2Node_MakeStruct>(G.Graph); Attack->StructType=FVoyageAttack::StaticStruct(); Attack->bMadeAfterOverridePinRemoval=true; G.Node(Attack);
    G.Link(G.Read(ShotAttack::DamageClass),G.Pin(Attack,ShotAttack::TypeClass)); G.Default(Attack,ShotAttack::Damage,ShotAttack::BaseDamage);
    G.Default(Attack,ShotAttack::Variance,N::Zero); G.Default(Attack,ShotAttack::Impulse,N::Zero); G.Default(Attack,ShotAttack::Id,N::Zero); G.Default(Attack,ShotAttack::Type,ShotAttack::Directional);
    G.Link(G.Pin(Hit,E::Hit),G.Pin(Attack,ShotAttack::Hit)); G.Link(G.Pin(Hit,Shot::Other),G.Pin(Attack,ShotAttack::Target));
    G.Link(G.Read(ShotAttack::Controller),G.Pin(Attack,ShotAttack::Instigator)); G.Link(OpticalSelf(G),G.Pin(Attack,ShotAttack::Causer));
    auto* Register=G.Call(UVoyageCombatSubsystem::StaticClass(),GET_FUNCTION_NAME_CHECKED(UVoyageCombatSubsystem,RegisterAttack));
    G.Link(Combat->GetCastResultPin(),G.Pin(Register,P::FunctionTarget)); G.Default(Register,ShotAttack::Duration,N::False);
    for(auto* Pin:Attack->Pins) if(Pin->Direction==EGPD_Output) G.Link(Pin,G.Pin(Register,ShotAttack::Attack)); G.Exec(Register);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP); FKismetEditorUtilities::CompileBlueprint(BP); check(BP->Status!=BS_Error);
    auto* CDO=BP->GeneratedClass->GetDefaultObject<AActor>(); CDO->PrimaryActorTick.bCanEverTick=true; CDO->PrimaryActorTick.bStartWithTickEnabled=true; CDO->PrimaryActorTick.TickGroup=TG_PrePhysics;
    check(SaveDedicatedAsset(BP)); return BP->GeneratedClass;
}

void AddCannonFire(FGraph& G)
{
    auto* Event=NewObject<UK2Node_EnhancedInputAction>(G.Graph); Event->InputAction=LoadObject<UInputAction>(nullptr,HarpoonInputNames::Fire); check(Event->InputAction); G.Node(Event); G.Tail=G.Pin(Event,DS::Started);
    G.Branch(ObserveCall(G,APawn::StaticClass(),GET_FUNCTION_NAME_CHECKED(APawn,IsPlayerControlled),OpticalSelf(G)));
    G.Write(Shot::SpawnedThisPress,nullptr,N::False); // no cooldown; each Started is independent
    G.Branch(G.Valid(G.Read(S::Anchor)));
    auto* Cannon=ObserveCall(G,UActorComponent::StaticClass(),OP::ComponentOwner,G.Read(S::Anchor));
    auto* Find=G.Call(AActor::StaticClass(),GET_FUNCTION_NAME_CHECKED(AActor,GetComponentsByTag));
    G.Link(Cannon,G.Pin(Find,P::FunctionTarget)); G.Pin(Find,OP::ComponentClass)->DefaultObject=USceneComponent::StaticClass(); G.Default(Find,ActorScanGraphNames::ComponentTag,*HarpoonModelContract::MuzzleTag.ToString());
    auto* Loop=ContextLoop(G,G.Pin(Find,P::ReturnValue));
    auto* Cast=NewObject<UK2Node_DynamicCast>(G.Graph); Cast->TargetType=USceneComponent::StaticClass(); Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail,G.Pin(Cast,P::Execute)); G.Link(G.Pin(Loop,CE::ArrayElement),Cast->GetCastSourcePin()); G.Tail=Cast->GetValidCastPin();
    auto* Already=G.Branch(G.Read(Shot::SpawnedThisPress)); G.Tail=G.Pin(Already,P::Else); // one shot even if duplicate muzzle tags exist
    auto* Location=ObserveCall(G,USceneComponent::StaticClass(),GET_FUNCTION_NAME_CHECKED(USceneComponent,K2_GetComponentLocation),Cast->GetCastResultPin());
    auto* Rotation=ObserveCall(G,USceneComponent::StaticClass(),GET_FUNCTION_NAME_CHECKED(USceneComponent,K2_GetComponentRotation),Cast->GetCastResultPin());
    auto* Transform=G.Call(UKismetMathLibrary::StaticClass(),GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,MakeTransform)); G.Link(Location,G.Pin(Transform,E::Location)); G.Link(Rotation,G.Pin(Transform,Shot::ActorRotation)); G.Default(Transform,E::Scale,N::UnitScale);
    auto* Spawn=G.Call(UGameplayStatics::StaticClass(),GET_FUNCTION_NAME_CHECKED(UGameplayStatics,BeginDeferredActorSpawnFromClass)); G.Pin(Spawn,E::ActorClass)->DefaultObject=Shot::Class; G.Link(G.Pin(Transform,P::ReturnValue),G.Pin(Spawn,P::SpawnTransform)); G.Default(Spawn,E::CollisionHandling,N::AlwaysSpawn); G.Exec(Spawn);
    auto* Typed=NewObject<UK2Node_DynamicCast>(G.Graph); Typed->TargetType=Shot::Class; Typed->SetPurity(false); G.Node(Typed); G.Link(G.Tail,G.Pin(Typed,P::Execute)); G.Link(G.Pin(Spawn,P::ReturnValue),Typed->GetCastSourcePin()); G.Tail=Typed->GetValidCastPin();
    ContextSet(G,Typed->GetCastResultPin(),Shot::Class,Shot::Cannon,Cannon);
    ContextSet(G,Typed->GetCastResultPin(),Shot::Class,Shot::Operator,G.Read(N::OriginalPawn));
    ContextSet(G,Typed->GetCastResultPin(),Shot::Class,Shot::Station,OpticalSelf(G));
    ContextSet(G,Typed->GetCastResultPin(),Shot::Class,ShotAttack::Controller,ObserveCall(G,APawn::StaticClass(),ShotAttack::GetController,OpticalSelf(G)));
    G.Write(Shot::SpawnedThisPress,nullptr,N::True);
    auto* Finish=G.Call(UGameplayStatics::StaticClass(),GET_FUNCTION_NAME_CHECKED(UGameplayStatics,FinishSpawningActor)); G.Link(G.Pin(Spawn,P::ReturnValue),G.Pin(Finish,P::Actor)); G.Link(G.Pin(Transform,P::ReturnValue),G.Pin(Finish,P::SpawnTransform)); G.Exec(Finish);
}
