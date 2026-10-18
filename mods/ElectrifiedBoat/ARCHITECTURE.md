# ElectrifiedBoat architecture

The additive DML actor watches wall-socket attachment events. For an Electric
Wall Socket attached to a Boat, it resolves a suitable stock electric port
from the Boat's owned actors and registers an external socket with the Boat's
module grid. On restoration, the same attachment path rebuilds the connection.
EndPlay removes only a registration still owned by that socket; stock sockets
away from a Boat keep their original behavior.

The package entry is `/Game/Mods/ElectrifiedBoat/ModActor.ModActor_C`.
[GAME_DERIVED_SOURCES.md](GAME_DERIVED_SOURCES.md) identifies the stock classes
and version-bound contracts. [Mod rules](MOD-RULES.md) give mutation limits.
