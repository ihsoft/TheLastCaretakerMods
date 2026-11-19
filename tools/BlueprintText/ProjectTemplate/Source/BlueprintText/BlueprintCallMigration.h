#pragma once

// One-time compatibility migration. Move the existing pins, including their
// identities, literal values and links; do not reconstruct or reinterpret them.
namespace BlueprintCallMigration
{
bool Migrate(const FString& Asset)
{
    auto* BP=LoadObject<UBlueprint>(nullptr,*Asset);if(!BP)return false;
    TArray<UEdGraph*> Graphs;BP->GetAllGraphs(Graphs);int32 Count=0;
    for(UEdGraph* Graph:Graphs)for(int32 I=0;I<Graph->Nodes.Num();++I)
    {
        auto* Old=Cast<UBlueprintTextCallFunction>(Graph->Nodes[I]);if(!Old)continue;
        const FName Name=Old->GetFName();
        auto* Call=NewObject<UK2Node_CallFunction>(Graph,NAME_None,Old->GetFlags());
        for(TFieldIterator<FProperty> It(UK2Node_CallFunction::StaticClass());It;++It)
            It->CopyCompleteValue_InContainer(Call,Old);
        Call->Pins=MoveTemp(Old->Pins);
        for(UEdGraphPin* Pin:Call->Pins)Pin->SetOwningNode(Call);
        Graph->Nodes[I]=Call;
        if(!Old->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional) ||
           !Call->Rename(*Name.ToString(),Graph,REN_DontCreateRedirectors|REN_NonTransactional))return false;
        ++Count;
    }
    if(!Count)return true;
    FKismetEditorUtilities::CompileBlueprint(BP);
    if(BP->Status==BS_Error || !BP->GeneratedClass)return false;
    auto Root=MakeShared<FJsonObject>();Root->SetStringField(ATNames::Valueasset,Asset);
    UE_LOG(LogTemp,Display,TEXT("Migrated %d legacy BPT calls in %s"),Count,*Asset);
    return AssetText::SaveImport(BP,Root);
}
}
