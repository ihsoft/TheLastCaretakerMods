#pragma once
#include "UObject/NameTypes.h"

// Engine-only graph pins for actor enumeration and exact string identity tests.
namespace ActorScanGraphNames
{
inline const FName ComponentTag(TEXT("Tag"));
// AActor::GetOwner also has a templated overload.
inline const FName GetActorOwner(TEXT("GetOwner"));
inline const FName ForEachLoop(TEXT("ForEachLoop"));
inline const FName Array(TEXT("Array"));
inline const FName ArrayElement(TEXT("Array Element"));
inline const FName RemoveItem(TEXT("Item"));
inline const FName LoopBody(TEXT("LoopBody"));
inline const FName OutActors(TEXT("OutActors"));
inline const FName SoftClassReference(TEXT("SoftClassReference"));
inline const FName SourceString(TEXT("SourceString"));
inline const FName Suffix(TEXT("InSuffix"));
inline const FName SearchCase(TEXT("SearchCase"));
inline constexpr TCHAR CaseSensitive[] = TEXT("CaseSensitive");
}
