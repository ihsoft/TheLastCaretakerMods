#include "BlueprintTextCommandlet.h"
#include "BlueprintTextNames.h"

#include "Dom/JsonObject.h"
#include "EdGraphUtilities.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "GameFramework/Actor.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace
{
constexpr int32 Schema = 1;
constexpr TCHAR IdentityPrefix[] = TEXT("BTP:id=");
constexpr TCHAR FormatCompact[] = TEXT("compact");
constexpr TCHAR FormatNative[] = TEXT("native");
constexpr TCHAR KindEvent[] = TEXT("event");
constexpr TCHAR KindCall[] = TEXT("call");
constexpr TCHAR KindSequence[] = TEXT("sequence");
constexpr TCHAR KindSetVariable[] = TEXT("setVariable");
constexpr TCHAR KindGetVariable[] = TEXT("getVariable");

bool ValidateKeys(const TSharedPtr<FJsonObject>& Object, const TCHAR* Context, std::initializer_list<const TCHAR*> Allowed)
{
    TSet<FString> AllowedSet;
    for (const TCHAR* Key : Allowed) AllowedSet.Add(Key);
    for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Object->Values)
    {
        if (!AllowedSet.Contains(Pair.Key))
        {
            UE_LOG(LogTemp, Error, TEXT("Unsupported %s field '%s'."), Context, *Pair.Key);
            return false;
        }
    }
    return true;
}

bool ReadJson(const FString& Path, TSharedPtr<FJsonObject>& Value)
{
    FString Text;
    return FFileHelper::LoadFileToString(Text, *Path) &&
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Value) && Value.IsValid();
}

bool WriteJson(const FString& Path, const TSharedRef<FJsonObject>& Value)
{
    FString Text;
    const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Text);
    if (!FJsonSerializer::Serialize(Value, Writer)) return false;
    return FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

FString ObjectPathToPackage(const FString& Asset)
{
    FString Package = Asset;
    int32 Dot = INDEX_NONE;
    if (Package.FindChar(TEXT('.'), Dot)) Package.LeftInline(Dot);
    return Package;
}

FString NodeId(const UEdGraphNode* Node)
{
    if (Node->NodeComment.StartsWith(IdentityPrefix))
    {
        FString FirstLine;
        if (Node->NodeComment.Split(BlueprintTextNames::LineBreak, &FirstLine, nullptr)) return FirstLine.Mid(FCString::Strlen(IdentityPrefix));
        return Node->NodeComment.Mid(FCString::Strlen(IdentityPrefix));
    }
    return BlueprintTextNames::GeneratedIdPrefix + Node->NodeGuid.ToString(EGuidFormats::Digits);
}

FString NodeUserComment(const UEdGraphNode* Node)
{
    if (!Node->NodeComment.StartsWith(IdentityPrefix)) return Node->NodeComment;
    FString FirstLine;
    FString Rest;
    return Node->NodeComment.Split(BlueprintTextNames::LineBreak, &FirstLine, &Rest) ? Rest : FString();
}

void SetNodeId(UEdGraphNode* Node, const FString& Id, const FString& Comment = FString())
{
    Node->NodeComment = FString(IdentityPrefix) + Id + (Comment.IsEmpty() ? TEXT("") : BlueprintTextNames::LineBreak + Comment);
}

bool SplitOperation(const FString& Operation, FString& OwnerPath, FString& FunctionName)
{
    return Operation.Split(BlueprintTextNames::OperationSeparator, &OwnerPath, &FunctionName, ESearchCase::CaseSensitive, ESearchDir::FromEnd) &&
        OwnerPath.StartsWith(BlueprintTextNames::ScriptPackagePrefix) && !FunctionName.IsEmpty();
}

TArray<UEdGraphPin*> DataParameterPins(UK2Node_CallFunction* Call, const EEdGraphPinDirection Direction)
{
    TArray<UEdGraphPin*> Result;
    for (UEdGraphPin* Pin : Call->Pins)
    {
        if (Pin->Direction != Direction || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec ||
            Pin->PinName == UEdGraphSchema_K2::PN_Self || Pin->bHidden) continue;
        Result.Add(Pin);
    }
    return Result;
}

FString AliasForPin(UEdGraphNode* Node, UEdGraphPin* Pin)
{
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
    {
        if (Pin->Direction == EGPD_Input) return BlueprintTextNames::AliasExec;
        if (Pin->PinName == UEdGraphSchema_K2::PN_Then) return BlueprintTextNames::AliasThen;
        const FString Name = Pin->PinName.ToString();
        if (Name.StartsWith(BlueprintTextNames::SequencePinPrefix)) return BlueprintTextNames::AliasThen + Name.Mid(5);
    }
    if (Pin->PinName == UEdGraphSchema_K2::PN_ReturnValue) return BlueprintTextNames::AliasResult;
    if (Pin->PinName == UEdGraphSchema_K2::PN_Self) return BlueprintTextNames::AliasTarget;
    if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
    {
        const TArray<UEdGraphPin*> Inputs = DataParameterPins(Call, EGPD_Input);
        if (Inputs.Num() == 2 && Inputs[0]->PinName == BlueprintTextNames::BinaryLeftPin && Inputs[1]->PinName == BlueprintTextNames::BinaryRightPin)
        {
            if (Pin == Inputs[0]) return BlueprintTextNames::AliasLeft;
            if (Pin == Inputs[1]) return BlueprintTextNames::AliasRight;
        }
    }
    if (UK2Node_VariableSet* Set = Cast<UK2Node_VariableSet>(Node))
    {
        if (Pin->Direction == EGPD_Input && Pin->PinName != UEdGraphSchema_K2::PN_Self &&
            Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec) return BlueprintTextNames::AliasValue;
    }
    return Pin->PinName.ToString();
}

UEdGraphPin* FindAliasPin(UEdGraphNode* Node, const FString& Alias, const EEdGraphPinDirection Direction)
{
    for (UEdGraphPin* Pin : Node->Pins) if (Pin->Direction == Direction && AliasForPin(Node, Pin) == Alias) return Pin;
    return nullptr;
}

