# HarpoonCannon source visuals

`harpoon_cannon_blockout.obj` is the mod-authored rough model accepted on
2026-08-30. It uses centimeters with `+X` forward, `+Y` right, and `+Z` up.
The generator splits named OBJ objects into static-mount, yaw-assembly, and
pitch-assembly meshes. For the first restored leaf-level visual checkpoint,
the mod-authored connector-proxy objects are included in the pitch mesh.

The final loaded head remains a direct reference to the current game's
`/Game/AssetSets/Sockets/SM_Mooring_CableSocket_Out` package, but that hard
reference is disabled in the first leaf visual checkpoint. After the
mod-authored proxy loads and renders in game, the generator may create a valid
editor-only stand-in at the stock identity for a separate one-variable test;
that stand-in must never enter the player container.

Source hashes for this accepted checkpoint:

- OBJ: `AADED3621C92225D5F4C6FC794C61B207FF3C8A5D1646077557E9AC97E1541A1`
- MTL: `74B1F2434337D7AAA6B11EAAE83D24F518DC42EB3BE741DFBC0EAA6382BE5C89`
- handoff JSON: `59078ABF3D9821BC927316904462B00DD95DDB8EF3C57F6C65768FC0DF0F4188`
