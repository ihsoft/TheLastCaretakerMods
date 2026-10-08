#include "StationRangeGraph.h"

#include "GraphCallHelpers.h"
#include "StationAttachmentGraph.h"
#include "StationOpticsGraph.h"
#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
UEdGraphPin* RangeLiteralText(FGraph& G, const TCHAR* Value)
{
    auto* Text = G.Call(UKismetTextLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
    G.Default(Text, E::StringValue, Value);
    return G.Pin(Text, P::ReturnValue);
}

UEdGraphPin* EqualRangeText(FGraph& G, UEdGraphPin* Left,
    UEdGraphPin* Right)
{
    auto* Equal = G.Call(UKismetTextLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, EqualEqual_TextText));
    G.Link(Left, G.Pin(Equal, P::Binary::LeftOperand));
    G.Link(Right, G.Pin(Equal, P::Binary::RightOperand));
    return G.Pin(Equal, P::ReturnValue);
}

void WriteRangeTextIfChanged(FGraph& G, FName Field, UEdGraphPin* Value)
{
    auto* Same = G.Branch(EqualRangeText(G, G.Read(Field), Value));
    G.Tail = G.Pin(Same, P::Else);
    G.Write(Field, Value);
    StationMerge(G, {G.Tail, G.Pin(Same, P::Then)});
}

void ClearStationRangeDisplay(FGraph& G)
{
    WriteRangeTextIfChanged(G, Range::TargetName,
        RangeLiteralText(G, N::EmptyText));
    WriteRangeTextIfChanged(G, Range::TargetRange,
        RangeLiteralText(G, Range::NoHitLabel));
}

void UpdateStationRange(FGraph& G)
{
    // First blocking optical hit only; any actor is a display target, not a fireable target.
    auto* Start = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, K2_GetComponentLocation), G.Read(O::Camera));
    auto* Direction = ObserveCall(G, USceneComponent::StaticClass(), GET_FUNCTION_NAME_CHECKED(USceneComponent, GetForwardVector), G.Read(O::Camera));
    auto* Ray = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Multiply_VectorFloat));
    G.Link(Direction, G.Pin(Ray, P::Binary::LeftOperand)); G.Default(Ray, P::Binary::RightOperand, Range::MaximumCentimeters);
    [[maybe_unused]] constexpr auto OwnerSignature = static_cast<AActor* (UActorComponent::*)() const>(&UActorComponent::GetOwner);
    auto* Railgun = ObserveCall(G, UActorComponent::StaticClass(), OP::ComponentOwner, G.Read(S::Anchor));
    auto* Trace = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, LineTraceSingle));
    G.Link(Start, G.Pin(Trace, E::TraceStart)); G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_VectorVector), Start, G.Pin(Ray, P::ReturnValue)), G.Pin(Trace, E::TraceEnd));
    // MakeArray infers its type from the first link: Actor admits Pawn, not vice versa.
    G.Link(G.ActorArray(Railgun, G.Read(N::OriginalPawn)), G.Pin(Trace, E::ActorsToIgnore));
    G.Default(Trace, E::TraceChannel, N::VisibilityTrace); G.Default(Trace, E::TraceComplex, N::False); G.Default(Trace, E::IgnoreSelf, N::True); G.Exec(Trace);
    auto* TraceHit = G.Branch(G.Pin(Trace, P::ReturnValue));
    auto* Hit = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult));
    G.Link(G.Pin(Trace, E::OutHit), G.Pin(Hit, E::Hit));
    auto* ActorValid = G.Branch(G.Valid(G.Pin(Hit, SP::HitActor)));
    // Resolve the same human-facing item provider used by the dismantle tool.
    // Reset first so an unsupported hit cannot retain the previous target name.
    G.Write(Range::PendingTargetName,
        RangeLiteralText(G, N::EmptyText));
    auto* Length = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), G.Pin(Hit, OP::ImpactPoint), Start), G.Pin(Length, E::VectorLengthInput));
    auto* Meters = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble));
    G.Link(G.Pin(Length, P::ReturnValue), G.Pin(Meters, P::Binary::LeftOperand)); G.Default(Meters, P::Binary::RightOperand, Range::CentimetersPerMeter);
    auto* ClampedMeters = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMin));
    G.Link(G.Pin(Meters, P::ReturnValue), G.Pin(ClampedMeters, P::Binary::LeftOperand));
    G.Default(ClampedMeters, P::Binary::RightOperand, Range::MaximumDisplayedMeters);
    auto* Rounded = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Round));
    G.Link(G.Pin(ClampedMeters, P::ReturnValue), G.Pin(Rounded, OP::AngleValue));
    auto* Digits = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_IntToText));
    G.Link(G.Pin(Rounded, P::ReturnValue), G.Pin(Digits, P::Value));
    G.Default(Digits, Range::UseGroupingPin, N::False);
    G.Default(Digits, Range::MinimumIntegralDigitsPin, Range::IntegralDigits);
    G.Default(Digits, Range::MaximumIntegralDigitsPin, Range::IntegralDigits);
    WriteRangeTextIfChanged(G, Range::TargetRange,
        G.Pin(Digits, P::ReturnValue));
    auto* ResolveItemProvider = G.Call(
        UVoyageMiscBlueprintFunctionLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UVoyageMiscBlueprintFunctionLibrary,
            GetDestructibleInterface));
    G.Link(G.Pin(Hit, E::HitComponent),
        G.Pin(ResolveItemProvider, P::Object));
    auto* ProviderValid = G.Branch(
        G.Valid(G.Pin(ResolveItemProvider, P::ReturnValue)));
    auto* ItemInterface = NewObject<UK2Node_DynamicCast>(G.Graph);
    ItemInterface->TargetType = UVoyageItemInterface::StaticClass();
    ItemInterface->SetPurity(true);
    G.Node(ItemInterface);
    G.Link(G.Pin(ResolveItemProvider, P::ReturnValue),
        ItemInterface->GetCastSourcePin());
    auto* ItemInterfaceValid = G.Branch(
        ItemInterface->GetBoolSuccessPin());
    auto* GetItemName = G.Call(UVoyageItemInterface::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(IVoyageItemInterface, GetItemName));
    G.Link(ItemInterface->GetCastResultPin(),
        G.Pin(GetItemName, P::FunctionTarget));
    auto* Empty = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, TextIsEmpty));
    G.Link(G.Pin(GetItemName, P::ReturnValue),
        G.Pin(Empty, E::WidgetText));
    auto* EmptyBranch = G.Branch(G.Pin(Empty, P::ReturnValue)); G.Tail = EmptyBranch->GetElsePin();
    G.Write(Range::PendingTargetName,
        G.Pin(GetItemName, P::ReturnValue));
    StationMerge(G, {G.Tail, G.Pin(EmptyBranch, P::Then),
        G.Pin(ItemInterfaceValid, P::Else), G.Pin(ProviderValid, P::Else)});
    WriteRangeTextIfChanged(G, Range::TargetName,
        G.Read(Range::PendingTargetName));
    UEdGraphPin* HitTail = G.Tail;

    G.Tail = G.Pin(ActorValid, P::Else);
    ClearStationRangeDisplay(G);
    UEdGraphPin* InvalidActorTail = G.Tail;
    G.Tail = G.Pin(TraceHit, P::Else);
    ClearStationRangeDisplay(G);
    StationMerge(G, {HitTail, InvalidActorTail, G.Tail});
}
}