TSharedRef<FJsonObject> PinTypeJson(const UEdGraphPin* Pin)
{
    const TSharedRef<FJsonObject> Type = MakeShared<FJsonObject>();
    Type->SetStringField(BlueprintTextNames::FieldReadable, UEdGraphSchema_K2::TypeToText(Pin->PinType).ToString());
    Type->SetStringField(BlueprintTextNames::FieldCategory, Pin->PinType.PinCategory.ToString());
    if (!Pin->PinType.PinSubCategory.IsNone()) Type->SetStringField(BlueprintTextNames::FieldSubcategory, Pin->PinType.PinSubCategory.ToString());
    if (UObject* Object = Pin->PinType.PinSubCategoryObject.Get()) Type->SetStringField(BlueprintTextNames::FieldTypePath, Object->GetPathName());
    return Type;
}

bool SetVariableType(const TSharedPtr<FJsonObject>& Variable, FEdGraphPinType& Type)
{
    const FString Kind = Variable->GetStringField(BlueprintTextNames::FieldType);
    if (Kind == BlueprintTextNames::TypeFloat) { Type.PinCategory = UEdGraphSchema_K2::PC_Real; Type.PinSubCategory = UEdGraphSchema_K2::PC_Float; return true; }
    if (Kind == BlueprintTextNames::TypeDouble) { Type.PinCategory = UEdGraphSchema_K2::PC_Real; Type.PinSubCategory = UEdGraphSchema_K2::PC_Double; return true; }
    if (Kind == BlueprintTextNames::TypeInt) { Type.PinCategory = UEdGraphSchema_K2::PC_Int; return true; }
    if (Kind == BlueprintTextNames::TypeBool) { Type.PinCategory = UEdGraphSchema_K2::PC_Boolean; return true; }
    UE_LOG(LogTemp, Error, TEXT("Unsupported variable type '%s'; supported: float, double, int, bool."), *Kind);
    return false;
}

bool SaveBlueprintPackage(UBlueprint* Blueprint)
{
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    const FString Filename = FPackageName::LongPackageNameToFilename(Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    return UPackage::SavePackage(Blueprint->GetOutermost(), Blueprint, *Filename, Args);
}

bool SaveBlueprint(UBlueprint* Blueprint)
{
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) { UE_LOG(LogTemp, Error, TEXT("Blueprint compilation failed.")); return false; }
    return SaveBlueprintPackage(Blueprint);
}

UBlueprint* LoadBlueprint(const FString& Asset)
{
    const FString Package = ObjectPathToPackage(Asset);
    const FString Name = FPackageName::GetLongPackageAssetName(Package);
    return LoadObject<UBlueprint>(nullptr, *(Package + BlueprintTextNames::MemberSeparator + Name));
}

bool ValidateVariable(const TSharedPtr<FJsonObject>& Variable, const int32 Index)
{
    if (!ValidateKeys(Variable, *FString::Printf(TEXT("variables[%d]"), Index), {BlueprintTextNames::FieldName, BlueprintTextNames::FieldType})) return false;
    FString Name;
    FString Type;
    if (!Variable->TryGetStringField(BlueprintTextNames::FieldName, Name) || Name.IsEmpty() ||
        !Variable->TryGetStringField(BlueprintTextNames::FieldType, Type) || Type.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("variables[%d] requires string name and type."), Index);
        return false;
    }
    return true;
}

bool ValidateCompactBody(const TSharedPtr<FJsonObject>& Body)
{
    if (!ValidateKeys(Body, TEXT("compact body"), {BlueprintTextNames::FieldNodes, BlueprintTextNames::FieldLinks})) return false;
    const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Links = nullptr;
    if (!Body->TryGetArrayField(BlueprintTextNames::FieldNodes, Nodes) || !Body->TryGetArrayField(BlueprintTextNames::FieldLinks, Links))
    {
        UE_LOG(LogTemp, Error, TEXT("Compact body requires nodes and links arrays."));
        return false;
    }
    for (int32 Index = 0; Index < Nodes->Num(); ++Index)
    {
        const TSharedPtr<FJsonObject> Node = (*Nodes)[Index]->AsObject();
        if (!Node.IsValid()) { UE_LOG(LogTemp, Error, TEXT("nodes[%d] must be an object."), Index); return false; }
        FString Kind;
        if (!Node->TryGetStringField(BlueprintTextNames::FieldKind, Kind)) { UE_LOG(LogTemp, Error, TEXT("nodes[%d] requires kind."), Index); return false; }
        const bool bKeysValid =
            (Kind == KindEvent && ValidateKeys(Node, TEXT("event node"), {BlueprintTextNames::FieldId, BlueprintTextNames::FieldKind, KindEvent, BlueprintTextNames::FieldComment, BlueprintTextNames::FieldPos, BlueprintTextNames::FieldEnabled})) ||
            (Kind == KindCall && ValidateKeys(Node, TEXT("call node"), {BlueprintTextNames::FieldId, BlueprintTextNames::FieldKind, BlueprintTextNames::FieldOperation, BlueprintTextNames::FieldArguments, BlueprintTextNames::FieldComment, BlueprintTextNames::FieldPos, BlueprintTextNames::FieldEnabled})) ||
            (Kind == KindSequence && ValidateKeys(Node, TEXT("sequence node"), {BlueprintTextNames::FieldId, BlueprintTextNames::FieldKind, BlueprintTextNames::FieldOutputs, BlueprintTextNames::FieldComment, BlueprintTextNames::FieldPos, BlueprintTextNames::FieldEnabled})) ||
            (Kind == KindSetVariable && ValidateKeys(Node, TEXT("setVariable node"), {BlueprintTextNames::FieldId, BlueprintTextNames::FieldKind, BlueprintTextNames::FieldVariable, BlueprintTextNames::FieldArguments, BlueprintTextNames::FieldComment, BlueprintTextNames::FieldPos, BlueprintTextNames::FieldEnabled})) ||
            (Kind == KindGetVariable && ValidateKeys(Node, TEXT("getVariable node"), {BlueprintTextNames::FieldId, BlueprintTextNames::FieldKind, BlueprintTextNames::FieldVariable, BlueprintTextNames::FieldComment, BlueprintTextNames::FieldPos, BlueprintTextNames::FieldEnabled}));
        if (!bKeysValid)
        {
            if (Kind != KindEvent && Kind != KindCall && Kind != KindSequence && Kind != KindSetVariable && Kind != KindGetVariable)
                UE_LOG(LogTemp, Error, TEXT("Unsupported compact node kind '%s'."), *Kind);
            return false;
        }
        FString Id;
        if (!Node->TryGetStringField(BlueprintTextNames::FieldId, Id) || Id.IsEmpty()) { UE_LOG(LogTemp, Error, TEXT("nodes[%d] requires nonempty id."), Index); return false; }
        bool bEnabled = true;
        if (Node->TryGetBoolField(BlueprintTextNames::FieldEnabled, bEnabled) && !bEnabled)
        {
            UE_LOG(LogTemp, Error, TEXT("Disabled compact nodes are outside the representable subset."));
            return false;
        }
        if (Kind == KindSequence)
        {
            double Outputs = 0;
            if (!Node->TryGetNumberField(BlueprintTextNames::FieldOutputs, Outputs) || Outputs < 2 || Outputs > 16 || Outputs != FMath::FloorToDouble(Outputs))
            {
                UE_LOG(LogTemp, Error, TEXT("Sequence outputs must be an integer from 2 through 16."));
                return false;
            }
        }
    }
    for (int32 Index = 0; Index < Links->Num(); ++Index)
    {
        const TSharedPtr<FJsonObject> Link = (*Links)[Index]->AsObject();
        if (!Link.IsValid() || !ValidateKeys(Link, TEXT("link"), {BlueprintTextNames::FieldFrom, BlueprintTextNames::FieldTo})) return false;
    }
    return true;
}

