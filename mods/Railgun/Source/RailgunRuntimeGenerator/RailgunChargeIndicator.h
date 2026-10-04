#pragma once

namespace RailgunChargeIndicator
{
inline const FName Owner(TEXT("RailgunChargeIndicatorOwner"));
inline const FName Component(TEXT("RailgunChargeIndicatorComponent"));
inline const FName Material(TEXT("RailgunChargeIndicatorMaterial"));
inline const FName LastLevel(TEXT("RailgunChargeIndicatorLastLevel"));
inline const FName ElementIndexPin(TEXT("ElementIndex"));
inline const FName OptionalNamePin(TEXT("OptionalName"));
inline const FName SourceMaterialPin(TEXT("SourceMaterial"));
inline const FName ParameterNamePin(TEXT("ParameterName"));
inline const FName ValuePin(TEXT("Value"));
inline const FName DoubleInputPin(TEXT("InDouble"));
inline const FName DynamicMaterialName(TEXT("RailgunChargeIndicatorMID"));
inline constexpr TCHAR FullLevel[] = TEXT("1.0");
}

UK2Node_MacroInstance* ContextLoop(FGraph& G, UEdGraphPin* Values);

void SetRailgunChargeIndicatorLevel(FGraph& G, UEdGraphPin* Level)
{
    auto* Set = G.Call(UMaterialInstanceDynamic::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UMaterialInstanceDynamic,
            SetScalarParameterValue));
    G.Link(G.Read(RailgunChargeIndicator::Material),
        G.Pin(Set, P::FunctionTarget));
    G.Default(Set, RailgunChargeIndicator::ParameterNamePin,
        *RailgunModelContract::ChargeIndicatorProgressParameter.ToString());
    if (Level)
    {
        auto* ToFloat = G.Call(UKismetMathLibrary::StaticClass(),
            GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
                Conv_DoubleToFloat));
        G.Link(Level,
            G.Pin(ToFloat, RailgunChargeIndicator::DoubleInputPin));
        G.Link(G.Pin(ToFloat, P::ReturnValue),
            G.Pin(Set, RailgunChargeIndicator::ValuePin));
    }
    else
    {
        G.Default(Set, RailgunChargeIndicator::ValuePin, N::Zero);
    }
    G.Exec(Set);
}

void EnsureRailgunChargeIndicator(FGraph& G)
{
    G.Branch(G.Valid(G.Read(S::Anchor)));
    UEdGraphPin* CurrentOwner = ObserveCall(G, UActorComponent::StaticClass(),
        OP::ComponentOwner, G.Read(S::Anchor));
    G.Branch(G.Valid(CurrentOwner));
    UEdGraphPin* OwnerMatches = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject),
        CurrentOwner, G.Read(RailgunChargeIndicator::Owner));
    UEdGraphPin* CacheObjectsValid = G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        G.Valid(G.Read(RailgunChargeIndicator::Component)),
        G.Valid(G.Read(RailgunChargeIndicator::Material)));
    auto* CacheReady = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        OwnerMatches, CacheObjectsValid));
    UEdGraphPin* CachedTail = G.Tail;

    G.Tail = G.Pin(CacheReady, P::Else);
    auto* Find = G.Call(AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, GetComponentsByTag));
    G.Link(CurrentOwner, G.Pin(Find, P::FunctionTarget));
    G.Pin(Find, OP::ComponentClass)->DefaultObject =
        UStaticMeshComponent::StaticClass();
    G.Default(Find, ActorScanGraphNames::ComponentTag,
        *RailgunModelContract::ChargeIndicatorTag.ToString());
    auto* Loop = ContextLoop(G, G.Pin(Find, P::ReturnValue));
    auto* Cast = NewObject<UK2Node_DynamicCast>(G.Graph);
    Cast->TargetType = UStaticMeshComponent::StaticClass();
    Cast->SetPurity(false);
    G.Node(Cast);
    G.Link(G.Tail, G.Pin(Cast, P::Execute));
    G.Link(G.Pin(Loop, CE::ArrayElement), Cast->GetCastSourcePin());
    G.Tail = Cast->GetValidCastPin();
    UEdGraphPin* IndicatorComponent = Cast->GetCastResultPin();

    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftObjectPath));
    G.Default(Path, E::PathString,
        RailgunModelContract::ChargeIndicatorMaterialObjectPath);
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
    auto* MaterialCast = NewObject<UK2Node_DynamicCast>(G.Graph);
    MaterialCast->TargetType = UMaterialInterface::StaticClass();
    MaterialCast->SetPurity(false);
    G.Node(MaterialCast);
    G.Link(G.Tail, G.Pin(MaterialCast, P::Execute));
    G.Link(G.Pin(Load, P::ReturnValue), MaterialCast->GetCastSourcePin());
    G.Tail = MaterialCast->GetValidCastPin();

    auto* Create = G.Call(UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent,
            CreateDynamicMaterialInstance));
    G.Link(IndicatorComponent, G.Pin(Create, P::FunctionTarget));
    G.Default(Create, RailgunChargeIndicator::ElementIndexPin, N::Zero);
    G.Link(MaterialCast->GetCastResultPin(),
        G.Pin(Create, RailgunChargeIndicator::SourceMaterialPin));
    G.Default(Create, RailgunChargeIndicator::OptionalNamePin,
        *RailgunChargeIndicator::DynamicMaterialName.ToString());
    G.Exec(Create);
    UEdGraphPin* DynamicMaterial = G.Pin(Create, P::ReturnValue);
    G.Branch(G.Valid(DynamicMaterial));
    G.Write(RailgunChargeIndicator::Owner, CurrentOwner);
    G.Write(RailgunChargeIndicator::Component, IndicatorComponent);
    G.Write(RailgunChargeIndicator::Material, DynamicMaterial);
    SetRailgunChargeIndicatorLevel(G, nullptr);
    G.Write(RailgunChargeIndicator::LastLevel, nullptr, N::Zero);
    StationMerge(G, {CachedTail, G.Tail});
}

void UpdateRailgunChargeIndicator(FGraph& G, UEdGraphPin* StoredEnergy)
{
    auto* Ratio = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Divide_DoubleDouble));
    G.Link(StoredEnergy, G.Pin(Ratio, P::Binary::LeftOperand));
    G.Link(RequiredEnergyAmount(G), G.Pin(Ratio, P::Binary::RightOperand));
    auto* Clamp = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, FClamp));
    G.Link(G.Pin(Ratio, P::ReturnValue), G.Pin(Clamp, OP::ClampValue));
    G.Default(Clamp, OP::ClampMinimum, N::Zero);
    G.Default(Clamp, OP::ClampMaximum,
        RailgunChargeIndicator::FullLevel);
    UEdGraphPin* Level = G.Pin(Clamp, P::ReturnValue);
    G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_DoubleDouble),
        Level, G.Read(RailgunChargeIndicator::LastLevel)));
    SetRailgunChargeIndicatorLevel(G, Level);
    G.Write(RailgunChargeIndicator::LastLevel, Level);
}
