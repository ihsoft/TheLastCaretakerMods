#pragma once
#include "UObject/NameTypes.h"

// Engine string-conversion parameter roles for read-only character reports.
namespace CharacterObservationGraphNames
{
inline const FName AppendTo(TEXT("AppendTo"));
inline const FName ObjectValue(TEXT("InObj"));
inline const FName IntegerValue(TEXT("InInt"));
inline const FName ByteValue(TEXT("InByte"));
inline const FName RotatorValue(TEXT("InRot"));
}