bool ValidateRepresentableBlueprint(const UBlueprint* Blueprint)
{
    if (!Blueprint) { UE_LOG(LogTemp, Error, TEXT("Blueprint asset was not found.")); return false; }
    bool bOnlyAutomaticConstructionGraph = true;
    for (const UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
    {
        if (!FunctionGraph || FunctionGraph->GetName() != BlueprintTextNames::ConstructionScript)
        {
            bOnlyAutomaticConstructionGraph = false;
            break;
        }
        if (FunctionGraph->Nodes.Num() != 1 || FunctionGraph->Nodes[0]->GetClass() != UK2Node_FunctionEntry::StaticClass())
        {
            UE_LOG(LogTemp, Error, TEXT("Construction-script logic is outside the schema-1 subset; only the automatic entry node is supported."));
            return false;
        }
        for (const UEdGraphPin* Pin : FunctionGraph->Nodes[0]->Pins)
        {
            if (Pin->LinkedTo.Num() != 0)
            {
                UE_LOG(LogTemp, Error, TEXT("Linked construction-script logic is outside the schema-1 subset."));
                return false;
            }
        }
    }
    if (Blueprint->UbergraphPages.Num() != 1 || Blueprint->UbergraphPages[0]->GetName() != BlueprintTextNames::EventGraph ||
        !bOnlyAutomaticConstructionGraph || Blueprint->MacroGraphs.Num() != 0 ||
        Blueprint->DelegateSignatureGraphs.Num() != 0 || Blueprint->Timelines.Num() != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Blueprint state is outside the schema-1 subset: exactly one EventGraph, only the automatic construction graph, and no user functions, macros, delegates, or timelines are supported."));
        return false;
    }
    if (Blueprint->SimpleConstructionScript)
    {
        for (const USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (!Node || Node->GetVariableName() != BlueprintTextNames::DefaultSceneRoot)
            {
                UE_LOG(LogTemp, Error, TEXT("Blueprint components other than the automatic DefaultSceneRoot are outside the schema-1 subset."));
                return false;
            }
        }
    }
    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarType.ContainerType != EPinContainerType::None || Variable.VarType.bIsReference || Variable.VarType.bIsConst)
        {
            UE_LOG(LogTemp, Error, TEXT("Variable '%s' has unsupported attributes or a container type."), *Variable.VarName.ToString());
            return false;
        }
        constexpr uint64 BaselinePropertyFlags = CPF_Edit | CPF_BlueprintVisible | CPF_DisableEditOnInstance;
        if (Variable.PropertyFlags != BaselinePropertyFlags || Variable.MetaDataArray.Num() != 0 ||
            !Variable.RepNotifyFunc.IsNone() || Variable.ReplicationCondition != COND_None || !Variable.DefaultValue.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("Variable '%s' has unsupported saved flags, metadata, replication, or a serialized default."), *Variable.VarName.ToString());
            return false;
        }
        const UObject* Defaults = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetDefaultObject(false) : nullptr;
        const FProperty* Property = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->FindPropertyByName(Variable.VarName) : nullptr;
        bool bNonzeroDefault = false;
        if (Defaults && Property)
        {
            if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property)) bNonzeroDefault = BoolProperty->GetPropertyValue_InContainer(Defaults);
            else if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property)) bNonzeroDefault = FloatProperty->GetPropertyValue_InContainer(Defaults) != 0.0f;
            else if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property)) bNonzeroDefault = DoubleProperty->GetPropertyValue_InContainer(Defaults) != 0.0;
            else if (const FIntProperty* IntProperty = CastField<FIntProperty>(Property)) bNonzeroDefault = IntProperty->GetPropertyValue_InContainer(Defaults) != 0;
        }
        if (bNonzeroDefault)
        {
            UE_LOG(LogTemp, Error, TEXT("Variable '%s' has a nonzero saved CDO default, which is outside the schema-1 subset."), *Variable.VarName.ToString());
            return false;
        }
    }
    return true;
}

