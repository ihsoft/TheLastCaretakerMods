// Schema 2: editor-owned property text and complete K2 graphs, not clipboard paste.
#include "BlueprintAssetTextCommandlet.h"
#include "Dom/JsonObject.h"
#include "EdGraphUtilities.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_EditablePinBase.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/SceneComponent.h"
#include "Factories.h"
#include "GameFramework/Actor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UObjectHash.h"
#include "Misc/EngineVersion.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

namespace ATNames
{
constexpr TCHAR Value_[] = TEXT(".");
constexpr TCHAR Value_Game_[] = TEXT("/Game/");
constexpr TCHAR ValueAsset_[] = TEXT("Asset=");
constexpr TCHAR ValueBlueprintAssetText[] = TEXT("BlueprintAssetText");
constexpr TCHAR ValueComponentClassOverrides[] = TEXT("ComponentClassOverrides");
constexpr TCHAR ValueComponentTemplates[] = TEXT("ComponentTemplates");
constexpr TCHAR ValueCrcLastCompiledCDO[] = TEXT("CrcLastCompiledCDO");
constexpr TCHAR ValueCrcLastCompiledSignature[] = TEXT("CrcLastCompiledSignature");
constexpr TCHAR ValueDefaultSceneRoot[] = TEXT("DefaultSceneRoot");
constexpr TCHAR ValueDelegateSignatureGraphs[] = TEXT("DelegateSignatureGraphs");
constexpr TCHAR ValueExport[] = TEXT("Export");
constexpr TCHAR ValueExtensions[] = TEXT("Extensions");
constexpr TCHAR ValueFunctionGraphs[] = TEXT("FunctionGraphs");
constexpr TCHAR ValueGeneratedClass[] = TEXT("GeneratedClass");
constexpr TCHAR ValueGeneratedVariables[] = TEXT("GeneratedVariables");
constexpr TCHAR ValueImplementedInterfaces[] = TEXT("ImplementedInterfaces");
constexpr TCHAR ValueImport[] = TEXT("Import");
constexpr TCHAR ValueInheritableComponentHandler[] = TEXT("InheritableComponentHandler");
constexpr TCHAR ValueInput_[] = TEXT("Input=");
constexpr TCHAR ValueLastEditedDocuments[] = TEXT("LastEditedDocuments");
constexpr TCHAR ValueMacroGraphs[] = TEXT("MacroGraphs");
constexpr TCHAR ValueMode_[] = TEXT("Mode=");
constexpr TCHAR ValueNewVariables[] = TEXT("NewVariables");
constexpr TCHAR ValueNodes[] = TEXT("Nodes");
constexpr TCHAR ValueOutput_[] = TEXT("Output=");
constexpr TCHAR ValueParentClass[] = TEXT("ParentClass");
constexpr TCHAR ValueRootComponent[] = TEXT("RootComponent");
constexpr TCHAR ValueSchema[] = TEXT("Schema");
constexpr TCHAR ValueSimpleConstructionScript[] = TEXT("SimpleConstructionScript");
constexpr TCHAR ValueSkeletonGeneratedClass[] = TEXT("SkeletonGeneratedClass");
constexpr TCHAR ValueStatus[] = TEXT("Status");
constexpr TCHAR ValueSubGraphs[] = TEXT("SubGraphs");
constexpr TCHAR ValueThumbnailInfo[] = TEXT("ThumbnailInfo");
constexpr TCHAR ValueTimelines[] = TEXT("Timelines");
constexpr TCHAR ValueUbergraphPages[] = TEXT("UbergraphPages");
constexpr TCHAR ValueVariable[] = TEXT("Variable");
constexpr TCHAR Valueasset[] = TEXT("asset");
constexpr TCHAR Valuedefaults[] = TEXT("defaults");
constexpr TCHAR Valueevent[] = TEXT("event");
constexpr TCHAR Valueformat[] = TEXT("format");
constexpr TCHAR Valuefunction[] = TEXT("function");
constexpr TCHAR Valuegraphs[] = TEXT("graphs");
constexpr TCHAR Valuekind[] = TEXT("kind");
constexpr TCHAR Valuename[] = TEXT("name");
constexpr TCHAR Valuenative_asset[] = TEXT("native-asset");
constexpr TCHAR ValuenodeCount[] = TEXT("nodeCount");
constexpr TCHAR Valueparent[] = TEXT("parent");
constexpr TCHAR Valueproperties[] = TEXT("properties");
constexpr TCHAR Valueschema[] = TEXT("schema");
constexpr TCHAR Valuetext[] = TEXT("text");
constexpr TCHAR Valuevariables[] = TEXT("variables");
}

