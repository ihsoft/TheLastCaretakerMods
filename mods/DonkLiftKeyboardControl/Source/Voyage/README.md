# Voyage game-API mirror

This module is a minimal, hand-reconstructed mirror of selected native types
from the game's `/Script/Voyage` module. Unreal needs the matching module,
class, function, property, enum, and struct identities while compiling the
mod's Blueprint assets.

It is not game source code and it is not a native runtime component of the
mod. `Voyage` is compiled only inside the editor project. Its DLL, generated
headers, objects, and other native build output must never be added to the
released IoStore container.

Every game-derived header begins with `HAND-RECONSTRUCTED GAME API MIRROR` and
records the validated Steam build, engine version, executable SHA-256, and
reproduction tools. When the current game fingerprint differs, stop before
building: recreate or revalidate every mirror against the new game version,
then update its provenance header.

`Voyage.h`, `Voyage.cpp`, and `Voyage.Build.cs` are our hand-written editor
scaffold around those mirrors. They are not extracted from the game, although
their purpose is to compile the version-bound mirror types.

See `../../GAME_DERIVED_SOURCES.md` for the complete update checklist.