UBlueprint* PrepareBlueprint(const TSharedPtr<FJsonObject>& Envelope)
{
    const FString PackageName = ObjectPathToPackage(Envelope->GetStringField(BlueprintTextNames::FieldAsset));
    if (!FPackageName::IsValidLongPackageName(PackageName) || !PackageName.StartsWith(BlueprintTextNames::GamePackagePrefix))
    {
        UE_LOG(LogTemp, Error, TEXT("Asset must be a valid /Game package path.")); return nullptr;
    }
    UClass* Parent = LoadObject<UClass>(nullptr, *Envelope->GetStringField(BlueprintTextNames::FieldParent));
    if (!Parent) { UE_LOG(LogTemp, Error, TEXT("Parent class was not found.")); return nullptr; }
    UBlueprint* Blueprint = LoadBlueprint(PackageName);
    if (!Blueprint)
    {
        UPackage* Package = CreatePackage(*PackageName);
        Blueprint = FKismetEditorUtilities::CreateBlueprint(Parent, Package, *FPackageName::GetLongPackageAssetName(PackageName),
            BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass(), BlueprintTextNames::CreationContext);
    }
    else if (Blueprint->ParentClass != Parent)
    {
        UE_LOG(LogTemp, Error, TEXT("Changing an existing Blueprint parent is unsupported.")); return nullptr;
    }
    if (!ValidateRepresentableBlueprint(Blueprint)) return nullptr;
    TArray<FName> ExistingVariables;
    for (const FBPVariableDescription& Variable : Blueprint->NewVariables) ExistingVariables.Add(Variable.VarName);
    for (const FName Name : ExistingVariables) FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, Name);
    UEdGraph* Graph = Blueprint->UbergraphPages[0];
    const TArray<UEdGraphNode*> ExistingNodes = Graph->Nodes;
    for (UEdGraphNode* Node : ExistingNodes) FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
    for (const TSharedPtr<FJsonValue>& Value : Envelope->GetArrayField(BlueprintTextNames::FieldVariables))
    {
        const TSharedPtr<FJsonObject> Variable = Value->AsObject();
        FEdGraphPinType Type;
        const FName Name(*Variable->GetStringField(BlueprintTextNames::FieldName));
        if (!SetVariableType(Variable, Type) || !FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, Type)) return nullptr;
    }
    return Blueprint;
}

bool ImportCompact(UBlueprint* Blueprint, const TSharedPtr<FJsonObject>& GraphBody)
{
    UEdGraph* Graph = Blueprint->UbergraphPages[0];
    TMap<FString, UEdGraphNode*> Nodes;
    for (const TSharedPtr<FJsonValue>& Value : GraphBody->GetArrayField(BlueprintTextNames::FieldNodes))
    {
        const TSharedPtr<FJsonObject> Item = Value->AsObject();
        const FString Id = Item->GetStringField(BlueprintTextNames::FieldId); const FString Kind = Item->GetStringField(BlueprintTextNames::FieldKind);
        if (Nodes.Contains(Id)) { UE_LOG(LogTemp, Error, TEXT("Duplicate compact node id: %s"), *Id); return false; }
        UEdGraphNode* Node = nullptr;
        if (Kind == KindEvent)
        {
            FString OwnerPath, FunctionName;
            if (!SplitOperation(Item->GetStringField(KindEvent), OwnerPath, FunctionName)) return false;
            UClass* Owner = LoadObject<UClass>(nullptr, *OwnerPath); UFunction* Function = Owner ? Owner->FindFunctionByName(*FunctionName) : nullptr;
            if (!Function) { UE_LOG(LogTemp, Error, TEXT("Event operation was not found: %s::%s"), *OwnerPath, *FunctionName); return false; }
            UK2Node_Event* Event = NewObject<UK2Node_Event>(Graph); Event->EventReference.SetExternalMember(Function->GetFName(), Owner); Node = Event;
        }
        else if (Kind == KindCall)
        {
            FString OwnerPath, FunctionName;
            if (!SplitOperation(Item->GetStringField(BlueprintTextNames::FieldOperation), OwnerPath, FunctionName)) return false;
            UClass* Owner = LoadObject<UClass>(nullptr, *OwnerPath); UFunction* Function = Owner ? Owner->FindFunctionByName(*FunctionName) : nullptr;
            if (!Function) { UE_LOG(LogTemp, Error, TEXT("Call operation was not found: %s::%s"), *OwnerPath, *FunctionName); return false; }
            UK2Node_CallFunction* Call = NewObject<UK2Node_CallFunction>(Graph); Call->SetFromFunction(Function); Node = Call;
        }
        else if (Kind == KindSequence) Node = NewObject<UK2Node_ExecutionSequence>(Graph);
        else if (Kind == KindSetVariable) { UK2Node_VariableSet* Set = NewObject<UK2Node_VariableSet>(Graph); Set->VariableReference.SetSelfMember(*Item->GetStringField(BlueprintTextNames::FieldVariable)); Node = Set; }
        else if (Kind == KindGetVariable) { UK2Node_VariableGet* Get = NewObject<UK2Node_VariableGet>(Graph); Get->VariableReference.SetSelfMember(*Item->GetStringField(BlueprintTextNames::FieldVariable)); Node = Get; }
        else { UE_LOG(LogTemp, Error, TEXT("Unsupported compact node kind '%s'."), *Kind); return false; }
        FString Comment;
        Item->TryGetStringField(BlueprintTextNames::FieldComment, Comment);
        Node->CreateNewGuid();
        Node->PostPlacedNewNode();
        Node->AllocateDefaultPins();
        SetNodeId(Node, Id, Comment);
        const TArray<TSharedPtr<FJsonValue>>* Position = nullptr;
        if (Item->TryGetArrayField(BlueprintTextNames::FieldPos, Position))
        {
            if (Position->Num() != 2) { UE_LOG(LogTemp, Error, TEXT("Node pos requires [x,y].")); return false; }
            Node->NodePosX = static_cast<int32>((*Position)[0]->AsNumber());
            Node->NodePosY = static_cast<int32>((*Position)[1]->AsNumber());
        }
        Graph->AddNode(Node, false, false);
        Nodes.Add(Id, Node);
        if (Kind == KindSequence)
        {
            UK2Node_ExecutionSequence* Sequence = CastChecked<UK2Node_ExecutionSequence>(Node);
            const int32 Outputs = Item->GetIntegerField(BlueprintTextNames::FieldOutputs);
            while (Sequence->Pins.FilterByPredicate([](const UEdGraphPin* Pin){ return Pin->Direction == EGPD_Output; }).Num() < Outputs) Sequence->AddInputPin();
        }
        if (Item->HasTypedField<EJson::Object>(BlueprintTextNames::FieldArguments))
        {
            for (const TPair<FString,TSharedPtr<FJsonValue>>& Pair : Item->GetObjectField(BlueprintTextNames::FieldArguments)->Values)
            {
                UEdGraphPin* Pin = FindAliasPin(Node, Pair.Key, EGPD_Input);
                if (!Pin || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) { UE_LOG(LogTemp, Error, TEXT("Unknown input alias %s on %s."), *Pair.Key, *Id); return false; }
                Pin->DefaultValue = Pair.Value->AsString();
            }
        }
    }
    for (const TSharedPtr<FJsonValue>& Value : GraphBody->GetArrayField(BlueprintTextNames::FieldLinks))
    {
        const TSharedPtr<FJsonObject> Link = Value->AsObject(); FString FromId, FromPin, ToId, ToPin;
        if (!Link->GetStringField(BlueprintTextNames::FieldFrom).Split(BlueprintTextNames::MemberSeparator, &FromId, &FromPin, ESearchCase::CaseSensitive, ESearchDir::FromEnd) ||
            !Link->GetStringField(BlueprintTextNames::FieldTo).Split(BlueprintTextNames::MemberSeparator, &ToId, &ToPin, ESearchCase::CaseSensitive, ESearchDir::FromEnd)) return false;
        UEdGraphNode* FromNode = Nodes.FindRef(FromId); UEdGraphNode* ToNode = Nodes.FindRef(ToId);
        UEdGraphPin* Output = FromNode ? FindAliasPin(FromNode, FromPin, EGPD_Output) : nullptr;
        UEdGraphPin* Input = ToNode ? FindAliasPin(ToNode, ToPin, EGPD_Input) : nullptr;
        if (!Output || !Input || !GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(Output, Input))
        {
            UE_LOG(LogTemp, Error, TEXT("Compact link is unresolved or incompatible: %s -> %s"), *Link->GetStringField(BlueprintTextNames::FieldFrom), *Link->GetStringField(BlueprintTextNames::FieldTo)); return false;
        }
    }
    return true;
}

