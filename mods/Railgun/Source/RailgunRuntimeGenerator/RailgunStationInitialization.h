#pragma once
#include "AssetLoadingGraphNames.h"
// Shared array loop used by the generated Railgun graphs.
UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values)
{
    auto* Macros = LoadObject<UBlueprint>(nullptr, CE::LoopPackage); check(Macros);
    UEdGraph* LoopGraph = nullptr; for (UEdGraph* Graph : Macros->MacroGraphs) if (Graph->GetFName() == CE::LoopGraph) LoopGraph = Graph;
    check(LoopGraph); auto* Loop = NewObject<UK2Node_MacroInstance>(G.Graph); Loop->SetMacroGraph(LoopGraph); G.Node(Loop);
    UEdGraphPin* Execute = nullptr;
    for (auto* Pin : Loop->Pins)
        if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) { check(!Execute); Execute = Pin; }
    check(Execute);
    G.Link(G.Tail, Execute); G.Link(Values, G.Pin(Loop, CE::Array)); G.Tail = G.Pin(Loop, CE::LoopBody); return Loop;
}

void PrepareRailgunStation(FGraph& G, UClass* StationClass, UEdGraphPin* Shell)
{
    // Resolve before spawning: old/missing shell contract must not create an
    // active fallback query around the gun body or orphan replacement stations.
    G.Write(CE::ModelEntry, nullptr, N::EmptyText);
    auto* FindEntry = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, GetComponentsByTag));
    G.Link(Shell, G.Pin(FindEntry, P::FunctionTarget));
    G.Pin(FindEntry, OP::ComponentClass)->DefaultObject = UBoxComponent::StaticClass();
    G.Default(FindEntry, ActorScanGraphNames::ComponentTag, *RailgunModelContract::EntryTag.ToString());
    auto* EntryLoop = ContextLoop(G, G.Pin(FindEntry, P::ReturnValue));
    auto* EntryCast = NewObject<UK2Node_DynamicCast>(G.Graph); EntryCast->TargetType = UBoxComponent::StaticClass(); EntryCast->SetPurity(false); G.Node(EntryCast);
    G.Link(G.Tail, G.Pin(EntryCast, P::Execute)); G.Link(G.Pin(EntryLoop, CE::ArrayElement), EntryCast->GetCastSourcePin()); G.Tail = EntryCast->GetValidCastPin();
    G.Write(CE::ModelEntry, EntryCast->GetCastResultPin()); G.Tail = G.Pin(EntryLoop, CE::Completed);
    G.Branch(G.Valid(G.Read(CE::ModelEntry)));
    auto* Root = ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), Shell);
    G.Branch(G.Valid(Root)); G.Write(S::Anchor, Root);
    // Fabricator visuals are not this class; exact native-built mount guards
    // also exclude transient/unattached editor previews of the same class.
    auto* Name = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName)); G.Link(Root, G.Pin(Name, P::Object));
    auto* Match = G.Call(UKismetStringLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetStringLibrary, EqualEqual_StrStr));
    G.Link(G.Pin(Name, P::ReturnValue), G.Pin(Match, P::Binary::LeftOperand)); G.Default(Match, P::Binary::RightOperand, S::RootName); G.Branch(G.Pin(Match, P::ReturnValue));
    G.Branch(G.Valid(ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), Root)));
    auto* Rotation = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentRotation), Root);
    auto* AnchorTransform = G.Transform(ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), Root), Rotation);
    auto* Transform = G.Transform(G.Offset(AnchorTransform, CombinedVehicleNames::Offset), Rotation);
    auto* Spawn = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BeginDeferredActorSpawnFromClass));
    G.Pin(Spawn, E::ActorClass)->DefaultObject = StationClass; G.Link(Transform, G.Pin(Spawn, P::SpawnTransform));
    G.Link(Shell, G.Pin(Spawn, CE::Owner)); G.Default(Spawn, E::CollisionHandling, N::AlwaysSpawn); G.Exec(Spawn);
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph); Cast->TargetType = StationClass; Cast->SetPurity(false); G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute)); G.Link(G.Pin(Spawn, P::ReturnValue), Cast->GetCastSourcePin()); G.Tail = Cast->GetValidCastPin();
    auto* Station = Cast->GetCastResultPin(); G.Write(V::Vehicle, Station);
    ContextSet(G, Station, StationClass, S::Anchor, G.Read(S::Anchor));
    ContextSet(G, Station, StationClass, CE::EntryAction, G.Read(CE::EntryAction));
    auto* Body = NewObject<UK2Node_DynamicCast>(G.Graph); Body->TargetType = UPrimitiveComponent::StaticClass(); Body->SetPurity(false); G.Node(Body);
    G.Link(G.Tail, G.Pin(Body, P::Execute));
    G.Link(ObserveCall(G, AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), Station), Body->GetCastSourcePin()); G.Tail = Body->GetValidCastPin(); G.Write(V::Body, Body->GetCastResultPin());
    auto* Physics = G.Call(UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetSimulatePhysics));
    G.Link(G.Read(V::Body), G.Pin(Physics, P::FunctionTarget)); G.Default(Physics, E::SimulatePhysics, N::False); G.Exec(Physics);
    auto* Collision = G.Call(UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, SetCollisionEnabled));
    const FName CollisionType(TEXT("NewType")); const TCHAR* NoCollision = TEXT("ECollisionEnabled::NoCollision");
    G.Link(G.Read(V::Body), G.Pin(Collision, P::FunctionTarget)); G.Default(Collision, CollisionType, NoCollision); G.Exec(Collision);
    auto* Controls = ReadNativeInputField(G, Station, AVoyageVehiclePawn::StaticClass(), NativeInputNames::ControlsField);
    G.Require(G.Valid(Controls), ExitActionNames::Failed);
    NativeInputFieldGuard(G, Controls, UVoyageInputControlsComponent::StaticClass(), NativeInputNames::ContextField, NativeInputNames::Context, true);
    auto* Finish = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, FinishSpawningActor));
    G.Link(Station, G.Pin(Finish, P::Actor)); G.Link(Transform, G.Pin(Finish, P::SpawnTransform)); G.Exec(Finish); G.Require(G.Valid(G.Pin(Finish, P::ReturnValue)), V::Failed);
    auto* Attach = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, K2_AttachToComponent));
    G.Link(Station, G.Pin(Attach, P::FunctionTarget)); G.Link(G.Read(S::Anchor), G.Pin(Attach, E::AttachmentParent));
    G.Default(Attach, E::LocationRule, N::KeepWorld); G.Default(Attach, E::RotationRule, N::KeepWorld); G.Default(Attach, E::ScaleRule, N::KeepWorld);
    G.Default(Attach, E::WeldBodies, N::False); G.Exec(Attach); G.Require(G.Pin(Attach, P::ReturnValue), V::Failed);
    ConfigureCombinedOperatorPoint(G);
    NativeInputFieldGuard(G, Controls, UVoyageInputControlsComponent::StaticClass(), NativeInputNames::ContextField, NativeInputNames::Context, false);
    auto* Interaction = ReadNativeInputField(G, Station, StationClass, CE::Interaction);
    auto* Query = ReadNativeInputField(G, Station, StationClass, CE::QueryBox);
    G.Require(G.Valid(Interaction), V::Failed); G.Require(G.Valid(Query), V::Failed);
    auto* PlaceEntry = G.Call(USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_SetWorldTransform));
    G.Link(Interaction, G.Pin(PlaceEntry, P::FunctionTarget));
    G.Link(ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentToWorld), G.Read(CE::ModelEntry)), G.Pin(PlaceEntry, ActorLifecycleGraphNames::NewTransform));
    G.Default(PlaceEntry, OP::Sweep, N::False); G.Exec(PlaceEntry);
    auto* ResizeEntry = G.Call(UBoxComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UBoxComponent, SetBoxExtent));
    G.Link(Query, G.Pin(ResizeEntry, P::FunctionTarget));
    G.Link(ObserveCall(G, UBoxComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UBoxComponent, GetUnscaledBoxExtent), G.Read(CE::ModelEntry)), G.Pin(ResizeEntry, CE::BoxExtentPin)); G.Exec(ResizeEntry);
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent), Query), Interaction), V::Failed);
    // Only own query box is enabled: native root remains NoCollision/nonphysical.
    auto* Enable = G.Call(AActor::StaticClass(), GET_FUNCTION_NAME_CHECKED(AActor, SetActorEnableCollision));
    G.Link(Station, G.Pin(Enable, P::FunctionTarget)); G.Default(Enable, SP::CollisionEnabled, N::True); G.Exec(Enable);
    // Observe the effective response after FinishSpawning; do not repair it here.
    auto* Response = G.Call(UPrimitiveComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, GetCollisionResponseToChannel));
    G.Link(Query, G.Pin(Response, P::FunctionTarget)); G.Default(Response, CE::CollisionChannelPin, CE::InteractChannelValue);
    ContextSet(G, Station, StationClass, CE::InteractBlocks,
        G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), G.Pin(Response, P::ReturnValue), CE::BlockResponseValue));
    ContextSet(G, Station, StationClass, CE::Ready, nullptr, N::True);
}

