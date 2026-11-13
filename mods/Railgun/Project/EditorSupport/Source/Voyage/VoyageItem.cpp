#include "VoyageItem.h"

UVoyageItem::UVoyageItem()
{
    // Editor serialization sentinel, not a runtime/gameplay default. The owned
    // gun explicitly clears this map, so its empty override survives cook.
    properties.Add(TEXT("__RailgunEditorSerializationSentinel"), nullptr);
}

void UVoyageItem::Serialize(FArchive& Ar)
{
    // The editor mirror's map sentinel is not a game default. Store complete
    // map values rather than removals relative to that sentinel. Keep ordinary
    // property-level deltas so absent native fields remain absent in the cook.
    const bool Previous = Ar.ArNoIntraPropertyDelta;
    Ar.ArNoIntraPropertyDelta = true;
    Super::Serialize(Ar);
    Ar.ArNoIntraPropertyDelta = Previous;
}
