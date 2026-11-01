#include "IniParserTestCommandlet.h"
#include "Dom/JsonObject.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "K2Node_CallFunction.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace IniTest
{
constexpr TCHAR InputArgument[]=TEXT("Input=");
constexpr TCHAR OutputArgument[]=TEXT("Output=");
constexpr TCHAR PackageA[]=TEXT("/Game/Mods/IniTestA/BP_IniFileParser.BP_IniFileParser_C");
constexpr TCHAR PackageB[]=TEXT("/Game/Mods/IniTestB/BP_IniFileParser.BP_IniFileParser_C");
constexpr TCHAR ReaderClass[]=TEXT("/Script/Voyage.VoyageEditorBlueprintFunctionLibrary");
constexpr TCHAR CounterFunction[]=TEXT("GetTestReadCount");
constexpr TCHAR FileName[]=TEXT("settings.ini");
constexpr TCHAR FileToken[]=TEXT("$file");
constexpr TCHAR Steps[]=TEXT("steps");
constexpr TCHAR Write[]=TEXT("write");
constexpr TCHAR Delete[]=TEXT("delete");
constexpr TCHAR Method[]=TEXT("method");
constexpr TCHAR Object[]=TEXT("object");
constexpr TCHAR Args[]=TEXT("args");
constexpr TCHAR Expected[]=TEXT("expected");
constexpr TCHAR Reads[]=TEXT("reads");
constexpr TCHAR ReturnValue[]=TEXT("ReturnValue");
constexpr TCHAR GetFloat[]=TEXT("GetFloat");
constexpr TCHAR GetDouble[]=TEXT("GetDouble");
constexpr TCHAR GetInt[]=TEXT("GetInt");
constexpr TCHAR GetBool[]=TEXT("GetBool");
constexpr TCHAR GetString[]=TEXT("GetString");
constexpr TCHAR Minimum[]=TEXT("Min");
constexpr TCHAR Maximum[]=TEXT("Max");
constexpr TCHAR Default[]=TEXT("Default");
constexpr TCHAR Label[]=TEXT("label");

bool Set(FProperty* Property,void* Address,const TSharedPtr<FJsonValue>& Json,const FString& File)
{
    if(auto* String=CastField<FStrProperty>(Property)){String->SetPropertyValue(Address,Json->AsString().Replace(FileToken,*File));return true;}
    if(auto* Bool=CastField<FBoolProperty>(Property)){Bool->SetPropertyValue(Address,Json->AsBool());return true;}
    if(auto* Number=CastField<FNumericProperty>(Property))
    {
        if(Number->IsFloatingPoint())Number->SetFloatingPointPropertyValue(Address,Json->AsNumber());
        else Number->SetIntPropertyValue(Address,static_cast<int64>(Json->AsNumber()));
        return true;
    }
    return false;
}
bool Equal(FProperty* Property,const void* Address,const TSharedPtr<FJsonValue>& ExpectedValue)
{
    if(auto* String=CastField<FStrProperty>(Property))return String->GetPropertyValue(Address).Equals(ExpectedValue->AsString(),ESearchCase::CaseSensitive);
    if(auto* Bool=CastField<FBoolProperty>(Property))return Bool->GetPropertyValue(Address)==ExpectedValue->AsBool();
    if(auto* Number=CastField<FNumericProperty>(Property))
    {
        const double Value=Number->IsFloatingPoint()?Number->GetFloatingPointPropertyValue(Address):double(Number->GetSignedIntPropertyValue(Address));
        const double Want=ExpectedValue->AsNumber();
        // Compare the actual storage value exactly; a float32 detour must fail double tests.
        return Value==(CastField<FFloatProperty>(Property)?double(float(Want)):Want);
    }
    return false;
}
bool Call(UObject* Instance,const TSharedPtr<FJsonObject>& Step,const FString& File)
{
    auto* Function=Instance->FindFunction(*Step->GetStringField(Method));if(!Function)return false;
    FStructOnScope Params(Function);
    const TSharedPtr<FJsonObject>* Arguments=nullptr;
    if(Step->TryGetObjectField(Args,Arguments))for(const auto& Pair:(*Arguments)->Values)
    {
        auto* Property=FindFProperty<FProperty>(Function,*Pair.Key);
        if(!Property || !Property->HasAnyPropertyFlags(CPF_Parm) ||
           !Set(Property,Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()),Pair.Value,File))return false;
    }
    Instance->ProcessEvent(Function,Params.GetStructMemory());
    if(Step->HasField(Expected))
    {
        auto* Result=FindFProperty<FProperty>(Function,ReturnValue);
        if(!Result)return false;
        const void* Address=Result->ContainerPtrToValuePtr<void>(Params.GetStructMemory());
        if(!Equal(Result,Address,Step->TryGetField(Expected)))
        {
            FString Actual;Result->ExportTextItem_Direct(Actual,Address,nullptr,Instance,PPF_None);
            UE_LOG(LogTemp,Error,TEXT("%s returned %s"),*Function->GetName(),*Actual);return false;
        }
    }
    return true;
}
bool DefaultPins(UClass* Class)
{
    auto* Blueprint=Cast<UBlueprint>(Class->ClassGeneratedBy);if(!Blueprint)return false;
    auto* Graph=NewObject<UEdGraph>(Blueprint);Graph->Schema=UEdGraphSchema_K2::StaticClass();
    for(const TCHAR* Name:{GetFloat,GetDouble,GetInt,GetBool,GetString})
    {
        auto* Function=Class->FindFunctionByName(Name);if(!Function || !Function->HasAnyFunctionFlags(FUNC_BlueprintPure))return false;
        auto* Node=NewObject<UK2Node_CallFunction>(Graph);Node->SetFromFunction(Function);Node->AllocateDefaultPins();
        const auto* DefaultPin=Node->FindPin(Default);if(!DefaultPin)return false;
        if(FString(Name)==GetString && !DefaultPin->DefaultValue.IsEmpty())return false;
        if(FString(Name)==GetBool && DefaultPin->DefaultValue.ToBool())return false;
        if(FString(Name)==GetDouble)
        {
            for(const TCHAR* Parameter:{Default,Minimum,Maximum,ReturnValue})
                if(!FindFProperty<FDoubleProperty>(Function,Parameter))return false;
        }
        if(FString(Name)==GetFloat || FString(Name)==GetDouble || FString(Name)==GetInt)
        {
            const auto* MinPin=Node->FindPin(Minimum);const auto* MaxPin=Node->FindPin(Maximum);
            if(!MinPin||!MaxPin)return false;
            if(FCString::Atod(*DefaultPin->DefaultValue)!=0.0)return false;
            const double Lower=FCString::Atod(*MinPin->DefaultValue),Upper=FCString::Atod(*MaxPin->DefaultValue);
            if(FString(Name)==GetInt){if(Lower!=MIN_int32||Upper!=MAX_int32)return false;}
            else if(FString(Name)==GetDouble){if(Lower!=-TNumericLimits<double>::Max()||Upper!=TNumericLimits<double>::Max())return false;}
            else if(Lower!=-double(MAX_flt)||Upper!=double(MAX_flt))return false;
        }
    }
    return true;
}
}

