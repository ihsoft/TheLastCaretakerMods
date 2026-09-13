#pragma once

namespace StationHudNames
{
inline const FName NativeWidget(TEXT("CapturedPlayingWidget"));
inline const FName PriorVisibility(TEXT("PlayingWidgetVisibilityBefore"));
inline const FName OwnsVisibility(TEXT("OwnsPlayingWidgetVisibility"));
inline const FName CharacterWidget(TEXT("CapturedCharacterHud"));
inline const FName CharacterPriorVisibility(TEXT("CharacterHudVisibilityBefore"));
inline const FName OwnsCharacterVisibility(TEXT("OwnsCharacterHudVisibility"));
inline const FName ScopePanel(TEXT("HarpoonScopePanel"));
inline const FName ScopeRows(TEXT("HarpoonScopeRows"));
inline const FName ScopeFooter(TEXT("HarpoonScopeFooter"));
inline constexpr TCHAR PlayingClass[] = TEXT("/Game/UI/GameStateWidgets/BP_PlayingWidget.BP_PlayingWidget_C");
inline constexpr TCHAR CharacterClass[] = TEXT("/Game/UI/Game/HUD/BP_VoyageIngameHud.BP_VoyageIngameHud_C");
inline constexpr TCHAR One[] = TEXT("1");
inline constexpr TCHAR CollapsedByte[] = TEXT("1");
inline constexpr TCHAR HiddenByte[] = TEXT("2");
inline constexpr TCHAR Refused[] = TEXT("HC16: playing HUD not ready; entry cancelled");
inline constexpr TCHAR CharacterRefused[] = TEXT("HC16: character HUD not ready; entry cancelled");
struct FRootContract { FName Widget; FName Prior; FName Owned; const TCHAR* ClassPath; const TCHAR* Refusal; };
inline const FRootContract Roots[] = {
    {NativeWidget, PriorVisibility, OwnsVisibility, PlayingClass, Refused},
    {CharacterWidget, CharacterPriorVisibility, OwnsCharacterVisibility, CharacterClass, CharacterRefused}
};
inline constexpr TCHAR Lost[] = TEXT("HarpoonCannon: interface changed; released");
inline constexpr TCHAR Footer[] = TEXT("x5  |  F8 EXIT");
inline constexpr int32 TargetFontSize = 20;
inline constexpr int32 NoticeFontSize = 14;
inline const FVector2D TargetSize(650.0f, 105.0f);
inline const FVector2D TargetOffset(0.0f, 45.0f);
inline const FVector2D NoticeSize(850.0f, 40.0f);
inline const FVector2D NoticeAnchor(0.5f, 1.0f);
inline const FVector2D NoticeOffset(0.0f, -60.0f);
inline const FVector2D ShadowOffset(1.0f, 1.0f);
}
namespace H = StationHudNames;

UEdGraphPin* CapturedVisibility(FGraph& G, const H::FRootContract& Root)
{
    return ObserveCall(G, UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, GetVisibility), G.Read(Root.Widget));
}

void SetCapturedVisibility(FGraph& G, const H::FRootContract& Root, UEdGraphPin* Captured = nullptr)
{
    auto* Set = G.Call(UWidget::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidget, SetVisibility));
    G.Link(G.Read(Root.Widget), G.Pin(Set, P::FunctionTarget));
    if (Captured) G.Link(Captured, G.Pin(Set, OP::Visibility));
    else G.Default(Set, OP::Visibility, O::Collapsed);
    G.Exec(Set);
}