bool ImportEnvelope(const FString& InputPath, FString& AssetOut)
{
    TSharedPtr<FJsonObject> Envelope;
    if (!ReadJson(InputPath, Envelope) || !ValidateKeys(Envelope, TEXT("envelope"), {BlueprintTextNames::FieldSchema, BlueprintTextNames::FieldAsset, BlueprintTextNames::FieldParent, BlueprintTextNames::FieldVariables, BlueprintTextNames::FieldGraphs}) ||
        Envelope->GetIntegerField(BlueprintTextNames::FieldSchema) != Schema)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Blueprint envelope."));
        return false;
    }
    const TArray<TSharedPtr<FJsonValue>>& Variables = Envelope->GetArrayField(BlueprintTextNames::FieldVariables);
    for (int32 Index = 0; Index < Variables.Num(); ++Index)
        if (!Variables[Index]->AsObject().IsValid() || !ValidateVariable(Variables[Index]->AsObject(), Index)) return false;
    const TArray<TSharedPtr<FJsonValue>>& Graphs = Envelope->GetArrayField(BlueprintTextNames::FieldGraphs);
    if (Graphs.Num() != 1 || Graphs[0]->AsObject()->GetStringField(BlueprintTextNames::FieldName) != BlueprintTextNames::EventGraph) { UE_LOG(LogTemp, Error, TEXT("Exactly one EventGraph is supported.")); return false; }
    const TSharedPtr<FJsonObject> Graph = Graphs[0]->AsObject();
    if (!ValidateKeys(Graph, TEXT("graph"), {BlueprintTextNames::FieldName, BlueprintTextNames::FieldFormat, BlueprintTextNames::FieldBody, BlueprintTextNames::FieldBodyFile})) return false;
    const FString Format = Graph->GetStringField(BlueprintTextNames::FieldFormat);
    if (Format == FormatCompact)
    {
        if (!Graph->HasTypedField<EJson::Object>(BlueprintTextNames::FieldBody) || Graph->HasField(BlueprintTextNames::FieldBodyFile) || !ValidateCompactBody(Graph->GetObjectField(BlueprintTextNames::FieldBody))) return false;
    }
    else if (Format == FormatNative)
    {
        const bool bInline = Graph->HasTypedField<EJson::String>(BlueprintTextNames::FieldBody);
        const bool bFile = Graph->HasTypedField<EJson::String>(BlueprintTextNames::FieldBodyFile);
        if (bInline == bFile) { UE_LOG(LogTemp, Error, TEXT("Native graph requires exactly one of body or bodyFile.")); return false; }
    }
    else { UE_LOG(LogTemp, Error, TEXT("Graph format must be compact or native.")); return false; }
    UBlueprint* Blueprint = PrepareBlueprint(Envelope); if (!Blueprint) return false;
    bool bImported = false;
    if (Format == FormatCompact) bImported = ImportCompact(Blueprint, Graph->GetObjectField(BlueprintTextNames::FieldBody));
    else
    {
        FString NativeText;
        if (Graph->HasField(BlueprintTextNames::FieldBody)) NativeText = Graph->GetStringField(BlueprintTextNames::FieldBody);
        else
        {
            const FString BodyFile = Graph->GetStringField(BlueprintTextNames::FieldBodyFile);
            if (BodyFile != FPaths::GetCleanFilename(BodyFile)) { UE_LOG(LogTemp, Error, TEXT("Native bodyFile must name an adjacent file.")); return false; }
            const FString BodyPath = FPaths::Combine(FPaths::GetPath(InputPath), BodyFile);
            if (!FFileHelper::LoadFileToString(NativeText, *BodyPath)) { UE_LOG(LogTemp, Error, TEXT("Native bodyFile could not be read: %s"), *BodyPath); return false; }
        }
        TSet<UEdGraphNode*> Imported;
        FEdGraphUtilities::ImportNodesFromText(Blueprint->UbergraphPages[0], NativeText, Imported);
        bImported = Imported.Num() > 0;
    }
    if (!bImported || !SaveBlueprint(Blueprint)) return false;
    AssetOut = ObjectPathToPackage(Envelope->GetStringField(BlueprintTextNames::FieldAsset)); return true;
}

