// Bounded Collapsed Graph ownership. Macro definitions and arbitrary inner objects
// are deliberately outside this contract.
#pragma once
#include "K2Node_Composite.h"
#include "K2Node_Tunnel.h"
#include "Exporters/Exporter.h"
#include "UnrealExporter.h"
#include "Misc/OutputDevice.h"
#include "Misc/StringOutputDevice.h"

namespace BlueprintNestedGraphs
{
constexpr TCHAR Owner[]=TEXT("owner");
constexpr TCHAR GraphKey[]=TEXT("graph");
constexpr TCHAR NodeKey[]=TEXT("node");
constexpr TCHAR EntryKey[]=TEXT("entry");
constexpr TCHAR ExitKey[]=TEXT("exit");
constexpr TCHAR Kind[]=TEXT("collapsed");

FString Validate(const UBlueprint* BP)
{
    TArray<UEdGraph*> All;BP->GetAllGraphs(All);
    TSet<const UEdGraph*> Seen;
    TSet<FGuid> Ids;
    TFunction<bool(const UEdGraph*)> Visit=[&](const UEdGraph* G)
    {
        if(!G || Seen.Contains(G) || !G->GraphGuid.IsValid() || Ids.Contains(G->GraphGuid) ||
           G->GetSchema()->GetClass()!=UEdGraphSchema_K2::StaticClass())return false;
        Seen.Add(G);Ids.Add(G->GraphGuid);
        TSet<const UEdGraph*> Children;
        for(const UEdGraphNode* N:G->Nodes)
        {
            if(!N || N->GetOuter()!=G)return false;
            if(const auto* C=Cast<UK2Node_Composite>(N))
            {
                const auto* Child=C->BoundGraph.Get();
                const auto* Entry=Cast<UK2Node_Tunnel>(C->InputSinkNode);
                const auto* Exit=Cast<UK2Node_Tunnel>(C->OutputSourceNode);
                if(!Child || Child->GetOuter()!=C || !Entry || !Exit || Entry==Exit ||
                   Entry->GetClass()!=UK2Node_Tunnel::StaticClass() || Exit->GetClass()!=UK2Node_Tunnel::StaticClass() ||
                   !Child->Nodes.Contains(Entry) || !Child->Nodes.Contains(Exit) ||
                   Entry->OutputSourceNode!=C || Entry->InputSinkNode || !Entry->bCanHaveOutputs || Entry->bCanHaveInputs ||
                   Exit->InputSinkNode!=C || Exit->OutputSourceNode || !Exit->bCanHaveInputs || Exit->bCanHaveOutputs)return false;
                int32 BoundaryCount=0;
                for(const UK2Node_Tunnel* T:{Entry,Exit})for(const UEdGraphPin* P:T->Pins)
                {
                    const auto Direction=UEdGraphPin::GetComplementaryDirection(P->Direction);
                    const auto* Mirror=C->FindPin(P->PinName,Direction);
                    if(!Mirror || Mirror->PinType!=P->PinType)return false;
                    ++BoundaryCount;
                }
                if(BoundaryCount!=C->Pins.Num() || !Visit(Child))return false;
                Children.Add(Child);
            }
            else if(const auto* T=Cast<UK2Node_Tunnel>(N))
            {
                // Function entries/results derive from Tunnel too.
                if(T->GetClass()==UK2Node_Tunnel::StaticClass())
                {
                    const auto* ParentComposite=Cast<UK2Node_Composite>(G->GetOuter());
                    if(!ParentComposite || (ParentComposite->InputSinkNode!=T && ParentComposite->OutputSourceNode!=T))return false;
                }
            }
        }
        if(Children.Num()!=G->SubGraphs.Num())return false;
        for(const UEdGraph* Child:G->SubGraphs)if(!Children.Contains(Child))return false;
        return true;
    };
    for(const UEdGraph* G:BP->UbergraphPages)if(!Visit(G))return TEXT("Invalid collapsed graph ownership or tunnel boundary.");
    for(const UEdGraph* G:BP->FunctionGraphs)if(!Visit(G))return TEXT("Invalid collapsed graph ownership or tunnel boundary.");
    for(const auto& I:BP->ImplementedInterfaces)for(const UEdGraph* G:I.Graphs)
        if(!Visit(G))return TEXT("Invalid collapsed graph ownership or tunnel boundary.");
    if(Seen.Num()!=All.Num())return TEXT("Unsupported graph outside the collapsed graph hierarchy.");
    return FString();
}

FString ExportNodes(UEdGraph* Graph,const TArray<UObject*>& Graphs)
{
    // Export each graph separately, without temporarily modifying the asset.
    // Ownership pointers have a single representation: the child's owner record.
    const FExportObjectInnerContext Context(Graphs);
    UnMarkAllObjects(EObjectMark(OBJECTMARK_TagExp|OBJECTMARK_TagImp));
    FString Text;
    for(UEdGraphNode* Node:Graph->Nodes)
    {
        FStringOutputDevice Out;
        UExporter::ExportToOutputDevice(&Context,Node,nullptr,Out,TEXT("copy"),0,
            PPF_ExportsNotFullyQualified|PPF_Copy|PPF_Delimited,false,Graph);
        const bool Boundary=Node->IsA<UK2Node_Composite>() || Node->GetClass()==UK2Node_Tunnel::StaticClass();
        TArray<FString> Lines;Out.ParseIntoArrayLines(Lines,false);
        for(const FString& Line:Lines)
        {
            const FString Trim=Line.TrimStart();
            if(Boundary && (Trim.StartsWith(TEXT("BoundGraph=")) || Trim.StartsWith(TEXT("InputSinkNode=")) ||
                            Trim.StartsWith(TEXT("OutputSourceNode="))))continue;
            Text+=Line+TEXT("\r\n");
        }
    }
    return Text;
}
}