UIniParserTestCommandlet::UIniParserTestCommandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
int32 UIniParserTestCommandlet::Main(const FString& Params)
{
    using namespace IniTest;
    FString Input,Output,Text;
    FParse::Value(*Params,InputArgument,Input);FParse::Value(*Params,OutputArgument,Output);
    if(!FFileHelper::LoadFileToString(Text,*Input))return 1;
    TSharedPtr<FJsonObject> Suite;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Suite))return 1;
    auto* ClassA=LoadObject<UClass>(nullptr,PackageA);auto* ClassB=LoadObject<UClass>(nullptr,PackageB);
    if(!ClassA||!ClassB||ClassA==ClassB||!DefaultPins(ClassA))
    {UE_LOG(LogTemp,Error,TEXT("INI parser classes or default call pins are invalid."));return 2;}
    TStrongObjectPtr<UObject> A(NewObject<UObject>(GetTransientPackage(),ClassA));
    TStrongObjectPtr<UObject> B(NewObject<UObject>(GetTransientPackage(),ClassB));
    UClass* Reader=LoadObject<UClass>(nullptr,ReaderClass);if(!Reader)return 2;
    UFunction* Count=Reader->FindFunctionByName(CounterFunction);if(!Count)return 2;
    const FString File=FPaths::GetPath(Output)/FileName;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
    int32 Index=0;
    for(const auto& Value:Suite->GetArrayField(Steps))
    {
        const auto Step=Value->AsObject();bool Ok=true;
        if(Step->HasField(Write))Ok=FFileHelper::SaveStringToFile(Step->GetStringField(Write),*File,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        else if(Step->HasField(Delete))Ok=IFileManager::Get().Delete(*File,false,true);
        else if(Step->HasField(Method))Ok=Call(Step->GetIntegerField(Object)==1?A.Get():B.Get(),Step,File);
        if(Ok && Step->HasField(Reads))
        {
            int32 Actual=0;Reader->GetDefaultObject()->ProcessEvent(Count,&Actual);Ok=Actual==Step->GetIntegerField(Reads);
        }
        if(!Ok){UE_LOG(LogTemp,Error,TEXT("INI parser test step %d failed: %s"),Index,*Step->GetStringField(Label));return 3;}
        ++Index;
    }
    IFileManager::Get().Delete(*File,false,true);
    const FString Report=FString::Printf(TEXT("{\"status\":\"passed\",\"steps\":%d,\"instances\":2,\"defaultPins\":true}"),Index);
    if(!FFileHelper::SaveStringToFile(Report,*Output,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))return 4;
    UE_LOG(LogTemp,Display,TEXT("INI parser tests passed: %d steps."),Index);return 0;
}
