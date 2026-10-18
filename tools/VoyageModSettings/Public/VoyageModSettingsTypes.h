#pragma once

#include "CoreMinimal.h"

namespace VoyageModSettings
{
enum class ESettingType : uint8
{
    Number,
    Boolean,
    String
};

enum class ENumberStorage : uint8
{
    Double,
    Float
};

struct FSetting
{
    const TCHAR* Key;
    FName Field;
    ESettingType Type;
    const TCHAR* Default;
    const TCHAR* Minimum;
    const TCHAR* Maximum;
    ENumberStorage NumberStorage;
    bool CreateMember;
};
}
