#pragma once

// Builds optical target/range tracing and HUD text graphs.
namespace StationRangeNames
{
// Retain the existing widget/property identity; HC33 generalizes its contents.
inline const FName TargetName(TEXT("DetectedSharkName"));
inline const FName TargetRange(TEXT("OpticalTargetRange"));
inline const FName PendingTargetName(TEXT("RailgunPendingTargetName"));
inline const FName DisplayedTargetName(TEXT("RailgunDisplayedTargetName"));
inline const FName DisplayedTargetRange(TEXT("RailgunDisplayedTargetRange"));
inline const FName DisplayInitialized(TEXT("RailgunRangeDisplayInitialized"));
inline constexpr TCHAR MaximumCentimeters[] = TEXT("100000.0");
inline constexpr TCHAR MaximumDisplayedMeters[] = TEXT("999.0");
inline constexpr TCHAR CentimetersPerMeter[] = TEXT("100.0");
inline constexpr TCHAR NoHitLabel[] = TEXT("---");
inline constexpr TCHAR IntegralDigits[] = TEXT("3");
inline const FName UseGroupingPin(TEXT("bUseGrouping"));
inline const FName MinimumIntegralDigitsPin(TEXT("MinimumIntegralDigits"));
inline const FName MaximumIntegralDigitsPin(TEXT("MaximumIntegralDigits"));
}
namespace Range = StationRangeNames;

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
    // Compute a guaranteed runtime UObject-name fallback first, but do not
    // publish it until optional live Voyage item/name resolution has finished.
    auto* ObjectName = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName));
    G.Link(G.Pin(Hit, SP::HitActor), G.Pin(ObjectName, P::Object));
    auto* Fallback = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
    G.Link(G.Pin(ObjectName, P::ReturnValue), G.Pin(Fallback, E::StringValue));
    G.Write(Range::PendingTargetName, G.Pin(Fallback, P::ReturnValue));
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
    [[maybe_unused]] constexpr auto ComponentSignature = static_cast<UActorComponent* (AActor::*)(TSubclassOf<UActorComponent>) const>(&AActor::GetComponentByClass);
    auto* Component = G.Call(AActor::StaticClass(), OP::FindComponent);
    G.Link(G.Pin(Hit, SP::HitActor), G.Pin(Component, P::FunctionTarget)); G.Pin(Component, OP::ComponentClass)->DefaultObject = UVoyageModuleComponent::StaticClass();
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph); Cast->TargetType = UVoyageModuleComponent::StaticClass(); Cast->SetPurity(true); G.Node(Cast);
    G.Link(G.Pin(Component, P::ReturnValue), Cast->GetCastSourcePin());
    auto* ComponentValid = G.Branch(G.Valid(Cast->GetCastResultPin()));
    auto* Item = NewObject<UK2Node_VariableGet>(G.Graph);
    const FName ItemName = GET_MEMBER_NAME_CHECKED(UVoyageModuleComponent, ItemAsset);
    Item->VariableReference.SetExternalMember(ItemName, UVoyageModuleComponent::StaticClass()); G.Node(Item);
    G.Link(Cast->GetCastResultPin(), G.Pin(Item, P::FunctionTarget));
    auto* ItemValid = G.Branch(G.Valid(G.Pin(Item, ItemName)));
    auto* DataCast = NewObject<UK2Node_DynamicCast>(G.Graph); DataCast->TargetType = UVoyageBaseDataAsset::StaticClass(); DataCast->SetPurity(true); G.Node(DataCast);
    G.Link(G.Pin(Item, ItemName), DataCast->GetCastSourcePin());
    auto* DataValid = G.Branch(G.Valid(DataCast->GetCastResultPin()));
    auto* Name = NewObject<UK2Node_VariableGet>(G.Graph);
    const FName NameField = GET_MEMBER_NAME_CHECKED(UVoyageBaseDataAsset, Name);
    Name->VariableReference.SetExternalMember(NameField, UVoyageBaseDataAsset::StaticClass()); G.Node(Name);
    G.Link(DataCast->GetCastResultPin(), G.Pin(Name, P::FunctionTarget));
    auto* Empty = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, TextIsEmpty));
    G.Link(G.Pin(Name, NameField), G.Pin(Empty, E::WidgetText));
    auto* EmptyBranch = G.Branch(G.Pin(Empty, P::ReturnValue)); G.Tail = EmptyBranch->GetElsePin();
    G.Write(Range::PendingTargetName, G.Pin(Name, NameField));
    StationMerge(G, {G.Tail, G.Pin(EmptyBranch, P::Then),
        G.Pin(DataValid, P::Else), G.Pin(ItemValid, P::Else),
        G.Pin(ComponentValid, P::Else)});
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
