// Editor-only regression fixtures for peer Blueprint component-template defaults.
#pragma once
#include "Components/ChildActorComponent.h"

namespace BlueprintComponentDefaultsTests
{
constexpr TCHAR HelperPath[]=TEXT("/Game/BPTDefaultsHelper");
constexpr TCHAR OwnerPath[]=TEXT("/Game/BPTDefaultsOwner");
constexpr TCHAR FixtureMode[]=TEXT("ComponentDefaultsFixture");
constexpr TCHAR CheckMode[]=TEXT("ComponentDefaultsCheck");

UBlueprint* Create(const TCHAR* Path)
{
    if(FPackageName::DoesPackageExist(Path))return nullptr;
    return FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(),CreatePackage(Path),
        *FPackageName::GetLongPackageAssetName(Path),BPTYPE_Normal);
}
bool Compile(UBlueprint* BP)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);return BP->Status!=BS_Error && BP->GeneratedClass;
}
bool Save(UBlueprint* BP)
{
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;
    return UPackage::SavePackage(BP->GetOutermost(),BP,
        *FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
}
bool Fixture()
{
    auto* Helper=Create(HelperPath);if(!Helper)return false;
    FEdGraphPinType Bool;Bool.PinCategory=UEdGraphSchema_K2::PC_Boolean;
    FEdGraphPinType Real;Real.PinCategory=UEdGraphSchema_K2::PC_Real;Real.PinSubCategory=UEdGraphSchema_K2::PC_Double;
    if(!FBlueprintEditorUtils::AddMemberVariable(Helper,TEXT("Enabled"),Bool) ||
       !FBlueprintEditorUtils::AddMemberVariable(Helper,TEXT("Rate"),Real) || !Compile(Helper))return false;
    auto* Enabled=FindFProperty<FBoolProperty>(Helper->GeneratedClass,TEXT("Enabled"));
    auto* Rate=FindFProperty<FDoubleProperty>(Helper->GeneratedClass,TEXT("Rate"));
    auto* CDO=CastChecked<AActor>(Helper->GeneratedClass->GetDefaultObject());
    Enabled->SetPropertyValue_InContainer(CDO,true);Rate->SetPropertyValue_InContainer(CDO,1.5);
    CDO->PrimaryActorTick.TickGroup=TG_PostPhysics;
    if(!Save(Helper))return false;
    auto* Owner=Create(OwnerPath);if(!Owner)return false;
    for(const FName Name:{FName(TEXT("Inherited")),FName(TEXT("Overridden"))})
    {
        auto* Node=Owner->SimpleConstructionScript->CreateNode(UChildActorComponent::StaticClass(),Name);
        Owner->SimpleConstructionScript->AddNode(Node);
        auto* Component=CastChecked<UChildActorComponent>(Node->ComponentTemplate);
        Component->SetChildActorClass(Helper->GeneratedClass.Get());
        auto* Template=Component->GetChildActorTemplate();if(!Template)return false;
        // AActor construction initializes Tick separately; author the fixture's
        // native value explicitly so this template matches the final class CDO.
        Template->PrimaryActorTick.TickGroup=TG_PostPhysics;
        if(Name==TEXT("Overridden"))
        {
            Enabled->SetPropertyValue_InContainer(Template,false);
            Rate->SetPropertyValue_InContainer(Template,4.25);
            Template->PrimaryActorTick.TickGroup=TG_PostUpdateWork;
        }
    }
    return Compile(Owner) && Save(Owner);
}
bool Check()
{
    auto* Helper=LoadObject<UBlueprint>(nullptr,TEXT("/Game/BPTDefaultsHelper.BPTDefaultsHelper"));
    auto* Owner=LoadObject<UBlueprint>(nullptr,TEXT("/Game/BPTDefaultsOwner.BPTDefaultsOwner"));
    if(!Helper || !Owner)return false;
    auto* Enabled=FindFProperty<FBoolProperty>(Helper->GeneratedClass,TEXT("Enabled"));
    auto* Rate=FindFProperty<FDoubleProperty>(Helper->GeneratedClass,TEXT("Rate"));
    auto* CDO=Cast<AActor>(Helper->GeneratedClass->GetDefaultObject());
    if(!Enabled || !Rate || !CDO || !Enabled->GetPropertyValue_InContainer(CDO) ||
       Rate->GetPropertyValue_InContainer(CDO)!=1.5 || CDO->PrimaryActorTick.TickGroup!=TG_PostPhysics)return false;
    int32 Checked=0;
    for(auto* Node:Owner->SimpleConstructionScript->GetAllNodes())
    {
        auto* Component=Cast<UChildActorComponent>(Node->ComponentTemplate);if(!Component)continue;
        const bool Inherited=Node->GetVariableName()==TEXT("Inherited");
        if(!Inherited && Node->GetVariableName()!=TEXT("Overridden"))return false;
        auto* Template=Component->GetChildActorTemplate();
        if(!Template || Template->GetClass()!=Helper->GeneratedClass ||
           Enabled->GetPropertyValue_InContainer(Template)!=Inherited ||
           Rate->GetPropertyValue_InContainer(Template)!=(Inherited?1.5:4.25) ||
           Template->PrimaryActorTick.TickGroup!=(Inherited?TG_PostPhysics:TG_PostUpdateWork) ||
           Template->PrimaryActorTick.bCanEverTick!=CDO->PrimaryActorTick.bCanEverTick)
        {
            UE_LOG(LogTemp,Error,TEXT("Component defaults regression: %s; template=%s, enabled=%d, rate=%g, tickGroup=%d, canTick=%d"),
                *Node->GetVariableName().ToString(),*GetNameSafe(Template),
                Template?Enabled->GetPropertyValue_InContainer(Template):-1,
                Template?Rate->GetPropertyValue_InContainer(Template):-1.0,
                Template?int32(Template->PrimaryActorTick.TickGroup):-1,
                Template?int32(Template->PrimaryActorTick.bCanEverTick):-1);
            return false;
        }
        ++Checked;
    }
    return Checked==2;
}
}