void AddRailgunStationInitializationFunction(UBlueprint* BP,
    UClass* StationClass)
{
    check(BP && StationClass);
    AddVariable(BP, CE::ModelEntry, UEdGraphSchema_K2::PC_Object, UBoxComponent::StaticClass());
    AddVariable(BP, CE::EntryAction, UEdGraphSchema_K2::PC_Object, UInputAction::StaticClass());
    AddVariable(BP, NativeInputNames::Context, UEdGraphSchema_K2::PC_Object,
        UVoyageInputContextAsset::StaticClass());
    AddVariable(BP, S::Anchor, UEdGraphSchema_K2::PC_Object,
        USceneComponent::StaticClass());
    AddVariable(BP, V::Vehicle, UEdGraphSchema_K2::PC_Object,
        AVoyageVehiclePawn::StaticClass());
    AddVariable(BP, V::Body, UEdGraphSchema_K2::PC_Object,
        UPrimitiveComponent::StaticClass());
    for (FName RuntimeReference : {CE::ModelEntry, CE::EntryAction,
        NativeInputNames::Context, S::Anchor, V::Vehicle, V::Body})
    {
        MarkVariableTransient(BP, RuntimeReference);
    }
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass);

    UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP,
        CE::InitializeStation, UEdGraph::StaticClass(),
        UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddFunctionGraph(BP, Graph, false,
        static_cast<UClass*>(nullptr));
    UK2Node_FunctionEntry* Entry = nullptr;
    for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Candidate = Cast<UK2Node_FunctionEntry>(Node)) Entry = Candidate;
    check(Entry);
    Entry->FindPinChecked(P::Then)->BreakAllPinLinks();

    FGraph G(Graph, nullptr);
    G.Tail = G.Pin(Entry, P::Then);
    // Idempotence is local to this shell. A destroyed station becomes invalid,
    // allowing a later genuine lifecycle event to recreate it without scans.
    auto* Existing = G.Branch(G.Valid(G.Read(V::Vehicle)));
    G.Tail = G.Pin(Existing, P::Else);
    G.Branch(ObserveCall(G, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, HasAuthority), OpticalSelf(G)));
    G.Write(CE::EntryAction, LoadStockInputReference(G, CE::InteractActionPath, UInputAction::StaticClass()));
    G.Write(NativeInputNames::Context, LoadStockInputReference(G, DS::ContextPath, UVoyageInputContextAsset::StaticClass()));
    PrepareRailgunStation(G, StationClass, OpticalSelf(G));
}

bool ConfigureRailgunStationInitialization(UClass* StationClass)
{
    check(StationClass);
    UBlueprint* BP = LoadObject<UBlueprint>(nullptr,
        RailgunInventoryShared::ModuleObjectPath);
    check(BP && BP->ParentClass == AVoyageModuleActor::StaticClass());
    AddRailgunStationInitializationFunction(BP, StationClass);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass &&
        BP->GeneratedClass->FindFunctionByName(CE::InitializeStation));

    // Both paths schedule exactly one next-tick initialization attempt. This
    // avoids player/controller readiness and global discovery. Whether native
    // construction has attached a newly built shell by then is a runtime gate.
    AddRailgunInventoryLimitInitialization(BP, CE::InitializeStation);
    AddRailgunAmmoVisualPostLoad(BP, CE::InitializeStation);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    check(BP->Status != BS_Error && BP->GeneratedClass);
    return SaveDedicatedAsset(BP);

}