namespace AssetText
{
using Json = TSharedRef<FJsonObject>;
using Values = TArray<TSharedPtr<FJsonValue>>;
constexpr TCHAR Dependencies[] = TEXT("dependencies");
constexpr TCHAR RootState[] = TEXT("root");
constexpr TCHAR RootNodeState[] = TEXT("node");
constexpr TCHAR RootTemplateState[] = TEXT("template");
const TSet<FName> RootOwned = {TEXT("ComponentTemplate"),TEXT("ChildNodes")};

// Graph/variable ownership is explicit; compiler products and editor session state
// are regenerated. Other non-transient Blueprint properties are carried as text.
const TSet<FName> BlueprintOwned = {
    ATNames::ValueParentClass, ATNames::ValueGeneratedClass, ATNames::ValueSkeletonGeneratedClass,
    ATNames::ValueNewVariables, ATNames::ValueUbergraphPages, ATNames::ValueFunctionGraphs,
    ATNames::ValueMacroGraphs, ATNames::ValueDelegateSignatureGraphs, ATNames::ValueSimpleConstructionScript,
    ATNames::ValueComponentTemplates, ATNames::ValueTimelines, ATNames::ValueInheritableComponentHandler,
    ATNames::ValueGeneratedVariables, ATNames::ValueImplementedInterfaces, ATNames::ValueComponentClassOverrides,
    ATNames::ValueExtensions, ATNames::ValueLastEditedDocuments, ATNames::ValueThumbnailInfo,
    ATNames::ValueStatus, ATNames::ValueCrcLastCompiledCDO, ATNames::ValueCrcLastCompiledSignature};
const TSet<FName> GraphOwned = {ATNames::ValueNodes, ATNames::ValueSubGraphs, ATNames::ValueSchema};
const TSet<FName> DefaultOwned = {ATNames::ValueRootComponent, ATNames::ValueDefaultSceneRoot};
constexpr TCHAR LabelsMode[] = TEXT("Labels");

bool Read(const FString& Path, TSharedPtr<FJsonObject>& Value)
{
    FString Text;
    return FFileHelper::LoadFileToString(Text,*Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Value) && Value.IsValid();
}
bool Write(const FString& Path, Json Value)
{
    FString Text;
    auto Writer=TJsonWriterFactory<TCHAR,TPrettyJsonPrintPolicy<TCHAR>>::Create(&Text);
    return FJsonSerializer::Serialize(Value,Writer) && FFileHelper::SaveStringToFile(Text,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
bool ExportLabels(const FString& Path)
{
    // UI metadata only: no Blueprint construction or asset serialization.
    FInternationalization::Get().SetCurrentCulture(TEXT("en"));
    auto Functions=MakeShared<FJsonObject>();
    auto Parents=MakeShared<FJsonObject>();
    for(TObjectIterator<UClass> It;It;++It)
    {
        UClass* Class=*It;
        const FString Owner=Class->GetPathName();
        if(!Class->HasAnyClassFlags(CLASS_Native) || !Owner.StartsWith(TEXT("/Script/Engine.")))continue;
        if(Class->GetSuperClass())Parents->SetStringField(Owner,Class->GetSuperClass()->GetPathName());
        for(TFieldIterator<UFunction> Function(Class,EFieldIteratorFlags::ExcludeSuper);Function;++Function)
        {
            UFunction* F=*Function;
            if(!F->HasAnyFunctionFlags(FUNC_BlueprintCallable|FUNC_BlueprintPure|FUNC_BlueprintEvent))continue;
            auto Entry=MakeShared<FJsonObject>();
            Entry->SetStringField(TEXT("title"),F->GetDisplayNameText().ToString());
            const FString Compact=F->GetMetaData(TEXT("CompactNodeTitle"));
            if(!Compact.IsEmpty())Entry->SetStringField(TEXT("compactTitle"),Compact);
            Functions->SetObjectField(Owner+TEXT("::")+F->GetName(),Entry);
        }
    }
    auto Root=MakeShared<FJsonObject>();
    Root->SetNumberField(ATNames::Valueschema,1);
    Root->SetStringField(TEXT("engineVersion"),FEngineVersion::Current().ToString());
    Root->SetStringField(TEXT("culture"),FInternationalization::Get().GetCurrentCulture()->GetName());
    Root->SetStringField(TEXT("coverage"),TEXT("Loaded native /Script/Engine classes: Blueprint callable, pure and event functions. Engine metadata only; no game or mod classes."));
    Root->SetObjectField(TEXT("functions"),Functions);
    Root->SetObjectField(TEXT("parents"),Parents);
    return Functions->Values.Num()>0 && Write(Path,Root);
}
bool Keys(const FJsonObject& Value, std::initializer_list<const TCHAR*> Allowed)
{
    TSet<FString> Names; for(const TCHAR* Name:Allowed)Names.Add(Name);
    for(const TPair<FString,TSharedPtr<FJsonValue>>& Pair:Value.Values)if(!Names.Contains(Pair.Key))
    { UE_LOG(LogTemp,Error,TEXT("Unknown asset-text field %s"),*Pair.Key);return false; }
    return true;
}
Json Properties(const UObject* Object, const UObject* Baseline, const TSet<FName>& Excluded, bool IncludeTransient=false, bool IncludeDuplicateTransient=false)
{
    auto Result=MakeShared<FJsonObject>();
    for(TFieldIterator<FProperty> It(Object->GetClass());It;++It)
    {
        const FProperty* P=*It;
        if(Excluded.Contains(P->GetFName()) || P->HasAnyPropertyFlags(CPF_Deprecated) ||
           (!IncludeTransient && P->HasAnyPropertyFlags(CPF_Transient)) ||
           (!IncludeDuplicateTransient && P->HasAnyPropertyFlags(CPF_DuplicateTransient)))continue;
        if(Baseline && P->GetOwnerClass()->IsChildOf(Baseline->GetClass()) && P->GetOwnerClass()!=Baseline->GetClass())continue;
        if(Baseline && P->Identical_InContainer(Object,Baseline))continue;
        FString Text;
        P->ExportTextItem_Direct(Text,P->ContainerPtrToValuePtr<void>(Object),P->ContainerPtrToValuePtr<void>(Object),const_cast<UObject*>(Object),PPF_None);
        Result->SetStringField(P->GetName(),Text);
    }
    return Result;
}
bool Apply(UObject* Object, const FJsonObject& Map, const TSet<FName>& Excluded)
{
    for(const TPair<FString,TSharedPtr<FJsonValue>>& Pair:Map.Values)
    {
        FProperty* P=FindFProperty<FProperty>(Object->GetClass(),*Pair.Key);
        FString Text;
        if(!P || Excluded.Contains(P->GetFName()) || !Pair.Value->TryGetString(Text))
        { UE_LOG(LogTemp,Error,TEXT("Unsupported property %s"),*Pair.Key);return false; }
        const TCHAR* End=P->ImportText_Direct(*Text,P->ContainerPtrToValuePtr<void>(Object),Object,PPF_None,GWarn);
        if(!End || *End!=0){UE_LOG(LogTemp,Error,TEXT("Cannot import property %s"),*Pair.Key);return false;}
    }
    return true;
}
// Component object identities are relative to their owning package. All objects
// are allocated before importing properties so cross-references can resolve.
constexpr TCHAR Components[] = TEXT("components");
constexpr TCHAR Objects[] = TEXT("objects");
constexpr TCHAR ObjectId[] = TEXT("id");
constexpr TCHAR ObjectClass[] = TEXT("class");
constexpr TCHAR ObjectOuter[] = TEXT("outer");
constexpr TCHAR ObjectFlags[] = TEXT("flags");
constexpr TCHAR ScriptRoot[] = TEXT("scs");
constexpr TCHAR GeneratedTemplates[] = TEXT("generatedTemplates");
constexpr TCHAR Templates[] = TEXT("templates");
constexpr TCHAR BlueprintOwner[] = TEXT("$blueprint");
constexpr TCHAR ClassOwner[] = TEXT("$class");
constexpr EObjectFlags StoredFlags = EObjectFlags(RF_Public|RF_Transactional|RF_ArchetypeObject|RF_DefaultSubObject);
FString ObjectKey(const UObject* Object,const UBlueprint* BP)
{
    if(Object==BP)return BlueprintOwner;
    if(Object==BP->GeneratedClass)return ClassOwner;
    return Object->GetPathName(BP->GetOutermost());
}
bool LegacyRoot(const UBlueprint* BP)
{
    const auto& Nodes=BP->SimpleConstructionScript->GetAllNodes();
    return BP->ParentClass==AActor::StaticClass() && !BP->ComponentTemplates.Num() &&
        !CastChecked<UBlueprintGeneratedClass>(BP->GeneratedClass)->ComponentTemplates.Num() && Nodes.Num()==1 &&
        Nodes[0]->GetVariableName()==ATNames::ValueDefaultSceneRoot && Nodes[0]->ComponentClass==USceneComponent::StaticClass();
}
Json ExportComponents(const UBlueprint* BP)
{
    TSet<UObject*> Owned;
    TFunction<void(UObject*)> Collect=[&](UObject* Object)
    {
        if(!Object || Owned.Contains(Object) || Object->HasAnyFlags(RF_Transient))return;
        Owned.Add(Object);
        TArray<UObject*> Children;GetObjectsWithOuter(Object,Children,EGetObjectsFlags::None);
        for(UObject* Child:Children)Collect(Child);
    };
    Collect(BP->SimpleConstructionScript);
    for(const USCS_Node* Node:BP->SimpleConstructionScript->GetAllNodes())Collect(Node->ComponentTemplate);
    for(UActorComponent* Template:BP->ComponentTemplates)Collect(Template);
    for(UActorComponent* Template:CastChecked<UBlueprintGeneratedClass>(BP->GeneratedClass)->ComponentTemplates)Collect(Template);
    // The hidden default-root node can own a template outside the SCS itself.
    TArray<UObject*> Initial=Owned.Array();
    for(UObject* Object:Initial)if(auto* Node=Cast<USCS_Node>(Object))Collect(Node->ComponentTemplate);
    TArray<UObject*> Sorted=Owned.Array();Sorted.Sort([&](const UObject& A,const UObject& B){return ObjectKey(&A,BP)<ObjectKey(&B,BP);});
    Values Rows;
    for(UObject* Object:Sorted)
    {
        auto Row=MakeShared<FJsonObject>();
        Row->SetStringField(ObjectId,ObjectKey(Object,BP));
        Row->SetStringField(ATNames::Valuename,Object->GetName());
        Row->SetStringField(ObjectClass,Object->GetClass()->GetPathName());
        Row->SetStringField(ObjectOuter,ObjectKey(Object->GetOuter(),BP));
        Row->SetNumberField(ObjectFlags,uint32(Object->GetFlags()&StoredFlags));
        Row->SetObjectField(ATNames::Valueproperties,Properties(Object,Object->GetClass()->GetDefaultObject(),{},false,true));
        Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    auto Data=MakeShared<FJsonObject>();Data->SetArrayField(Objects,Rows);
    Data->SetStringField(ScriptRoot,ObjectKey(BP->SimpleConstructionScript,BP));
    Values References;for(UActorComponent* Template:BP->ComponentTemplates)References.Add(MakeShared<FJsonValueString>(ObjectKey(Template,BP)));
    Data->SetArrayField(Templates,References);
    References.Empty();
    for(UActorComponent* Template:CastChecked<UBlueprintGeneratedClass>(BP->GeneratedClass)->ComponentTemplates)References.Add(MakeShared<FJsonValueString>(ObjectKey(Template,BP)));
    Data->SetArrayField(GeneratedTemplates,References);return Data;
}
bool ImportComponents(UBlueprint* BP,const FJsonObject& Data)
{
    if(!Keys(Data,{Objects,ScriptRoot,Templates,GeneratedTemplates}))return false;
    // Remove only the factory's disposable automatic root, before allocating source identities.
    for(USCS_Node* Node:BP->SimpleConstructionScript->GetAllNodes())
        if(Node->ComponentTemplate)Node->ComponentTemplate->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors);
    BP->SimpleConstructionScript->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors);
    BP->SimpleConstructionScript=nullptr;BP->ComponentTemplates.Empty();
    TMap<FString,UObject*> Allocated;Allocated.Add(BlueprintOwner,BP);Allocated.Add(ClassOwner,BP->GeneratedClass);
    TMap<FString,TSharedPtr<FJsonObject>> Pending;
    for(const auto& Value:Data.GetArrayField(Objects))
    {
        auto Row=Value->AsObject();
        if(!Row || !Keys(*Row,{ObjectId,ATNames::Valuename,ObjectClass,ObjectOuter,ObjectFlags,ATNames::Valueproperties}))return false;
        const FString Id=Row->GetStringField(ObjectId);
        if(Allocated.Contains(Id) || Pending.Contains(Id))return false;
        Pending.Add(Id,Row);
    }
    while(Pending.Num())
    {
        bool Progress=false;
        for(auto It=Pending.CreateIterator();It;++It)
        {
            const auto& Row=It.Value();UObject** Outer=Allocated.Find(Row->GetStringField(ObjectOuter));
            if(!Outer)continue;
            UClass* Class=LoadObject<UClass>(nullptr,*Row->GetStringField(ObjectClass));
            const FString Name=Row->GetStringField(ATNames::Valuename);
            if(!Class || Name.IsEmpty() || Class->IsChildOf(UClass::StaticClass()))return false;
            const uint32 Flags=uint32(Row->GetIntegerField(ObjectFlags));
            if(Flags&~uint32(StoredFlags)){UE_LOG(LogTemp,Error,TEXT("Unsupported component object flags."));return false;}
            UObject* Object=FindObject<UObject>(*Outer,*Name);
            if(Object && Object->GetClass()!=Class)return false;
            if(!Object)Object=NewObject<UObject>(*Outer,Class,*Name,EObjectFlags(Flags));
            if(!Object || ObjectKey(Object,BP)!=It.Key())return false;
            Allocated.Add(It.Key(),Object);It.RemoveCurrent();Progress=true;
        }
        if(!Progress){UE_LOG(LogTemp,Error,TEXT("Unresolved component object owners."));return false;}
    }
    for(const auto& Value:Data.GetArrayField(Objects))
    {
        auto Row=Value->AsObject();
        if(!Apply(Allocated.FindChecked(Row->GetStringField(ObjectId)),*Row->GetObjectField(ATNames::Valueproperties),{}))return false;
    }
    BP->SimpleConstructionScript=Cast<USimpleConstructionScript>(Allocated.FindRef(Data.GetStringField(ScriptRoot)));
    if(!BP->SimpleConstructionScript)return false;
    for(const auto& Value:Data.GetArrayField(Templates))
    {
        auto* Template=Cast<UActorComponent>(Allocated.FindRef(Value->AsString()));
        if(!Template)return false;BP->ComponentTemplates.Add(Template);
    }
    for(const auto& Value:Data.GetArrayField(GeneratedTemplates))
    {
        auto* Template=Cast<UActorComponent>(Allocated.FindRef(Value->AsString()));
        if(!Template)return false;BP->ComponentTemplates.AddUnique(Template);
    }
    return true;
}
bool Supported(const UBlueprint* BP)
{
    if(!BP || (!BP->ParentClass || !BP->ParentClass->IsChildOf(AActor::StaticClass())) || BP->BlueprintType!=BPTYPE_Normal ||
       BP->MacroGraphs.Num() || BP->DelegateSignatureGraphs.Num() || BP->Timelines.Num() ||
       BP->ComponentClassOverrides.Num() ||
       BP->ImplementedInterfaces.Num() || BP->GetExtensions().Num() || BP->GeneratedVariables.Num() || BP->InheritableComponentHandler)
    {UE_LOG(LogTemp,Error,TEXT("Schema 2 currently supports Actor Blueprints without component overrides, interfaces, macro definitions, extensions or timelines."));return false;}
    if(!BP->SimpleConstructionScript)return false;
    TArray<UEdGraph*> Graphs;BP->GetAllGraphs(Graphs);
    if(Graphs.Num()!=BP->UbergraphPages.Num()+BP->FunctionGraphs.Num())
    {UE_LOG(LogTemp,Error,TEXT("Nested graphs are outside schema 2."));return false;}
    for(const UEdGraph* Graph:Graphs)
        if(Graph->GetSchema()->GetClass()!=UEdGraphSchema_K2::StaticClass())return false;
    return true;
}
bool Export(const FString& Asset, const FString& Path)
{
    UBlueprint* BP=LoadObject<UBlueprint>(nullptr,*(Asset+ATNames::Value_+FPackageName::GetLongPackageAssetName(Asset)));
    if(!Supported(BP) || !BP->GeneratedClass)return false;
    auto Root=MakeShared<FJsonObject>();
    Root->SetNumberField(ATNames::Valueschema,2);Root->SetStringField(ATNames::Valueformat,ATNames::Valuenative_asset);
    Root->SetStringField(ATNames::Valueasset,Asset);Root->SetStringField(ATNames::Valueparent,BP->ParentClass->GetPathName());
    Root->SetObjectField(ATNames::Valueproperties,Properties(BP,BP->GetClass()->GetDefaultObject(),BlueprintOwned));
    if(LegacyRoot(BP))
    {
    const USCS_Node* RootNode=BP->SimpleConstructionScript->GetAllNodes()[0];
    auto RootData=MakeShared<FJsonObject>();
    RootData->SetObjectField(RootNodeState,Properties(RootNode,RootNode->GetClass()->GetDefaultObject(),RootOwned));
    RootData->SetObjectField(RootTemplateState,Properties(RootNode->ComponentTemplate,RootNode->ComponentClass->GetDefaultObject(),{}));
    Root->SetObjectField(RootState,RootData);
    }
    else Root->SetObjectField(Components,ExportComponents(BP));
    Values Variables;
    for(const FBPVariableDescription& Var:BP->NewVariables)
    {
        FString Text;FBPVariableDescription::StaticStruct()->ExportText(Text,&Var,nullptr,BP,PPF_None,nullptr);
        Variables.Add(MakeShared<FJsonValueString>(Text));
    }
    Root->SetArrayField(ATNames::Valuevariables,Variables);
    // Compare inherited fields against the parent CDO; export every authored variable,
    // including transient initial values, separately from inherited defaults.
    UObject* CDO=BP->GeneratedClass->GetDefaultObject();
    auto Defaults=Properties(CDO,BP->ParentClass->GetDefaultObject(),DefaultOwned);
    for(const auto& Var:BP->NewVariables)
    {
        const FProperty* P=FindFProperty<FProperty>(BP->GeneratedClass,Var.VarName);
        if(!P)return false;
        FString Text;P->ExportTextItem_Direct(Text,P->ContainerPtrToValuePtr<void>(CDO),P->ContainerPtrToValuePtr<void>(CDO),CDO,PPF_None);
        Defaults->SetStringField(Var.VarName.ToString(),Text);
    }
    Root->SetObjectField(ATNames::Valuedefaults,Defaults);
    Values Graphs;
    TSet<FString> DependencyPaths;
    auto ExportGraph=[&](UEdGraph* Graph,const TCHAR* Kind)
    {
        auto G=MakeShared<FJsonObject>();G->SetStringField(ATNames::Valuename,Graph->GetName());G->SetStringField(ATNames::Valuekind,Kind);
        G->SetObjectField(ATNames::Valueproperties,Properties(Graph,Graph->GetClass()->GetDefaultObject(),GraphOwned));
        TSet<UObject*> Selection;
        for(UEdGraphNode* Node:Graph->Nodes)
        {
            // Match the engine's save-time normalization of user-pin metadata.
            if(auto* Editable=Cast<UK2Node_EditablePinBase>(Node))Editable->UpdateUserDefinedPinDefaultValues();
            Selection.Add(Node);
            for(const UEdGraphPin* Pin:Node->Pins)
                if(Pin->DefaultObject && Pin->DefaultObject->GetOutermost()!=BP->GetOutermost())
                    DependencyPaths.Add(Pin->DefaultObject->GetPathName());
        }
        FString Text;FEdGraphUtilities::ExportNodesToText(Selection,Text);
        G->SetStringField(ATNames::Valuetext,Text);G->SetNumberField(ATNames::ValuenodeCount,Graph->Nodes.Num());
        Graphs.Add(MakeShared<FJsonValueObject>(G));
    };
    for(UEdGraph* Graph:BP->UbergraphPages)ExportGraph(Graph,ATNames::Valueevent);
    for(UEdGraph* Graph:BP->FunctionGraphs)ExportGraph(Graph,ATNames::Valuefunction);
    Root->SetArrayField(ATNames::Valuegraphs,Graphs);
    TArray<FString> SortedDependencies=DependencyPaths.Array();SortedDependencies.Sort();
    Values DependencyValues;for(const FString& Dependency:SortedDependencies)DependencyValues.Add(MakeShared<FJsonValueString>(Dependency));
    Root->SetArrayField(Dependencies,DependencyValues);
    return Write(Path,Root);
}
struct GraphFactory final : FCustomizableTextObjectFactory
{
    TSet<UEdGraphNode*> Nodes;
    bool Rejected=false;
    GraphFactory():FCustomizableTextObjectFactory(GWarn){}
    bool CanCreateClass(UClass* Class,bool& Omit)const override
    { Omit=true;return Class->IsChildOf(UEdGraphNode::StaticClass()); }
    void ProcessConstructedObject(UObject* Object)override
    {
        UEdGraphNode* Node=CastChecked<UEdGraphNode>(Object);
        Node->GetGraph()->Nodes.Add(Node);Nodes.Add(Node);
    }
};
bool Import(const FString& Path)
{
    TSharedPtr<FJsonObject> Root;
    if(!Read(Path,Root) || !Keys(*Root,{ATNames::Valueschema,ATNames::Valueformat,ATNames::Valueasset,ATNames::Valueparent,ATNames::Valueproperties,ATNames::Valuevariables,ATNames::Valuedefaults,ATNames::Valuegraphs,Dependencies,RootState,Components}) ||
       Root->GetIntegerField(ATNames::Valueschema)!=2 || Root->GetStringField(ATNames::Valueformat)!=ATNames::Valuenative_asset)return false;
    const FString Asset=Root->GetStringField(ATNames::Valueasset);
    if(!Asset.StartsWith(ATNames::Value_Game_) || !FPackageName::IsValidLongPackageName(Asset))return false;
    // First implementation rebuilds absent assets only. No existing asset is overwritten.
    if(FPackageName::DoesPackageExist(Asset))
    {UE_LOG(LogTemp,Error,TEXT("Import requires an absent target asset; preserve/move the test baseline first: %s"),*Asset);return false;}
    UClass* Parent=LoadObject<UClass>(nullptr,*Root->GetStringField(ATNames::Valueparent));
    if(!Parent || !Parent->IsChildOf(AActor::StaticClass()))return false;
    UPackage* Package=CreatePackage(*Asset);
    UBlueprint* BP=FKismetEditorUtilities::CreateBlueprint(Parent,Package,*FPackageName::GetLongPackageAssetName(Asset),BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass(),ATNames::ValueBlueprintAssetText);
    if(!BP || !Apply(BP,*Root->GetObjectField(ATNames::Valueproperties),BlueprintOwned))return false;
    if(Root->HasField(Components))
    {
        if(Root->HasField(RootState) || !ImportComponents(BP,*Root->GetObjectField(Components)))return false;
    }
    else
    {
    if(!BP->SimpleConstructionScript || BP->SimpleConstructionScript->GetAllNodes().Num()!=1)return false;
    USCS_Node* RootNode=BP->SimpleConstructionScript->GetAllNodes()[0];
    auto RootData=Root->GetObjectField(RootState);
    if(!Keys(*RootData,{RootNodeState,RootTemplateState}) ||
       !Apply(RootNode,*RootData->GetObjectField(RootNodeState),RootOwned) ||
       !Apply(RootNode->ComponentTemplate,*RootData->GetObjectField(RootTemplateState),{}))return false;
    }
    for(const auto& Value:Root->GetArrayField(Dependencies))
    {
        FString Dependency;
        if(!Value->TryGetString(Dependency) || !LoadObject<UObject>(nullptr,*Dependency))
        {UE_LOG(LogTemp,Error,TEXT("Cannot load pin default dependency: %s"),*Dependency);return false;}
    }
    BP->NewVariables.Empty();
    for(const auto& Value:Root->GetArrayField(ATNames::Valuevariables))
    {
        FString Text;if(!Value->TryGetString(Text))return false;
        FBPVariableDescription Var;
        const TCHAR* End=FBPVariableDescription::StaticStruct()->ImportText(*Text,&Var,BP,PPF_None,GWarn,ATNames::ValueVariable);
        if(!End || *End!=0 || Var.VarName.IsNone())return false;
        BP->NewVariables.Add(Var);
    }
    TArray<UEdGraph*> OldGraphs;BP->GetAllGraphs(OldGraphs);
    BP->UbergraphPages.Empty();BP->FunctionGraphs.Empty();
    for(UEdGraph* Graph:OldGraphs)Graph->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors);
    TArray<TPair<UEdGraph*,TSharedPtr<FJsonObject>>> Pending;
    TSet<FString> Names;
    for(const auto& Value:Root->GetArrayField(ATNames::Valuegraphs))
    {
        auto G=Value->AsObject();
        if(!G.IsValid() || !Keys(*G,{ATNames::Valuename,ATNames::Valuekind,ATNames::Valueproperties,ATNames::Valuetext,ATNames::ValuenodeCount}))return false;
        const FString Name=G->GetStringField(ATNames::Valuename);
        if(Name.IsEmpty() || Names.Contains(Name))return false;Names.Add(Name);
        UEdGraph* Graph=FBlueprintEditorUtils::CreateNewGraph(BP,*Name,UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
        if(!Apply(Graph,*G->GetObjectField(ATNames::Valueproperties),GraphOwned))return false;
        const FString Kind=G->GetStringField(ATNames::Valuekind);
        if(Kind==ATNames::Valueevent)BP->UbergraphPages.Add(Graph);
        else if(Kind==ATNames::Valuefunction)BP->FunctionGraphs.Add(Graph);
        else return false;
        Pending.Add({Graph,G});
    }
    for(const auto& Pair:Pending)
    {
        GraphFactory Factory;Factory.ProcessBuffer(Pair.Key,RF_Transactional,Pair.Value->GetStringField(ATNames::Valuetext));
        UEdGraphPin::ResolveAllPinReferences();
        if(Pair.Key->Nodes.Num()!=Pair.Value->GetIntegerField(ATNames::ValuenodeCount))
        {UE_LOG(LogTemp,Error,TEXT("Native node count mismatch in %s"),*Pair.Key->GetName());return false;}
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    if(BP->Status==BS_Error || !BP->GeneratedClass)
    {UE_LOG(LogTemp,Error,TEXT("Rebuilt Blueprint failed compilation."));return false;}
    if(Root->HasField(Components))
    {
        auto Data=Root->GetObjectField(Components);
        BP->ComponentTemplates.Empty();
        auto* GC=CastChecked<UBlueprintGeneratedClass>(BP->GeneratedClass);GC->ComponentTemplates.Empty();
        for(const TCHAR* List:{Templates,GeneratedTemplates})for(const auto& Value:Data->GetArrayField(List))
        {
            auto* Template=FindObject<UActorComponent>(BP->GetOutermost(),*Value->AsString());
            if(!Template){UE_LOG(LogTemp,Error,TEXT("Component template missing after compilation: %s"),*Value->AsString());return false;}
            // Pre-save migration/compilation takes its templates from the Blueprint.
            // Post-load pruning may remove these editor entries; the generated class
            // must still retain every template used by name at runtime.
            BP->ComponentTemplates.AddUnique(Template);
            if(List==GeneratedTemplates)GC->ComponentTemplates.Add(Template);
        }
    }
    if(!Apply(BP->GeneratedClass->GetDefaultObject(),*Root->GetObjectField(ATNames::Valuedefaults),DefaultOwned))return false;
    if(!Supported(BP))return false;
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    const FString Filename=FPackageName::LongPackageNameToFilename(Asset,FPackageName::GetAssetPackageExtension());
    return UPackage::SavePackage(Package,BP,*Filename,Args);
}
}

UBlueprintAssetTextCommandlet::UBlueprintAssetTextCommandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UBlueprintAssetTextCommandlet::Main(const FString& Params)
{
    FString Mode,Input,Output,Asset;
    FParse::Value(*Params,ATNames::ValueMode_,Mode);FParse::Value(*Params,ATNames::ValueInput_,Input);
    FParse::Value(*Params,ATNames::ValueOutput_,Output);FParse::Value(*Params,ATNames::ValueAsset_,Asset);
    if(Mode==ATNames::ValueExport)return AssetText::Export(Asset,Output)?0:2;
    if(Mode==ATNames::ValueImport)return AssetText::Import(Input)?0:3;
    if(Mode==AssetText::LabelsMode)return AssetText::ExportLabels(Output)?0:4;
    return 1;
}