TSharedRef<FJsonObject> VariableJson(const FBPVariableDescription& Variable)
{
    const TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>(); Result->SetStringField(BlueprintTextNames::FieldName, Variable.VarName.ToString());
    FString Type = BlueprintTextNames::UnsupportedType;
    if (Variable.VarType.PinCategory == UEdGraphSchema_K2::PC_Real) Type = Variable.VarType.PinSubCategory == UEdGraphSchema_K2::PC_Float ? BlueprintTextNames::TypeFloat : BlueprintTextNames::TypeDouble;
    else if (Variable.VarType.PinCategory == UEdGraphSchema_K2::PC_Int) Type = BlueprintTextNames::TypeInt;
    else if (Variable.VarType.PinCategory == UEdGraphSchema_K2::PC_Boolean) Type = BlueprintTextNames::TypeBool;
    Result->SetStringField(BlueprintTextNames::FieldType, Type);
    return Result;
}

TSharedRef<FJsonObject> ExportCompactGraph(UBlueprint* Blueprint)
{
    const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Nodes; TArray<TSharedPtr<FJsonValue>> Links;
    TMap<const UEdGraphNode*, FString> Ids; for (UEdGraphNode* Node : Blueprint->UbergraphPages[0]->Nodes) Ids.Add(Node, NodeId(Node));
    for (UEdGraphNode* Node : Blueprint->UbergraphPages[0]->Nodes)
    {
        if (Node->GetDesiredEnabledState() != ENodeEnabledState::Enabled)
        {
            UE_LOG(LogTemp, Error, TEXT("Compact export does not support disabled nodes."));
            return MakeShared<FJsonObject>();
        }
        const TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
        Item->SetStringField(BlueprintTextNames::FieldId, Ids.FindRef(Node));
        if (Node->GetClass() == UK2Node_Event::StaticClass())
        {
            UK2Node_Event* Event = CastChecked<UK2Node_Event>(Node);
            UFunction* Function = Event->FindEventSignatureFunction();
            if (!Function) return MakeShared<FJsonObject>();
            Item->SetStringField(BlueprintTextNames::FieldKind, KindEvent);
            Item->SetStringField(KindEvent, Function->GetOuterUClass()->GetPathName() + BlueprintTextNames::OperationSeparator + Function->GetName());
        }
        else if (Node->GetClass() == UK2Node_CallFunction::StaticClass())
        {
            UK2Node_CallFunction* Call = CastChecked<UK2Node_CallFunction>(Node);
            UFunction* Function = Call->GetTargetFunction();
            if (!Function) return MakeShared<FJsonObject>();
            Item->SetStringField(BlueprintTextNames::FieldKind, KindCall);
            Item->SetStringField(BlueprintTextNames::FieldOperation, Function->GetOuterUClass()->GetPathName() + BlueprintTextNames::OperationSeparator + Function->GetName());
        }
        else if (Node->GetClass() == UK2Node_ExecutionSequence::StaticClass())
        {
            UK2Node_ExecutionSequence* Sequence = CastChecked<UK2Node_ExecutionSequence>(Node);
            Item->SetStringField(BlueprintTextNames::FieldKind, KindSequence);
            Item->SetNumberField(BlueprintTextNames::FieldOutputs, Sequence->Pins.FilterByPredicate([](const UEdGraphPin* Pin){ return Pin->Direction == EGPD_Output; }).Num());
        }
        else if (Node->GetClass() == UK2Node_VariableSet::StaticClass())
        {
            UK2Node_VariableSet* Set = CastChecked<UK2Node_VariableSet>(Node);
            Item->SetStringField(BlueprintTextNames::FieldKind, KindSetVariable);
            Item->SetStringField(BlueprintTextNames::FieldVariable, Set->VariableReference.GetMemberName().ToString());
        }
        else if (Node->GetClass() == UK2Node_VariableGet::StaticClass())
        {
            UK2Node_VariableGet* Get = CastChecked<UK2Node_VariableGet>(Node);
            Item->SetStringField(BlueprintTextNames::FieldKind, KindGetVariable);
            Item->SetStringField(BlueprintTextNames::FieldVariable, Get->VariableReference.GetMemberName().ToString());
        }
        else { UE_LOG(LogTemp, Error, TEXT("Compact export does not support node class %s."), *Node->GetClass()->GetPathName()); return MakeShared<FJsonObject>(); }
        const FString Comment = NodeUserComment(Node);
        if (!Comment.IsEmpty()) Item->SetStringField(BlueprintTextNames::FieldComment, Comment);
        if (Node->NodePosX != 0 || Node->NodePosY != 0)
        {
            TArray<TSharedPtr<FJsonValue>> Position;
            Position.Add(MakeShared<FJsonValueNumber>(Node->NodePosX));
            Position.Add(MakeShared<FJsonValueNumber>(Node->NodePosY));
            Item->SetArrayField(BlueprintTextNames::FieldPos, Position);
        }
        const TSharedRef<FJsonObject> Arguments=MakeShared<FJsonObject>();
        for(UEdGraphPin* Pin:Node->Pins) if(Pin->Direction==EGPD_Input&&Pin->PinType.PinCategory!=UEdGraphSchema_K2::PC_Exec&&Pin->LinkedTo.Num()==0&&Pin->PinName!=UEdGraphSchema_K2::PN_Self&&!Pin->bHidden) Arguments->SetStringField(AliasForPin(Node,Pin),Pin->DefaultValue);
        if(Arguments->Values.Num())Item->SetObjectField(BlueprintTextNames::FieldArguments,Arguments);Nodes.Add(MakeShared<FJsonValueObject>(Item));
        for(UEdGraphPin* Pin:Node->Pins)if(Pin->Direction==EGPD_Output)for(UEdGraphPin* Other:Pin->LinkedTo){const FString OtherId=Ids.FindRef(Other->GetOwningNode());if(!OtherId.IsEmpty()){const TSharedRef<FJsonObject>L=MakeShared<FJsonObject>();L->SetStringField(BlueprintTextNames::FieldFrom,Ids.FindRef(Node)+BlueprintTextNames::MemberSeparator+AliasForPin(Node,Pin));L->SetStringField(BlueprintTextNames::FieldTo,OtherId+BlueprintTextNames::MemberSeparator+AliasForPin(Other->GetOwningNode(),Other));Links.Add(MakeShared<FJsonValueObject>(L));}}
    }
    Body->SetArrayField(BlueprintTextNames::FieldNodes,Nodes);Body->SetArrayField(BlueprintTextNames::FieldLinks,Links);return Body;
}

