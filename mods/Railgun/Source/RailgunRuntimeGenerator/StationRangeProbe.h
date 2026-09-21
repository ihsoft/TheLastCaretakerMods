#pragma once

namespace StationRangeNames
{
// Retain the existing widget/property identity; HC33 generalizes its contents.
inline const FName TargetName(TEXT("DetectedSharkName"));
inline const FName TargetRange(TEXT("OpticalTargetRange"));
inline constexpr TCHAR MaximumCentimeters[] = TEXT("100000.0");
inline constexpr TCHAR MaximumDisplayedMeters[] = TEXT("999.0");
inline constexpr TCHAR CentimetersPerMeter[] = TEXT("100.0");
inline constexpr TCHAR NoHit[] = TEXT("RANGE: no hit within 1000m");
inline constexpr TCHAR NoHitLabel[] = TEXT("---");
inline constexpr TCHAR Distance[] = TEXT("TARGET distance from sight (m): ");
inline constexpr TCHAR IntegralDigits[] = TEXT("3");
inline const FName UseGroupingPin(TEXT("bUseGrouping"));
inline const FName MinimumIntegralDigitsPin(TEXT("MinimumIntegralDigits"));
inline const FName MaximumIntegralDigitsPin(TEXT("MaximumIntegralDigits"));
}
namespace Range = StationRangeNames;

void UpdateStationRange(FGraph& G, bool StoreOnStation = false)
{
    auto TargetText = [&](FName Field, UEdGraphPin* Value = nullptr)
    {
        if (!StoreOnStation) { G.Text(Field, N::EmptyText, Value); return; }
        if (!Value)
        {
            auto* Empty = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
            G.Default(Empty, E::StringValue, N::EmptyText); Value = G.Pin(Empty, P::ReturnValue);
        }
        G.Write(Field, Value);
    };
    // First blocking optical hit only; any actor is a display target, not a fireable target.
    if (!StoreOnStation) G.Text(N::Current, Range::NoHit);
    TargetText(Range::TargetName);
    auto* NoHitText = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
    G.Default(NoHitText, E::StringValue, Range::NoHitLabel); TargetText(Range::TargetRange, G.Pin(NoHitText, P::ReturnValue));
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
    G.Branch(G.Pin(Trace, P::ReturnValue));
    auto* Hit = G.Call(UGameplayStatics::StaticClass(), GET_FUNCTION_NAME_CHECKED(UGameplayStatics, BreakHitResult));
    G.Link(G.Pin(Trace, E::OutHit), G.Pin(Hit, E::Hit)); G.Branch(G.Valid(G.Pin(Hit, SP::HitActor)));
    // Write a guaranteed runtime UObject-name fallback and range BEFORE optional
    // Voyage item lookup. Missing component/data/name must not erase a real hit.
    auto* ObjectName = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, GetObjectName));
    G.Link(G.Pin(Hit, SP::HitActor), G.Pin(ObjectName, P::Object));
    auto* Fallback = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText));
    G.Link(G.Pin(ObjectName, P::ReturnValue), G.Pin(Fallback, E::StringValue)); TargetText(Range::TargetName, G.Pin(Fallback, P::ReturnValue));
    auto* Length = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, VSize));
    G.Link(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Subtract_VectorVector), G.Pin(Hit, OP::ImpactPoint), Start), G.Pin(Length, E::VectorLengthInput));
    auto* Meters = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble));
    G.Link(G.Pin(Length, P::ReturnValue), G.Pin(Meters, P::Binary::LeftOperand)); G.Default(Meters, P::Binary::RightOperand, Range::CentimetersPerMeter);
    auto* ClampedMeters = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FMin));
    G.Link(G.Pin(Meters, P::ReturnValue), G.Pin(ClampedMeters, P::Binary::LeftOperand));
    G.Default(ClampedMeters, P::Binary::RightOperand, Range::MaximumDisplayedMeters);
    auto* Rounded = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Round));
    G.Link(G.Pin(ClampedMeters, P::ReturnValue), G.Pin(Rounded, OP::AngleValue));
    if (!StoreOnStation) G.Number(N::Current, Range::Distance, G.Pin(Rounded, P::ReturnValue));
    auto* Digits = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, Conv_IntToText));
    G.Link(G.Pin(Rounded, P::ReturnValue), G.Pin(Digits, P::Value));
    G.Default(Digits, Range::UseGroupingPin, N::False);
    G.Default(Digits, Range::MinimumIntegralDigitsPin, Range::IntegralDigits);
    G.Default(Digits, Range::MaximumIntegralDigitsPin, Range::IntegralDigits);
    TargetText(Range::TargetRange, G.Pin(Digits, P::ReturnValue));
    [[maybe_unused]] constexpr auto ComponentSignature = static_cast<UActorComponent* (AActor::*)(TSubclassOf<UActorComponent>) const>(&AActor::GetComponentByClass);
    auto* Component = G.Call(AActor::StaticClass(), OP::FindComponent);
    G.Link(G.Pin(Hit, SP::HitActor), G.Pin(Component, P::FunctionTarget)); G.Pin(Component, OP::ComponentClass)->DefaultObject = UVoyageModuleComponent::StaticClass();
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph); Cast->TargetType = UVoyageModuleComponent::StaticClass(); Cast->SetPurity(true); G.Node(Cast);
    G.Link(G.Pin(Component, P::ReturnValue), Cast->GetCastSourcePin()); G.Branch(G.Valid(Cast->GetCastResultPin()));
    auto* Item = NewObject<UK2Node_VariableGet>(G.Graph);
    const FName ItemName = GET_MEMBER_NAME_CHECKED(UVoyageModuleComponent, ItemAsset);
    Item->VariableReference.SetExternalMember(ItemName, UVoyageModuleComponent::StaticClass()); G.Node(Item);
    G.Link(Cast->GetCastResultPin(), G.Pin(Item, P::FunctionTarget)); G.Branch(G.Valid(G.Pin(Item, ItemName)));
    auto* DataCast = NewObject<UK2Node_DynamicCast>(G.Graph); DataCast->TargetType = UVoyageBaseDataAsset::StaticClass(); DataCast->SetPurity(true); G.Node(DataCast);
    G.Link(G.Pin(Item, ItemName), DataCast->GetCastSourcePin()); G.Branch(G.Valid(DataCast->GetCastResultPin()));
    auto* Name = NewObject<UK2Node_VariableGet>(G.Graph);
    const FName NameField = GET_MEMBER_NAME_CHECKED(UVoyageBaseDataAsset, Name);
    Name->VariableReference.SetExternalMember(NameField, UVoyageBaseDataAsset::StaticClass()); G.Node(Name);
    G.Link(DataCast->GetCastResultPin(), G.Pin(Name, P::FunctionTarget));
    auto* Empty = G.Call(UKismetTextLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetTextLibrary, TextIsEmpty));
    G.Link(G.Pin(Name, NameField), G.Pin(Empty, E::WidgetText));
    auto* EmptyBranch = G.Branch(G.Pin(Empty, P::ReturnValue)); G.Tail = EmptyBranch->GetElsePin();
    TargetText(Range::TargetName, G.Pin(Name, NameField));
}
