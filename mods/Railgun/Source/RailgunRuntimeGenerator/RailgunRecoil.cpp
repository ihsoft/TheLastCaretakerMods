#include "RailgunRecoil.h"

#include "GraphCallHelpers.h"
#include "RailgunWater.h"
#include "StationAttachmentGraph.h"
#include "StationEntryGraph.h"
#include "RailgunRuntimeGeneratorPrivate.h"

namespace Railgun::Runtime
{
void ClearRailgunRecoilVisited(FGraph& G)
{
    auto* Clear = G.ArrayCall(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Clear));
    G.Link(G.Read(RailgunRecoil::VisitedComponents),
        G.Pin(Clear, P::TargetArray));
    G.Exec(Clear);
}

UEdGraphPin* RailgunRecoilVisitedContains(FGraph& G, UEdGraphPin* Item)
{
    auto* Contains = G.ArrayCall(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Contains));
    G.Link(G.Read(RailgunRecoil::VisitedComponents),
        G.Pin(Contains, P::TargetArray));
    G.Link(Item, G.Pin(Contains, RailgunRecoil::ItemToFindPin));
    return G.Pin(Contains, P::ReturnValue);
}

void AddRailgunRecoilVisited(FGraph& G, UEdGraphPin* Item)
{
    auto* Add = G.ArrayCall(
        GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Add));
    G.Link(G.Read(RailgunRecoil::VisitedComponents),
        G.Pin(Add, P::TargetArray));
    G.Link(Item, G.Pin(Add, RailgunRecoil::NewItemPin));
    G.Exec(Add);
}

void ApplyRailgunShipRecoil(FGraph& G, UEdGraphPin* TypedRailgun)
{
    G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
        Greater_DoubleDouble), G.Read(RailgunRecoil::ShipStrength),
        RailgunRecoil::MinimumEnabledStrength));
    UEdGraphPin* Root = ObserveCall(G, AActor::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(AActor, K2_GetRootComponent), TypedRailgun);
    G.Branch(G.Valid(Root));
    G.Write(RailgunRecoil::SearchComponent, Root);
    ClearRailgunRecoilVisited(G);

    auto* Loop = RailgunWaterLoop(G, nullptr,
        RailgunRecoil::MaximumSearchIndex, N::Zero);
    auto* CurrentValid = G.Branch(
        G.Valid(G.Read(RailgunRecoil::SearchComponent)));
    G.Link(G.Pin(CurrentValid, P::Else),
        G.Pin(Loop, RailgunWater::BreakLoop));
    auto* Repeated = G.Branch(RailgunRecoilVisitedContains(G,
        G.Read(RailgunRecoil::SearchComponent)));
    G.Link(G.Tail, G.Pin(Loop, RailgunWater::BreakLoop));
    G.Tail = G.Pin(Repeated, P::Else);
    AddRailgunRecoilVisited(G, G.Read(RailgunRecoil::SearchComponent));
    UEdGraphPin* Parent = ObserveCall(G, USceneComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(USceneComponent, GetAttachParent),
        G.Read(RailgunRecoil::SearchComponent));

    auto* Primitive = NewObject<UK2Node_DynamicCast>(G.Graph);
    Primitive->TargetType = UPrimitiveComponent::StaticClass();
    Primitive->SetPurity(false);
    G.Node(Primitive);
    G.Link(G.Tail, G.Pin(Primitive, P::Execute));
    G.Link(G.Read(RailgunRecoil::SearchComponent),
        Primitive->GetCastSourcePin());
    G.Tail = Primitive->GetValidCastPin();
    UEdGraphPin* ContinueFromCastFailure = Primitive->GetInvalidCastPin();

    UEdGraphPin* Simulating = ObserveCall(G,
        UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent,
            IsSimulatingPhysics),
        Primitive->GetCastResultPin());
    auto* IsSimulating = G.Branch(Simulating);
    UEdGraphPin* ContinueFromStatic = G.Pin(IsSimulating, P::Else);
    UEdGraphPin* Mass = ObserveCall(G, UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent, GetMass),
        Primitive->GetCastResultPin());
    UEdGraphPin* PositiveMass = G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Greater_DoubleDouble), Mass, N::Zero);
    UEdGraphPin* FiniteMass = G.Compare(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            LessEqual_DoubleDouble), Mass,
        RailgunRecoil::MaximumFiniteMass);
    auto* UsableMass = G.Branch(G.Binary(
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND),
        PositiveMass, FiniteMass));
    UEdGraphPin* ContinueFromInvalidMass = G.Pin(UsableMass, P::Else);

    auto* MassAndStrength = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Multiply_DoubleDouble));
    G.Link(Mass, G.Pin(MassAndStrength, P::Binary::LeftOperand));
    G.Link(G.Read(RailgunRecoil::ShipStrength),
        G.Pin(MassAndStrength, P::Binary::RightOperand));
    auto* ImpulseMagnitude = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Multiply_DoubleDouble));
    G.Link(G.Pin(MassAndStrength, P::ReturnValue),
        G.Pin(ImpulseMagnitude, P::Binary::LeftOperand));
    G.Default(ImpulseMagnitude, P::Binary::RightOperand,
        RailgunRecoil::BaselineVelocityChangeCmPerSecond);
    auto* ImpulseVector = G.Call(UKismetMathLibrary::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary,
            Multiply_VectorFloat));
    G.Link(G.Read(RailgunRecoil::ShotDirection),
        G.Pin(ImpulseVector, P::Binary::LeftOperand));
    G.Link(G.Pin(ImpulseMagnitude, P::ReturnValue),
        G.Pin(ImpulseVector, P::Binary::RightOperand));
    auto* AddImpulse = G.Call(UPrimitiveComponent::StaticClass(),
        GET_FUNCTION_NAME_CHECKED(UPrimitiveComponent,
            AddImpulseAtLocation));
    G.Link(Primitive->GetCastResultPin(),
        G.Pin(AddImpulse, P::FunctionTarget));
    G.Link(G.Pin(ImpulseVector, P::ReturnValue),
        G.Pin(AddImpulse, RailgunRecoil::ImpulsePin));
    G.Link(G.Read(RailgunRecoil::ModuleLocation),
        G.Pin(AddImpulse, RailgunRecoil::LocationPin));
    G.Default(AddImpulse, RailgunRecoil::BoneNamePin,
        RailgunRecoil::None);
    G.Exec(AddImpulse);
    G.Link(G.Tail, G.Pin(Loop, RailgunWater::BreakLoop));

    StationMerge(G, {ContinueFromCastFailure, ContinueFromStatic,
        ContinueFromInvalidMass});
    G.Write(RailgunRecoil::SearchComponent, Parent);
    G.Tail = G.Pin(Loop, CE::Completed);
}
}