bool ExportEnvelope(const FString& Asset, const FString& Format, const FString& OutputPath)
{
    UBlueprint* Blueprint = LoadBlueprint(Asset);
    if (!ValidateRepresentableBlueprint(Blueprint)) return false;
    const TSharedRef<FJsonObject> Root=MakeShared<FJsonObject>();Root->SetNumberField(BlueprintTextNames::FieldSchema,Schema);Root->SetStringField(BlueprintTextNames::FieldAsset,Blueprint->GetOutermost()->GetName());Root->SetStringField(BlueprintTextNames::FieldParent,Blueprint->ParentClass->GetPathName());
    TArray<TSharedPtr<FJsonValue>> Variables;
    for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        const TSharedRef<FJsonObject> Value = VariableJson(Variable);
        if (Value->GetStringField(BlueprintTextNames::FieldType) == BlueprintTextNames::UnsupportedType)
        {
            UE_LOG(LogTemp, Error, TEXT("Variable '%s' has an unsupported type."), *Variable.VarName.ToString());
            return false;
        }
        Variables.Add(MakeShared<FJsonValueObject>(Value));
    }
    Root->SetArrayField(BlueprintTextNames::FieldVariables,Variables);
    const TSharedRef<FJsonObject> Graph=MakeShared<FJsonObject>();Graph->SetStringField(BlueprintTextNames::FieldName,BlueprintTextNames::EventGraph);Graph->SetStringField(BlueprintTextNames::FieldFormat,Format);
    if (Format == FormatNative)
    {
        TSet<UObject*> Selection;
        for (UEdGraphNode* Node : Blueprint->UbergraphPages[0]->Nodes) Selection.Add(Node);
        FString NativeText;
        FEdGraphUtilities::ExportNodesToText(Selection, NativeText);
        const FString BodyPath = FPaths::ChangeExtension(OutputPath, BlueprintTextNames::NativeBodyExtension);
        if (!FFileHelper::SaveStringToFile(NativeText, *BodyPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
        Graph->SetStringField(BlueprintTextNames::FieldBodyFile, FPaths::GetCleanFilename(BodyPath));
    }
    else if(Format==FormatCompact){const TSharedRef<FJsonObject>Body=ExportCompactGraph(Blueprint);if(!Body->HasField(BlueprintTextNames::FieldNodes))return false;Graph->SetObjectField(BlueprintTextNames::FieldBody,Body);}
    else{UE_LOG(LogTemp,Error,TEXT("Export format must be compact or native."));return false;}
    TArray<TSharedPtr<FJsonValue>>Graphs;Graphs.Add(MakeShared<FJsonValueObject>(Graph));Root->SetArrayField(BlueprintTextNames::FieldGraphs,Graphs);return WriteJson(OutputPath,Root);
}

bool BuildCatalog(const FString& OutputPath)
{
    UPackage* CatalogPackage=CreatePackage(BlueprintTextNames::CatalogPackage);
    UBlueprint* CatalogBlueprint=FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(),CatalogPackage,BlueprintTextNames::CatalogBlueprint,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass(),BlueprintTextNames::CreationContext);
    if(!CatalogBlueprint||CatalogBlueprint->UbergraphPages.Num()!=1){UE_LOG(LogTemp,Error,TEXT("Could not create transient catalog Blueprint context."));return false;}
    UEdGraph* Graph=CatalogBlueprint->UbergraphPages[0];TArray<TSharedPtr<FJsonValue>>Entries;
    for(TFieldIterator<UFunction>It(UKismetMathLibrary::StaticClass(),EFieldIteratorFlags::ExcludeSuper);It;++It){UFunction*F=*It;if(!F->HasAnyFunctionFlags(FUNC_BlueprintCallable|FUNC_BlueprintPure))continue;UK2Node_CallFunction*Call=NewObject<UK2Node_CallFunction>(Graph);Call->SetFromFunction(F);Call->CreateNewGuid();Call->AllocateDefaultPins();
        const TSharedRef<FJsonObject>E=MakeShared<FJsonObject>();E->SetStringField(BlueprintTextNames::FieldIdentity,F->GetOuterUClass()->GetPathName()+BlueprintTextNames::OperationSeparator+F->GetName());E->SetStringField(BlueprintTextNames::FieldKind,KindCall);E->SetStringField(BlueprintTextNames::FieldTitle,F->GetDisplayNameText().ToString());E->SetStringField(BlueprintTextNames::FieldKeywords,F->GetMetaData(BlueprintTextNames::FunctionKeywords));
        TArray<TSharedPtr<FJsonValue>>Pins;for(UEdGraphPin*Pin:Call->Pins){if(Pin->bHidden)continue;const TSharedRef<FJsonObject>P=MakeShared<FJsonObject>();P->SetStringField(BlueprintTextNames::FieldAlias,AliasForPin(Call,Pin));P->SetStringField(BlueprintTextNames::FieldName,Pin->PinName.ToString());P->SetStringField(BlueprintTextNames::FieldDirection,Pin->Direction==EGPD_Input?BlueprintTextNames::DirectionInput:BlueprintTextNames::DirectionOutput);P->SetObjectField(BlueprintTextNames::FieldType,PinTypeJson(Pin));Pins.Add(MakeShared<FJsonValueObject>(P));}E->SetArrayField(BlueprintTextNames::FieldPins,Pins);
        const TSharedRef<FJsonObject>T=MakeShared<FJsonObject>();T->SetStringField(BlueprintTextNames::FieldId,BlueprintTextNames::TemplateId);T->SetStringField(BlueprintTextNames::FieldKind,KindCall);T->SetStringField(BlueprintTextNames::FieldOperation,E->GetStringField(BlueprintTextNames::FieldIdentity));E->SetObjectField(BlueprintTextNames::FieldCompactTemplate,T);TSet<UObject*>Selection;Selection.Add(Call);FString Native;FEdGraphUtilities::ExportNodesToText(Selection,Native);E->SetStringField(BlueprintTextNames::FieldNativeInsertion,Native);Entries.Add(MakeShared<FJsonValueObject>(E));}
    UK2Node_ExecutionSequence*Sequence=NewObject<UK2Node_ExecutionSequence>(Graph);Sequence->CreateNewGuid();Sequence->AllocateDefaultPins();const TSharedRef<FJsonObject>S=MakeShared<FJsonObject>();S->SetStringField(BlueprintTextNames::FieldIdentity,BlueprintTextNames::SequenceOperation);S->SetStringField(BlueprintTextNames::FieldKind,KindSequence);S->SetStringField(BlueprintTextNames::FieldTitle,TEXT("Sequence"));S->SetStringField(BlueprintTextNames::FieldKeywords,TEXT("flow execution sequence"));const TSharedRef<FJsonObject>ST=MakeShared<FJsonObject>();ST->SetStringField(BlueprintTextNames::FieldId,BlueprintTextNames::TemplateId);ST->SetStringField(BlueprintTextNames::FieldKind,KindSequence);ST->SetNumberField(BlueprintTextNames::FieldOutputs,2);S->SetObjectField(BlueprintTextNames::FieldCompactTemplate,ST);TSet<UObject*>SS;SS.Add(Sequence);FString SN;FEdGraphUtilities::ExportNodesToText(SS,SN);S->SetStringField(BlueprintTextNames::FieldNativeInsertion,SN);Entries.Add(MakeShared<FJsonValueObject>(S));
    const TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(BlueprintTextNames::FieldSchema, 1);
    Root->SetStringField(BlueprintTextNames::FieldEngineVersion, FEngineVersion::Current().ToString());
    Root->SetStringField(BlueprintTextNames::FieldCoverage, TEXT("KismetMathLibrary BlueprintCallable/BlueprintPure reflection plus supported flow.sequence; not the full BlueprintActionDatabase"));
    Root->SetStringField(BlueprintTextNames::FieldRefresh, TEXT("Invoke-BlueprintText.ps1 -Action RefreshCatalog -ProjectRoot <prepared project> -Force"));
    Root->SetArrayField(BlueprintTextNames::FieldEntries, Entries);
    return WriteJson(OutputPath, Root);
}
}

