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
records the validated Steam build, engine version, and reconstruction method.
When the current Steam build ID differs, the public producer may attempt a
candidate with `-IgnorePinnedGameVersion`. Revisit these mirrors if compilation,
package checks, or the game test exposes an incompatible contract; update their
provenance only after the relevant contract is revalidated.

`Voyage.h`, `Voyage.cpp`, and `Voyage.Build.cs` are our hand-written editor
scaffold around those mirrors. They are not extracted from the game, although
their purpose is to compile the version-bound mirror types.

See `../../GAME_DERIVED_SOURCES.md` for the complete update checklist.
