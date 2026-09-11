// Hand-written graph generator. Native identities reviewed for Steam 25191271;
// provenance and revalidation requirements: ../Voyage/ModulesConnectCable.h.
#include "GenerateMooringCommandlet.h"
#include "ModulesConnectCable.h"
#include "BlueprintGraphNames.h"
#include "TimerGraphNames.h"
#include "Modules/ModuleManager.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "HAL/FileManager.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
IMPLEMENT_MODULE(FDefaultModuleImpl, MooringGenerator)
namespace P = BlueprintGraphNames::Pins;
namespace
{
constexpr TCHAR OriginalPackage[] = TEXT("/Game/Blueprints/Cables/BP_ModuleCable_Mooring");
constexpr TCHAR ParentPackage[] = TEXT("/Game/Blueprints/Cables/BP_ModuleCable_Orig060");
constexpr TCHAR FreeLimit[] = TEXT("6000");
constexpr TCHAR AttachedLimit[] = TEXT("2000");
constexpr TCHAR RefreshSeconds[] = TEXT("0.05");
constexpr TCHAR TrueLiteral[] = TEXT("true");
const FName& BeginPlayName = TimerGraphNames::ActorBeginPlay;
const FName RefreshName(TEXT("MooringRefreshLimit"));
const FName LimitName(TEXT("MaxLength"));
const FName SocketAName(TEXT("CachedSocketA"));
const FName SocketBName(TEXT("CachedSocketB"));
const FName& ValidObjectPin = P::Object;
const FName& TimerObjectPin = TimerGraphNames::Object;
const FName& TimerFunctionPin = TimerGraphNames::Function;
const FName& TimerTimePin = TimerGraphNames::Interval;
const FName& TimerLoopPin = TimerGraphNames::Looping;

struct FGraph
{
    UEdGraph* Graph;
    template<class T> T* Add(T* Node)
    {
        Node->CreateNewGuid(); Node->PostPlacedNewNode(); Node->AllocateDefaultPins();
        Graph->AddNode(Node, false, false); return Node;
    }
    UEdGraphPin* Pin(UEdGraphNode* Node, FName Name)
    {
        auto* Result = Node->FindPin(Name);
        checkf(Result, TEXT("Missing pin %s"), *Name.ToString()); return Result;
    }
    void Link(UEdGraphPin* From, UEdGraphPin* To)
    {
        checkf(GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(From, To),
            TEXT("Cannot connect %s to %s"), *From->PinName.ToString(), *To->PinName.ToString());
    }
    void Value(UEdGraphNode* Node, FName Name, const FString& Text)
    {
        GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Pin(Node, Name), Text);
    }
    UK2Node_CallFunction* Call(UClass* Owner, FName Name)
    {
        auto* Function = Owner->FindFunctionByName(Name);
        checkf(Function, TEXT("Missing function %s"), *Name.ToString());
        auto* Node = NewObject<UK2Node_CallFunction>(Graph); Node->SetFromFunction(Function);
        return Add(Node);
    }
    UEdGraphPin* ValidSocket(FName Name)
    {
        auto* Read = NewObject<UK2Node_VariableGet>(Graph);
        Read->VariableReference.SetExternalMember(Name, AModulesConnectCable::StaticClass()); Add(Read);
        auto* Self = Add(NewObject<UK2Node_Self>(Graph));
        Link(Pin(Self, P::FunctionTarget), Pin(Read, P::FunctionTarget));
        auto* Valid = Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, IsValid));
        Link(Pin(Read, Name), Pin(Valid, ValidObjectPin)); return Pin(Valid, P::ReturnValue);
    }
    void SetLimit(UEdGraphPin* Exec, const TCHAR* Limit)
    {
        auto* Write = NewObject<UK2Node_VariableSet>(Graph);
        Write->VariableReference.SetExternalMember(LimitName, AModulesConnectCable::StaticClass()); Add(Write);
        auto* Self = Add(NewObject<UK2Node_Self>(Graph));
        Link(Pin(Self, P::FunctionTarget), Pin(Write, P::FunctionTarget));
        Value(Write, LimitName, Limit); Link(Exec, Pin(Write, P::Execute));
    }
};

UBlueprint* Make(const TCHAR* Path, UClass* Parent)
{
    auto* Package = CreatePackage(Path);
    auto* BP = FKismetEditorUtilities::CreateBlueprint(Parent, Package,
        FName(*FPackageName::GetLongPackageAssetName(Path)), BPTYPE_Normal,
        UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    check(BP); return BP;
}
void Save(UBlueprint* BP)
{
    FKismetEditorUtilities::CompileBlueprint(BP);
    checkf(BP->Status != BS_Error, TEXT("Blueprint compile failed: %s"), *BP->GetPathName());
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    const FString Filename = FPackageName::LongPackageNameToFilename(BP->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    checkf(UPackage::SavePackage(BP->GetOutermost(), BP, *Filename, Args), TEXT("Save failed: %s"), *Filename);
}
}
UGenerateMooringCommandlet::UGenerateMooringCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true;
}
int32 UGenerateMooringCommandlet::Main(const FString& Params)
{
    auto* Parent = Make(ParentPackage, AModulesConnectCable::StaticClass());
    Save(Parent); // Editor placeholder only; replaced by freshly relocated stock package.
    auto* Child = Make(OriginalPackage, Parent->GeneratedClass);
    check(Child->UbergraphPages.Num() == 1);
    FGraph G{Child->UbergraphPages[0]};
    auto* Begin = NewObject<UK2Node_Event>(G.Graph);
    Begin->EventReference.SetExternalMember(BeginPlayName, AActor::StaticClass());
    Begin->bOverrideFunction = true; G.Add(Begin);
    auto* Timer = G.Call(UKismetSystemLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, K2_SetTimer));
    auto* Self = G.Add(NewObject<UK2Node_Self>(G.Graph));
    G.Link(G.Pin(Self, P::FunctionTarget), G.Pin(Timer, TimerObjectPin));
    G.Value(Timer, TimerFunctionPin, RefreshName.ToString());
    G.Value(Timer, TimerTimePin, RefreshSeconds); G.Value(Timer, TimerLoopPin, TrueLiteral);
    G.Link(G.Pin(Begin, P::Then), G.Pin(Timer, P::Execute));
    auto* Refresh = NewObject<UK2Node_CustomEvent>(G.Graph); Refresh->CustomFunctionName = RefreshName; G.Add(Refresh);
    auto* Both = G.Call(UKismetMathLibrary::StaticClass(), GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanAND));
    G.Link(G.ValidSocket(SocketAName), G.Pin(Both, P::Binary::LeftOperand));
    G.Link(G.ValidSocket(SocketBName), G.Pin(Both, P::Binary::RightOperand));
    auto* Branch = G.Add(NewObject<UK2Node_IfThenElse>(G.Graph));
    G.Link(G.Pin(Refresh, P::Then), G.Pin(Branch, P::Execute));
    G.Link(G.Pin(Both, P::ReturnValue), G.Pin(Branch, P::Condition));
    G.SetLimit(G.Pin(Branch, P::Then), AttachedLimit);
    G.SetLimit(G.Pin(Branch, P::Else), FreeLimit);
    Save(Child);
    UE_LOG(LogTemp, Display, TEXT("Mooring limit graph generated; native runtime validation pending."));
    return 0;
}
