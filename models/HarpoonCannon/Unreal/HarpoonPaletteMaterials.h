// Model-owned, editor-only material factory. No geometry or gameplay mutation.
// Shared shell hook coordinated with its owner; runtime validation is separate.
#pragma once

#include "HarpoonPaletteData.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "UObject/Package.h"

namespace HarpoonPaletteMaterials
{
inline constexpr TCHAR MaterialPrefix[] = TEXT("MI_HarpoonPalette_");
inline constexpr TCHAR MasterName[] = TEXT("M_HarpoonPaletteSurface");
inline constexpr TCHAR BaseColorNode[] = TEXT("BaseColor");
inline constexpr TCHAR RoughnessNode[] = TEXT("Roughness");
inline constexpr TCHAR MetallicNode[] = TEXT("Metallic");
inline constexpr TCHAR EmissiveNode[] = TEXT("Emissive");

inline UMaterialExpressionVectorParameter* Color(
    UMaterial* Material, const TCHAR* Name, const FLinearColor& Value)
{
    auto* Expression = NewObject<UMaterialExpressionVectorParameter>(Material, FName(Name));
    Expression->ParameterName = FName(Name);
    Expression->DefaultValue = Value;
    Expression->UpdateParameterGuid(true, true);
    Material->GetExpressionCollection().AddExpression(Expression);
    return Expression;
}

inline UMaterialExpressionScalarParameter* Scalar(
    UMaterial* Material, const TCHAR* Name, const float Value)
{
    auto* Expression = NewObject<UMaterialExpressionScalarParameter>(Material, FName(Name));
    Expression->ParameterName = FName(Name);
    Expression->DefaultValue = Value;
    Expression->UpdateParameterGuid(true, true);
    Material->GetExpressionCollection().AddExpression(Expression);
    return Expression;
}

// Proposed integration: package-local sibling materials, referenced by mesh slots.
// This preserves package path inventory; cook/export/shader retention MUST be
// checked before installation. Never silently fall back to a grey material.
inline UMaterialInterface* Create(UPackage* MeshPackage, const FString& SlotName)
{
    const HarpoonPaletteData::FSlot* Slot = nullptr;
    for (const auto& Candidate : HarpoonPaletteData::Slots)
    {
        if (SlotName.Equals(Candidate.Name, ESearchCase::CaseSensitive))
        {
            Slot = &Candidate;
            break;
        }
    }
    if (!Slot)
    {
        UE_LOG(LogTemp, Error, TEXT("No model palette entry for material slot %s"), *SlotName);
        return nullptr;
    }
    UMaterial* Master = FindObject<UMaterial>(MeshPackage, MasterName);
    if (!Master)
    {
        Master = NewObject<UMaterial>(MeshPackage, FName(MasterName), RF_Public | RF_Standalone);
        if (!Master) return nullptr;
        Master->MaterialDomain = MD_Surface;
        Master->BlendMode = BLEND_Opaque;
        Master->TwoSided = false;
        Master->SetShadingModel(MSM_DefaultLit);
        auto* Data = Master->GetEditorOnlyData();
        Data->BaseColor.Expression = Color(Master, BaseColorNode, FLinearColor::Gray);
        Data->Roughness.Expression = Scalar(Master, RoughnessNode, 0.65f);
        Data->Metallic.Expression = Scalar(Master, MetallicNode, 0.0f);
        Data->EmissiveColor.Expression = Color(Master, EmissiveNode, FLinearColor::Black);
        Master->PostEditChange();
        Master->MarkPackageDirty();
    }
    const FName Name(*(FString(MaterialPrefix) + SlotName));
    auto* Instance = NewObject<UMaterialInstanceConstant>(MeshPackage, Name, RF_Public | RF_Standalone);
    if (!Instance) return nullptr;
    Instance->SetParentEditorOnly(Master);
    Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(FName(BaseColorNode)), Slot->BaseColor);
    Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(FName(EmissiveNode)), Slot->Emissive);
    Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName(RoughnessNode)), Slot->Roughness);
    Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(FName(MetallicNode)), Slot->Metallic);
    Instance->PostEditChange();
    Instance->MarkPackageDirty();
    return Instance;
}
}