UBlueprintTextCommandlet::UBlueprintTextCommandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UBlueprintTextCommandlet::Main(const FString& Params)
{
    FString Mode,Input,Output,Asset,Format,Variable;FParse::Value(*Params,BlueprintTextNames::ArgumentMode,Mode);FParse::Value(*Params,BlueprintTextNames::ArgumentInput,Input);FParse::Value(*Params,BlueprintTextNames::ArgumentOutput,Output);FParse::Value(*Params,BlueprintTextNames::ArgumentAsset,Asset);FParse::Value(*Params,BlueprintTextNames::ArgumentFormat,Format);FParse::Value(*Params,BlueprintTextNames::ArgumentVariable,Variable);
    if(Mode==BlueprintTextNames::ModeImport){FString Created;return ImportEnvelope(Input,Created)?0:2;}
    if(Mode==BlueprintTextNames::ModeExport)return ExportEnvelope(Asset,Format,Output)?0:3;
    if(Mode==BlueprintTextNames::ModeCatalog)return BuildCatalog(Output)?0:4;
    if(Mode==BlueprintTextNames::ModeSimulateEdit)
    {
        UBlueprint* BP = LoadBlueprint(Asset);
        FEdGraphPinType Type;
        Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
        if(!BP || Variable.IsEmpty() || !FBlueprintEditorUtils::AddMemberVariable(BP, *Variable, Type)) return 5;
        return SaveBlueprint(BP) ? 0 : 5;
    }
    if (Mode == BlueprintTextNames::ModeSimulateUnsupportedGraph)
    {
        UBlueprint* BP = LoadBlueprint(Asset);
        if (!BP) return 6;
        UEdGraph* Graph = FBlueprintEditorUtils::CreateNewGraph(BP, BlueprintTextNames::FixtureFunction, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
        BP->FunctionGraphs.Add(Graph);
        return SaveBlueprintPackage(BP) ? 0 : 6;
    }
    if (Mode == BlueprintTextNames::ModeSimulateNonzeroDefault)
    {
        UBlueprint* BP = LoadBlueprint(Asset);
        if (!BP || !BP->GeneratedClass) return 7;
        FFloatProperty* Property = FindFProperty<FFloatProperty>(BP->GeneratedClass, *Variable);
        UObject* Defaults = BP->GeneratedClass->GetDefaultObject();
        if (!Property || !Defaults) return 7;
        Property->SetPropertyValue_InContainer(Defaults, 12.0f);
        BP->MarkPackageDirty();
        return SaveBlueprintPackage(BP) ? 0 : 7;
    }
    if (Mode == BlueprintTextNames::ModeSimulateConstructionLogic)
    {
        UBlueprint* BP = LoadBlueprint(Asset);
        UEdGraph* Graph = nullptr;
        if (BP)
        {
            for (UEdGraph* Candidate : BP->FunctionGraphs)
                if (Candidate && Candidate->GetName() == BlueprintTextNames::ConstructionScript) { Graph = Candidate; break; }
        }
        if (!Graph) return 8;
        UK2Node_ExecutionSequence* Node = NewObject<UK2Node_ExecutionSequence>(Graph);
        Node->CreateNewGuid(); Node->PostPlacedNewNode(); Node->AllocateDefaultPins(); Graph->AddNode(Node, false, false);
        BP->MarkPackageDirty();
        return SaveBlueprintPackage(BP) ? 0 : 8;
    }
    UE_LOG(LogTemp,Error,TEXT("Mode must be Import, Export, Catalog, SimulateEdit, or SimulateUnsupportedGraph."));return 1;
}