void PrepareHudRoot(FGraph& G, const H::FRootContract& Root)
{
    // Resolve an already-loaded stock class. No load, spawn, replacement or native mirror.
    auto* Path = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeSoftClassPath));
    G.Default(Path, E::PathString, Root.ClassPath);
    auto* Ref = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftClassPathToSoftClassRef));
    G.Link(G.Pin(Path, P::ReturnValue), G.Pin(Ref, E::SoftClassPath));
    auto* Class = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, Conv_SoftClassReferenceToClass));
    G.Link(G.Pin(Ref, P::ReturnValue), G.Pin(Class, OP::SoftClassReference));
    auto* Cast = NewObject<UK2Node_ClassDynamicCast>(G.Graph); Cast->TargetType = UUserWidget::StaticClass(); Cast->SetPurity(true); G.Node(Cast);
    G.Link(G.Pin(Class, P::ReturnValue), Cast->GetCastSourcePin()); G.Require(Cast->GetBoolSuccessPin(), Root.Refusal);
    auto* Find = G.Call(UWidgetBlueprintLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UWidgetBlueprintLibrary, GetAllWidgetsOfClass));
    G.Link(Cast->GetCastResultPin(), G.Pin(Find, OP::WidgetClass)); G.Default(Find, OP::TopLevelOnly, N::False); G.Exec(Find);
    auto* LengthFunction = UKismetArrayLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length));
    check(LengthFunction);
    auto* Count = NewObject<UK2Node_CallArrayFunction>(G.Graph); Count->SetFromFunction(LengthFunction); G.Node(Count);
    G.Link(G.Pin(Find, OP::FoundWidgets), G.Pin(Count, P::TargetArray));
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_IntInt), G.Pin(Count, P::ReturnValue), H::One), Root.Refusal);
    // Only read the object reference; no array element is ever mutated.
    auto* Item = G.Node(NewObject<UK2Node_GetArrayItem>(G.Graph));
    G.Link(G.Pin(Find, OP::FoundWidgets), Item->GetTargetArrayPin());
    G.Write(Root.Widget, Item->GetResultPin()); G.Require(G.Valid(G.Read(Root.Widget)), Root.Refusal);
    [[maybe_unused]] constexpr auto OwnerSignature = static_cast<APlayerController* (UWidget::*)() const>(&UWidget::GetOwningPlayer);
    auto* Owner = ObserveCall(G, UWidget::StaticClass(), OP::OwningPlayerGetter, G.Read(Root.Widget));
    G.Require(G.Binary(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ObjectObject), Owner, G.Read(S::Controller)), Root.Refusal);
    G.Write(Root.Prior, CapturedVisibility(G, Root));
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_ByteByte), G.Read(Root.Prior), H::CollapsedByte), Root.Refusal);
    G.Require(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, NotEqual_ByteByte), G.Read(Root.Prior), H::HiddenByte), Root.Refusal);
}

void PrepareScopeHud(FGraph& G)
{
    for (const auto& Root : H::Roots) PrepareHudRoot(G, Root);
}

void BeginScopeHud(FGraph& G)
{
    for (const auto& Root : H::Roots) { G.Write(Root.Owned, nullptr, N::True); SetCapturedVisibility(G, Root); }
}

void ReleaseScopeHud(FGraph& G)
{
    for (const auto& Root : H::Roots)
    {
        auto* Owned = G.Branch(G.Read(Root.Owned)); G.Write(Root.Owned, nullptr, N::False);
        auto* Valid = G.Branch(G.Valid(G.Read(Root.Widget)));
        auto* StillOurs = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), CapturedVisibility(G, Root), H::CollapsedByte));
        SetCapturedVisibility(G, Root, G.Read(Root.Prior));
        StationMerge(G, {G.Tail, G.Pin(StillOurs, P::Else), G.Pin(Valid, P::Else), G.Pin(Owned, P::Else)});
    }
}

void CheckScopeHud(FGraph& G)
{
    for (const auto& Root : H::Roots)
    {
        auto* Valid = G.Branch(G.Valid(G.Read(Root.Widget))); auto* Good = G.Tail;
        G.Tail = G.Pin(Valid, P::Else); ReleaseStation(G); G.Text(N::FreezeStatus, H::Lost); G.Tail = Good;
        auto* Hidden = G.Branch(G.Compare(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_ByteByte), CapturedVisibility(G, Root), H::CollapsedByte)); Good = G.Tail;
        G.Tail = G.Pin(Hidden, P::Else); ReleaseStation(G); G.Text(N::FreezeStatus, H::Lost); G.Tail = Good;
    }
}
